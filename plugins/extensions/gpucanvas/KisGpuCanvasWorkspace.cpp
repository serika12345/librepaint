/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuCanvasWorkspace.h"
#include "KisGpuKraLoadJob.h"
#include "KisGpuKraSaveJob.h"
#include <KoStore.h>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDomDocument>
#include <QEventLoop>
#include <QFileDialog>
#include <QProgressDialog>
#include <QScopeGuard>
#include <stdexcept>

namespace {
constexpr quint64 FileSnapshotBytes = 64 * 1024 * 1024;
QByteArray savedDocumentInfo(const QString &path)
{
    std::unique_ptr<KoStore> store(KoStore::createStore(path, KoStore::Read));
    if (!store || store->bad() || !store->open(QStringLiteral("documentinfo.xml"))) return {};
    const auto size = store->size();
    if (size <= 0 || quint64(size) > FileSnapshotBytes) return {};
    const auto bytes = store->read(size);
    QDomDocument xml;
    if (bytes.size() != size || !xml.setContent(bytes)
        || xml.documentElement().tagName() != QStringLiteral("document-info")) return {};
    return bytes;
}
}

struct KisGpuCanvasWorkspace::Entry {
    quint64 id = 0, savedRevision = 0;
    KisGpuKraMetadata metadata;
    QString path, destination;
    std::unique_ptr<KisGpuKraSaveJob> saving;
    bool metadataAvailable = true;
};

KisGpuCanvasWorkspace::KisGpuCanvasWorkspace(KisGpuDocumentDevice &documents, KisGpuApplicationState &application,
    QWidget *windowParent, QObject *parent)
    : QObject(parent), m_documents(documents), m_application(application), m_parent(windowParent)
{
    if (m_parent) m_parent->installEventFilter(this);
    m_poll.setInterval(16);
    connect(&m_poll, &QTimer::timeout, this, &KisGpuCanvasWorkspace::pollSaves);
    connect(&documents, &KisGpuDocumentDevice::aboutToReleaseGpu, this, [this] { releaseReading(); }, Qt::DirectConnection);
    connect(&documents, &KisGpuDocumentDevice::documentAboutToClose, this, &KisGpuCanvasWorkspace::releaseReading, Qt::DirectConnection);
    connect(qApp, &QCoreApplication::aboutToQuit, this, [this] {
        m_stopping = true;
        m_poll.stop();
        while (!m_entries.empty()) delete m_entries.begin()->first;
    });
}

KisGpuCanvasWorkspace::~KisGpuCanvasWorkspace()
{
    m_poll.stop();
    if (m_parent) m_parent->removeEventFilter(this);
    while (!m_entries.empty()) delete m_entries.begin()->first;
}

bool KisGpuCanvasWorkspace::eventFilter(QObject *object, QEvent *event)
{
    if (object == m_parent && event->type() == QEvent::Close) {
        QVector<QPointer<KisGpuDocumentWindow>> windows;
        for (const auto &[window, entry] : m_entries) { Q_UNUSED(entry); windows.append(window); }
        for (const auto &window : windows) {
            if (window && !window->close()) { event->ignore(); return true; }
        }
    }
    return QObject::eventFilter(object, event);
}

