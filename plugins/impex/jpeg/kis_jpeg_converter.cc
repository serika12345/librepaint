/*
 *  SPDX-FileCopyrightText: 2005 Cyrille Berger <cberger@cberger.net>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "kis_jpeg_converter.h"
#include "KisJpegCoefficientStore_p.h"
#include "KisImportExportErrorCode.h"
#include <KisExportRasterRows.h>
#include "KoID.h"
#include "KoIntegerMaths.h"
#include "kis_assert.h"
#include "kis_debug.h"
#include "kis_global.h"
#include "kis_meta_data_io_backend.h"
#include "kis_types.h"

#include <cstdlib>
#include <cstring>
#include <exiv2/types.hpp>
#include <jpeglib.h>
#include <math.h>
#include <QtGlobal>
#include <qstringview.h>
#include <stdexcept>
#include <memory>
#include <optional>
#include <vector>
#include <stdint.h>

#include <KoConfig.h>
#ifdef HAVE_LCMS2
#else
#   include <lcms.h>
#endif

extern "C" {
#include <iccjpeg.h>
}

#include <exiv2/version.hpp>
#if EXIV2_TEST_VERSION(0,28,0)
#include <exiv2/photoshop.hpp>
#endif

#include <QFile>
#include <QBuffer>
#include <QApplication>
#include <QScopeGuard>
#include <QTemporaryFile>

#include <klocalizedstring.h>

#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>
#include <KoColorProfile.h>
#include <KoColor.h>
#include <KoCompositeOp.h>
#include <KoUnit.h>
#include "KoColorModelStandardIds.h"

#include <document/KisDocument.h>
#include <kis_group_layer.h>
#include <kis_image.h>
#include <kis_iterator_ng.h>
#include <kis_jpeg_destination.h>
#include <kis_jpeg_source.h>
#include <kis_meta_data_backend_registry.h>
#include <kis_meta_data_entry.h>
#include <kis_meta_data_store.h>
#include <kis_meta_data_value.h>
#include <kis_paint_device.h>
#include <kis_paint_layer.h>
#include <kis_painter.h>
#include <kis_transform_worker.h>

#define ICC_MARKER  (JPEG_APP0 + 2) /* JPEG marker code for ICC */
#define ICC_OVERHEAD_LEN  14    /* size of non-profile data in APP2 */
#define MAX_BYTES_IN_MARKER  65533  /* maximum data len of a JPEG marker */
#define MAX_DATA_BYTES_IN_MARKER  (MAX_BYTES_IN_MARKER - ICC_OVERHEAD_LEN)

const char photoshopMarker[] = "Photoshop 3.0\0";
//const char photoshopBimId_[] = "8BIM";
const uint16_t photoshopIptc = 0x0404;
const char xmpMarker[] = "http://ns.adobe.com/xap/1.0/\0";
const QByteArray photoshopIptc_((char*)&photoshopIptc, 2);

namespace
{

void jpegErrorExit (j_common_ptr cinfo)
{
    char jpegLastErrorMsg[JMSG_LENGTH_MAX];

    ( *( cinfo->err->format_message ) ) ( cinfo, jpegLastErrorMsg );

    throw std::runtime_error(jpegLastErrorMsg);
}

J_COLOR_SPACE getColorTypeforColorSpace(const KoColorSpace * cs)
{
    if (KoID(cs->id()) == KoID("GRAYA") || cs->id() == "GRAYAU16" || cs->id() == "GRAYA16") {
        return JCS_GRAYSCALE;
    }
    if (KoID(cs->id()) == KoID("RGBA") || KoID(cs->id()) == KoID("RGBA16")) {
        return JCS_RGB;
    }
    if (KoID(cs->id()) == KoID("CMYK") || KoID(cs->id()) == KoID("CMYKAU16")) {
        return JCS_CMYK;
    }
    return JCS_UNKNOWN;
}

QString getColorSpaceModelForColorType(J_COLOR_SPACE color_type)
{
    dbgFile << "color_type =" << color_type;
    if (color_type == JCS_GRAYSCALE) {
        return GrayAColorModelID.id();
    } else if (color_type == JCS_RGB) {
        return RGBAColorModelID.id();
    } else if (color_type == JCS_CMYK) {
        return CMYKAColorModelID.id();
    }
    return "";
}

}

