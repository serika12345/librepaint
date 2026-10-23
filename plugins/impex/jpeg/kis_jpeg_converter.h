/*
 *  SPDX-FileCopyrightText: 2005 Cyrille Berger <cberger@cberger.net>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _KIS_JPEG_CONVERTER_H_
#define _KIS_JPEG_CONVERTER_H_

#include <stdio.h>

extern "C" {
#include <jpeglib.h>
}

#include <QColor>
#include <memory>

#include "kis_types.h"
#include "kis_annotation.h"
#include <KisImportExportErrorCode.h>
class KisDocument;
struct KisExportRasterRows;
namespace LibrePaint::Image::Gpu { class Execution; }

namespace KisMetaData
{
class Filter;
}

struct KisJPEGOptions {
    int quality;
    bool progressive;
    bool optimize;
    int smooth;
    bool baseLineJPEG;
    int subsampling;
    bool exif;
    bool iptc;
    bool xmp;
    QList<const KisMetaData::Filter*> filters;
    QColor transparencyFillColor;
    bool forceSRGB;
    bool saveProfile;
    bool storeDocumentMetaData; //this is for getting the metadata from the document info.
    bool storeAuthor; //this is for storing author data from the document info.
};

namespace KisMetaData
{
class Store;
}

class KisJPEGConverter : public QObject
{
    Q_OBJECT
public:
    KisJPEGConverter(KisDocument *doc, bool batchMode = false);
    ~KisJPEGConverter() override;
public:
    KisImportExportErrorCode buildImage(QIODevice *io);
    /** Decode supported RGB8 input into temporary tiles before acquiring a GPU.
     * Failure preserves the converter's previous image and the document.
     */
    KisImportExportErrorCode prepareGpuImage(QIODevice *io);
    /** Consume prepared pixels using shared execution on its owning thread. */
    KisImportExportErrorCode buildGpuImage(std::shared_ptr<LibrePaint::Image::Gpu::Execution> execution);
    KisImportExportErrorCode buildFile(QIODevice *io, KisPaintLayerSP layer, KisJPEGOptions options, KisMetaData::Store* metaData);
    KisImportExportErrorCode buildFile(QIODevice *io, const KisExportRasterRows &rows,
        qreal xRes, qreal yRes, KisJPEGOptions options, KisMetaData::Store *metaData);
    /** Retrieve the constructed image
    */
    KisImageSP image();
public Q_SLOTS:
    virtual void cancel();
private:
    KisImportExportErrorCode decode(QIODevice *io, bool gpu);
    KisImportExportErrorCode encodeRows(QIODevice *io, const KisExportRasterRows &rows,
        qreal xRes, qreal yRes, const KisJPEGOptions &options, KisMetaData::Store *metaData);
private:
    struct Private;
    QScopedPointer<Private> m_d;
};

#endif
