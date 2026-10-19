/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_PAINT_DOCUMENT_H
#define KIS_GPU_PAINT_DOCUMENT_H

#include <gpu/KisGpuPaintImage.h>
#include <QObject>
#include <memory>
#include <kritadocumentpainting_export.h>

namespace LibrePaint::ImportExport::Kra { struct ImageMetadata; struct MetadataOutcome; }
class QIODevice;
namespace LibrePaint::Document::Gpu {
namespace Paint = Image::Gpu;
enum class FileCode { Success, InvalidData, UnsupportedData, CapacityExceeded, IoFailure, Busy, DeviceFailure, Closed };
struct FileOutcome
{
    FileCode code = FileCode::InvalidData;
    Paint::Snapshot version;
    QString diagnostic;
};
struct FileOpenOutcome;

/** Owns file identity, format metadata and a shared editable image.
 * All calls and destruction use the GUI/device thread. The borrowed execution
 * owner outlives this document, retained images and display windows. Destruction
 * releases the image reference; close() closes the shared image. Image edits
 * forward notifications after request admission. Saving and recovery notify
 * with adopted file metadata.
 * Construction throws std::invalid_argument for invalid values and
 * std::length_error when the execution owner cannot admit another document.
 */
class KRITADOCUMENTPAINTING_EXPORT PaintDocument : public QObject
{
    Q_OBJECT
public:
    explicit PaintDocument(Paint::Execution &execution, QSize size, Paint::Limits limits = {});
    explicit PaintDocument(Paint::Execution &execution, const Paint::RasterImage &image, Paint::Limits limits = {});
    /** Adopts an open shared image on its thread without allocating another attachment. */
    explicit PaintDocument(Paint::PaintImageSP image, Paint::Limits limits = {});
    ~PaintDocument() override;
    Paint::PaintImageSP image() const;
    QString filePath() const;
    void close();

    /** Reads KRA before admitting a new document; failures return no attachment. */
    static FileOpenOutcome openKra(Paint::Execution &execution, const QString &path, Paint::Limits limits = {});
    /** Retains shared execution through the loaded image and subsequent clones. */
    static FileOpenOutcome openKra(std::shared_ptr<Paint::Execution> execution, const QString &path, Paint::Limits limits = {});
    ImportExport::Kra::MetadataOutcome kraImageMetadata() const;
    /** Saves a committed fixed version atomically; later edits remain modified. */
    FileOutcome saveKra(const QString &path, const Paint::Snapshot &version = {});
    /** Writes a fixed version while preserving file identity and saved state. */
    FileOutcome exportKra(const QString &path, const Paint::Snapshot &version = {});
    /** Writes a committed fixed version to a borrowed open, empty, seekable
     * device, preserving file identity and saved state. Leaves the device open.
     * Uses the document thread; failures return a diagnostic.
     */
    FileOutcome exportKra(QIODevice &destination, const Paint::Snapshot &version = {});
    /** Adopts the identity and fixed saved version of a completed format write.
     * Later edits retain their modified state. Invalid versions preserve identity.
     */
    FileOutcome adoptSavedFile(const QString &path, const Paint::Snapshot &version);
    /** Validates image metadata before replacing the retained KRA description and files. */
    FileOutcome setKraImageMetadata(const ImportExport::Kra::ImageMetadata &metadata);
    /** Bounded logical recovery, including file metadata and pending input. */
    QByteArray recoveryData() const;
    /** Validates before adopting a fresh operation identity on the same handle. */
    Paint::Result restoreRecovery(const QByteArray &data);

Q_SIGNALS:
    void changed();

private:
    PaintDocument(Paint::Execution &execution, Paint::PaintImageSP image, Paint::Limits limits);
    bool onThread() const;
    static FileOpenOutcome openKraImpl(Paint::Execution &execution, std::shared_ptr<Paint::Execution> retainedExecution,
                                       const QString &path, Paint::Limits limits);
    FileOutcome writeKra(const QString &path, const Paint::Snapshot &version, bool adoptFile);
    struct Private;
    std::unique_ptr<Private> m_d;
};
struct FileOpenOutcome
{
    FileCode code = FileCode::InvalidData;
    std::unique_ptr<PaintDocument> document;
    QString diagnostic;
};
}
#endif
