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
#include <QCryptographicHash>
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
#include <gpu/KisGpuPaintImage.h>

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

void orientJpegRaster(LibrePaint::Image::Gpu::RasterImage &image, int orientation)
{
    if (orientation < 2 || orientation > 8) return;
    using LibrePaint::Image::Gpu::RgbaTile;
    const int width = image.size.width(), height = image.size.height();
    // Device origins remain public layer positions; tile coordinates stay in image space.
    if (orientation == 3) image.layers[0].position = QPoint(width - 1, height - 1);
    else if (orientation >= 5 && orientation <= 7) image.layers[0].position = QPoint(height - 1, 0);
    else if (orientation == 8) image.layers[0].position = QPoint(0, width - 1);
    const QSize size = orientation >= 5 ? QSize(height, width) : image.size;
    const int columns = (size.width() + RgbaTile::Size - 1) / RgbaTile::Size;
    const int rows = (size.height() + RgbaTile::Size - 1) / RgbaTile::Size;
    QList<RgbaTile> tiles;
    for (int y = 0; y < rows; ++y)
        for (int x = 0; x < columns; ++x)
            tiles.append({QPoint(x, y), QByteArray(RgbaTile::ByteCount, '\0')});
    const int sourceColumns = (width + RgbaTile::Size - 1) / RgbaTile::Size;
    const auto &source = image.layers[0].tiles;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int dx = x, dy = y;
            switch (orientation) {
            case 2: dx = width - 1 - x; break;
            case 3: dx = width - 1 - x; dy = height - 1 - y; break;
            case 4: dy = height - 1 - y; break;
            case 5: dx = y; dy = x; break;
            case 6: dx = height - 1 - y; dy = x; break;
            case 7: dx = height - 1 - y; dy = width - 1 - x; break;
            case 8: dx = y; dy = width - 1 - x; break;
            }
            const auto &input = source[(y / RgbaTile::Size) * sourceColumns + x / RgbaTile::Size];
            auto &output = tiles[(dy / RgbaTile::Size) * columns + dx / RgbaTile::Size];
            std::memcpy(output.rgba.data() + ((dy % RgbaTile::Size) * RgbaTile::Size + dx % RgbaTile::Size) * 4,
                        input.rgba.constData() + ((y % RgbaTile::Size) * RgbaTile::Size + x % RgbaTile::Size) * 4, 4);
        }
    }
    image.layers[0].tiles = std::move(tiles);
    image.size = size;
}

}

struct KisJPEGConverter::Private
{
    struct PreparedImage
    {
        LibrePaint::Image::Gpu::RasterImage raster;
        const KoColorSpace *colorSpace = nullptr;
        QPointF resolution{1, 1};
        KisMetaData::Store metadata;
    };
    Private(KisDocument *doc, bool batchMode)
        : doc(doc),
          stop(false),
          batchMode(batchMode)
    {}

    KisImageSP image;
    KisDocument *doc;
    bool stop;
    bool batchMode;
    std::unique_ptr<PreparedImage> prepared;
};

KisJPEGConverter::KisJPEGConverter(KisDocument *doc, bool batchMode)
    : m_d(new Private(doc, batchMode))
{
}

KisJPEGConverter::~KisJPEGConverter()
{
}