struct KisJPEGConverter::Private
{
    Private(KisDocument *doc, bool batchMode)
        : doc(doc),
          stop(false),
          batchMode(batchMode)
    {}

    KisImageSP image;
    KisDocument *doc;
    bool stop;
    bool batchMode;
};

KisJPEGConverter::KisJPEGConverter(KisDocument *doc, bool batchMode)
    : m_d(new Private(doc, batchMode))
{
}

KisJPEGConverter::~KisJPEGConverter()
{
}

KisImportExportErrorCode KisJPEGConverter::decode(QIODevice *io)
{
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jerr.error_exit = jpegErrorExit;

    try {
        jpeg_create_decompress(&cinfo);

        KisJPEGSource::setSource(&cinfo, io);

        jpeg_save_markers(&cinfo, JPEG_COM, 0xFFFF);
        /* Save APP0..APP15 markers */
        for (int m = 0; m < 16; m++)
            jpeg_save_markers(&cinfo, JPEG_APP0 + m, 0xFFFF);


        //     setup_read_icc_profile(&cinfo);
        // read header
        jpeg_read_header(&cinfo, (boolean)true);

        // start reading
        jpeg_start_decompress(&cinfo);

        // Get the colorspace
        QString modelId = getColorSpaceModelForColorType(cinfo.out_color_space);
        if (modelId.isEmpty()) {
            dbgFile << "unsupported colorspace :" << cinfo.out_color_space;
            jpeg_destroy_decompress(&cinfo);
            return ImportExportCodes::FormatColorSpaceUnsupported;
        }

        uint8_t* profile_data = nullptr;
        uint32_t profile_len = 0;

        const KoColorProfile* profile = nullptr;

        if (read_icc_profile(&cinfo, &profile_data, &profile_len)) {
            QByteArray profile_rawdata(reinterpret_cast<char*>(profile_data), static_cast<int>(profile_len));
            // read_icc_profile allocates memory, and we need to free it
            free(profile_data);

            profile = KoColorSpaceRegistry::instance()->createColorProfile(modelId, Integer8BitsColorDepthID.id(), profile_rawdata);
            dbgFile <<"profile name:" << profile->name() <<" product information:" << profile->info();
            //This is stadard for profiles like this
            if (!profile->isSuitableForWorkspace()) {
              dbgFile << "the profile is not suitable for workspace and therefore cannot be used in LibrePaint, we need to convert the image to a standard profile";
            }
        }

        const QString colorSpaceId =
            KoColorSpaceRegistry::instance()->colorSpaceId(modelId, Integer8BitsColorDepthID.id());

        // Check that the profile is used by the color space
        if (profile
            && (!KoColorSpaceRegistry::instance()->profileIsCompatible(profile, colorSpaceId)
            ||  !(profile->isSuitableForInput() || profile->isSuitableForOutput()))) {
            warnFile << "The profile " << profile->name() << " is not compatible with the color space model " << modelId;
            profile = 0;
        }

        // Retrieve a pointer to the colorspace
        const KoColorSpace* cs = nullptr;
        if (profile && profile->isSuitableForWorkspace()) {
            dbgFile << "image has embedded profile:" << profile -> name() << "";
            cs = KoColorSpaceRegistry::instance()->colorSpace(modelId, Integer8BitsColorDepthID.id(), profile);
        } else
            cs = KoColorSpaceRegistry::instance()->colorSpace(modelId, Integer8BitsColorDepthID.id(), "");

        if (cs == 0) {
            dbgFile << "unknown colorspace";
            jpeg_destroy_decompress(&cinfo);
            return ImportExportCodes::FormatColorSpaceUnsupported;
        }

        // Create the cmsTransform if needed
        KoColorTransformation* transform = 0;
        if (profile && !profile->isSuitableForWorkspace() && profile->isSuitableForInput()) {
            transform = KoColorSpaceRegistry::instance()
                ->colorSpace(modelId, Integer8BitsColorDepthID.id(), profile)
                ->createColorConverter(cs, KoColorConversionTransformation::internalRenderingIntent(), KoColorConversionTransformation::internalConversionFlags());
        }

        // Apparently an invalid transform was created from the profile. See bug https://bugs.kde.org/show_bug.cgi?id=255451.
        // After 2.3: warn the user!
        if (transform && !transform->isValid()) {
            delete transform;
            transform = 0;
        }

        // Creating the KisImageSP
        if (!m_d->image) {
            m_d->image = new KisImage(m_d->doc->createUndoStore(),  cinfo.image_width,  cinfo.image_height, cs, "built image");
            Q_CHECK_PTR(m_d->image);
        }

        // Set resolution
        double xres = 72, yres = 72;
        if (cinfo.density_unit == 1) {
            xres = cinfo.X_density;
            yres = cinfo.Y_density;
        } else if (cinfo.density_unit == 2) {
            xres = cinfo.X_density * 2.54;
            yres = cinfo.Y_density * 2.54;
        }
        if (xres < 72) {
            xres = 72;
        }
        if (yres < 72) {
            yres = 72;
        }
        m_d->image->setResolution(POINT_TO_INCH(xres), POINT_TO_INCH(yres));   // It is the "invert" macro because we convert from pointer-per-inch to points

        // Create layer
        KisPaintLayerSP layer = KisPaintLayerSP(new KisPaintLayer(m_d->image.data(), m_d->image -> nextLayerName(), quint8_MAX));

        // Read data
        JSAMPROW row_pointer = new JSAMPLE[cinfo.image_width*cinfo.num_components];

        for (; cinfo.output_scanline < cinfo.image_height;) {
            KisHLineIteratorSP it = layer->paintDevice()->createHLineIteratorNG(0, cinfo.output_scanline, cinfo.image_width);
            jpeg_read_scanlines(&cinfo, &row_pointer, 1);
            quint8 *src = row_pointer;
            switch (cinfo.out_color_space) {
            case JCS_GRAYSCALE:
                do {
                    quint8 *d = it->rawData();
                    d[0] = *(src++);
                    if (transform) transform->transform(d, d, 1);
                    d[1] = quint8_MAX;

                } while (it->nextPixel());
                break;
            case JCS_RGB:
                do {
                    quint8 *d = it->rawData();
                    d[2] = *(src++);
                    d[1] = *(src++);
                    d[0] = *(src++);
                    if (transform) transform->transform(d, d, 1);
                    d[3] = quint8_MAX;

                } while (it->nextPixel());
                break;
            case JCS_CMYK:
                do {
                    quint8 *d = it->rawData();
                    d[0] = quint8_MAX - *(src++);
                    d[1] = quint8_MAX - *(src++);
                    d[2] = quint8_MAX - *(src++);
                    d[3] = quint8_MAX - *(src++);
                    if (transform) transform->transform(d, d, 1);
                    d[4] = quint8_MAX;

                } while (it->nextPixel());
                break;
            default:
                return ImportExportCodes::FormatFeaturesUnsupported;
            }
        }

        m_d->image->addNode(KisNodeSP(layer.data()), m_d->image->rootLayer().data());

        // Read exif information

        dbgFile << "Looking for exif information";

        for (jpeg_saved_marker_ptr marker = cinfo.marker_list; marker != 0; marker = marker->next) {
            dbgFile << "Marker is" << marker->marker;
            if (marker->marker != (JOCTET)(JPEG_APP0 + 1)
                    || marker->data_length < 14) {
                continue; /* Exif data is in an APP1 marker of at least 14 octets */
            }

            if (GETJOCTET(marker->data[0]) != (JOCTET) 0x45 ||
                    GETJOCTET(marker->data[1]) != (JOCTET) 0x78 ||
                    GETJOCTET(marker->data[2]) != (JOCTET) 0x69 ||
                    GETJOCTET(marker->data[3]) != (JOCTET) 0x66 ||
                    GETJOCTET(marker->data[4]) != (JOCTET) 0x00 ||
                    GETJOCTET(marker->data[5]) != (JOCTET) 0x00)
                continue; /* no Exif header */

            dbgFile << "Found exif information of length :" << marker->data_length;
            KisMetaData::IOBackend *exifIO = KisMetadataBackendRegistry::instance()->value("exif");
            Q_ASSERT(exifIO);
            QByteArray byteArray((const char*)marker->data + 6, marker->data_length - 6);
            QBuffer buf(&byteArray);
            exifIO->loadFrom(layer->metaData(), &buf);
            // Interpret orientation tag
            if (layer->metaData()->containsEntry("http://ns.adobe.com/tiff/1.0/", "Orientation")) {
                KisMetaData::Entry& entry = layer->metaData()->getEntry("http://ns.adobe.com/tiff/1.0/", "Orientation");
                if (entry.value().type() == KisMetaData::Value::Variant) {
                    switch (entry.value().asVariant().toInt()) {
                    case 2:
                        KisTransformWorker::mirrorX(layer->paintDevice());
                        break;
                    case 3:
                        image()->rotateImage(M_PI);
                        break;
                    case 4:
                        KisTransformWorker::mirrorY(layer->paintDevice());
                        break;
                    case 5:
                        image()->rotateImage(M_PI / 2);
                        KisTransformWorker::mirrorY(layer->paintDevice());
                        break;
                    case 6:
                        image()->rotateImage(M_PI / 2);
                        break;
                    case 7:
                        image()->rotateImage(M_PI / 2);
                        KisTransformWorker::mirrorX(layer->paintDevice());
                        break;
                    case 8:
                        image()->rotateImage(-M_PI / 2 + M_PI*2);
                        break;
                    default:
                        break;
                    }
                }
                entry.value().setVariant(1);
            }
            break;
        }

        dbgFile << "Looking for IPTC information";

        for (jpeg_saved_marker_ptr marker = cinfo.marker_list; marker != 0; marker = marker->next) {
            dbgFile << "Marker is" << marker->marker;
            if (marker->marker != (JOCTET)(JPEG_APP0 + 13) ||  marker->data_length < 14) {
                continue; /* IPTC data is in an APP13 marker of at least 16 octets */
            }
            if (memcmp(marker->data, photoshopMarker, 14) != 0) {
                for (int i = 0; i < 14; i++) {
                    dbgFile << (int)(*(marker->data + i)) << "" << (int)(photoshopMarker[i]);
                }
                dbgFile << "No photoshop marker";
                continue; /* No IPTC Header */
            }

            dbgFile << "Found Photoshop information of length :" << marker->data_length;
            KisMetaData::IOBackend *iptcIO = KisMetadataBackendRegistry::instance()->value("iptc");
            Q_ASSERT(iptcIO);
            const Exiv2::byte *record = 0;
            uint32_t sizeIptc = 0;
            uint32_t sizeHdr = 0;
            // Find actual Iptc data within the APP13 segment
            if (!Exiv2::Photoshop::locateIptcIrb((Exiv2::byte*)(marker->data + 14),
#if EXIV2_TEST_VERSION(0,28,0)
                                                 marker->data_length - 14, &record, sizeHdr, sizeIptc)) {
#else
                                                 marker->data_length - 14, &record, &sizeHdr, &sizeIptc)) {
#endif
                if (sizeIptc) {
                    // Decode the IPTC data
                    QByteArray byteArray((const char*)(record + sizeHdr), sizeIptc);
                    QBuffer buf(&byteArray);
                    iptcIO->loadFrom(layer->metaData(), &buf);
                } else {
                    dbgFile << "IPTC Not found in Photoshop marker";
                }
            }
            break;
        }

        dbgFile << "Looking for XMP information";

        for (jpeg_saved_marker_ptr marker = cinfo.marker_list; marker != 0; marker = marker->next) {
            dbgFile << "Marker is" << marker->marker;
            if (marker->marker != (JOCTET)(JPEG_APP0 + 1) || marker->data_length < 31) {
                continue; /* XMP data is in an APP1 marker of at least 31 octets */
            }
            if (memcmp(marker->data, xmpMarker, 29) != 0) {
                dbgFile << "Not XMP marker";
                continue; /* No xmp Header */
            }
            dbgFile << "Found XMP Marker of length " << marker->data_length;
            QByteArray byteArray((const char*)marker->data + 29, marker->data_length - 29);
            KisMetaData::IOBackend *xmpIO = KisMetadataBackendRegistry::instance()->value("xmp");
            Q_ASSERT(xmpIO);
            xmpIO->loadFrom(layer->metaData(), new QBuffer(&byteArray));
            break;
        }

        // Dump loaded metadata
        layer->metaData()->debugDump();

        // Check whether the metadata has resolution info, too...
        if (cinfo.density_unit == 0 && layer->metaData()->containsEntry("tiff:XResolution") && layer->metaData()->containsEntry("tiff:YResolution")) {
            double xres = layer->metaData()->getEntry("tiff:XResolution").value().asDouble();
            double yres = layer->metaData()->getEntry("tiff:YResolution").value().asDouble();
            if (xres != 0 && yres != 0) {
                m_d->image->setResolution(POINT_TO_INCH(xres), POINT_TO_INCH(yres));   // It is the "invert" macro because we convert from pointer-per-inch to points
            }
        }

        // Finish decompression
        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);
        delete [] row_pointer;
        return ImportExportCodes::OK;
    }
    catch( std::runtime_error &) {
        jpeg_destroy_decompress(&cinfo);
        return ImportExportCodes::FileFormatIncorrect;
    }
}



