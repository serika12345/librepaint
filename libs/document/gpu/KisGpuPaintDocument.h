/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_PAINT_DOCUMENT_H
#define KIS_GPU_PAINT_DOCUMENT_H

#include <gpu/KisGpuOperationDocument.h>
#include <QObject>
#include <memory>
#include <kritadocumentpainting_export.h>

namespace Krita::Image::Gpu { class Execution; struct ExecutionOutcome; }
namespace Krita::Document::Gpu {
namespace Paint = Image::Gpu;
enum class FileCode { Success, InvalidData, UnsupportedData, CapacityExceeded, IoFailure, Busy, DeviceFailure, Closed };
struct FileOutcome
{
    FileCode code = FileCode::InvalidData;
    Paint::Snapshot version;
    QString diagnostic;
};
struct FileOpenOutcome;

/** Owns one painting document, history, file identity and GPU attachment.
 * All calls and destruction use the GUI/device thread. The borrowed execution
 * owner outlives this document and its display windows. Windows suspend their
 * document handle when close() is called. Accepted mutations update the latest
 * GPU request before changed() is emitted. GPU failures retain the logical edit.
 * Construction throws std::invalid_argument for invalid values and
 * std::length_error when the execution owner cannot admit another document.
 */
class KRITADOCUMENTPAINTING_EXPORT PaintDocument : public QObject
{
    Q_OBJECT
public:
    explicit PaintDocument(Paint::Execution &execution, QSize size, Paint::Limits limits = {});
    explicit PaintDocument(Paint::Execution &execution, const Paint::RasterImage &image, Paint::Limits limits = {});
    ~PaintDocument() override;
    const Paint::OperationDocument &operations() const;
    quint64 handle() const;
    QString filePath() const;
    /** Last request admission; a failure preserves the accepted logical edit. */
    Paint::ExecutionOutcome lastRequestOutcome() const;

    Paint::OperationId beginStroke(Paint::LayerId layer, const Paint::Brush &brush, quint64 seed);
    Paint::Result appendSamples(Paint::OperationId id, const QList<Paint::InputSample> &samples);
    Paint::Result updateShape(Paint::OperationId id, const Paint::Shape &shape);
    Paint::Result commitStroke(Paint::OperationId id);
    Paint::Result cancelStroke(Paint::OperationId id);
    Paint::Result replaceTiles(Paint::LayerId layer, const QList<Paint::RgbaTile> &tiles);
    Paint::LayerId addLayer(const QString &name);
    Paint::Result selectLayer(Paint::LayerId layer);
    Paint::Result setLayerVisible(Paint::LayerId layer, bool visible);
    Paint::Result undo();
    Paint::Result redo();
    void close();

    /** Reads KRA before admitting a new document; failures return no attachment. */
    static FileOpenOutcome openKra(Paint::Execution &execution, const QString &path, Paint::Limits limits = {});
    /** Saves a committed fixed version atomically; later edits remain modified. */
    FileOutcome saveKra(const QString &path, const Paint::Snapshot &version = {});
    /** Bounded logical recovery, including file metadata and pending input. */
    QByteArray recoveryData() const;
    /** Validates before adopting a fresh operation identity on the same handle. */
    Paint::Result restoreRecovery(const QByteArray &data);

Q_SIGNALS:
    void changed();

private:
    PaintDocument(Paint::Execution &execution, std::unique_ptr<Paint::OperationDocument> document, Paint::Limits limits);
    bool onThread() const;
    Paint::Result publish(Paint::Result result);
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
