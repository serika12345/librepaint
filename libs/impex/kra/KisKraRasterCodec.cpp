/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisKraRasterCodec.h"
#include "KisKraRgbaTiles_p.h"
#include "KisKraPreview_p.h"
#include <QBuffer>
#include <QColorSpace>
#include <QColorTransform>
#include <QDomDocument>
#include <QDateTime>
#include <QTimeZone>
#include <QImage>
#include <QImageWriter>
#include <QSet>
#include <quazip.h>
#include <quazipfile.h>
#include <quazipnewinfo.h>
#include <limits>
#include <cmath>

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
    const auto background = image.firstChildElement(QStringLiteral("ProjectionBackgroundColor"));
    if (!background.isNull()) {
        const auto decoded = QByteArray::fromBase64Encoding(background.attribute(QStringLiteral("ColorData")).toLatin1(),
                                                          QByteArray::AbortOnBase64DecodingErrors);
        if (!background.nextSiblingElement(QStringLiteral("ProjectionBackgroundColor")).isNull() || !decoded || decoded.decoded.size() != 4)
            return invalid(QStringLiteral("Invalid KRA canvas background color"));
    }
    return {Status::Success, {}};
}
QDomElement createDescription(QDomDocument &xml)
{
    QDomImplementation implementation;
    const auto type = implementation.createDocumentType(QStringLiteral("DOC"), QStringLiteral("-//KDE//DTD krita 2.0//EN"),
                                                        QStringLiteral("http://www.calligra.org/DTD/krita-2.0.dtd"));
    xml = implementation.createDocument(QStringLiteral("http://www.calligra.org/DTD/krita"), QStringLiteral("DOC"), type);
    auto root = xml.documentElement();
    xml.insertBefore(xml.createProcessingInstruction(QStringLiteral("xml"), QStringLiteral("version=\"1.0\" encoding=\"UTF-8\"")), root);
    root.setAttribute(QStringLiteral("syntaxVersion"), QStringLiteral("2.0"));
    root.setAttribute(QStringLiteral("editor"), QStringLiteral("LibrePaint"));
    auto image = xml.createElement(QStringLiteral("IMAGE"));
    image.setAttribute(QStringLiteral("name"), QStringLiteral("image"));
    image.setAttribute(QStringLiteral("description"), QString());
    image.setAttribute(QStringLiteral("colorspacename"), QStringLiteral("RGBA"));
    image.setAttribute(QStringLiteral("profile"), QStringLiteral("sRGB built-in"));
    image.setAttribute(QStringLiteral("mime"), QStringLiteral("application/x-kra"));
    image.setAttribute(QStringLiteral("x-res"), QStringLiteral("72"));
    image.setAttribute(QStringLiteral("y-res"), QStringLiteral("72"));
    root.appendChild(image);
    image.appendChild(xml.createElement(QStringLiteral("layers")));
    auto background = xml.createElement(QStringLiteral("ProjectionBackgroundColor"));
    background.setAttribute(QStringLiteral("ColorData"), QStringLiteral("AAAAAA=="));
    image.appendChild(background);
    image.appendChild(xml.createElement(QStringLiteral("Palettes")));
    image.appendChild(xml.createElement(QStringLiteral("resources")));
    return image;
}
QByteArray serializeDescription(QDomDocument &xml)
{
    // Parsing loses the root's namespace representation and rewrites the XML
    // declaration. Reconstruct those owned fields through the DOM so both
    // image owners emit the same native document header.
    const auto old = xml.documentElement();
    auto root = xml.createElementNS(QStringLiteral("http://www.calligra.org/DTD/krita"), QStringLiteral("DOC"));
    const auto attributes = old.attributes();
    for (int i = 0; i < attributes.count(); ++i) {
        const auto attribute = attributes.item(i).toAttr();
        if (attribute.name() != QLatin1String("xmlns")) root.setAttributeNode(attribute.cloneNode().toAttr());
    }
    while (!old.firstChild().isNull()) root.appendChild(old.firstChild());
    xml.replaceChild(root, old);
    for (auto node = xml.firstChild(); !node.isNull();) {
        const auto next = node.nextSibling();
        if (node.isProcessingInstruction() && node.toProcessingInstruction().target() == QLatin1String("xml")) xml.removeChild(node);
        node = next;
    }
    xml.insertBefore(xml.createProcessingInstruction(QStringLiteral("xml"), QStringLiteral("version=\"1.0\" encoding=\"UTF-8\"")), xml.firstChild());
    return xml.toByteArray();
}
bool integer(const QDomElement &element, const QString &name, int &value, const QString &fallback = {})
{
    bool ok = false;
    value = element.attribute(name, fallback).toInt(&ok);
    return ok;
}
Outcome layerDescription(const QDomElement &element, QPoint &offset)
{
    const auto type = element.attribute(QStringLiteral("nodetype"));
    const bool group = type == QLatin1String("grouplayer");
    const auto child = element.firstChildElement();
    if (element.tagName() != QLatin1String("layer") || (!group && type != QLatin1String("paintlayer")) ||
        (element.attribute(QStringLiteral("compositeop")) != QLatin1String("normal") &&
         element.attribute(QStringLiteral("compositeop")) != QLatin1String("multiply") &&
         element.attribute(QStringLiteral("compositeop")) != QLatin1String("screen") &&
         element.attribute(QStringLiteral("compositeop")) != QLatin1String("darken") &&
         element.attribute(QStringLiteral("compositeop")) != QLatin1String("lighten") &&
         element.attribute(QStringLiteral("compositeop")) != QLatin1String("add") &&
         element.attribute(QStringLiteral("compositeop")) != QLatin1String("linear_dodge")) ||
        element.attribute(QStringLiteral("colorspacename"), QStringLiteral("RGBA")) != QLatin1String("RGBA") ||
        element.hasAttribute(QStringLiteral("keyframes")) ||
        (!group && !child.isNull()) || (group && !child.isNull() &&
            (child.tagName() != QLatin1String("layers") || !child.nextSiblingElement().isNull())))
        return unsupported(QStringLiteral("KRA requires RGBA8 paint layers and groups with normal, multiply, screen, darken, lighten, addition or linear dodge compositing"));
    const auto passThrough = element.attribute(QStringLiteral("passthrough"), QStringLiteral("0"));
    if ((passThrough != QLatin1String("0") && passThrough != QLatin1String("1")) ||
        (!group && passThrough != QLatin1String("0")))
        return invalid(QStringLiteral("Invalid KRA group pass-through mode"));
    const auto projectionFlags = element.attribute(QStringLiteral("channelflags"));
    if (!projectionFlags.isEmpty() && (projectionFlags.size() != 4 ||
        std::any_of(projectionFlags.begin(), projectionFlags.end(), [](QChar flag) { return flag != QLatin1Char('0') && flag != QLatin1Char('1'); })))
        return invalid(QStringLiteral("Invalid KRA projection channel flags"));
    const auto paintFlags = element.attribute(QStringLiteral("channellockflags"));
    if (!paintFlags.isEmpty() && (paintFlags.size() != 4 ||
        std::any_of(paintFlags.begin(), paintFlags.end(), [](QChar flag) { return flag != QLatin1Char('0') && flag != QLatin1Char('1'); })))
        return invalid(QStringLiteral("Invalid KRA paint channel flags"));
    const auto visible = element.attribute(QStringLiteral("visible"));
    int x = 0, y = 0, opacity = 255, colorLabel = 0;
    const auto locked = element.attribute(QStringLiteral("locked"), QStringLiteral("0"));
    const auto timeline = element.attribute(QStringLiteral("intimeline"), QStringLiteral("0"));
    if ((visible != QLatin1String("0") && visible != QLatin1String("1")) ||
        (locked != QLatin1String("0") && locked != QLatin1String("1")) ||
        (timeline != QLatin1String("0") && timeline != QLatin1String("1")) ||
        !integer(element, QStringLiteral("opacity"), opacity, QStringLiteral("255")) || opacity < 0 || opacity > 255 ||
        !integer(element, QStringLiteral("colorlabel"), colorLabel, QStringLiteral("0")) ||
        !component(element.attribute(QStringLiteral("filename"))) || !validName(element.attribute(QStringLiteral("name"))) ||
        !integer(element, QStringLiteral("x"), x, QStringLiteral("0")) ||
        !integer(element, QStringLiteral("y"), y, QStringLiteral("0"))) return invalid(QStringLiteral("Invalid KRA layer values"));
    offset = {x, y};
    return {Status::Success, {}};
}