KisImportExportErrorCode KisJPEGConverter::buildImage(QIODevice *io)
{
    return decode(io);
}


KisImageSP KisJPEGConverter::image()
{
    return m_d->image;
}


KisImportExportErrorCode KisJPEGConverter::buildFile(QIODevice *io, const KisExportRasterRows &rows,
    qreal xRes, qreal yRes, KisJPEGOptions options, KisMetaData::Store *metaData)
{
    if (m_d->stop) return ImportExportCodes::Cancelled;
    if (!rows.colorSpace || !rows.readRow || rows.size.isEmpty()) return ImportExportCodes::InternalError;
    if (qint64(rows.size.width()) * rows.colorSpace->pixelSize() > INT_MAX)
        return ImportExportCodes::InsufficientMemory;
    const KoColorSpace *destination = rows.colorSpace;
    if (getColorTypeforColorSpace(destination) == JCS_UNKNOWN)
        destination = KoColorSpaceRegistry::instance()->rgb8();
    if (!destination) return ImportExportCodes::FormatColorSpaceUnsupported;
    const auto intermediate = destination;
    if (options.forceSRGB) {
        destination = KoColorSpaceRegistry::instance()->colorSpace(RGBAColorModelID.id(),
            destination->colorDepthId().id(), "sRGB built-in - (lcms internal)");
    }
    if (!destination) return ImportExportCodes::FormatColorSpaceUnsupported;
    if (qint64(rows.size.width()) * destination->pixelSize() > INT_MAX)
        return ImportExportCodes::InsufficientMemory;
    struct Conversion {
        KisExportRasterRows source;
        const KoColorSpace *intermediate;
        const KoColorSpace *destination;
        KoColor background;
        QByteArray input, converted, profiled, filled;
        std::unique_ptr<KoColorConversionTransformation> modelTransform, profileTransform;
        Conversion(const KisExportRasterRows &source, const KoColorSpace *intermediate,
                   const KoColorSpace *destination, const QColor &fill)
            : source(source), intermediate(intermediate), destination(destination), background(fill, destination)
        {
            if (source.colorSpace != intermediate)
                modelTransform.reset(source.colorSpace->createColorConverter(intermediate,
                    KoColorConversionTransformation::internalRenderingIntent(),
                    KoColorConversionTransformation::internalConversionFlags()));
            if (intermediate != destination)
                profileTransform.reset(intermediate->createColorConverter(destination,
                    KoColorConversionTransformation::internalRenderingIntent(),
                    KoColorConversionTransformation::internalConversionFlags()));
        }
        KisImportExportErrorCode read(int row, QByteArray &output)
        {
            auto result = source.readRow(row, input);
            if (!result.isOk()) return result;
            if (input.size() != qsizetype(source.size.width()) * source.colorSpace->pixelSize())
                return ImportExportCodes::InternalError;
            const quint8 *pixels = reinterpret_cast<const quint8 *>(input.constData());
            const auto size = qsizetype(source.size.width()) * destination->pixelSize();
            if (modelTransform) {
                converted.resize(qsizetype(source.size.width()) * intermediate->pixelSize());
                modelTransform->transform(pixels, reinterpret_cast<quint8 *>(converted.data()), source.size.width());
                pixels = reinterpret_cast<const quint8 *>(converted.constData());
            }
            if (profileTransform) {
                profiled.resize(size);
                profileTransform->transform(pixels, reinterpret_cast<quint8 *>(profiled.data()), source.size.width());
                pixels = reinterpret_cast<const quint8 *>(profiled.constData());
            }
            filled.resize(size);
            for (int x = 0; x < source.size.width(); ++x)
                std::memcpy(filled.data() + qsizetype(x) * destination->pixelSize(), background.data(), destination->pixelSize());
            KoCompositeOp::ParameterInfo parameters;
            parameters.dstRowStart = reinterpret_cast<quint8 *>(filled.data());
            parameters.dstRowStride = int(size);
            parameters.srcRowStart = pixels;
            parameters.srcRowStride = int(size);
            parameters.rows = 1;
            parameters.cols = source.size.width();
            parameters.setOpacityAndAverage(1.0f, 1.0f);
            parameters.flow = 1.0f;
            destination->bitBlt(destination, parameters, destination->compositeOp(QStringLiteral("normal")),
                KoColorConversionTransformation::internalRenderingIntent(),
                KoColorConversionTransformation::internalConversionFlags());
            output = filled;
            return ImportExportCodes::OK;
        }
    };
    const auto conversion = std::make_shared<Conversion>(rows, intermediate, destination, options.transparencyFillColor);
    if ((rows.colorSpace != intermediate && !conversion->modelTransform) ||
        (intermediate != destination && !conversion->profileTransform)) return ImportExportCodes::FormatColorSpaceUnsupported;
    KisExportRasterRows prepared;
    prepared.colorSpace = destination;
    prepared.size = rows.size;
    prepared.readRow = [conversion](int row, QByteArray &bytes) { return conversion->read(row, bytes); };
    return encodeRows(io, prepared, xRes, yRes, options, metaData);
}

