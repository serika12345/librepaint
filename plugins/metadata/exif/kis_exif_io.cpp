/*
 *  SPDX-FileCopyrightText: 2007 Cyrille Berger <cberger@cberger.net>
 *  SPDX-FileCopyrightText: 2021 L. E. Segovia <amy@amyspark.me>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "kis_exif_io.h"

#include <exiv2/error.hpp>
#include <exiv2/exif.hpp>

#include <QByteArray>
#include <QDate>
#include <QDateTime>
#include <QIODevice>
#include <QTextCodec>
#include <QTime>
#include <QVariant>
#include <QtEndian>

#include <limits>

#include <kis_debug.h>
#include <kis_exiv2_common.h>
#include <kis_meta_data_entry.h>
#include <kis_meta_data_schema.h>
#include <kis_meta_data_schema_registry.h>
#include <kis_meta_data_store.h>
#include <kis_meta_data_tags.h>
#include <kis_meta_data_value.h>

// ---- Exception conversion functions ---- //

// convert ExifVersion and FlashpixVersion to a KisMetaData value
#if EXIV2_TEST_VERSION(0,28,0)
KisMetaData::Value exifVersionToKMDValue(const Exiv2::Value::UniquePtr value)
#else
KisMetaData::Value exifVersionToKMDValue(const Exiv2::Value::AutoPtr value)
#endif
{
    const Exiv2::DataValue *dvalue = dynamic_cast<const Exiv2::DataValue *>(&*value);
    if (dvalue) {
        Q_ASSERT(dvalue);
        QByteArray array(dvalue->count(), 0);
        dvalue->copy((Exiv2::byte *)array.data());
        return KisMetaData::Value(QString(array));
    } else {
        Q_ASSERT(value->typeId() == Exiv2::asciiString);
        return KisMetaData::Value(QString::fromLatin1(value->toString().c_str()));
    }
}

// convert from KisMetaData value to ExifVersion and FlashpixVersion
Exiv2::Value *kmdValueToExifVersion(const KisMetaData::Value &value)
{
    Exiv2::DataValue *dvalue = new Exiv2::DataValue;
    QString ver = value.asVariant().toString();
    dvalue->read((const Exiv2::byte *)ver.toLatin1().constData(), ver.size());
    return dvalue;
}

// Convert an exif array of integer string to a KisMetaData array of integer
#if EXIV2_TEST_VERSION(0,28,0)
KisMetaData::Value exifArrayToKMDIntOrderedArray(const Exiv2::Value::UniquePtr value)
#else
KisMetaData::Value exifArrayToKMDIntOrderedArray(const Exiv2::Value::AutoPtr value)
#endif
{
    QList<KisMetaData::Value> v;
    const Exiv2::DataValue *dvalue = dynamic_cast<const Exiv2::DataValue *>(&*value);
    if (dvalue) {
#if EXIV2_TEST_VERSION(0,28,0)
        for (size_t i = 0; i < dvalue->count(); i++) {
            v.push_back({(int)dvalue->toUint32(i)});
#else
        for (long i = 0; i < dvalue->count(); i++) {
            v.push_back({(int)dvalue->toLong(i)});
#endif
        }
    } else {
        Q_ASSERT(value->typeId() == Exiv2::asciiString);
        QString str = QString::fromLatin1(value->toString().c_str());
        v.push_back(KisMetaData::Value(str.toInt()));
    }
    return KisMetaData::Value(v, KisMetaData::Value::OrderedArray);
}

// Convert a KisMetaData array of integer to an exif array of integer string
Exiv2::Value *kmdIntOrderedArrayToExifArray(const KisMetaData::Value &value)
{
    std::vector<Exiv2::byte> v;
    for (const KisMetaData::Value &it : value.asArray()) {
        v.push_back(static_cast<uint8_t>(it.asVariant().toInt(0)));
    }
    return new Exiv2::DataValue(v.data(), static_cast<long>(v.size()));
}

#if EXIV2_TEST_VERSION(0,28,0)
QDateTime exivValueToDateTime(const Exiv2::Value::UniquePtr value)
#else
QDateTime exivValueToDateTime(const Exiv2::Value::AutoPtr value)
#endif
{
    return QDateTime::fromString(value->toString().c_str(), Qt::ISODate);
}

template<typename T>
T readInteger(const char *data, Exiv2::ByteOrder order)
{
    switch (order) {
    case Exiv2::invalidByteOrder:
        return 0;
    case Exiv2::littleEndian:
        return qFromLittleEndian<T>(reinterpret_cast<const uchar *>(data));
    case Exiv2::bigEndian:
        return qFromBigEndian<T>(reinterpret_cast<const uchar *>(data));
    }
    warnKrita << "KisExifIO: unknown byte order";
    return 0;
}

Exiv2::ByteOrder invertByteOrder(Exiv2::ByteOrder order)
{
    switch (order) {
    case Exiv2::littleEndian:
        return Exiv2::bigEndian;
    case Exiv2::bigEndian:
        return Exiv2::littleEndian;
    case Exiv2::invalidByteOrder:
        warnKrita << "KisExifIO: Can't invert Exiv2::invalidByteOrder";
        return Exiv2::invalidByteOrder;
    }
    return Exiv2::invalidByteOrder;
}

struct StructuredExifDimensions {
    int columns = 0;
    int rows = 0;
    qsizetype valueCount = 0;
};

bool copyDataValue(const Exiv2::Value &value, QByteArray *array)
{
    const Exiv2::DataValue *dataValue = dynamic_cast<const Exiv2::DataValue *>(&value);
    if (!dataValue) {
        return false;
    }

    const quint64 byteCount = static_cast<quint64>(dataValue->count());
    if (byteCount > static_cast<quint64>(std::numeric_limits<qsizetype>::max())) {
        return false;
    }

    array->resize(static_cast<qsizetype>(byteCount));
    dataValue->copy(reinterpret_cast<Exiv2::byte *>(array->data()));
    return true;
}

bool readStructuredDimensions(const QByteArray &array,
                              Exiv2::ByteOrder byteOrder,
                              quint64 bytesPerValue,
                              bool allowsColumnNames,
                              StructuredExifDimensions *dimensions)
{
    if (byteOrder == Exiv2::invalidByteOrder || array.size() < 4) {
        return false;
    }

    const quint16 columns = readInteger<quint16>(array.constData(), byteOrder);
    const quint16 rows = readInteger<quint16>(array.constData() + 2, byteOrder);
    if (columns == 0 || rows == 0) {
        return false;
    }

    const quint64 valueCount = static_cast<quint64>(columns) * rows;
    const quint64 valueBytes = valueCount * bytesPerValue;
    const quint64 availableBytes = static_cast<quint64>(array.size() - 4);
    if (valueBytes > availableBytes || (!allowsColumnNames && valueBytes != availableBytes)
        || valueCount > static_cast<quint64>(std::numeric_limits<qsizetype>::max())) {
        return false;
    }

    dimensions->columns = columns;
    dimensions->rows = rows;
    dimensions->valueCount = static_cast<qsizetype>(valueCount);
    return true;
}

KisMetaData::Value oecfArrayToMetadata(const QByteArray &array, Exiv2::ByteOrder byteOrder)
{
    StructuredExifDimensions dimensions;
    if (!readStructuredDimensions(array, byteOrder, 8, true, &dimensions)) {
        return {};
    }

    const qsizetype valuesOffset = array.size() - dimensions.valueCount * 8;
    QList<KisMetaData::Value> names;
    names.reserve(dimensions.columns);

    if (valuesOffset == 4) {
        for (int column = 0; column < dimensions.columns; ++column) {
            names.append(KisMetaData::Value(QString()));
        }
    } else {
        qsizetype nameOffset = 4;
        for (int column = 0; column < dimensions.columns; ++column) {
            const qsizetype terminatorOffset = array.indexOf('\0', nameOffset);
            if (terminatorOffset < nameOffset || terminatorOffset >= valuesOffset) {
                return {};
            }
            names.append(KisMetaData::Value(
                QString::fromLatin1(array.constData() + nameOffset, terminatorOffset - nameOffset)));
            nameOffset = terminatorOffset + 1;
        }
        if (nameOffset != valuesOffset) {
            return {};
        }
    }

    QList<KisMetaData::Value> values;
    values.reserve(dimensions.valueCount);
    for (qsizetype offset = valuesOffset; offset < array.size(); offset += 8) {
        values.append(KisMetaData::Value(KisMetaData::Rational(
            readInteger<qint32>(array.constData() + offset, byteOrder),
            readInteger<qint32>(array.constData() + offset + 4, byteOrder))));
    }

    QMap<QString, KisMetaData::Value> structure;
    structure["Columns"] = KisMetaData::Value(dimensions.columns);
    structure["Rows"] = KisMetaData::Value(dimensions.rows);
    structure["Names"] = KisMetaData::Value(names, KisMetaData::Value::OrderedArray);
    structure["Values"] = KisMetaData::Value(values, KisMetaData::Value::OrderedArray);
    return KisMetaData::Value(structure);
}

KisMetaData::Value cfaArrayToMetadata(const QByteArray &array, Exiv2::ByteOrder byteOrder)
{
    StructuredExifDimensions dimensions;
    if (!readStructuredDimensions(array, byteOrder, 1, false, &dimensions)) {
        return {};
    }

    QList<KisMetaData::Value> values;
    values.reserve(dimensions.valueCount);
    for (qsizetype offset = 4; offset < array.size(); ++offset) {
        values.append(KisMetaData::Value(static_cast<quint8>(array.at(offset))));
    }

    QMap<QString, KisMetaData::Value> structure;
    structure["Columns"] = KisMetaData::Value(dimensions.columns);
    structure["Rows"] = KisMetaData::Value(dimensions.rows);
    structure["Values"] = KisMetaData::Value(values, KisMetaData::Value::OrderedArray);
    return KisMetaData::Value(structure);
}

KisMetaData::Value exifOECFToKMDOECFStructure(const Exiv2::Value &value, Exiv2::ByteOrder order)
{
    QByteArray array;
    if (!copyDataValue(value, &array)) {
        return {};
    }

    KisMetaData::Value result = oecfArrayToMetadata(array, order);
    if (result.type() == KisMetaData::Value::Invalid) {
        result = oecfArrayToMetadata(array, invertByteOrder(order));
    }
    if (result.type() == KisMetaData::Value::Invalid) {
        warnMetaData << "Ignoring invalid EXIF OECF data with" << array.size() << "bytes";
    }
    return result;
}

Exiv2::Value *kmdOECFStructureToExifOECF(const KisMetaData::Value &value)
{
    QMap<QString, KisMetaData::Value> oecfStructure = value.asStructure();
    const quint16 columns = static_cast<quint16>(oecfStructure["Columns"].asVariant().toUInt());
    const quint16 rows = static_cast<quint16>(oecfStructure["Rows"].asVariant().toUInt());

    QList<KisMetaData::Value> names = oecfStructure["Names"].asArray();
    QList<KisMetaData::Value> values = oecfStructure["Values"].asArray();
    Q_ASSERT(columns * rows == values.size());
    int length = 4 + rows * columns * 8; // The 4 byte for storing rows/columns and the rows*columns*sizeof(rational)
    bool saveNames = (!names.empty() && names[0].asVariant().toString().size() > 0);
    if (saveNames) {
        for (int i = 0; i < columns; i++) {
            length += names[i].asVariant().toString().size() + 1;
        }
    }
    QByteArray array(length, 0);
    (reinterpret_cast<quint16 *>(array.data()))[0] = columns;
    (reinterpret_cast<quint16 *>(array.data()))[1] = rows;
    int index = 4;
    if (saveNames) {
        for (int i = 0; i < columns; i++) {
            QByteArray name = names[i].asVariant().toString().toLatin1();
            name.append((char)0);
            memcpy(array.data() + index, name.data(), static_cast<size_t>(name.size()));
            index += name.size();
        }
    }
    qint32 *dataIt = reinterpret_cast<qint32 *>(array.data() + index);
    for (const KisMetaData::Value &it : values) {
        dataIt[0] = it.asRational().numerator;
        dataIt[1] = it.asRational().denominator;
        dataIt += 2;
    }
    return new Exiv2::DataValue((const Exiv2::byte *)array.data(), array.size());
}

#if EXIV2_TEST_VERSION(0,28,0)
KisMetaData::Value deviceSettingDescriptionExifToKMD(const Exiv2::Value::UniquePtr value)
#else
KisMetaData::Value deviceSettingDescriptionExifToKMD(const Exiv2::Value::AutoPtr value)
#endif
{
    QMap<QString, KisMetaData::Value> deviceSettingStructure;
    QByteArray array;

    const Exiv2::DataValue *dvalue = dynamic_cast<const Exiv2::DataValue *>(&*value);
    if (dvalue) {
        array.resize(dvalue->count());
        dvalue->copy((Exiv2::byte *)array.data());
    } else {
        Q_ASSERT(value->typeId() == Exiv2::unsignedShort);
        array.resize(2 * value->count());
        value->copy((Exiv2::byte *)array.data(), Exiv2::littleEndian);
    }
    int columns = (reinterpret_cast<quint16 *>(array.data()))[0];
    int rows = (reinterpret_cast<quint16 *>(array.data()))[1];
    deviceSettingStructure["Columns"] = KisMetaData::Value(columns);
    deviceSettingStructure["Rows"] = KisMetaData::Value(rows);
    QList<KisMetaData::Value> settings;
    QByteArray null(2, 0);

    for (int index = 4; index < array.size();) {
        const int lastIndex = array.indexOf(null, index);
        if (lastIndex < 0)
            break; // Data is not a String, ignore
        const int numChars = (lastIndex - index) / 2; // including trailing zero

        QString setting = QString::fromUtf16((char16_t *)(void *)(array.data() + index), numChars);
        index = lastIndex + 2;
        dbgMetaData << "Setting << " << setting;
        settings.append(KisMetaData::Value(setting));
    }
    deviceSettingStructure["Settings"] = KisMetaData::Value(settings, KisMetaData::Value::OrderedArray);
    return KisMetaData::Value(deviceSettingStructure);
}

Exiv2::Value *deviceSettingDescriptionKMDToExif(const KisMetaData::Value &value)
{
    QMap<QString, KisMetaData::Value> deviceSettingStructure = value.asStructure();
    const quint16 columns = static_cast<quint16>(deviceSettingStructure["Columns"].asVariant().toUInt());
    quint16 rows = static_cast<quint16>(deviceSettingStructure["Rows"].asVariant().toUInt());

    QTextCodec *codec = QTextCodec::codecForName("UTF-16");

    QList<KisMetaData::Value> settings = deviceSettingStructure["Settings"].asArray();
    QByteArray array(4, 0);
    (reinterpret_cast<quint16 *>(array.data()))[0] = columns;
    (reinterpret_cast<quint16 *>(array.data()))[1] = rows;
    for (const KisMetaData::Value &v : settings) {
        const QString str = v.asVariant().toString();
        QByteArray setting = codec->fromUnicode(str);
        array.append(setting);
    }
    return new Exiv2::DataValue((const Exiv2::byte *)array.data(), array.size());
}

KisMetaData::Value cfaPatternExifToKMD(const Exiv2::Value &value, Exiv2::ByteOrder order)
{
    QByteArray array;
    if (!copyDataValue(value, &array)) {
        return {};
    }

    KisMetaData::Value result = cfaArrayToMetadata(array, order);
    if (result.type() == KisMetaData::Value::Invalid) {
        result = cfaArrayToMetadata(array, invertByteOrder(order));
    }
    if (result.type() == KisMetaData::Value::Invalid) {
        warnMetaData << "Ignoring invalid EXIF CFA pattern data with" << array.size() << "bytes";
    }
    return result;
}

Exiv2::Value *cfaPatternKMDToExif(const KisMetaData::Value &value)
{
    QMap<QString, KisMetaData::Value> cfaStructure = value.asStructure();
    const quint16 columns = static_cast<quint16>(cfaStructure["Columns"].asVariant().toUInt());
    const quint16 rows = static_cast<quint16>(cfaStructure["Rows"].asVariant().toUInt());

    QList<KisMetaData::Value> values = cfaStructure["Values"].asArray();
    Q_ASSERT(columns * rows == values.size());
    QByteArray array(4 + columns * rows, 0);
    (reinterpret_cast<quint16 *>(array.data()))[0] = columns;
    (reinterpret_cast<quint16 *>(array.data()))[1] = rows;
    for (int i = 0; i < columns * rows; i++) {
        const quint8 val = (quint8)values[i].asVariant().toUInt();
        *(array.data() + 4 + i) = (char)val;
    }
    dbgMetaData << "Cfa Array " << ppVar(columns) << ppVar(rows) << ppVar(array.size());
    return new Exiv2::DataValue((const Exiv2::byte *)array.data(), array.size());
}

// Read and write Flash //

#if EXIV2_TEST_VERSION(0,28,0)
KisMetaData::Value flashExifToKMD(const Exiv2::Value::UniquePtr value)
#else
KisMetaData::Value flashExifToKMD(const Exiv2::Value::AutoPtr value)
#endif
{
#if EXIV2_TEST_VERSION(0,28,0)
    const uint16_t v = static_cast<uint16_t>(value->toUint32());
#else
    const uint16_t v = static_cast<uint16_t>(value->toLong());
#endif
    QMap<QString, KisMetaData::Value> flashStructure;
    bool fired = (v & 0x01); // bit 1 is whether flash was fired or not
    flashStructure["Fired"] = QVariant(fired);
    int ret = ((v >> 1) & 0x03); // bit 2 and 3 are Return
    flashStructure["Return"] = QVariant(ret);
    int mode = ((v >> 3) & 0x03); // bit 4 and 5 are Mode
    flashStructure["Mode"] = QVariant(mode);
    bool function = ((v >> 5) & 0x01); // bit 6 if function
    flashStructure["Function"] = QVariant(function);
    bool redEye = ((v >> 6) & 0x01); // bit 7 if function
    flashStructure["RedEyeMode"] = QVariant(redEye);
    return KisMetaData::Value(flashStructure);
}

Exiv2::Value *flashKMDToExif(const KisMetaData::Value &value)
{
    uint16_t v = 0;
    QMap<QString, KisMetaData::Value> flashStructure = value.asStructure();
    v = flashStructure["Fired"].asVariant().toBool();
    v |= ((flashStructure["Return"].asVariant().toInt() & 0x03) << 1);
    v |= ((flashStructure["Mode"].asVariant().toInt() & 0x03) << 3);
    v |= ((flashStructure["Function"].asVariant().toInt() & 0x03) << 5);
    v |= ((flashStructure["RedEyeMode"].asVariant().toInt() & 0x03) << 6);
    return new Exiv2::ValueType<uint16_t>(v);
}

// ---- Implementation of KisExifIO ----//
KisExifIO::KisExifIO()
    : KisMetaData::IOBackend()
{
}

KisExifIO::~KisExifIO()
{
}

bool KisExifIO::saveTo(const KisMetaData::Store *store, QIODevice *ioDevice, HeaderType headerType) const
{
    ioDevice->open(QIODevice::WriteOnly);
    Exiv2::ExifData exifData;
    if (headerType == KisMetaData::IOBackend::JpegHeader) {
        QByteArray header(6, 0);
        header[0] = 0x45;
        header[1] = 0x78;
        header[2] = 0x69;
        header[3] = 0x66;
        header[4] = 0x00;
        header[5] = 0x00;
        ioDevice->write(header);
    }

    for (const KisMetaData::Entry &entry : *store) {
        try {
            dbgMetaData << "Trying to save: " << entry.name() << " of " << entry.schema()->prefix() << ":"
                        << entry.schema()->uri();
            QString exivKey;
            if (entry.schema()->uri() == KisMetaData::Schema::TIFFSchemaUri) {
                exivKey = "Exif.Image." + entry.name();
            } else if (entry.schema()->uri()
                       == KisMetaData::Schema::EXIFSchemaUri) { // Distinguish between exif and gps
                if (entry.name().left(3) == "GPS") {
                    exivKey = "Exif.GPSInfo." + entry.name();
                } else {
                    exivKey = "Exif.Photo." + entry.name();
                }
            } else if (entry.schema()->uri() == KisMetaData::Schema::DublinCoreSchemaUri) {
                if (entry.name() == "description") {
                    exivKey = "Exif.Image.ImageDescription";
                } else if (entry.name() == "creator") {
                    exivKey = "Exif.Image.Artist";
                } else if (entry.name() == "rights") {
                    exivKey = "Exif.Image.Copyright";
                }
            } else if (entry.schema()->uri() == KisMetaData::Schema::XMPSchemaUri) {
                if (entry.name() == "ModifyDate") {
                    exivKey = "Exif.Image.DateTime";
                } else if (entry.name() == "CreatorTool") {
                    exivKey = "Exif.Image.Software";
                }
            } else if (entry.schema()->uri() == KisMetaData::Schema::MakerNoteSchemaUri) {
                if (entry.name() == "RawData") {
                    exivKey = "Exif.Photo.MakerNote";
                }
            }
            dbgMetaData << "Saving " << entry.name() << " to " << exivKey;
            if (exivKey.isEmpty()) {
                dbgMetaData << entry.qualifiedName() << " is unsavable to EXIF";
            } else {
                Exiv2::ExifKey exifKey(qPrintable(exivKey));
                Exiv2::Value *v = 0;
                if (exivKey == "Exif.Photo.ExifVersion" || exivKey == "Exif.Photo.FlashpixVersion") {
                    v = kmdValueToExifVersion(entry.value());
                } else if (exivKey == "Exif.Photo.FileSource") {
                    char s[] = {0x03};
                    v = new Exiv2::DataValue((const Exiv2::byte *)s, 1);
                } else if (exivKey == "Exif.Photo.SceneType") {
                    char s[] = {0x01};
                    v = new Exiv2::DataValue((const Exiv2::byte *)s, 1);
                } else if (exivKey == "Exif.Photo.ComponentsConfiguration") {
                    v = kmdIntOrderedArrayToExifArray(entry.value());
                } else if (exivKey == "Exif.Image.Artist") { // load as dc:creator
                    KisMetaData::Value creator = entry.value();
                    if (entry.value().asArray().size() > 0) {
                        creator = entry.value().asArray()[0];
                    }
#if !EXIV2_TEST_VERSION(0, 21, 0)
                    v = kmdValueToExivValue(creator, Exiv2::ExifTags::tagType(exifKey.tag(), exifKey.ifdId()));
#else
                    v = kmdValueToExivValue(creator, exifKey.defaultTypeId());
#endif
                } else if (exivKey == "Exif.Photo.OECF") {
                    v = kmdOECFStructureToExifOECF(entry.value());
                } else if (exivKey == "Exif.Photo.DeviceSettingDescription") {
                    v = deviceSettingDescriptionKMDToExif(entry.value());
                } else if (exivKey == "Exif.Photo.CFAPattern") {
                    v = cfaPatternKMDToExif(entry.value());
                } else if (exivKey == "Exif.Photo.Flash") {
                    v = flashKMDToExif(entry.value());
                } else if (exivKey == "Exif.Photo.UserComment") {
                    Q_ASSERT(entry.value().type() == KisMetaData::Value::LangArray);
                    QMap<QString, KisMetaData::Value> langArr = entry.value().asLangArray();
                    if (langArr.contains("x-default")) {
#if !EXIV2_TEST_VERSION(0, 21, 0)
                        v = kmdValueToExivValue(langArr.value("x-default"),
                                                Exiv2::ExifTags::tagType(exifKey.tag(), exifKey.ifdId()));
#else
                        v = kmdValueToExivValue(langArr.value("x-default"), exifKey.defaultTypeId());
#endif
                    } else if (langArr.size() > 0) {
#if !EXIV2_TEST_VERSION(0, 21, 0)
                        v = kmdValueToExivValue(langArr.begin().value(),
                                                Exiv2::ExifTags::tagType(exifKey.tag(), exifKey.ifdId()));
#else
                        v = kmdValueToExivValue(langArr.begin().value(), exifKey.defaultTypeId());
#endif
                    }
                } else {
                    dbgMetaData << exifKey.tag();
#if !EXIV2_TEST_VERSION(0, 21, 0)
                    v = kmdValueToExivValue(entry.value(), Exiv2::ExifTags::tagType(exifKey.tag(), exifKey.ifdId()));
#else
                    v = kmdValueToExivValue(entry.value(), exifKey.defaultTypeId());
#endif
                }
                if (v && v->typeId() != Exiv2::invalidTypeId) {
                    dbgMetaData << "Saving key" << exivKey << " of KMD value" << entry.value();
                    exifData.add(exifKey, v);
                } else {
                    dbgMetaData << "No exif value was created for" << entry.qualifiedName() << " as"
                                << exivKey; // << " of KMD value" << entry.value();
                }
            }
#if EXIV2_TEST_VERSION(0,28,0)
        } catch (Exiv2::Error &e) {
#else
        } catch (Exiv2::AnyError &e) {
#endif
            dbgMetaData << "exiv error " << e.what();
        }
    }
#if !EXIV2_TEST_VERSION(0, 18, 0)
    Exiv2::DataBuf rawData = exifData.copy();
    ioDevice->write((const char *)rawData.pData_, rawData.size_);
#else
    Exiv2::Blob rawData;
    Exiv2::ExifParser::encode(rawData, Exiv2::littleEndian, exifData);
    ioDevice->write((const char *)&*rawData.begin(), static_cast<int>(rawData.size()));
#endif
    ioDevice->close();
    return true;
}

bool KisExifIO::canSaveAllEntries(KisMetaData::Store * /*store*/) const
{
    return false; // It's a known fact that exif can't save all information, but TODO: write the check
}