KisImportExportErrorCode KisJPEGConverter::decode(QIODevice *io, bool gpu)
{
    namespace Gpu = LibrePaint::Image::Gpu;
    if (!io || !io->isReadable()) return ImportExportCodes::NoAccessToRead;
    struct jpeg_decompress_struct cinfo{};
    struct jpeg_error_mgr jerr;
    QCryptographicHash inputHash{QCryptographicHash::Sha256};
    const bool newImage = !m_d->image;
    auto prepared = gpu ? std::make_unique<Private::PreparedImage>() : nullptr;
    const auto cleanup = qScopeGuard([&] { if (cinfo.mem) jpeg_destroy_decompress(&cinfo); });

    cinfo.err = jpeg_std_error(&jerr);
    jerr.error_exit = jpegErrorExit;

    try {
        jpeg_create_decompress(&cinfo);

        KisJPEGSource::setSource(&cinfo, io, &inputHash);

        jpeg_save_markers(&cinfo, JPEG_COM, 0xFFFF);
        /* Save APP0..APP15 markers */
        for (int m = 0; m < 16; m++)
            jpeg_save_markers(&cinfo, JPEG_APP0 + m, 0xFFFF);


        //     setup_read_icc_profile(&cinfo);
        // read header
        jpeg_read_header(&cinfo, (boolean)true);

        if (gpu) {
            if (!cinfo.image_width || !cinfo.image_height || cinfo.image_width > Gpu::OperationDocument::CoordinateLimit ||
                cinfo.image_height > Gpu::OperationDocument::CoordinateLimit)
                return ImportExportCodes::FormatFeaturesUnsupported;
            const quint64 columns = (quint64(cinfo.image_width) + Gpu::RgbaTile::Size - 1) / Gpu::RgbaTile::Size;
            const quint64 rows = (quint64(cinfo.image_height) + Gpu::RgbaTile::Size - 1) / Gpu::RgbaTile::Size;
            if (columns * rows > quint64(Gpu::Limits{}.rasterBytes / Gpu::RgbaTile::ByteCount))
                return ImportExportCodes::InsufficientMemory;
            if (cinfo.data_precision != 8) return ImportExportCodes::FormatColorSpaceUnsupported;
        }

        // Get the colorspace
        QString modelId = getColorSpaceModelForColorType(cinfo.out_color_space);
        if (modelId.isEmpty()) {
            dbgFile << "unsupported colorspace :" << cinfo.out_color_space;
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
            if (!profile) return ImportExportCodes::FileFormatIncorrect;
            if (gpu && profile_rawdata != KoColorSpaceRegistry::instance()->rgb8()->profile()->rawData())
                return ImportExportCodes::FormatColorSpaceUnsupported;
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
            return ImportExportCodes::FormatColorSpaceUnsupported;
        }

        // Create the cmsTransform if needed
        std::unique_ptr<KoColorTransformation> transform;
        if (profile && !profile->isSuitableForWorkspace() && profile->isSuitableForInput()) {
            transform.reset(KoColorSpaceRegistry::instance()
                ->colorSpace(modelId, Integer8BitsColorDepthID.id(), profile)
                ->createColorConverter(cs, KoColorConversionTransformation::internalRenderingIntent(), KoColorConversionTransformation::internalConversionFlags()));
        }

        // Apparently an invalid transform was created from the profile. See bug https://bugs.kde.org/show_bug.cgi?id=255451.
        // After 2.3: warn the user!
        if (transform && !transform->isValid()) {
            transform.reset();
        }

        // Creating the KisImageSP
        if (gpu) {
            if (cinfo.out_color_space != JCS_RGB || cs->id() != QStringLiteral("RGBA") || transform ||
                cs->profile()->rawData() != KoColorSpaceRegistry::instance()->rgb8()->profile()->rawData())
                return ImportExportCodes::FormatColorSpaceUnsupported;
            prepared->colorSpace = cs;
            prepared->raster.size = QSize(cinfo.image_width, cinfo.image_height);
            Gpu::RasterLayer layer;
            layer.name = i18nc("Name for the bottom-most layer in the layerstack", "Background");
            const int columns = (cinfo.image_width + Gpu::RgbaTile::Size - 1) / Gpu::RgbaTile::Size;
            const int rows = (cinfo.image_height + Gpu::RgbaTile::Size - 1) / Gpu::RgbaTile::Size;
            for (int y = 0; y < rows; ++y)
                for (int x = 0; x < columns; ++x)
                    layer.tiles.append({QPoint(x, y), QByteArray(Gpu::RgbaTile::ByteCount, '\0')});
            prepared->raster.layers.append(std::move(layer));
        } else if (!m_d->image) {
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
        if (gpu) prepared->resolution = QPointF(POINT_TO_INCH(xres), POINT_TO_INCH(yres));
        else m_d->image->setResolution(POINT_TO_INCH(xres), POINT_TO_INCH(yres));

        // Create layer
        KisPaintLayerSP layer;
        if (!gpu) layer = new KisPaintLayer(m_d->image.data(), m_d->image->nextLayerName(), quint8_MAX);
        auto *metadata = gpu ? &prepared->metadata : layer->metaData();
        int orientation = 1;

        // Read data
        jpeg_start_decompress(&cinfo);
        auto row = std::make_unique<JSAMPLE[]>(size_t(cinfo.image_width) * cinfo.num_components);
        JSAMPROW row_pointer = row.get();

        for (; cinfo.output_scanline < cinfo.image_height;) {
            const auto y = cinfo.output_scanline;
            KisHLineIteratorSP it;
            if (!gpu) it = layer->paintDevice()->createHLineIteratorNG(0, y, cinfo.image_width);
            jpeg_read_scanlines(&cinfo, &row_pointer, 1);
            quint8 *src = row_pointer;
            if (gpu) {
                const int columns = (cinfo.image_width + Gpu::RgbaTile::Size - 1) / Gpu::RgbaTile::Size;
                auto &tiles = prepared->raster.layers[0].tiles;
                for (quint32 x = 0; x < cinfo.image_width; ++x) {
                    auto &tile = tiles[(y / Gpu::RgbaTile::Size) * columns + x / Gpu::RgbaTile::Size];
                    auto *destination = reinterpret_cast<quint8 *>(tile.rgba.data()) +
                        ((y % Gpu::RgbaTile::Size) * Gpu::RgbaTile::Size + x % Gpu::RgbaTile::Size) * 4;
                    destination[0] = src[x * 3];
                    destination[1] = src[x * 3 + 1];
                    destination[2] = src[x * 3 + 2];
                    destination[3] = 255;
                }
                continue;
            }
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

        if (!gpu) m_d->image->addNode(KisNodeSP(layer.data()), m_d->image->rootLayer().data());

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
            buf.open(QIODevice::ReadOnly);
            exifIO->loadFrom(metadata, &buf);
            // Interpret orientation tag
            if (metadata->containsEntry("http://ns.adobe.com/tiff/1.0/", "Orientation")) {
                KisMetaData::Entry& entry = metadata->getEntry("http://ns.adobe.com/tiff/1.0/", "Orientation");
                if (entry.value().type() == KisMetaData::Value::Variant) {
                    if (gpu) orientation = entry.value().asVariant().toInt();
                    else switch (entry.value().asVariant().toInt()) {
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
                        // Finish direct pixel writes before queuing rotation.
                        KisTransformWorker::mirrorY(layer->paintDevice());
                        image()->rotateImage(M_PI / 2);
                        break;
                    case 6:
                        image()->rotateImage(M_PI / 2);
                        break;
                    case 7:
                        KisTransformWorker::mirrorX(layer->paintDevice());
                        image()->rotateImage(M_PI / 2);
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
                    buf.open(QIODevice::ReadOnly);
                    iptcIO->loadFrom(metadata, &buf);
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
            QBuffer buf(&byteArray);
            buf.open(QIODevice::ReadOnly);
            xmpIO->loadFrom(metadata, &buf);
            break;
        }

        // Dump loaded metadata
        metadata->debugDump();

        // Check whether the metadata has resolution info, too...
        if (cinfo.density_unit == 0 && metadata->containsEntry("tiff:XResolution") && metadata->containsEntry("tiff:YResolution")) {
            double xres = metadata->getEntry("tiff:XResolution").value().asDouble();
            double yres = metadata->getEntry("tiff:YResolution").value().asDouble();
            if (xres != 0 && yres != 0) {
                if (gpu) prepared->resolution = QPointF(POINT_TO_INCH(xres), POINT_TO_INCH(yres));
                else m_d->image->setResolution(POINT_TO_INCH(xres), POINT_TO_INCH(yres));
            }
        }

        // Finish decompression
        jpeg_finish_decompress(&cinfo);
        if (gpu || newImage) {
            // JPEG carries no layer identity. The versioned format namespace
            // and consumed input give repeated imports the same identity.
            const auto uuid = QUuid::createUuidV5(QUuid(QStringLiteral("{4463a421-f12b-40ef-bbd7-8b7885ad3698}")), inputHash.result());
            if (gpu) prepared->raster.layers[0].uuid = uuid;
            else layer->setUuid(uuid);
        }
        if (gpu) {
            orientJpegRaster(prepared->raster, orientation);
            m_d->prepared = std::move(prepared);
        }
        return ImportExportCodes::OK;
    }
    catch( std::runtime_error &) {
        return ImportExportCodes::FileFormatIncorrect;
    }
}



KisImportExportErrorCode KisJPEGConverter::buildImage(QIODevice *io)
{
    return decode(io, false);
}

KisImportExportErrorCode KisJPEGConverter::prepareGpuImage(QIODevice *io)
{
    m_d->prepared.reset();
    try { return decode(io, true); }
    catch (const std::bad_alloc &) { return ImportExportCodes::InsufficientMemory; }
}

KisImportExportErrorCode KisJPEGConverter::buildGpuImage(std::shared_ptr<LibrePaint::Image::Gpu::Execution> execution)
{
    if (!m_d->prepared || !execution) return ImportExportCodes::Failure;
    auto prepared = std::move(m_d->prepared);
    prepared->raster.colorProfile = prepared->colorSpace->profile()->rawData();
    try {
        auto image = KisImage::fromGpuImage(
            LibrePaint::Image::Gpu::PaintImageSP::create(execution, prepared->raster),
            QStringLiteral("built image"), prepared->colorSpace);
        image->setResolution(prepared->resolution.x(), prepared->resolution.y());
        auto *layer = qobject_cast<KisLayer *>(image->root()->firstChild().data());
        layer->metaData()->copyFrom(&prepared->metadata);
        m_d->image = image;
        return ImportExportCodes::OK;
    } catch (const std::bad_alloc &) { return ImportExportCodes::InsufficientMemory; }
    catch (const std::exception &error) { return KisImportExportErrorCode(QString::fromUtf8(error.what())); }
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