KisImportExportErrorCode KisJPEGConverter::buildFile(QIODevice *io, KisPaintLayerSP layer, KisJPEGOptions options, KisMetaData::Store* metaData)
{
    KIS_ASSERT_RECOVER_RETURN_VALUE(layer, ImportExportCodes::InternalError);

    KisImageSP image = KisImageSP(layer->image());
    KIS_ASSERT_RECOVER_RETURN_VALUE(image, ImportExportCodes::InternalError);

    const KoColorSpace * cs = layer->colorSpace();
    J_COLOR_SPACE color_type = getColorTypeforColorSpace(cs);

    if (color_type == JCS_UNKNOWN) {
        layer->paintDevice()->convertTo(KoColorSpaceRegistry::instance()->rgb8(), KoColorConversionTransformation::internalRenderingIntent(), KoColorConversionTransformation::internalConversionFlags());
        cs = KoColorSpaceRegistry::instance()->rgb8();
        color_type = JCS_RGB;
    }

    if (options.forceSRGB) {
        const KoColorSpace* dst = KoColorSpaceRegistry::instance()->colorSpace(RGBAColorModelID.id(), layer->colorSpace()->colorDepthId().id(), "sRGB built-in - (lcms internal)");
        layer->paintDevice()->convertTo(dst);
        cs = dst;
        color_type = JCS_RGB;
    }

    uint height = image->height();
    uint width = image->width();
    KisPaintDeviceSP dev = new KisPaintDevice(layer->colorSpace());
    KoColor background(options.transparencyFillColor, layer->colorSpace());
    dev->fill(QRect(0, 0, width, height), background);
    KisPainter painter(dev);
    painter.bitBlt(QPoint(0, 0), layer->paintDevice(), QRect(0, 0, width, height));
    painter.end();
    KisExportRasterRows rows;
    rows.colorSpace = dev->colorSpace();
    rows.size = image->size();
    rows.readRow = [dev, width](int row, QByteArray &bytes) {
        bytes.resize(qsizetype(width) * dev->pixelSize());
        dev->readBytes(reinterpret_cast<quint8 *>(bytes.data()), 0, row, width, 1);
        return KisImportExportErrorCode(ImportExportCodes::OK);
    };
    return encodeRows(io, rows, image->xRes(), image->yRes(), options, metaData);
}

