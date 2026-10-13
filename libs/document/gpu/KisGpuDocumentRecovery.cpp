/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisGpuPaintDocument_p.h"
#include <QBuffer>
#include <QCryptographicHash>
#include <QDataStream>
#include <limits>

namespace Krita::Document::Gpu {
using namespace Image::Gpu;
namespace Kra = ImportExport::Kra;
namespace {
constexpr quint32 Magic = 0x4c504744; // LPGD
constexpr quint16 Format = 1;
constexpr qsizetype DigestSize = 32;
constexpr qsizetype MinimumSize = 54;
void configure(QDataStream &stream)
{
    stream.setVersion(QDataStream::Qt_5_15);
    stream.setByteOrder(QDataStream::BigEndian);
}
QByteArray digest(const QByteArray &bytes, qsizetype size)
{ return QCryptographicHash::hash(QByteArrayView(bytes.constData(), size), QCryptographicHash::Sha256); }
bool validText(const QByteArray &bytes, QString &text)
{
    text = QString::fromUtf8(bytes);
    return text.toUtf8() == bytes && !text.contains(QChar::Null);
}
void writeField(QDataStream &stream, const QByteArray &bytes)
{
    stream << quint32(bytes.size());
    stream.writeRawData(bytes.constData(), int(bytes.size()));
}
QByteArray readField(QDataStream &stream, qsizetype end, qsizetype maximum, bool &ok)
{
    if (!ok || stream.device()->pos() > end - 4) { ok = false; return {}; }
    quint32 size = 0;
    stream >> size;
    if (stream.status() != QDataStream::Ok || size > quint64(maximum) || size > quint64(end - stream.device()->pos())) {
        ok = false;
        return {};
    }
    const auto bytes = stream.device()->read(size);
    if (bytes.size() != size) ok = false;
    return bytes;
}
}

QByteArray PaintDocument::recoveryData() const
{
    const auto &d = *m_d;
    if (!onThread() || !d.handle) return {};
    const auto model = d.document->recoveryData();
    const auto path = d.path.toUtf8();
    QString decoded;
    const auto limits = Private::formatLimits(d.limits);
    if (model.isEmpty() || path.size() > 65536 || !validText(path, decoded) || decoded != d.path ||
        Kra::RasterCodec::validateMetadata(d.description, d.files, limits).code != Kra::Status::Success) return {};
    qint64 size = MinimumSize + model.size() + path.size() + d.description.size();
    for (auto file = d.files.cbegin(); file != d.files.cend(); ++file) size += 8 + file.key().toUtf8().size() + file.value().size();
    if (size > d.limits.recoveryBytes || size > std::numeric_limits<int>::max()) return {};
    QByteArray bytes;
    bytes.reserve(qsizetype(size));
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    configure(stream);
    stream << Magic << Format;
    writeField(stream, model);
    writeField(stream, path);
    writeField(stream, d.description);
    stream << quint32(d.files.size());
    for (auto file = d.files.cbegin(); file != d.files.cend(); ++file) {
        writeField(stream, file.key().toUtf8());
        writeField(stream, file.value());
    }
    if (stream.status() != QDataStream::Ok) return {};
    bytes.append(digest(bytes, bytes.size()));
    return bytes;
}

Result PaintDocument::restoreRecovery(const QByteArray &data)
{
    auto &d = *m_d;
    if (!onThread()) return Result::InvalidRecovery;
    if (!d.handle) return Result::Closed;
    if (data.size() < MinimumSize || data.size() > d.limits.recoveryBytes || data.size() > std::numeric_limits<int>::max())
        return Result::InvalidRecovery;
    const auto end = data.size() - DigestSize;
    if (digest(data, end) != data.last(DigestSize)) return Result::InvalidRecovery;
    QBuffer input;
    input.setData(data);
    input.open(QIODevice::ReadOnly);
    QDataStream stream(&input);
    configure(stream);
    quint32 magic = 0;
    quint16 format = 0;
    stream >> magic >> format;
    if (magic != Magic || format != Format) return Result::InvalidRecovery;
    bool ok = true;
    const auto model = readField(stream, end, d.limits.recoveryBytes, ok);
    const auto encodedPath = readField(stream, end, 65536, ok);
    const auto limits = Private::formatLimits(d.limits);
    const auto description = readField(stream, end, limits.xmlBytes, ok);
    QString path;
    if (!ok || model.isEmpty() || !validText(encodedPath, path) || input.pos() > end - 4) return Result::InvalidRecovery;
    quint32 count = 0;
    stream >> count;
    if (stream.status() != QDataStream::Ok || count > quint64(limits.entries) || count > quint64((end - input.pos()) / 8))
        return Result::InvalidRecovery;
    QMap<QString, QByteArray> files;
    qsizetype fileBytes = 0;
    for (quint32 i = 0; i < count; ++i) {
        const auto encodedName = readField(stream, end, 1024, ok);
        const auto bytes = readField(stream, end, limits.archiveBytes - fileBytes, ok);
        QString name;
        if (!ok || !validText(encodedName, name) || files.contains(name)) return Result::InvalidRecovery;
        fileBytes += bytes.size();
        files.insert(name, bytes);
    }
    if (input.pos() != end || Kra::RasterCodec::validateMetadata(description, files, limits).code != Kra::Status::Success)
        return Result::InvalidRecovery;
    auto document = std::make_unique<OperationDocument>(QSize(1, 1), d.limits);
    if (document->restoreRecovery(model) != Result::Success) return Result::InvalidRecovery;
    const auto replaced = d.execution.replaceDocument(d.handle, document->preview());
    if (replaced.code != ExecutionCode::Success)
        return replaced.code == ExecutionCode::CapacityExceeded ? Result::CapacityExceeded : Result::InvalidRecovery;
    d.document.swap(document);
    d.path = path;
    d.description = description;
    d.files = std::move(files);
    d.lastRequest = replaced;
    Q_EMIT changed();
    return Result::Success;
}
}
