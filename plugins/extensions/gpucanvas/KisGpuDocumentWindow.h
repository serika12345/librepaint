/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_DOCUMENT_WINDOW_H
#define KIS_GPU_DOCUMENT_WINDOW_H
#include "kritagpucanvaswindow_export.h"
#include "KisGpuCanvasSession.h"
#include <QColor>
#include <QMainWindow>
#include <QTimer>
class QComboBox;
class QDoubleSpinBox;

/** Editing controls and one native canvas for a registered GPU document.
 * Successful construction transfers document-close responsibility to this window.
 * The borrowed document and application owners outlive the window on the GUI thread.
 */
class KRITAGPUCANVASWINDOW_EXPORT KisGpuDocumentWindow : public QMainWindow {
    Q_OBJECT
public:
    KisGpuDocumentWindow(KisGpuDocumentDevice &documents, KisGpuApplicationState &application,
                         KisGpuDocumentDevice::DocumentId id, QRect bounds, QWidget *parent = nullptr);
    ~KisGpuDocumentWindow() override;
    KisGpuDocumentDevice::DocumentId documentId() const { return m_id; }
    QRect canvasBounds() const { return m_bounds; }
    quint64 revision();
    bool setDisplayLookup(std::shared_ptr<const QByteArray> values);
    void saveSucceeded(quint64 revision, const QString &path);
    void saveFailed(const QString &message = {});
Q_SIGNALS:
    void saveRequested(bool saveAs);
protected:
    bool eventFilter(QObject *object, QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
private:
    void requestSave(bool saveAs);
    void updateView();
    void updateLayers();
    void updateTool();
    KisGpuDocumentDevice &m_documents;
    KisGpuDocumentDevice::DocumentId m_id;
    const QRect m_bounds;
    QWindow *m_window = nullptr;
    QComboBox *m_layers = nullptr;
    QDoubleSpinBox *m_size = nullptr;
    QAction *m_eraser = nullptr;
    QColor m_color{Qt::black};
    std::unique_ptr<KisGpuCanvasSession> m_canvas;
    std::shared_ptr<const QByteArray> m_displayLookup;
    QTimer m_refresh;
    QVector<std::weak_ptr<const KisGpuLayerDocument::RecoveryLayer>> m_observedFrame;
    quint64 m_revision = 0;
    bool m_observed = false, m_savePending = false, m_saveAs = false, m_closeAfterSave = false;
};
#endif
