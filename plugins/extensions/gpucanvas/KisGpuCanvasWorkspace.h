/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_CANVAS_WORKSPACE_H
#define KIS_GPU_CANVAS_WORKSPACE_H
#include "KisGpuDocumentWindow.h"
#include <QPointer>
#include <map>
struct KisGpuKraMetadata;

/** Product document creation and KRA operations for owned editing windows.
 * The shared document and application owners outlive this GUI-thread workspace.
 * Native parsing uses its synchronous contract; saves advance through a timer.
 */
class KisGpuCanvasWorkspace : public QObject {
    Q_OBJECT
public:
    KisGpuCanvasWorkspace(KisGpuDocumentDevice &documents, KisGpuApplicationState &application,
                          QWidget *windowParent = nullptr, QObject *parent = nullptr);
    ~KisGpuCanvasWorkspace() override;
    KisGpuDocumentWindow *createCanvas(QSize size);
    KisGpuDocumentWindow *openCanvas(const QString &path);
    bool saveCanvas(KisGpuDocumentWindow &window, const QString &path);
    QString lastError() const { return m_error; }
protected:
    bool eventFilter(QObject *object, QEvent *event) override;
private:
    struct Entry;
    KisGpuDocumentWindow *showDocument(quint64 id, KisGpuKraMetadata metadata, QString path);
    void pollSaves();
    void releaseReading(quint64 closing = 0);
    KisGpuDocumentDevice &m_documents;
    KisGpuApplicationState &m_application;
    QPointer<QWidget> m_parent;
    std::map<KisGpuDocumentWindow *, std::unique_ptr<Entry>> m_entries;
    QTimer m_poll;
    QString m_error;
    bool m_stopping = false;
};
#endif