KisGpuDocumentWindow *KisGpuCanvasWorkspace::showDocument(quint64 id, KisGpuKraMetadata metadata, QString path)
{
    try {
        auto window = std::make_unique<KisGpuDocumentWindow>(m_documents, m_application, id,
            QRect(QPoint(0, 0), metadata.canvasSize), m_parent);
        window->setAttribute(Qt::WA_DeleteOnClose);
        auto entry = std::make_unique<Entry>();
        entry->id = id; entry->metadata = std::move(metadata); entry->path = std::move(path);
        if (!entry->path.isEmpty()) window->saveSucceeded(window->revision(), entry->path);
        auto *editor = window.get();
        m_entries.emplace(editor, std::move(entry));
        connect(editor, &QObject::destroyed, this, [this, editor] {
            // Detach before a native save's destructor can enter an event loop.
            auto removed = m_entries.extract(editor);
        });
        connect(editor, &KisGpuDocumentWindow::saveRequested, this, [this, editor](bool saveAs) {
            const auto found = m_entries.find(editor);
            if (found == m_entries.end()) return;
            auto path = found->second->path;
            const QPointer<KisGpuCanvasWorkspace> workspace(this);
            const QPointer<KisGpuDocumentWindow> window(editor);
            if (saveAs || path.isEmpty()) path = QFileDialog::getSaveFileName(editor, tr("Save Canvas"), path, tr("KRA documents (*.kra)"));
            if (!workspace || !window) return;
            if (path.isEmpty()) { editor->saveFailed(); return; }
            if (!path.endsWith(QStringLiteral(".kra"), Qt::CaseInsensitive)) path += QStringLiteral(".kra");
            saveCanvas(*editor, path);
        });
        editor->show();
        m_error.clear();
        return window.release();
    } catch (const std::exception &) {
        m_documents.closeDocument(id);
        m_error = tr("The canvas could not be created.");
        return nullptr;
    }
}

KisGpuDocumentWindow *KisGpuCanvasWorkspace::createCanvas(QSize size)
{
    if (m_stopping) return nullptr;
    if (size.isEmpty()) { m_error = tr("Canvas dimensions must be positive."); return nullptr; }
    m_application.resumeForInput();
    const auto id = m_documents.createDocument({});
    if (!id) { m_error = tr("Drawing resources are unavailable."); return nullptr; }
    if (m_documents.document(id)->addLayer(tr("Paint")).result != KisGpuLayerDocument::Result::Accepted) {
        m_documents.closeDocument(id); m_error = tr("The paint layer could not be created."); return nullptr;
    }
    return showDocument(id, {size, tr("Untitled")}, {});
}

KisGpuDocumentWindow *KisGpuCanvasWorkspace::openCanvas(const QString &path)
{
    if (m_stopping) return nullptr;
    auto &documents = m_documents;
    const QPointer<KisGpuCanvasWorkspace> workspace(this);
    m_application.resumeForInput();
    const auto id = m_documents.createDocument({});
    if (!id) { m_error = tr("Drawing resources are unavailable."); return nullptr; }
    bool adopted = false;
    const auto cleanup = qScopeGuard([&documents, id, &adopted] { if (!adopted) documents.closeDocument(id); });
    KisGpuKraLoadJob load(documents.document(id), path, FileSnapshotBytes);
    QPointer<QProgressDialog> progress = new QProgressDialog(tr("Opening canvas…"), tr("Cancel"), 0, 0, m_parent);
    const auto closeProgress = qScopeGuard([&progress] { delete progress.data(); });
    progress->setWindowModality(Qt::WindowModal);
    QEventLoop loop;
    QTimer timer;
    bool cancelled = false;
    connect(progress, &QProgressDialog::canceled, &loop, [&] { cancelled = true; loop.quit(); });
    connect(progress, &QObject::destroyed, &loop, [&] { cancelled = true; loop.quit(); });
    connect(this, &QObject::destroyed, &loop, &QEventLoop::quit);
    connect(qApp, &QCoreApplication::aboutToQuit, &loop, &QEventLoop::quit);
    connect(&documents, &KisGpuDocumentDevice::documentAboutToClose, &loop, [&](auto closing) {
        if (closing == id) loop.quit();
    });
    connect(&timer, &QTimer::timeout, &loop, [&] {
        if (!workspace) { loop.quit(); return; }
        try { load.poll(); }
        catch (const std::exception &) {
            if (workspace) workspace->m_error = tr("The document could not be opened.");
            loop.quit(); return;
        }
        if (load.state() == KisGpuKraLoadJob::State::Succeeded || load.state() == KisGpuKraLoadJob::State::Failed) loop.quit();
    });
    m_error.clear();
    timer.start(16);
    progress->show();
    loop.exec();
    timer.stop();
    if (progress) progress->hide();
    if (!workspace || m_stopping) return nullptr;
    if (cancelled) { m_error.clear(); return nullptr; }
    if (load.state() != KisGpuKraLoadJob::State::Succeeded) {
        if (m_error.isEmpty()) m_error = load.error() == KisGpuKraLoadJob::Error::UnsupportedDocument
            ? tr("This canvas supports RGBA8 sRGB raster layers. The document contains unsupported content.")
            : tr("The document could not be opened within the available resources.");
        return nullptr;
    }
    auto *window = showDocument(id, *load.metadata(), path);
    adopted = window;
    return window;
}

