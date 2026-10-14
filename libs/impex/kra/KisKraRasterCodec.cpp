/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisKraRasterCodec.h"
#include "KisKraRgbaTiles_p.h"
#include <QBuffer>
#include <QColorSpace>
#include <QColorTransform>
#include <QDomDocument>
#include <QImage>
#include <QSet>
#include <quazip.h>
#include <quazipfile.h>
#include <quazipnewinfo.h>
#include <limits>

namespace LibrePaint::ImportExport::Kra {
using namespace Image::Gpu;
namespace {
Outcome invalid(const QString &detail) { return {Status::InvalidData, detail}; }
Outcome unsupported(const QString &detail) { return {Status::UnsupportedData, detail}; }
Outcome capacity() { return {Status::CapacityExceeded, QStringLiteral("KRA capacity exceeded")}; }
Outcome ioFailure() { return {Status::IoFailure, QStringLiteral("KRA device operation failed")}; }
bool validLimits(const Limits &limits)
{
    const qsizetype maximum = std::numeric_limits<int>::max();
    return limits.rasterBytes > 0 && limits.rasterBytes <= maximum &&
        limits.archiveBytes > 0 && limits.archiveBytes <= maximum &&
        limits.xmlBytes > 0 && limits.xmlBytes <= maximum &&
        limits.entries > 0 && limits.entries <= maximum && limits.layers > 0 && limits.layers <= 4096;
}
bool validPath(const QString &path)
{
    if (path.isEmpty() || path.toUtf8().size() > 1024 || path.contains(QChar::Null) || path.contains(QLatin1Char('\\')) ||
        QString::fromUtf8(path.toUtf8()) != path) return false;
    const auto parts = path.split(QLatin1Char('/'));
    for (const auto &part : parts) {
        if (part.isEmpty() || part == QLatin1String(".") || part == QLatin1String("..")) return false;
    }
    return true;
}
bool component(const QString &value) { return validPath(value) && !value.contains(QLatin1Char('/')); }
bool validName(const QString &value)
{
    return !value.contains(QChar::Null) && value.toUtf8().size() <= 65536 && QString::fromUtf8(value.toUtf8()) == value;
}
Outcome readArchive(QIODevice &source, Limits limits, QMap<QString, QByteArray> &files)
{
    if (!source.isOpen() || !source.isReadable() || source.isSequential() || !source.seek(0)) return ioFailure();
    if (source.size() > limits.archiveBytes) return capacity();
    QuaZip archive(&source);
    archive.setAutoClose(false);
    if (!archive.open(QuaZip::mdUnzip)) return invalid(QStringLiteral("Invalid KRA ZIP archive"));
    const int count = archive.getEntriesCount();
    if (count <= 0) return invalid(QStringLiteral("Empty KRA archive"));
    if (count > limits.entries) return capacity();
    qsizetype total = 0;
    int visited = 0;
    QSet<QString> names;
    for (bool more = archive.goToFirstFile(); more; more = archive.goToNextFile()) {
        QuaZipFileInfo64 info;
        if (!archive.getCurrentFileInfo(&info)) return invalid(QStringLiteral("Invalid KRA entry"));
        const bool directory = info.name.endsWith(QLatin1Char('/'));
        if (!validPath(directory ? info.name.chopped(1) : info.name) || names.contains(info.name) || info.isEncrypted())
            return invalid(QStringLiteral("Invalid or duplicate KRA entry name"));
        names.insert(info.name);
        ++visited;
        if (info.uncompressedSize > quint64(limits.archiveBytes - total)) return capacity();
        if (directory) {
            if (info.uncompressedSize) return invalid(QStringLiteral("Invalid KRA directory"));
            continue;
        }
        if (info.name == QLatin1String("maindoc.xml") && info.uncompressedSize > quint64(limits.xmlBytes)) return capacity();
        QuaZipFile file(&archive);
        if (!file.open(QIODevice::ReadOnly)) return invalid(QStringLiteral("Unreadable KRA entry"));
        QByteArray bytes;
        bytes.reserve(qsizetype(info.uncompressedSize));
        while (!file.atEnd()) {
            const auto part = file.read(qMin<quint64>(65536, info.uncompressedSize - bytes.size() + 1));
            if (part.isEmpty() || quint64(bytes.size() + part.size()) > info.uncompressedSize)
                return invalid(QStringLiteral("Invalid KRA entry length"));
            bytes += part;
        }
        file.close();
        if (file.getZipError() || quint64(bytes.size()) != info.uncompressedSize)
            return invalid(QStringLiteral("KRA entry checksum or length mismatch"));
        total += bytes.size();
        files.insert(info.name, std::move(bytes));
    }
    if (archive.getZipError() || visited != count) return invalid(QStringLiteral("Incomplete KRA archive"));
    archive.close();
    if (archive.getZipError()) return invalid(QStringLiteral("Invalid KRA archive trailer"));
    return {Status::Success, {}};
}
Outcome description(const QByteArray &bytes, QDomDocument &xml, QDomElement &image)
{
    if (!xml.setContent(bytes)) return invalid(QStringLiteral("Invalid KRA document XML"));
    const auto root = xml.documentElement();
    image = root.firstChildElement(QStringLiteral("IMAGE"));
    const auto layers = image.firstChildElement(QStringLiteral("layers"));
    const auto syntaxVersion = root.attribute(QStringLiteral("syntaxVersion"));
    if (root.tagName() != QLatin1String("DOC") || (syntaxVersion != QLatin1String("2") && syntaxVersion != QLatin1String("2.0")) ||
        image.isNull() || !image.nextSiblingElement(QStringLiteral("IMAGE")).isNull() ||
        layers.isNull() || !layers.nextSiblingElement(QStringLiteral("layers")).isNull() ||
        !component(image.attribute(QStringLiteral("name")))) return invalid(QStringLiteral("Invalid KRA image description"));
    if (image.attribute(QStringLiteral("colorspacename")) != QLatin1String("RGBA"))
        return unsupported(QStringLiteral("KRA image requires RGBA8"));
    return {Status::Success, {}};
}
QDomElement createDescription(QDomDocument &xml)
{
    xml = QDomDocument(QDomImplementation().createDocumentType(QStringLiteral("DOC"), QStringLiteral("-//KDE//DTD krita 2.0//EN"),
                                                              QStringLiteral("http://www.calligra.org/DTD/krita-2.0.dtd")));
    auto root = xml.createElement(QStringLiteral("DOC"));
    root.setAttribute(QStringLiteral("xmlns"), QStringLiteral("http://www.calligra.org/DTD/krita"));
    root.setAttribute(QStringLiteral("syntaxVersion"), QStringLiteral("2"));
    root.setAttribute(QStringLiteral("editor"), QStringLiteral("LibrePaint"));
    xml.appendChild(root);
    auto image = xml.createElement(QStringLiteral("IMAGE"));
    image.setAttribute(QStringLiteral("name"), QStringLiteral("image"));
    image.setAttribute(QStringLiteral("colorspacename"), QStringLiteral("RGBA"));
    image.setAttribute(QStringLiteral("profile"), QStringLiteral("sRGB built-in"));
    image.setAttribute(QStringLiteral("mime"), QStringLiteral("application/x-kra"));
    image.setAttribute(QStringLiteral("x-res"), QStringLiteral("72"));
    image.setAttribute(QStringLiteral("y-res"), QStringLiteral("72"));
    root.appendChild(image);
    image.appendChild(xml.createElement(QStringLiteral("layers")));
    return image;
}
bool integer(const QDomElement &element, const QString &name, int &value, const QString &fallback = {})
{
    bool ok = false;
    value = element.attribute(name, fallback).toInt(&ok);
    return ok;
}
Outcome layerDescription(const QDomElement &element, QPoint &offset)
{
    if (element.tagName() != QLatin1String("layer") || element.attribute(QStringLiteral("nodetype")) != QLatin1String("paintlayer") ||
        element.attribute(QStringLiteral("compositeop")) != QLatin1String("normal") ||
        element.attribute(QStringLiteral("opacity")) != QLatin1String("255") ||
        element.attribute(QStringLiteral("colorspacename"), QStringLiteral("RGBA")) != QLatin1String("RGBA") ||
        element.attribute(QStringLiteral("locked"), QStringLiteral("0")) != QLatin1String("0") ||
        !element.firstChildElement().isNull() || element.hasAttribute(QStringLiteral("keyframes")))
        return unsupported(QStringLiteral("KRA layer requires a flat unlocked RGBA8 paint layer with normal compositing"));
    for (const auto &name : {QStringLiteral("channelflags"), QStringLiteral("channellockflags")}) {
        const auto flags = element.attribute(name);
        if (!flags.isEmpty() && flags != QLatin1String("1111")) return unsupported(QStringLiteral("KRA channel flags require all channels"));
    }
    const auto visible = element.attribute(QStringLiteral("visible"));
    int x = 0, y = 0;
    if ((visible != QLatin1String("0") && visible != QLatin1String("1")) ||
        !component(element.attribute(QStringLiteral("filename"))) || !validName(element.attribute(QStringLiteral("name"))) ||
        !integer(element, QStringLiteral("x"), x, QStringLiteral("0")) ||
        !integer(element, QStringLiteral("y"), y, QStringLiteral("0"))) return invalid(QStringLiteral("Invalid KRA layer values"));
    offset = {x, y};
    return {Status::Success, {}};
}
bool srgbSpace(const QColorSpace &space)
{
    if (!space.isValid() || space.primaries() != QColorSpace::Primaries::SRgb) return false;
    if (space.transferFunction() == QColorSpace::TransferFunction::SRgb) return true;
    if (space.transferFunction() != QColorSpace::TransferFunction::Custom ||
        space.transformModel() != QColorSpace::TransformModel::ThreeComponentMatrix) return false;
    // Matrix profiles can encode sRGB curves as tables. Their independent
    // channel curves must preserve every encoded 8-bit channel value.
    const auto transform = space.transformationToColorSpace(QColorSpace(QColorSpace::SRgb));
    for (int value = 0; value < 256; ++value) {
        const auto expected = qRgb(value, value, value);
        if (transform.map(expected) != expected) return false;
    }
    return true;
}

Outcome profile(const QMap<QString, QByteArray> &files, const QString &path, const QString &name)
{
    if (!files.contains(path)) {
        return name == QLatin1String("sRGB built-in") ? Outcome{Status::Success, {}} :
            unsupported(QStringLiteral("KRA requires an sRGB profile"));
    }
    const auto space = QColorSpace::fromIccProfile(files.value(path));
    if (!space.isValid()) return invalid(QStringLiteral("Invalid KRA color profile"));
    if (!srgbSpace(space))
        return unsupported(QStringLiteral("KRA requires an sRGB profile"));
    return {Status::Success, {}};
}
Outcome validateFiles(const QMap<QString, QByteArray> &files, Limits limits)
{
    if (files.size() > limits.entries) return capacity();
    qsizetype total = 0;
    for (auto file = files.cbegin(); file != files.cend(); ++file) {
        if (!validPath(file.key())) return invalid(QStringLiteral("Invalid KRA entry name"));
        if (file.value().size() > limits.archiveBytes - total) return capacity();
        total += file.value().size();
    }
    return {Status::Success, {}};
}
Outcome extractMetadata(const QDomElement &image, const QMap<QString, QByteArray> &files,
                        Limits limits, ImageMetadata &metadata)
{
    bool xOk = false, yOk = false;
    const auto x = image.attribute(QStringLiteral("x-res"), QStringLiteral("72")).toDouble(&xOk);
    const auto y = image.attribute(QStringLiteral("y-res"), QStringLiteral("72")).toDouble(&yOk);
    if (!xOk || !yOk || !qIsFinite(x) || !qIsFinite(y) || x <= 0 || y <= 0)
        return invalid(QStringLiteral("Invalid KRA image resolution"));
    metadata.resolution = {x, y};
    metadata.documentInfo = files.value(QStringLiteral("documentinfo.xml"));
    if (metadata.documentInfo.size() > limits.xmlBytes) return capacity();
    if (files.contains(QStringLiteral("documentinfo.xml"))) {
        QDomDocument info;
        if (!info.setContent(metadata.documentInfo) || info.documentElement().tagName() != QLatin1String("document-info"))
            return invalid(QStringLiteral("Invalid KRA document information"));
    }
    const auto prefix = image.attribute(QStringLiteral("name")) + QStringLiteral("/annotations/");
    metadata.iccProfile = files.value(prefix + QStringLiteral("icc"));
    const auto annotations = image.firstChildElement(QStringLiteral("Annotations"));
    if (!annotations.nextSiblingElement(QStringLiteral("Annotations")).isNull())
        return invalid(QStringLiteral("Duplicate KRA annotation containers"));
    QSet<QString> types;
    for (auto entry = annotations.firstChildElement(); !entry.isNull(); entry = entry.nextSiblingElement()) {
        Annotation annotation{entry.attribute(QStringLiteral("type")), entry.attribute(QStringLiteral("description")), {}};
        if (!component(annotation.type) || annotation.type == QLatin1String("icc") ||
            !validName(annotation.description) || types.contains(annotation.type) || !files.contains(prefix + annotation.type))
            return invalid(QStringLiteral("Invalid, duplicate or missing KRA annotation"));
        types.insert(annotation.type);
        annotation.bytes = files.value(prefix + annotation.type);
        metadata.annotations.append(std::move(annotation));
    }
    if (!types.contains(QStringLiteral("exif")) && files.contains(prefix + QStringLiteral("exif")))
        metadata.annotations.append({QStringLiteral("exif"), {}, files.value(prefix + QStringLiteral("exif"))});
    return {Status::Success, {}};
}
class ArchiveBuffer : public QBuffer
{
public:
    ArchiveBuffer(QByteArray *bytes, qsizetype maximum) : QBuffer(bytes), m_maximum(maximum) {}
    bool exceeded = false;
protected:
    qint64 writeData(const char *data, qint64 size) override
    {
        if (pos() > m_maximum || size > m_maximum - pos()) { exceeded = true; return -1; }
        return QBuffer::writeData(data, size);
    }
private:
    qsizetype m_maximum;
};
Outcome writeArchive(QIODevice &destination, const QMap<QString, QByteArray> &files, Limits limits)
{
    QByteArray bytes;
    ArchiveBuffer buffer(&bytes, limits.archiveBytes);
    if (!buffer.open(QIODevice::WriteOnly)) return ioFailure();
    QuaZip archive(&buffer);
    archive.setAutoClose(false);
    if (!archive.open(QuaZip::mdCreate)) return ioFailure();
    auto write = [&](const QString &name, int method) {
        QuaZipFile file(&archive);
        if (!file.open(QIODevice::WriteOnly, QuaZipNewInfo(name), nullptr, 0, method)) return false;
        const auto &bytes = files[name];
        if (file.write(bytes) != bytes.size()) return false;
        file.close();
        return file.getZipError() == 0;
    };
    if (!write(QStringLiteral("mimetype"), 0)) return buffer.exceeded ? capacity() : ioFailure();
    for (auto file = files.cbegin(); file != files.cend(); ++file) {
        if (file.key() != QLatin1String("mimetype") && !write(file.key(), Z_DEFLATED)) return buffer.exceeded ? capacity() : ioFailure();
    }
    archive.close();
    if (buffer.exceeded) return capacity();
    if (archive.getZipError() || destination.size() != 0 || !destination.seek(0) || destination.write(bytes) != bytes.size()) return ioFailure();
    return {Status::Success, {}};
}
}

Outcome RasterCodec::updateImageMetadata(Document &document, const ImageMetadata &metadata, Limits limits)
{
    auto result = validateMetadata(document.description, document.files, limits);
    if (result.code != Status::Success) return result;
    if (!qIsFinite(metadata.resolution.x()) || !qIsFinite(metadata.resolution.y()) ||
        metadata.resolution.x() <= 0 || metadata.resolution.y() <= 0)
        return invalid(QStringLiteral("Invalid KRA image resolution"));
    if (metadata.documentInfo.size() > limits.xmlBytes) return capacity();
    if (!metadata.documentInfo.isEmpty()) {
        QDomDocument info;
        if (!info.setContent(metadata.documentInfo) || info.documentElement().tagName() != QLatin1String("document-info"))
            return invalid(QStringLiteral("Invalid KRA document information"));
    }
    QDomDocument xml;
    QDomElement image;
    if (document.description.isEmpty()) image = createDescription(xml);
    else {
        result = description(document.description, xml, image);
        if (result.code != Status::Success) return result;
    }
    auto files = document.files;
    const auto folder = image.attribute(QStringLiteral("name"));
    const auto prefix = folder + QStringLiteral("/annotations/");
    auto annotations = image.firstChildElement(QStringLiteral("Annotations"));
    for (auto entry = annotations.firstChildElement(); !entry.isNull(); entry = entry.nextSiblingElement())
        files.remove(prefix + entry.attribute(QStringLiteral("type")));
    if (!annotations.isNull()) image.removeChild(annotations);
    annotations = xml.createElement(QStringLiteral("Annotations"));
    files.remove(prefix + QStringLiteral("exif"));
    QSet<QString> types;
    for (const auto &annotation : metadata.annotations) {
        if (!component(annotation.type) || annotation.type == QLatin1String("icc") ||
            !validName(annotation.description) || types.contains(annotation.type))
            return invalid(QStringLiteral("Invalid or duplicate KRA annotation"));
        types.insert(annotation.type);
        files.insert(prefix + annotation.type, annotation.bytes);
        if (annotation.type != QLatin1String("exif")) {
            auto entry = xml.createElement(QStringLiteral("Annotation"));
            entry.setAttribute(QStringLiteral("type"), annotation.type);
            entry.setAttribute(QStringLiteral("description"), annotation.description);
            annotations.appendChild(entry);
        }
    }
    if (annotations.hasChildNodes()) image.appendChild(annotations);
    if (!metadata.iccProfile.isEmpty()) files.insert(prefix + QStringLiteral("icc"), metadata.iccProfile);
    if (metadata.documentInfo.isEmpty()) files.remove(QStringLiteral("documentinfo.xml"));
    else files.insert(QStringLiteral("documentinfo.xml"), metadata.documentInfo);
    image.setAttribute(QStringLiteral("x-res"), QString::number(metadata.resolution.x(), 'g', 17));
    image.setAttribute(QStringLiteral("y-res"), QString::number(metadata.resolution.y(), 'g', 17));
    const auto bytes = xml.toByteArray();
    files.insert(QStringLiteral("maindoc.xml"), bytes);
    result = validateMetadata(bytes, files, limits);
    if (result.code != Status::Success) return result;
    document.description = bytes;
    document.files = std::move(files);
    return {Status::Success, {}};
}

Outcome RasterCodec::validateMetadata(const QByteArray &bytes, const QMap<QString, QByteArray> &files, Limits limits)
{
    if (!validLimits(limits)) return invalid(QStringLiteral("Invalid KRA limits"));
    auto result = validateFiles(files, limits);
    if (result.code != Status::Success) return result;
    if (bytes.size() > limits.xmlBytes) return capacity();
    QDomDocument xml;
    QDomElement image;
    if (bytes.isEmpty()) image = createDescription(xml);
    else result = description(bytes, xml, image);
    if (result.code != Status::Success) return result;
    const auto folder = image.attribute(QStringLiteral("name"));
    result = profile(files, folder + QStringLiteral("/annotations/icc"), image.attribute(QStringLiteral("profile")));
    if (result.code != Status::Success) return result;
    const auto layers = image.firstChildElement(QStringLiteral("layers"));
    qsizetype count = 0;
    QSet<QString> filenames;
    QSet<QUuid> uuids;
    for (auto element = layers.firstChildElement(); !element.isNull(); element = element.nextSiblingElement()) {
        if (++count > limits.layers) return capacity();
        QPoint offset;
        result = layerDescription(element, offset);
        if (result.code != Status::Success) return result;
        const QUuid uuid(element.attribute(QStringLiteral("uuid")));
        const auto filename = element.attribute(QStringLiteral("filename"));
        if ((element.hasAttribute(QStringLiteral("uuid")) && (uuid.isNull() || uuids.contains(uuid))) || filenames.contains(filename))
            return invalid(QStringLiteral("Invalid KRA layer identity"));
        uuids.insert(uuid);
        filenames.insert(filename);
        result = profile(files, folder + QStringLiteral("/layers/") + filename + QStringLiteral(".icc"), image.attribute(QStringLiteral("profile")));
        if (result.code != Status::Success) return result;
    }
    ImageMetadata metadata;
    return extractMetadata(image, files, limits, metadata);
}

MetadataOutcome RasterCodec::readImageMetadata(const QByteArray &bytes, const QMap<QString, QByteArray> &files, Limits limits)
{
    const auto validated = validateMetadata(bytes, files, limits);
    if (validated.code != Status::Success) return {validated.code, {}, validated.diagnostic};
    QDomDocument xml;
    QDomElement image;
    if (bytes.isEmpty()) image = createDescription(xml);
    else description(bytes, xml, image);
    ImageMetadata metadata;
    const auto extracted = extractMetadata(image, files, limits, metadata);
    if (extracted.code != Status::Success) return {extracted.code, {}, extracted.diagnostic};
    return {Status::Success, std::move(metadata), {}};
}

LoadOutcome RasterCodec::read(QIODevice &source, Limits limits)
{
    auto fail = [](const Outcome &result) { return LoadOutcome{result.code, {}, result.diagnostic}; };
    if (!validLimits(limits)) return fail(invalid(QStringLiteral("Invalid KRA limits")));
    Document document;
    auto result = readArchive(source, limits, document.files);
    if (result.code != Status::Success) return fail(result);
    const auto mime = document.files.value(QStringLiteral("mimetype"));
    if (mime != QByteArrayLiteral("application/x-krita") && mime != QByteArrayLiteral("application/x-kra"))
        return fail(invalid(QStringLiteral("Invalid KRA MIME type")));
    document.description = document.files.value(QStringLiteral("maindoc.xml"));
    result = validateMetadata(document.description, document.files, limits);
    if (result.code != Status::Success) return fail(result);
    QDomDocument xml;
    QDomElement image;
    result = description(document.description, xml, image);
    if (result.code != Status::Success) return fail(result);
    int width = 0, height = 0;
    if (!integer(image, QStringLiteral("width"), width) || !integer(image, QStringLiteral("height"), height) ||
        width <= 0 || height <= 0 || width > RgbaTile::CoordinateLimit || height > RgbaTile::CoordinateLimit)
        return fail(invalid(QStringLiteral("Invalid KRA image dimensions")));
    document.image.size = {width, height};
    const auto folder = image.attribute(QStringLiteral("name"));
    result = profile(document.files, folder + QStringLiteral("/annotations/icc"), image.attribute(QStringLiteral("profile")));
    if (result.code != Status::Success) return fail(result);
    QSet<QUuid> uuids;
    QSet<QString> filenames;
    qsizetype rasterBytes = 0;
    int selected = -1;
    bool generatedUuids = false;
    const auto layers = image.firstChildElement(QStringLiteral("layers"));
    for (auto element = layers.firstChildElement(); !element.isNull(); element = element.nextSiblingElement()) {
        if (document.image.layers.size() >= limits.layers) return fail(capacity());
        QPoint offset;
        result = layerDescription(element, offset);
        if (result.code != Status::Success) return fail(result);
        RasterLayer layer;
        layer.name = element.attribute(QStringLiteral("name"));
        layer.visible = element.attribute(QStringLiteral("visible")) == QLatin1String("1");
        layer.uuid = element.hasAttribute(QStringLiteral("uuid")) ? QUuid(element.attribute(QStringLiteral("uuid"))) : QUuid::createUuid();
        if (!element.hasAttribute(QStringLiteral("uuid"))) {
            element.setAttribute(QStringLiteral("uuid"), layer.uuid.toString());
            generatedUuids = true;
        }
        const auto filename = element.attribute(QStringLiteral("filename"));
        if (layer.uuid.isNull() || uuids.contains(layer.uuid) || filenames.contains(filename))
            return fail(invalid(QStringLiteral("Duplicate or invalid KRA layer identity")));
        uuids.insert(layer.uuid);
        filenames.insert(filename);
        const auto path = folder + QStringLiteral("/layers/") + filename;
        if (!document.files.contains(path)) return fail(invalid(QStringLiteral("Missing KRA layer pixels")));
        result = profile(document.files, path + QStringLiteral(".icc"), image.attribute(QStringLiteral("profile")));
        if (result.code != Status::Success) return fail(result);
        if (document.files.contains(path + QStringLiteral(".defaultpixel"))) {
            const auto pixel = document.files.value(path + QStringLiteral(".defaultpixel"));
            if (pixel.size() != 4) return fail(invalid(QStringLiteral("Invalid KRA default pixel")));
            layer.defaultPixel = {quint8(pixel[2]), quint8(pixel[1]), quint8(pixel[0]), quint8(pixel[3])};
        }
        result = Detail::readTiles(document.files.value(path), layer.defaultPixel, offset, limits.rasterBytes - rasterBytes, layer.tiles);
        if (result.code != Status::Success) return fail(result);
        rasterBytes += layer.tiles.size() * RgbaTile::ByteCount;
        document.files.remove(path);
        document.files.remove(path + QStringLiteral(".defaultpixel"));
        if (element.attribute(QStringLiteral("selected")) == QLatin1String("true")) {
            if (selected >= 0) return fail(invalid(QStringLiteral("Multiple selected KRA layers")));
            selected = document.image.layers.size();
        }
        document.image.layers.prepend(std::move(layer));
    }
    document.image.activeLayer = document.image.layers.isEmpty() ? 0 : document.image.layers.size() - 1 - qMax(0, selected);
    if (generatedUuids) document.description = xml.toByteArray();
    for (const auto &name : {QStringLiteral("maindoc.xml"), QStringLiteral("mimetype"), QStringLiteral("mergedimage.png"), QStringLiteral("preview.png")})
        document.files.remove(name);
    return {Status::Success, std::move(document), {}};
}

Outcome RasterCodec::write(QIODevice &destination, const Document &document, const QImage &projection, Limits limits)
{
    if (!destination.isOpen() || !destination.isWritable() || destination.isSequential()) return ioFailure();
    if (!validLimits(limits)) return invalid(QStringLiteral("Invalid KRA limits"));
    const auto &value = document.image;
    if (value.size.width() <= 0 || value.size.height() <= 0 || value.size.width() > RgbaTile::CoordinateLimit ||
        value.size.height() > RgbaTile::CoordinateLimit || projection.isNull() || projection.size() != value.size ||
        value.activeLayer < 0 || (value.layers.isEmpty() ? value.activeLayer != 0 : value.activeLayer >= value.layers.size()))
        return invalid(QStringLiteral("Invalid KRA image or projection"));
    if (value.layers.size() > limits.layers || document.description.size() > limits.xmlBytes) return capacity();
    if (projection.colorSpace().isValid() && !srgbSpace(projection.colorSpace()))
        return unsupported(QStringLiteral("KRA projection requires sRGB"));
    auto result = validateMetadata(document.description, document.files, limits);
    if (result.code != Status::Success) return result;
    QDomDocument xml;
    QDomElement image;
    if (document.description.isEmpty()) {
        image = createDescription(xml);
    } else {
        result = description(document.description, xml, image);
        if (result.code != Status::Success) return result;
    }
    const auto folder = image.attribute(QStringLiteral("name"));
    auto files = document.files;
    const auto imageProfile = folder + QStringLiteral("/annotations/icc");
    if (!files.contains(imageProfile)) files.insert(imageProfile, QColorSpace(QColorSpace::SRgb).iccProfile());
    result = profile(files, imageProfile, image.attribute(QStringLiteral("profile")));
    if (result.code != Status::Success) return result;
    QMap<QUuid, QDomElement> original;
    QSet<QString> filenames;
    auto layers = image.firstChildElement(QStringLiteral("layers"));
    if (layers.isNull()) { layers = xml.createElement(QStringLiteral("layers")); image.appendChild(layers); }
    for (auto element = layers.firstChildElement(); !element.isNull(); element = element.nextSiblingElement()) {
        QPoint offset;
        result = layerDescription(element, offset);
        if (result.code != Status::Success) return result;
        const QUuid uuid(element.attribute(QStringLiteral("uuid")));
        const auto filename = element.attribute(QStringLiteral("filename"));
        if (uuid.isNull() || original.contains(uuid) || filenames.contains(filename)) return invalid(QStringLiteral("Invalid KRA layer identity"));
        original.insert(uuid, element.cloneNode(true).toElement());
        filenames.insert(filename);
    }
    while (!layers.firstChild().isNull()) layers.removeChild(layers.firstChild());
    QSet<QUuid> uuids;
    qsizetype rasterBytes = 0;
    for (qsizetype i = value.layers.size(); i-- > 0;) {
        const auto &layer = value.layers[i];
        if (layer.uuid.isNull() || uuids.contains(layer.uuid) || !validName(layer.name)) return invalid(QStringLiteral("Invalid KRA layer identity or name"));
        uuids.insert(layer.uuid);
        QDomElement element = original.value(layer.uuid);
        if (element.isNull()) {
            element = xml.createElement(QStringLiteral("layer"));
            QString filename;
            for (int suffix = 1; filename.isEmpty(); ++suffix) {
                const auto candidate = QStringLiteral("layer%1").arg(suffix);
                if (!filenames.contains(candidate)) filename = candidate;
            }
            filenames.insert(filename);
            element.setAttribute(QStringLiteral("filename"), filename);
            element.setAttribute(QStringLiteral("x"), QStringLiteral("0"));
            element.setAttribute(QStringLiteral("y"), QStringLiteral("0"));
        }
        const auto path = folder + QStringLiteral("/layers/") + element.attribute(QStringLiteral("filename"));
        const QPoint offset(element.attribute(QStringLiteral("x"), QStringLiteral("0")).toInt(),
                            element.attribute(QStringLiteral("y"), QStringLiteral("0")).toInt());
        QByteArray pixels;
        result = Detail::writeTiles(layer.tiles, layer.defaultPixel, offset, limits.rasterBytes - rasterBytes, pixels);
        if (result.code != Status::Success) return result;
        rasterBytes += layer.tiles.size() * RgbaTile::ByteCount;
        files.insert(path, std::move(pixels));
        files.insert(path + QStringLiteral(".defaultpixel"), Detail::bgra(layer.defaultPixel));
        if (!files.contains(path + QStringLiteral(".icc"))) files.insert(path + QStringLiteral(".icc"), files.value(imageProfile));
        result = profile(files, path + QStringLiteral(".icc"), image.attribute(QStringLiteral("profile")));
        if (result.code != Status::Success) return result;
        element.setAttribute(QStringLiteral("name"), layer.name);
        element.setAttribute(QStringLiteral("uuid"), layer.uuid.toString());
        element.setAttribute(QStringLiteral("visible"), layer.visible ? QStringLiteral("1") : QStringLiteral("0"));
        element.setAttribute(QStringLiteral("selected"), i == value.activeLayer ? QStringLiteral("true") : QStringLiteral("false"));
        element.setAttribute(QStringLiteral("nodetype"), QStringLiteral("paintlayer"));
        element.setAttribute(QStringLiteral("compositeop"), QStringLiteral("normal"));
        element.setAttribute(QStringLiteral("opacity"), QStringLiteral("255"));
        element.setAttribute(QStringLiteral("colorspacename"), QStringLiteral("RGBA"));
        element.setAttribute(QStringLiteral("locked"), QStringLiteral("0"));
        layers.appendChild(element);
    }
    for (auto old = original.cbegin(); old != original.cend(); ++old) {
        if (!uuids.contains(old.key())) {
            const auto path = folder + QStringLiteral("/layers/") + old.value().attribute(QStringLiteral("filename"));
            files.remove(path); files.remove(path + QStringLiteral(".defaultpixel")); files.remove(path + QStringLiteral(".icc"));
        }
    }
    image.setAttribute(QStringLiteral("width"), value.size.width());
    image.setAttribute(QStringLiteral("height"), value.size.height());
    files.insert(QStringLiteral("maindoc.xml"), xml.toByteArray());
    if (files.value(QStringLiteral("maindoc.xml")).size() > limits.xmlBytes) return capacity();
    files.insert(QStringLiteral("mimetype"), QByteArrayLiteral("application/x-krita"));
    auto png = [&](QImage image, const QString &name) {
        image.setColorSpace(QColorSpace::SRgb);
        QByteArray bytes;
        QBuffer buffer(&bytes);
        if (!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "PNG")) return false;
        files.insert(name, std::move(bytes));
        return true;
    };
    if (!png(projection, QStringLiteral("mergedimage.png")) ||
        !png(projection.scaled(256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation), QStringLiteral("preview.png"))) return ioFailure();
    result = validateFiles(files, limits);
    return result.code == Status::Success ? writeArchive(destination, files, limits) : result;
}
}