bool KisExifIO::loadFrom(KisMetaData::Store *store, QIODevice *ioDevice) const
{
    if (!ioDevice->open(QIODevice::ReadOnly)) {
        return false;
    }
    QByteArray arr(ioDevice->readAll());
    Exiv2::ExifData exifData;
    Exiv2::ByteOrder byteOrder;
#if !EXIV2_TEST_VERSION(0, 18, 0)
    exifData.load((const Exiv2::byte *)arr.data(), arr.size());
    byteOrder = exifData.byteOrder();
#else
    try {
        byteOrder =
            Exiv2::ExifParser::decode(exifData, (const Exiv2::byte *)arr.data(), static_cast<uint32_t>(arr.size()));
    } catch (const std::exception &ex) {
        warnKrita << "Received exception trying to parse exiv data" << ex.what();
        return false;
    } catch (...) {
        dbgKrita << "Received unknown exception trying to parse exiv data";
        return false;
    }
#endif
    dbgMetaData << "Byte order = " << byteOrder << ppVar(Exiv2::bigEndian) << ppVar(Exiv2::littleEndian);
    dbgMetaData << "There are" << exifData.count() << " entries in the exif section";
    const KisMetaData::Schema *tiffSchema =
        KisMetaData::SchemaRegistry::instance()->schemaFromUri(KisMetaData::Schema::TIFFSchemaUri);
    Q_ASSERT(tiffSchema);
    const KisMetaData::Schema *exifSchema =
        KisMetaData::SchemaRegistry::instance()->schemaFromUri(KisMetaData::Schema::EXIFSchemaUri);
    Q_ASSERT(exifSchema);
    const KisMetaData::Schema *dcSchema =
        KisMetaData::SchemaRegistry::instance()->schemaFromUri(KisMetaData::Schema::DublinCoreSchemaUri);
    Q_ASSERT(dcSchema);
    const KisMetaData::Schema *xmpSchema =
        KisMetaData::SchemaRegistry::instance()->schemaFromUri(KisMetaData::Schema::XMPSchemaUri);
    Q_ASSERT(xmpSchema);
    const KisMetaData::Schema *makerNoteSchema =
        KisMetaData::SchemaRegistry::instance()->schemaFromUri(KisMetaData::Schema::MakerNoteSchemaUri);
    Q_ASSERT(makerNoteSchema);

    for (const Exiv2::Exifdatum &it : exifData) {
        const uint16_t tag = it.tag();

        if (tag == Exif::Image::StripOffsets || tag == Exif::Image::RowsPerStrip || tag == Exif::Image::StripByteCounts
            || tag == Exif::Image::JPEGInterchangeFormat || tag == Exif::Image::JPEGInterchangeFormatLength
            || it.tagName() == "0x0000") {
            dbgMetaData << it.key().c_str() << " is ignored";
        } else if (tag == Exif::Photo::MakerNote) {
            store->addEntry({makerNoteSchema, "RawData", exivValueToKMDValue(it.getValue(), false)});
        } else if (tag == Exif::Image::DateTime) { // load as xmp:ModifyDate
            store->addEntry({xmpSchema, "ModifyDate", exivValueToKMDValue(it.getValue(), false)});
        } else if (tag == Exif::Image::ImageDescription) { // load as "dc:description"
            store->addEntry({dcSchema, "description", exivValueToKMDValue(it.getValue(), false)});
        } else if (tag == Exif::Image::Software) { // load as "xmp:CreatorTool"
            store->addEntry({xmpSchema, "CreatorTool", exivValueToKMDValue(it.getValue(), false)});
        } else if (tag == Exif::Image::Artist) { // load as dc:creator
            QList<KisMetaData::Value> creators = {exivValueToKMDValue(it.getValue(), false)};
            store->addEntry({dcSchema, "creator", {creators, KisMetaData::Value::OrderedArray}});
        } else if (tag == Exif::Image::Copyright) { // load as dc:rights
            store->addEntry({dcSchema, "rights", exivValueToKMDValue(it.getValue(), false)});
        } else if (it.groupName() == "Image") {
            // Tiff tags
            const QString fixedTN(it.tagName().c_str());
            if (tag == Exif::Image::ExifTag || tag == Exif::Image::GPSTag) {
                dbgMetaData << "Ignoring " << it.key().c_str();
            } else if (KisMetaData::Entry::isValidName(fixedTN)) {
                store->addEntry({tiffSchema, fixedTN, exivValueToKMDValue(it.getValue(), false)});
            } else {
                dbgMetaData << "Invalid tag name: " << fixedTN;
            }
        } else if (it.groupName() == "Photo") {
            // Exif tags
            KisMetaData::Value metaDataValue;
            if (tag == Exif::Photo::ExifVersion || tag == Exif::Photo::FlashpixVersion) {
                metaDataValue = exifVersionToKMDValue(it.getValue());
            } else if (tag == Exif::Photo::FileSource) {
                metaDataValue = KisMetaData::Value(3);
            } else if (tag == Exif::Photo::SceneType) {
                metaDataValue = KisMetaData::Value(1);
            } else if (tag == Exif::Photo::ComponentsConfiguration) {
                metaDataValue = exifArrayToKMDIntOrderedArray(it.getValue());
            } else if (tag == Exif::Photo::OECF) {
                metaDataValue = exifOECFToKMDOECFStructure(*it.getValue(), byteOrder);
            } else if (tag == Exif::Photo::DateTimeDigitized || tag == Exif::Photo::DateTimeOriginal) {
                metaDataValue = exivValueToKMDValue(it.getValue(), false);
            } else if (tag == Exif::Photo::DeviceSettingDescription) {
                metaDataValue = deviceSettingDescriptionExifToKMD(it.getValue());
            } else if (tag == Exif::Photo::CFAPattern) {
                metaDataValue = cfaPatternExifToKMD(*it.getValue(), byteOrder);
            } else if (tag == Exif::Photo::Flash) {
                metaDataValue = flashExifToKMD(it.getValue());
            } else if (tag == Exif::Photo::UserComment) {
                if (it.getValue()->typeId() != Exiv2::undefined) {
                    KisMetaData::Value vUC = exivValueToKMDValue(it.getValue(), false);
                    Q_ASSERT(vUC.type() == KisMetaData::Value::Variant);
                    QVariant commentVar = vUC.asVariant();
                    QString comment;
                    if (commentVar.type() == QMetaType::QString) {
                        comment = commentVar.toString();
                    } else if (commentVar.type() == QMetaType::QByteArray) {
                        const QByteArray commentString = commentVar.toByteArray();
                        comment = QString::fromLatin1(commentString.constData(), commentString.size());
                    } else {
                        warnKrita << "KisExifIO: Unhandled UserComment value type.";
                    }
                    KisMetaData::Value vcomment(comment);
                    vcomment.addPropertyQualifier("xml:lang", KisMetaData::Value("x-default"));
                    QList<KisMetaData::Value> alt;
                    alt.append(vcomment);
                    metaDataValue = KisMetaData::Value(alt, KisMetaData::Value::LangArray);
                }
            } else {
                bool forceSeq = false;
                KisMetaData::Value::ValueType arrayType = KisMetaData::Value::UnorderedArray;
                if (tag == Exif::Photo::ISOSpeedRatings) {
                    forceSeq = true;
                    arrayType = KisMetaData::Value::OrderedArray;
                }
                metaDataValue = exivValueToKMDValue(it.getValue(), forceSeq, arrayType);
            }
            if (tag == Exif::Photo::InteroperabilityTag || tag == 0xea1d
                || metaDataValue.type() == KisMetaData::Value::Invalid) { // InteroperabilityTag isn't useful for XMP,
                // 0xea1d isn't a valid Exif tag
                warnMetaData << "Ignoring " << it.key().c_str();

            } else {
                store->addEntry({exifSchema, it.tagName().c_str(), metaDataValue});
            }
        } else if (it.groupName() == "Thumbnail") {
            dbgMetaData << "Ignoring thumbnail tag :" << it.key().c_str();
        } else if (it.groupName() == "GPSInfo") {
            store->addEntry({exifSchema, it.tagName().c_str(), exivValueToKMDValue(it.getValue(), false)});
        } else {
            dbgMetaData << "Unknown exif tag, cannot load:" << it.key().c_str();
        }
    }
    ioDevice->close();
    return true;
}