bool KisGpuCanvasWorkspace::saveCanvas(KisGpuDocumentWindow &window, const QString &path)
{
    const auto found = m_entries.find(&window);
    const auto model = m_documents.document(window.documentId());
    auto *canvas = window.findChild<KisGpuCanvasSession *>();
    if (m_stopping || found == m_entries.end() || found->second->saving || path.isEmpty() || !model || !canvas
        || canvas->hasPendingInput() || model->state() != KisGpuLayerDocument::State::Idle) {
        m_error = tr("Finish the current document operation before saving.");
        window.saveFailed(m_error); return false;
    }
    auto &entry = *found->second;
    if (!entry.metadataAvailable) {
        m_error = tr("The saved document information could not be read. Reopen the saved file before saving again.");
        window.saveFailed(m_error); return false;
    }
    entry.savedRevision = window.revision();
    entry.destination = path;
    entry.saving = std::make_unique<KisGpuKraSaveJob>(m_documents.store(entry.id), model->layers(),
        model->recovery().nextId, entry.metadata, path, FileSnapshotBytes);
    if (entry.saving->state() == KisGpuKraSaveJob::State::Failed) {
        m_error = entry.saving->errorMessage(); entry.saving.reset(); window.saveFailed(m_error); return false;
    }
    m_error.clear();
    if (!m_poll.isActive()) m_poll.start();
    return true;
}

void KisGpuCanvasWorkspace::releaseReading(quint64 closing)
{
    for (const auto &[window, entry] : m_entries) {
        if (entry->saving && (!closing || closing == entry->id)
            && entry->saving->state() == KisGpuKraSaveJob::State::Reading) {
            entry->saving.reset();
            window->saveFailed(tr("Drawing resources were released before saving completed. Save again to retry."));
        }
    }
}

void KisGpuCanvasWorkspace::pollSaves()
{
    bool pending = false;
    QVector<QPointer<KisGpuDocumentWindow>> windows;
    for (const auto &[window, entry] : m_entries) if (entry->saving) windows.append(window);
    for (const auto &window : windows) {
        if (!window) continue;
        auto found = m_entries.find(window);
        if (found == m_entries.end() || !found->second->saving) continue;
        found->second->saving->poll();
        found = m_entries.find(window);
        if (!window || found == m_entries.end() || !found->second->saving) continue;
        auto &entry = *found->second;
        const auto state = entry.saving->state();
        if (state == KisGpuKraSaveJob::State::Succeeded) {
            const auto information = savedDocumentInfo(entry.destination);
            if (information.isEmpty()) {
                entry.saving.reset(); entry.metadataAvailable = false;
                m_error = tr("The file was written, but its document information could not be read. Reopen the saved file before saving again.");
                window->saveFailed(m_error); continue;
            }
            entry.metadata.documentInfoXml = information;
            entry.saving.reset(); entry.path = entry.destination;
            window->saveSucceeded(entry.savedRevision, entry.path);
        } else if (state == KisGpuKraSaveJob::State::Failed) {
            m_error = entry.saving->errorMessage(); entry.saving.reset(); window->saveFailed(m_error);
        } else pending = true;
    }
    if (!pending) m_poll.stop();
}