Outcome collectLayerElements(const QDomElement &layers, qsizetype maximum, QList<QDomElement> &elements)
{
    QList<QDomElement> pending;
    const auto append = [&](const QDomElement &container) {
        for (auto child = container.lastChildElement(); !child.isNull(); child = child.previousSiblingElement()) {
            if (pending.size() + elements.size() >= maximum) return false;
            pending.append(child);
        }
        return true;
    };
    if (!append(layers)) return capacity();
    while (!pending.isEmpty()) {
        const auto element = pending.takeLast();
        elements.append(element);
        if (element.attribute(QStringLiteral("nodetype")) == QLatin1String("grouplayer") &&
            !append(element.firstChildElement(QStringLiteral("layers")))) return capacity();
    }
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
    const auto mirrorAxis = image.firstChildElement(QStringLiteral("MirrorAxis"));
    if (!mirrorAxis.nextSiblingElement(QStringLiteral("MirrorAxis")).isNull())
        return invalid(QStringLiteral("Duplicate KRA mirror axis settings"));
    if (!mirrorAxis.isNull()) {
        QDomDocument value;
        value.appendChild(value.importNode(mirrorAxis, true));
        metadata.mirrorAxis = value.toByteArray();
    }
    for (const auto &tag : {QStringLiteral("ColorHistory"), QStringLiteral("GlobalAssistantsColor")}) {
        const auto setting = image.firstChildElement(tag);
        if (!setting.nextSiblingElement(tag).isNull())
            return invalid(QStringLiteral("Duplicate KRA %1 settings").arg(tag));
        if (!setting.isNull()) {
            QDomDocument value;
            value.appendChild(value.importNode(setting, true));
            (tag == QLatin1String("ColorHistory") ? metadata.colorHistory : metadata.assistantsGlobalColor) = value.toByteArray();
        }
    }
    const auto animation = image.firstChildElement(QStringLiteral("animation"));
    if (!animation.nextSiblingElement(QStringLiteral("animation")).isNull())
        return invalid(QStringLiteral("Duplicate KRA animation settings"));
    if (!animation.isNull()) {
        QDomDocument value;
        value.appendChild(value.importNode(animation, true));
        metadata.animation = value.toByteArray();
    }
    const auto animationPath = image.attribute(QStringLiteral("name")) + QStringLiteral("/animation/index.xml");
    if (files.contains(animationPath)) {
        metadata.animationMetadata = files.value(animationPath);
        if (metadata.animationMetadata.size() > limits.xmlBytes) return capacity();
        QDomDocument value;
        if (!value.setContent(metadata.animationMetadata) || value.documentElement().tagName() != QLatin1String("animation-metadata"))
            return invalid(QStringLiteral("Invalid KRA animation metadata"));
    }
    metadata.documentInfo = files.value(QStringLiteral("documentinfo.xml"));
    if (metadata.documentInfo.size() > limits.xmlBytes) return capacity();
    if (files.contains(QStringLiteral("documentinfo.xml"))) {
        QDomDocument info;
        if (!info.setContent(metadata.documentInfo) || info.documentElement().tagName() != QLatin1String("document-info"))
            return invalid(QStringLiteral("Invalid KRA document information"));
    }
    const auto prefix = image.attribute(QStringLiteral("name")) + QStringLiteral("/annotations/");
    metadata.iccProfile = files.value(prefix + QStringLiteral("icc"));
    metadata.profileName = image.attribute(QStringLiteral("profile"));
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
Outcome writeArchive(QIODevice &destination, const QMap<QString, QByteArray> &files, const QDomElement &image,
                     bool compressRaster, Limits limits)
{
    QByteArray bytes;
    ArchiveBuffer buffer(&bytes, limits.archiveBytes);
    if (!buffer.open(QIODevice::WriteOnly)) return ioFailure();
    QuaZip archive(&buffer);
    archive.setAutoClose(false);
    archive.setDataDescriptorWritingEnabled(false);
    archive.setZip64Enabled(false);
    archive.setFileNameCodec("UTF-8");
    archive.setOsCode(3); // ZIP creator code for Unix, shared by all saving hosts.
    if (!archive.open(QuaZip::mdCreate)) return ioFailure();
    auto write = [&](const QString &name, int method, int level) {
        QuaZipFile file(&archive);
        QuaZipNewInfo info(name);
        info.dateTime = QDateTime(QDate(1980, 1, 1), QTime(0, 0), QTimeZone(QTimeZone::UTC));
        info.setPermissions(QFileDevice::ReadOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther);
        if (!file.open(QIODevice::WriteOnly, info, nullptr, 0, method, level)) return false;
        const auto &bytes = files[name];
        if (file.write(bytes) != bytes.size()) return false;
        file.close();
        return file.getZipError() == 0;
    };
    QStringList order;
    QSet<QString> emitted;
    QSet<QString> rasterFiles;
    const auto append = [&](const QString &name) {
        if (files.contains(name) && !emitted.contains(name)) { order.append(name); emitted.insert(name); }
    };
    for (const auto &name : {QStringLiteral("mimetype"), QStringLiteral("maindoc.xml"),
                             QStringLiteral("documentinfo.xml"), QStringLiteral("preview.png")}) append(name);
    const auto folder = image.attribute(QStringLiteral("name"));
    append(folder + QStringLiteral("/layers/.xmp.metadata"));
    QList<QDomElement> layers;
    const auto result = collectLayerElements(image.firstChildElement(QStringLiteral("layers")), limits.layers, layers);
    if (result.code != Status::Success) return result;
    for (const auto &layer : layers) {
        const auto path = folder + QStringLiteral("/layers/") + layer.attribute(QStringLiteral("filename"));
        if (layer.attribute(QStringLiteral("nodetype")) == QLatin1String("paintlayer")) {
            append(path); append(path + QStringLiteral(".defaultpixel"));
            rasterFiles.insert(path); rasterFiles.insert(path + QStringLiteral(".defaultpixel"));
            append(path + QStringLiteral(".icc"));
        }
        append(path + QStringLiteral(".xmp.metadata"));
    }
    append(folder + QStringLiteral("/annotations/exif"));
    append(folder + QStringLiteral("/annotations/icc"));
    const auto annotations = image.firstChildElement(QStringLiteral("Annotations"));
    for (auto annotation = annotations.firstChildElement(); !annotation.isNull(); annotation = annotation.nextSiblingElement())
        append(folder + QStringLiteral("/annotations/") + annotation.attribute(QStringLiteral("type")));
    append(QStringLiteral("mergedimage.png"));
    append(folder + QStringLiteral("/animation/index.xml"));
    // Associated data retained by the format owner follows the owned entries.
    for (auto file = files.cbegin(); file != files.cend(); ++file) append(file.key());
    for (const auto &name : order) {
        const bool plain = name == QLatin1String("mimetype") || name == QLatin1String("mergedimage.png") ||
            (!compressRaster && (rasterFiles.contains(name) || name == folder + QStringLiteral("/animation/index.xml")));
        if (!write(name, name == QLatin1String("mimetype") ? 0 : Z_DEFLATED, plain ? Z_NO_COMPRESSION : Z_DEFAULT_COMPRESSION))
            return buffer.exceeded ? capacity() : ioFailure();
    }
    archive.close();
    if (buffer.exceeded) return capacity();
    if (archive.getZipError() || destination.size() != 0 || !destination.seek(0) || destination.write(bytes) != bytes.size()) return ioFailure();
    return {Status::Success, {}};
}
Outcome prepareLayerDescriptions(QDomDocument &xml, QDomElement image, const RasterImage &value,
                                 QMap<QString, QByteArray> &files, Limits limits, QMap<QUuid, QString> &layerFiles)
{
    const auto folder = image.attribute(QStringLiteral("name"));
    QMap<QUuid, QDomElement> original;
    QSet<QString> filenames;
    auto layers = image.firstChildElement(QStringLiteral("layers"));
    if (layers.isNull()) { layers = xml.createElement(QStringLiteral("layers")); image.appendChild(layers); }
    QList<QDomElement> elements;
    auto result = collectLayerElements(layers, limits.layers, elements);
    if (result.code != Status::Success) return result;
    for (const auto &element : elements) {
        QPoint offset;
        result = layerDescription(element, offset);
        if (result.code != Status::Success) return result;
        const QUuid uuid(element.attribute(QStringLiteral("uuid")));
        const auto filename = element.attribute(QStringLiteral("filename"));
        if (uuid.isNull() || original.contains(uuid) || filenames.contains(filename)) return invalid(QStringLiteral("Invalid KRA layer identity"));
        original.insert(uuid, element.cloneNode(true).toElement());
        filenames.insert(filename);
    }
    const QString metadataPrefix = folder + QStringLiteral("/layers/");
    const QString metadataSuffix = QStringLiteral(".xmp.metadata");
    for (auto it = files.cbegin(); it != files.cend(); ++it) {
        if (it.key().startsWith(metadataPrefix) && it.key().endsWith(metadataSuffix)) {
            const auto name = it.key().mid(metadataPrefix.size(), it.key().size() - metadataPrefix.size() - metadataSuffix.size());
            if (component(name)) filenames.insert(name);
        }
    }
    while (!layers.firstChild().isNull()) layers.removeChild(layers.firstChild());
    QSet<QUuid> uuids;
    QMap<QUuid, QDomElement> prepared;
    for (qsizetype i = value.layers.size(); i-- > 0;) {
        const auto &layer = value.layers[i];
        if (layer.kind == LayerKind::Group && (!layer.tiles.isEmpty() ||
            layer.defaultPixel != std::array<quint8, 4>{} || layer.channelWriteMask != 15))
            return invalid(QStringLiteral("KRA groups derive their pixels from children"));
        if (layer.uuid.isNull() || uuids.contains(layer.uuid) || !validName(layer.name)) return invalid(QStringLiteral("Invalid KRA layer identity or name"));
        uuids.insert(layer.uuid);
        QDomElement element = original.value(layer.uuid);
        if (element.isNull()) {
            element = xml.createElement(QStringLiteral("layer"));
            QString filename;
            for (int suffix = 2; filename.isEmpty(); ++suffix) {
                const auto candidate = QStringLiteral("layer%1").arg(suffix);
                if (!filenames.contains(candidate)) filename = candidate;
            }
            filenames.insert(filename);
            element.setAttribute(QStringLiteral("filename"), filename);
            element.setAttribute(QStringLiteral("x"), QStringLiteral("0"));
            element.setAttribute(QStringLiteral("y"), QStringLiteral("0"));
            element.setAttribute(QStringLiteral("collapsed"), QStringLiteral("0"));
            if (layer.kind == LayerKind::Paint) element.setAttribute(QStringLiteral("onionskin"), QStringLiteral("0"));
        }
        const auto path = folder + QStringLiteral("/layers/") + element.attribute(QStringLiteral("filename"));
        const QPoint offset = layer.position;
        element.setAttribute(QStringLiteral("x"), offset.x());
        element.setAttribute(QStringLiteral("y"), offset.y());
        if (layer.kind == LayerKind::Paint) {
            const QString imageProfile = folder + QStringLiteral("/annotations/icc");
            const QString layerProfile = path + QStringLiteral(".icc");
            if (!files.contains(layerProfile) && files.contains(imageProfile)) files.insert(layerProfile, files.value(imageProfile));
            result = profile(files, layerProfile, image.attribute(QStringLiteral("profile")));
            if (result.code != Status::Success) return result;
            const auto children = element.firstChildElement(QStringLiteral("layers"));
            if (!children.isNull()) element.removeChild(children);
            element.removeAttribute(QStringLiteral("passthrough"));
        } else {
            files.remove(path);
            files.remove(path + QStringLiteral(".defaultpixel"));
            auto children = element.firstChildElement(QStringLiteral("layers"));
            if (children.isNull()) {
                children = xml.createElement(QStringLiteral("layers"));
                element.appendChild(children);
            }
            while (!children.firstChild().isNull()) children.removeChild(children.firstChild());
            element.setAttribute(QStringLiteral("passthrough"), layer.passThrough ? QStringLiteral("1") : QStringLiteral("0"));
            if (!element.hasAttribute(QStringLiteral("profile")))
                element.setAttribute(QStringLiteral("profile"), image.attribute(QStringLiteral("profile")));
        }
        element.setAttribute(QStringLiteral("name"), layer.name);
        element.setAttribute(QStringLiteral("uuid"), layer.uuid.toString());
        element.setAttribute(QStringLiteral("visible"), layer.visible ? QStringLiteral("1") : QStringLiteral("0"));
        if (i == value.activeLayer) element.setAttribute(QStringLiteral("selected"), QStringLiteral("true"));
        else element.removeAttribute(QStringLiteral("selected"));
        element.setAttribute(QStringLiteral("nodetype"), layer.kind == LayerKind::Paint ? QStringLiteral("paintlayer") : QStringLiteral("grouplayer"));
        element.setAttribute(QStringLiteral("compositeop"), layer.blendMode == LayerBlendMode::Multiply ? QStringLiteral("multiply") :
                             layer.blendMode == LayerBlendMode::Screen ? QStringLiteral("screen") :
                             layer.blendMode == LayerBlendMode::Darken ? QStringLiteral("darken") :
                             layer.blendMode == LayerBlendMode::Lighten ? QStringLiteral("lighten") :
                             layer.blendMode == LayerBlendMode::Addition ? QStringLiteral("add") :
                             layer.blendMode == LayerBlendMode::LinearDodge ? QStringLiteral("linear_dodge") : QStringLiteral("normal"));
        element.setAttribute(QStringLiteral("opacity"), int(layer.opacity));
        element.setAttribute(QStringLiteral("colorspacename"), QStringLiteral("RGBA"));
        element.setAttribute(QStringLiteral("locked"), layer.userLocked ? QStringLiteral("1") : QStringLiteral("0"));
        element.setAttribute(QStringLiteral("intimeline"), layer.pinnedToTimeline ? QStringLiteral("1") : QStringLiteral("0"));
        element.setAttribute(QStringLiteral("colorlabel"), layer.colorLabelIndex);
        if (layer.projectionChannelMask > 15) return invalid(QStringLiteral("Invalid KRA projection channel mask"));
        QString projectionFlags;
        if (layer.projectionChannelMask != 15) {
            for (int channel = 0; channel < 4; ++channel)
                projectionFlags += layer.projectionChannelMask & (1u << (channel == 3 ? 3 : 2 - channel)) ? QLatin1Char('1') : QLatin1Char('0');
        }
        element.setAttribute(QStringLiteral("channelflags"), projectionFlags);
        if (layer.channelWriteMask > 15) return invalid(QStringLiteral("Invalid KRA paint channel mask"));
        if (layer.channelWriteMask != 15) {
            QString paintFlags;
            for (int channel = 0; channel < 4; ++channel)
                paintFlags += layer.channelWriteMask & (1u << (channel == 3 ? 3 : 2 - channel)) ? QLatin1Char('1') : QLatin1Char('0');
            element.setAttribute(QStringLiteral("channellockflags"), paintFlags);
        } else {
            if (layer.kind == LayerKind::Paint) element.setAttribute(QStringLiteral("channellockflags"), QString());
            else element.removeAttribute(QStringLiteral("channellockflags"));
        }
        prepared.insert(layer.uuid, element);
    }
    for (qsizetype i = value.layers.size(); i-- > 0;) {
        const auto &layer = value.layers[i];
        if (layer.parentUuid.isNull()) layers.appendChild(prepared.value(layer.uuid));
        else prepared.value(layer.parentUuid).firstChildElement(QStringLiteral("layers")).appendChild(prepared.value(layer.uuid));
    }
    for (auto old = original.cbegin(); old != original.cend(); ++old) {
        if (!uuids.contains(old.key())) {
            const auto path = folder + QStringLiteral("/layers/") + old.value().attribute(QStringLiteral("filename"));
            files.remove(path); files.remove(path + QStringLiteral(".defaultpixel")); files.remove(path + QStringLiteral(".icc"));
            files.remove(path + QStringLiteral(".xmp.metadata"));
        }
    }
    image.setAttribute(QStringLiteral("width"), value.size.width());
    image.setAttribute(QStringLiteral("height"), value.size.height());
    for (auto it = prepared.cbegin(); it != prepared.cend(); ++it)
        layerFiles.insert(it.key(), it.value().attribute(QStringLiteral("filename")));
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
    if (!validName(metadata.producerVersion)) return invalid(QStringLiteral("Invalid KRA producer version"));
    if (!validName(metadata.profileName)) return invalid(QStringLiteral("Invalid KRA profile name"));
    if (metadata.documentInfo.size() > limits.xmlBytes || metadata.mirrorAxis.size() > limits.xmlBytes) return capacity();
    QDomDocument mirrorAxis;
    if (!metadata.mirrorAxis.isEmpty() &&
        (!mirrorAxis.setContent(metadata.mirrorAxis) || mirrorAxis.documentElement().tagName() != QLatin1String("MirrorAxis")))
        return invalid(QStringLiteral("Invalid KRA mirror axis settings"));
    QDomDocument info;
    if (!metadata.documentInfo.isEmpty()) {
        if (!info.setContent(metadata.documentInfo) || info.documentElement().tagName() != QLatin1String("document-info"))
            return invalid(QStringLiteral("Invalid KRA document information"));
    }
    if (metadata.animation.size() > limits.xmlBytes || metadata.animationMetadata.size() > limits.xmlBytes) return capacity();
    QDomDocument animation, animationMetadata;
    if ((!metadata.animation.isEmpty() &&
         (!animation.setContent(metadata.animation) || animation.documentElement().tagName() != QLatin1String("animation"))) ||
        (!metadata.animationMetadata.isEmpty() &&
         (!animationMetadata.setContent(metadata.animationMetadata) ||
          animationMetadata.documentElement().tagName() != QLatin1String("animation-metadata"))))
        return invalid(QStringLiteral("Invalid KRA animation settings"));
    QDomDocument xml;
    QDomElement image;
    if (document.description.isEmpty()) image = createDescription(xml);
    else {
        result = description(document.description, xml, image);
        if (result.code != Status::Success) return result;
    }
    if (!metadata.producerVersion.isEmpty()) xml.documentElement().setAttribute(QStringLiteral("kritaVersion"), metadata.producerVersion);
    for (auto old = image.firstChildElement(QStringLiteral("MirrorAxis")); !old.isNull();) {
        const auto next = old.nextSiblingElement(QStringLiteral("MirrorAxis"));
        image.removeChild(old);
        old = next;
    }
    if (!metadata.mirrorAxis.isEmpty()) image.appendChild(xml.importNode(mirrorAxis.documentElement(), true));
    for (const auto &tag : {QStringLiteral("ColorHistory"), QStringLiteral("GlobalAssistantsColor")}) {
        const auto &bytes = tag == QLatin1String("ColorHistory") ? metadata.colorHistory : metadata.assistantsGlobalColor;
        if (bytes.size() > limits.xmlBytes) return capacity();
        QDomDocument value;
        if (!bytes.isEmpty() && (!value.setContent(bytes) || value.documentElement().tagName() != tag))
            return invalid(QStringLiteral("Invalid KRA %1 settings").arg(tag));
        for (auto old = image.firstChildElement(tag); !old.isNull();) {
            const auto next = old.nextSiblingElement(tag);
            image.removeChild(old);
            old = next;
        }
        if (!bytes.isEmpty()) image.appendChild(xml.importNode(value.documentElement(), true));
    }
    if (!metadata.animation.isEmpty()) {
        const auto old = image.firstChildElement(QStringLiteral("animation"));
        if (!old.isNull()) image.removeChild(old);
        image.appendChild(xml.importNode(animation.documentElement(), true));
    }
    auto files = document.files;
    auto folder = image.attribute(QStringLiteral("name"));
    const auto title = info.documentElement().firstChildElement(QStringLiteral("about"))
        .firstChildElement(QStringLiteral("title")).text();
    const auto oldPrefix = folder + QLatin1Char('/');
    // Reserve raster filenames allocated after the metadata update, including retained layer names.
    const bool usableTitle = component(title) &&
        validPath(title + QStringLiteral("/layers/layer%1.defaultpixel").arg(2 * limits.layers)) &&
        std::all_of(metadata.annotations.cbegin(), metadata.annotations.cend(), [&](const Annotation &annotation) {
            return validPath(title + QStringLiteral("/annotations/") + annotation.type);
        }) && std::all_of(files.keyBegin(), files.keyEnd(), [&](const QString &name) {
            return !name.startsWith(oldPrefix) || validPath(title + name.mid(folder.size()));
        });
    if (usableTitle && title != folder) {
        if (std::any_of(files.keyBegin(), files.keyEnd(), [&](const QString &name) { return name.startsWith(title + QLatin1Char('/')); }))
            return invalid(QStringLiteral("KRA image rename conflicts with an associated file"));
        const auto originalFiles = files;
        for (auto file = originalFiles.cbegin(); file != originalFiles.cend(); ++file) {
            if (!file.key().startsWith(oldPrefix)) continue;
            const auto renamed = title + file.key().mid(folder.size());
            files.remove(file.key());
            files.insert(renamed, file.value());
        }
        folder = title;
        image.setAttribute(QStringLiteral("name"), folder);
    }
    if (!metadata.animationMetadata.isEmpty())
        files.insert(folder + QStringLiteral("/animation/index.xml"), metadata.animationMetadata);
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
    if (!metadata.profileName.isEmpty()) image.setAttribute(QStringLiteral("profile"), metadata.profileName);
    if (metadata.documentInfo.isEmpty()) files.remove(QStringLiteral("documentinfo.xml"));
    else files.insert(QStringLiteral("documentinfo.xml"), metadata.documentInfo);
    image.setAttribute(QStringLiteral("x-res"), QString::number(metadata.resolution.x(), 'g', 17));
    image.setAttribute(QStringLiteral("y-res"), QString::number(metadata.resolution.y(), 'g', 17));
    if (metadata.layerXmp || metadata.layerCollapsed) {
        const auto &value = document.image;
        if (value.size.width() <= 0 || value.size.height() <= 0 || value.size.width() > RgbaTile::CoordinateLimit ||
            value.size.height() > RgbaTile::CoordinateLimit || value.activeLayer < 0 ||
            (value.layers.isEmpty() ? value.activeLayer != 0 : value.activeLayer >= value.layers.size()) ||
            !validRasterLayerHierarchy(value.layers))
            return invalid(QStringLiteral("Invalid KRA image structure for layer metadata"));
        if (value.layers.size() > limits.layers) return capacity();
        QMap<QUuid, QString> layerFiles;
        result = prepareLayerDescriptions(xml, image, value, files, limits, layerFiles);
        if (result.code != Status::Success) return result;
        if (metadata.layerXmp) {
            const QString prefix = folder + QStringLiteral("/layers/");
            for (const auto &filename : layerFiles) files.remove(prefix + filename + QStringLiteral(".xmp.metadata"));
            for (auto it = metadata.layerXmp->cbegin(); it != metadata.layerXmp->cend(); ++it) {
                if (it.key().isNull() || !layerFiles.contains(it.key())) return invalid(QStringLiteral("KRA metadata requires a current layer identity"));
                if (!it.value().isEmpty()) files.insert(prefix + layerFiles.value(it.key()) + QStringLiteral(".xmp.metadata"), it.value());
            }
        }
        if (metadata.layerCollapsed) {
            for (auto it = metadata.layerCollapsed->cbegin(); it != metadata.layerCollapsed->cend(); ++it)
                if (it.key().isNull() || !layerFiles.contains(it.key()))
                    return invalid(QStringLiteral("KRA collapsed state requires a current layer identity"));
            QList<QDomElement> elements;
            result = collectLayerElements(image.firstChildElement(QStringLiteral("layers")), limits.layers, elements);
            if (result.code != Status::Success) return result;
            for (auto element : elements)
                element.setAttribute(QStringLiteral("collapsed"), metadata.layerCollapsed->value(QUuid(element.attribute(QStringLiteral("uuid")))) ? 1 : 0);
        }
    }
    if (metadata.rootLayerXmp) {
        const QString path = folder + QStringLiteral("/layers/.xmp.metadata");
        if (metadata.rootLayerXmp->isEmpty()) files.remove(path);
        else files.insert(path, *metadata.rootLayerXmp);
    }
    // Native image fields follow the same semantic order on both image owners.
    QDomNode previous = image.firstChildElement(QStringLiteral("layers"));
    for (const auto &tag : {QStringLiteral("ProjectionBackgroundColor"), QStringLiteral("GlobalAssistantsColor"),
                            QStringLiteral("MirrorAxis"), QStringLiteral("ColorHistory"), QStringLiteral("Palettes"),
                            QStringLiteral("resources"), QStringLiteral("animation"), QStringLiteral("Annotations")}) {
        auto element = image.firstChildElement(tag);
        if (!element.isNull()) { image.insertAfter(element, previous); previous = element; }
    }
    const auto bytes = serializeDescription(xml);
    files.insert(QStringLiteral("maindoc.xml"), bytes);
    result = validateMetadata(bytes, files, limits);
    if (result.code != Status::Success) return result;
    document.description = bytes;
    document.files = std::move(files);
    document.compressRaster = metadata.compressRaster;
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
    QList<QDomElement> elements;
    result = collectLayerElements(layers, limits.layers, elements);
    if (result.code != Status::Success) return result;
    QSet<QString> filenames;
    QSet<QUuid> uuids;
    for (const auto &element : elements) {
        QPoint offset;
        result = layerDescription(element, offset);
        if (result.code != Status::Success) return result;
        const QUuid uuid(element.attribute(QStringLiteral("uuid")));
        const auto filename = element.attribute(QStringLiteral("filename"));
        if ((element.hasAttribute(QStringLiteral("uuid")) && (uuid.isNull() || uuids.contains(uuid))) || filenames.contains(filename))
            return invalid(QStringLiteral("Invalid KRA layer identity"));
        uuids.insert(uuid);
        filenames.insert(filename);
        const auto layerProfile = folder + QStringLiteral("/layers/") + filename + QStringLiteral(".icc");
        if (element.attribute(QStringLiteral("nodetype")) == QLatin1String("paintlayer") || files.contains(layerProfile)) {
            result = profile(files, layerProfile, image.attribute(QStringLiteral("profile")));
            if (result.code != Status::Success) return result;
        } else if (element.attribute(QStringLiteral("profile"), image.attribute(QStringLiteral("profile"))) != image.attribute(QStringLiteral("profile"))) {
            return unsupported(QStringLiteral("KRA groups require the image's sRGB profile"));
        }
    }
    ImageMetadata metadata;
    return extractMetadata(image, files, limits, metadata);
}

MetadataOutcome RasterCodec::readImageMetadata(const QByteArray &bytes, const QMap<QString, QByteArray> &files, Limits limits)
{
    const auto validated = validateMetadata(bytes, files, limits);
    if (validated.code != Status::Success) return {validated.code, {}, validated.diagnostic, {}};
    QDomDocument xml;
    QDomElement image;
    if (bytes.isEmpty()) image = createDescription(xml);
    else description(bytes, xml, image);
    ImageMetadata metadata;
    const auto extracted = extractMetadata(image, files, limits, metadata);
    if (extracted.code != Status::Success) return {extracted.code, {}, extracted.diagnostic, {}};
    QList<QDomElement> elements;
    const auto collected = collectLayerElements(image.firstChildElement(QStringLiteral("layers")), limits.layers, elements);
    if (collected.code != Status::Success) return {collected.code, {}, collected.diagnostic, {}};
    QHash<QUuid, int> labels;
    const QString rootXmp = image.attribute(QStringLiteral("name")) + QStringLiteral("/layers/.xmp.metadata");
    if (files.contains(rootXmp)) metadata.rootLayerXmp = files.value(rootXmp);
    metadata.layerXmp.emplace();
    metadata.layerCollapsed.emplace();
    for (const auto &element : elements) {
        const QUuid uuid(element.attribute(QStringLiteral("uuid")));
        if (!uuid.isNull()) {
            labels.insert(uuid, element.attribute(QStringLiteral("colorlabel"), QStringLiteral("0")).toInt());
            metadata.layerCollapsed->insert(uuid, element.attribute(QStringLiteral("collapsed"), QStringLiteral("0")) != QLatin1String("0"));
            const QString path = image.attribute(QStringLiteral("name")) + QStringLiteral("/layers/") + element.attribute(QStringLiteral("filename")) + QStringLiteral(".xmp.metadata");
            if (files.contains(path)) metadata.layerXmp->insert(uuid, files.value(path));
        }
    }
    return {Status::Success, std::move(metadata), {}, std::move(labels)};
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
    const auto background = image.firstChildElement(QStringLiteral("ProjectionBackgroundColor"));
    if (!background.isNull()) {
        const auto bgra = QByteArray::fromBase64(background.attribute(QStringLiteral("ColorData")).toLatin1());
        document.image.projectionBackground = {quint8(bgra[2]), quint8(bgra[1]), quint8(bgra[0]), quint8(bgra[3])};
    }
    const auto folder = image.attribute(QStringLiteral("name"));
    result = profile(document.files, folder + QStringLiteral("/annotations/icc"), image.attribute(QStringLiteral("profile")));
    if (result.code != Status::Success) return fail(result);
    QSet<QUuid> uuids;
    QSet<QString> filenames;
    qsizetype rasterBytes = 0;
    int selected = -1;
    bool generatedUuids = false;
    const auto layers = image.firstChildElement(QStringLiteral("layers"));
    QList<QDomElement> elements;
    result = collectLayerElements(layers, limits.layers, elements);
    if (result.code != Status::Success) return fail(result);
    for (auto element : elements) {
        QPoint offset;
        result = layerDescription(element, offset);
        if (result.code != Status::Success) return fail(result);
        RasterLayer layer;
        layer.kind = element.attribute(QStringLiteral("nodetype")) == QLatin1String("grouplayer") ? LayerKind::Group : LayerKind::Paint;
        layer.passThrough = element.attribute(QStringLiteral("passthrough"), QStringLiteral("0")) == QLatin1String("1");
        layer.blendMode = element.attribute(QStringLiteral("compositeop")) == QLatin1String("multiply") ? LayerBlendMode::Multiply :
            element.attribute(QStringLiteral("compositeop")) == QLatin1String("screen") ? LayerBlendMode::Screen :
            element.attribute(QStringLiteral("compositeop")) == QLatin1String("darken") ? LayerBlendMode::Darken :
            element.attribute(QStringLiteral("compositeop")) == QLatin1String("lighten") ? LayerBlendMode::Lighten :
            element.attribute(QStringLiteral("compositeop")) == QLatin1String("add") ? LayerBlendMode::Addition :
            element.attribute(QStringLiteral("compositeop")) == QLatin1String("linear_dodge") ? LayerBlendMode::LinearDodge : LayerBlendMode::Normal;
        const auto parent = element.parentNode().parentNode().toElement();
        if (parent.tagName() == QLatin1String("layer")) layer.parentUuid = QUuid(parent.attribute(QStringLiteral("uuid")));
        layer.position = offset;
        layer.name = element.attribute(QStringLiteral("name"));
        layer.visible = element.attribute(QStringLiteral("visible")) == QLatin1String("1");
        layer.opacity = quint8(element.attribute(QStringLiteral("opacity"), QStringLiteral("255")).toUInt());
        layer.userLocked = element.attribute(QStringLiteral("locked"), QStringLiteral("0")) == QLatin1String("1");
        layer.pinnedToTimeline = element.attribute(QStringLiteral("intimeline"), QStringLiteral("0")) == QLatin1String("1");
        layer.colorLabelIndex = element.attribute(QStringLiteral("colorlabel"), QStringLiteral("0")).toInt();
        const auto projectionFlags = element.attribute(QStringLiteral("channelflags"));
        if (!projectionFlags.isEmpty()) {
            layer.projectionChannelMask = 0;
            for (int channel = 0; channel < 4; ++channel)
                if (projectionFlags[channel] == QLatin1Char('1')) layer.projectionChannelMask |= 1u << (channel == 3 ? 3 : 2 - channel);
        }
        const auto paintFlags = element.attribute(QStringLiteral("channellockflags"), QStringLiteral("1111"));
        if (layer.kind == LayerKind::Paint && !paintFlags.isEmpty()) {
            layer.channelWriteMask = 0;
            for (int channel = 0; channel < 4; ++channel)
                if (paintFlags[channel] == QLatin1Char('1')) layer.channelWriteMask |= 1u << (channel == 3 ? 3 : 2 - channel);
        }
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
        if (layer.kind == LayerKind::Paint) {
            if (!document.files.contains(path)) return fail(invalid(QStringLiteral("Missing KRA layer pixels")));
            result = profile(document.files, path + QStringLiteral(".icc"), image.attribute(QStringLiteral("profile")));
            if (result.code != Status::Success) return fail(result);
            if (document.files.contains(path + QStringLiteral(".defaultpixel"))) {
                const auto pixel = document.files.value(path + QStringLiteral(".defaultpixel"));
                if (pixel.size() != 4) return fail(invalid(QStringLiteral("Invalid KRA default pixel")));
                layer.defaultPixel = {quint8(pixel[2]), quint8(pixel[1]), quint8(pixel[0]), quint8(pixel[3])};
            }
            result = Detail::readTiles(document.files.value(path), layer.defaultPixel, offset, limits.rasterBytes - rasterBytes,
                                       layer.tiles, &layer.writtenAreas);
            if (result.code != Status::Success) return fail(result);
            rasterBytes += layer.tiles.size() * RgbaTile::ByteCount;
            document.files.remove(path);
            document.files.remove(path + QStringLiteral(".defaultpixel"));
        }
        if (element.attribute(QStringLiteral("selected")) == QLatin1String("true")) {
            if (selected >= 0) return fail(invalid(QStringLiteral("Multiple selected KRA layers")));
            selected = document.image.layers.size();
        }
        document.image.layers.prepend(std::move(layer));
    }
    if (!validRasterLayerHierarchy(document.image.layers)) return fail(invalid(QStringLiteral("Invalid KRA layer hierarchy")));
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
    if (!validRasterLayerHierarchy(value.layers)) return invalid(QStringLiteral("Invalid KRA layer hierarchy"));
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
    auto background = image.firstChildElement(QStringLiteral("ProjectionBackgroundColor"));
    if (background.isNull()) {
        background = xml.createElement(QStringLiteral("ProjectionBackgroundColor"));
        image.appendChild(background);
    }
    background.setAttribute(QStringLiteral("ColorData"), QString::fromLatin1(Detail::bgra(value.projectionBackground).toBase64()));
    const auto imageProfile = folder + QStringLiteral("/annotations/icc");
    if (!files.contains(imageProfile)) files.insert(imageProfile, QColorSpace(QColorSpace::SRgb).iccProfile());
    result = profile(files, imageProfile, image.attribute(QStringLiteral("profile")));
    if (result.code != Status::Success) return result;
    QMap<QUuid, QString> layerFiles;
    result = prepareLayerDescriptions(xml, image, value, files, limits, layerFiles);
    if (result.code != Status::Success) return result;
    qsizetype rasterBytes = 0;
    for (qsizetype i = value.layers.size(); i-- > 0;) {
        const auto &layer = value.layers[i];
        if (layer.kind != LayerKind::Paint) continue;
        const auto path = folder + QStringLiteral("/layers/") + layerFiles.value(layer.uuid);
        const QPoint offset = layer.position;
        QByteArray pixels;
        result = Detail::writeTiles(layer.tiles, layer.defaultPixel, offset, limits.rasterBytes - rasterBytes, pixels, layer.writtenAreas);
        if (result.code != Status::Success) return result;
        rasterBytes += layer.tiles.size() * RgbaTile::ByteCount;
        files.insert(path, std::move(pixels));
        files.insert(path + QStringLiteral(".defaultpixel"), Detail::bgra(layer.defaultPixel));
    }
    files.insert(QStringLiteral("maindoc.xml"), serializeDescription(xml));
    if (files.value(QStringLiteral("maindoc.xml")).size() > limits.xmlBytes) return capacity();
    files.insert(QStringLiteral("mimetype"), QByteArrayLiteral("application/x-krita"));
    const auto density = [](qreal dpi) {
        return int(qBound(1.0, std::floor(dpi / 0.0254 + 0.5), double(std::numeric_limits<int>::max())));
    };
    const int densityX = density(image.attribute(QStringLiteral("x-res"), QStringLiteral("72")).toDouble());
    const int densityY = density(image.attribute(QStringLiteral("y-res"), QStringLiteral("72")).toDouble());
    auto png = [&](QImage image, const QString &name) {
        image.setColorSpace({});
        image.setDotsPerMeterX(densityX);
        image.setDotsPerMeterY(densityY);
        QByteArray bytes;
        QBuffer buffer(&bytes);
        if (!buffer.open(QIODevice::WriteOnly)) return false;
        QImageWriter encoder(&buffer, "PNG");
        // Qt's PNG scale maps 33 to the KRA merged-image compression level 3.
        if (name == QLatin1String("mergedimage.png")) encoder.setCompression(33);
        if (!encoder.write(image)) return false;
        files.insert(name, std::move(bytes));
        return true;
    };
    if (!png(projection, QStringLiteral("mergedimage.png")) ||
        !png(Detail::makePreview(projection), QStringLiteral("preview.png"))) return ioFailure();
    result = validateFiles(files, limits);
    return result.code == Status::Success ? writeArchive(destination, files, image, document.compressRaster, limits) : result;
}
}