KisImportExportErrorCode KisJPEGConverter::encodeRows(QIODevice *io, const KisExportRasterRows &rows,
    qreal xRes, qreal yRes, const KisJPEGOptions &options, KisMetaData::Store *metaData)
{
    if (m_d->stop) return ImportExportCodes::Cancelled;
    if (!io || !rows.colorSpace || !rows.readRow || rows.size.isEmpty()) return ImportExportCodes::InternalError;
    const auto cs = rows.colorSpace;
    const auto color_type = getColorTypeforColorSpace(cs);
    if (color_type == JCS_UNKNOWN) return ImportExportCodes::FormatColorSpaceUnsupported;
    const uint width = rows.size.width(), height = rows.size.height();
    const qint64 rowBytes = qint64(width) * cs->pixelSize();
    if (rowBytes > INT_MAX) return ImportExportCodes::InsufficientMemory;
    struct jpeg_compress_struct cinfo{};
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jerr.error_exit = jpegErrorExit;
    QTemporaryFile coefficientBacking;
    std::optional<KisJpegCoefficientStore> coefficients;
    const auto destroy = qScopeGuard([&] { jpeg_destroy_compress(&cinfo); });
    try {
        jpeg_create_compress(&cinfo);
        coefficients.emplace(&cinfo, coefficientBacking, 8 * 1024 * 1024);
        KisJPEGDestination::setDestination(&cinfo, io);
        cinfo.image_width = width;  // image width and height, in pixels
        cinfo.image_height = height;
        cinfo.input_components = cs->colorChannelCount(); // number of color channels per pixel */
        cinfo.in_color_space = color_type;   // colorspace of input image

        // Set default compression parameters
        jpeg_set_defaults(&cinfo);
        // Customize them
        jpeg_set_quality(&cinfo, options.quality, (boolean)options.baseLineJPEG);

        if (options.progressive) {
            jpeg_simple_progression(&cinfo);
        }
        // Optimize ?
        cinfo.optimize_coding = (boolean)options.optimize;

        // Smoothing
        cinfo.smoothing_factor = (boolean)options.smooth;

        // Subsampling
        switch (options.subsampling) {
        default:
        case 0: {
            cinfo.comp_info[0].h_samp_factor = 2;
            cinfo.comp_info[0].v_samp_factor = 2;
            cinfo.comp_info[1].h_samp_factor = 1;
            cinfo.comp_info[1].v_samp_factor = 1;
            cinfo.comp_info[2].h_samp_factor = 1;
            cinfo.comp_info[2].v_samp_factor = 1;

        }
            break;
        case 1: {
            cinfo.comp_info[0].h_samp_factor = 2;
            cinfo.comp_info[0].v_samp_factor = 1;
            cinfo.comp_info[1].h_samp_factor = 1;
            cinfo.comp_info[1].v_samp_factor = 1;
            cinfo.comp_info[2].h_samp_factor = 1;
            cinfo.comp_info[2].v_samp_factor = 1;
        }
            break;
        case 2: {
            cinfo.comp_info[0].h_samp_factor = 1;
            cinfo.comp_info[0].v_samp_factor = 2;
            cinfo.comp_info[1].h_samp_factor = 1;
            cinfo.comp_info[1].v_samp_factor = 1;
            cinfo.comp_info[2].h_samp_factor = 1;
            cinfo.comp_info[2].v_samp_factor = 1;
        }
            break;
        case 3: {
            cinfo.comp_info[0].h_samp_factor = 1;
            cinfo.comp_info[0].v_samp_factor = 1;
            cinfo.comp_info[1].h_samp_factor = 1;
            cinfo.comp_info[1].v_samp_factor = 1;
            cinfo.comp_info[2].h_samp_factor = 1;
            cinfo.comp_info[2].v_samp_factor = 1;
        }
            break;
        }

        // Save resolution
        cinfo.X_density = INCH_TO_POINT(xRes); // It is the "invert" macro because we convert from pointer-per-inch to points
        cinfo.Y_density = INCH_TO_POINT(yRes); // It is the "invert" macro because we convert from pointer-per-inch to points
        cinfo.density_unit = 1;
        cinfo.write_JFIF_header = (boolean)true;

        // Start compression
        jpeg_start_compress(&cinfo, (boolean)true);
        // Save exif and iptc information if any available

        if (metaData && !metaData->empty()) {
            metaData->applyFilters(options.filters);
            // Save EXIF
            if (options.exif) {
                dbgFile << "Trying to save exif information";

                KisMetaData::IOBackend *exifIO = KisMetadataBackendRegistry::instance()->value("exif");
                Q_ASSERT(exifIO);

                QBuffer buffer;
                exifIO->saveTo(metaData, &buffer, KisMetaData::IOBackend::JpegHeader);

                dbgFile << "Exif information size is" << buffer.data().size();
                QByteArray data = buffer.data();
                if (data.size() < MAX_DATA_BYTES_IN_MARKER) {
                    jpeg_write_marker(&cinfo, JPEG_APP0 + 1, (const JOCTET*)data.data(), data.size());
                } else {
                    dbgFile << "EXIF information could not be saved."; // TODO: warn the user ?
                }
            }
            // Save IPTC
            if (options.iptc) {
                dbgFile << "Trying to save exif information";
                KisMetaData::IOBackend *iptcIO = KisMetadataBackendRegistry::instance()->value("iptc");
                Q_ASSERT(iptcIO);

                QBuffer buffer;
                iptcIO->saveTo(metaData, &buffer, KisMetaData::IOBackend::JpegHeader);

                dbgFile << "IPTC information size is" << buffer.data().size();
                QByteArray data = buffer.data();
                if (data.size() < MAX_DATA_BYTES_IN_MARKER) {
                    jpeg_write_marker(&cinfo, JPEG_APP0 + 13, (const JOCTET*)data.data(), data.size());
                } else {
                    dbgFile << "IPTC information could not be saved."; // TODO: warn the user ?
                }
            }
            // Save XMP
            if (options.xmp) {
                dbgFile << "Trying to save XMP information";
                KisMetaData::IOBackend *xmpIO = KisMetadataBackendRegistry::instance()->value("xmp");
                Q_ASSERT(xmpIO);

                QBuffer buffer;
                xmpIO->saveTo(metaData, &buffer, KisMetaData::IOBackend::JpegHeader);

                dbgFile << "XMP information size is" << buffer.data().size();
                QByteArray data = buffer.data();
                if (data.size() < MAX_DATA_BYTES_IN_MARKER) {
                    jpeg_write_marker(&cinfo, JPEG_APP0 + 14, (const JOCTET*)data.data(), data.size());
                } else {
                    dbgFile << "XMP information could not be saved."; // TODO: warn the user ?
                }
            }
        }



        if (options.saveProfile) {
            const auto data = cs->profile()->rawData();
            write_icc_profile(&cinfo, reinterpret_cast<const uchar *>(data.constData()), data.size());
        }
        std::vector<JSAMPLE> encoded(qsizetype(width) * cinfo.input_components);
        QByteArray input;
        const int channelBits = 8 * cs->pixelSize() / cs->channelCount();
        while (cinfo.next_scanline < height) {
            if (m_d->stop) return ImportExportCodes::Cancelled;
            auto result = rows.readRow(int(cinfo.next_scanline), input);
            if (!result.isOk()) return result;
            if (input.size() != rowBytes) return ImportExportCodes::InternalError;
            if (m_d->stop) return ImportExportCodes::Cancelled;
            auto *destination = encoded.data();
            for (uint x = 0; x < width; ++x) {
                const auto *pixel = reinterpret_cast<const quint8 *>(input.constData()) + qsizetype(x) * cs->pixelSize();
                const auto channel = [&](int index) { return channelBits == 16 ? cs->scaleToU8(pixel, index) : pixel[index]; };
                if (color_type == JCS_GRAYSCALE) *destination++ = channel(0);
                else if (color_type == JCS_RGB) {
                    *destination++ = channel(2);
                    *destination++ = channel(1);
                    *destination++ = channel(0);
                } else {
                    for (int index = 0; index < 4; ++index) *destination++ = quint8_MAX - channel(index);
                }
            }
            JSAMPROW scanline = encoded.data();
            jpeg_write_scanlines(&cinfo, &scanline, 1);
        }
        if (m_d->stop) return ImportExportCodes::Cancelled;
        jpeg_finish_compress(&cinfo);
        return ImportExportCodes::OK;
    } catch (const std::bad_alloc &) {
        return ImportExportCodes::InsufficientMemory;
    } catch (const std::runtime_error &) {
        return ImportExportCodes::ErrorWhileWriting;
    }
}


void KisJPEGConverter::cancel()
{
    m_d->stop = true;
}

