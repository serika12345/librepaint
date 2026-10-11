/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuDocumentWindow.h"
#include <QAction>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QToolBar>
#include <QWindow>
#include <algorithm>

KisGpuDocumentWindow::KisGpuDocumentWindow(KisGpuDocumentDevice &documents, KisGpuApplicationState &application,
    KisGpuDocumentDevice::DocumentId id, QRect bounds, QWidget *parent)
    : QMainWindow(parent, Qt::Window), m_documents(documents), m_id(id), m_bounds(bounds)
{
    setWindowTitle(tr("GPU Canvas"));
    resize(800, 700);
    m_window = new QWindow;
    m_window->setObjectName(QStringLiteral("gpuDocumentCanvas-%1").arg(id));
    Krita::Canvas::SurfaceRenderer::prepareWindow(*m_window);
    setCentralWidget(QWidget::createWindowContainer(m_window, this));
    centralWidget()->setFocusPolicy(Qt::StrongFocus);
    m_canvas = std::make_unique<KisGpuCanvasSession>(documents, application, id, *m_window, bounds, this);
    m_window->installEventFilter(this);
    connect(m_canvas.get(), &KisGpuCanvasSession::documentClosed, this, [this] {
        m_id = 0;
        m_refresh.stop();
        close();
    });
    connect(m_canvas.get(), &KisGpuCanvasSession::inputRejected, this, [this] {
        statusBar()->showMessage(tr("The drawing could not be accepted. Try again after freeing document memory."));
    });
    auto *toolbar = addToolBar(tr("Painting"));
    auto *undo = toolbar->addAction(tr("Undo"));
    undo->setObjectName(QStringLiteral("gpuUndo"));
    undo->setShortcuts(QKeySequence::Undo);
    connect(undo, &QAction::triggered, this, [this] {
        if (auto model = m_documents.document(m_id); model && model->undo()) updateLayers();
    });
    auto *redo = toolbar->addAction(tr("Redo"));
    redo->setObjectName(QStringLiteral("gpuRedo"));
    redo->setShortcuts(QKeySequence::Redo);
    connect(redo, &QAction::triggered, this, [this] {
        if (auto model = m_documents.document(m_id); model && model->redo()) updateLayers();
    });
    toolbar->addSeparator();
    m_size = new QDoubleSpinBox(toolbar);
    m_size->setRange(1, 2048);
    m_size->setValue(24);
    m_size->setSuffix(tr(" px"));
    m_size->setAccessibleName(tr("Brush size"));
    toolbar->addWidget(m_size);
    connect(m_size, &QDoubleSpinBox::valueChanged, this, &KisGpuDocumentWindow::updateTool);
    auto *color = toolbar->addAction(tr("Color"));
    connect(color, &QAction::triggered, this, [this] {
        const auto selected = QColorDialog::getColor(m_color, this, tr("Brush color"), QColorDialog::ShowAlphaChannel);
        if (selected.isValid()) { m_color = selected; updateTool(); }
    });
    m_eraser = toolbar->addAction(tr("Eraser"));
    m_eraser->setCheckable(true);
    connect(m_eraser, &QAction::toggled, this, &KisGpuDocumentWindow::updateTool);
    toolbar->addSeparator();
    m_layers = new QComboBox(toolbar);
    m_layers->setAccessibleName(tr("Paint layer"));
    toolbar->addWidget(m_layers);
    connect(m_layers, &QComboBox::currentIndexChanged, this, &KisGpuDocumentWindow::updateTool);
    auto *add = toolbar->addAction(tr("Add Layer"));
    add->setObjectName(QStringLiteral("gpuAddLayer"));
    connect(add, &QAction::triggered, this, [this] {
        const auto model = m_documents.document(m_id);
        if (!model) return;
        const auto layer = model->addLayer(tr("Paint %1").arg(model->layers().size() + 1));
        if (layer.result != KisGpuLayerDocument::Result::Accepted) return;
        updateLayers();
        m_layers->setCurrentIndex(m_layers->findData(QVariant::fromValue(layer.id)));
    });
    updateLayers();
    updateView();
    m_refresh.setInterval(100);
    connect(&m_refresh, &QTimer::timeout, this, [this] {
        updateLayers();
        if (!m_canvas->lastError().isEmpty()) statusBar()->showMessage(tr("Waiting for drawing resources."));
    });
    m_refresh.start();
}

KisGpuDocumentWindow::~KisGpuDocumentWindow()
{
    m_refresh.stop();
    m_window->removeEventFilter(this);
    m_canvas.reset();
    if (m_id) m_documents.closeDocument(m_id);
}

bool KisGpuDocumentWindow::eventFilter(QObject *object, QEvent *event)
{
    if (object == m_window && (event->type() == QEvent::Resize || event->type() == QEvent::DevicePixelRatioChange))
        updateView();
    return QMainWindow::eventFilter(object, event);
}

void KisGpuDocumentWindow::updateView()
{
    if (!m_canvas || !m_window->width() || !m_window->height()) return;
    const qreal ratio = m_window->devicePixelRatio();
    const qreal scale = std::min(qreal(m_window->width()) / m_bounds.width(), qreal(m_window->height()) / m_bounds.height()) * ratio;
    Krita::Canvas::GpuRenderer::View view;
    view.canvasToTarget.translate((m_window->width() * ratio - m_bounds.width() * scale) / 2,
                                 (m_window->height() * ratio - m_bounds.height() * scale) / 2);
    view.canvasToTarget.scale(scale, scale);
    view.canvasToTarget.translate(-m_bounds.x(), -m_bounds.y());
    m_canvas->setView(view);
}

void KisGpuDocumentWindow::updateLayers()
{
    const auto model = m_documents.document(m_id);
    if (!model) return;
    const auto layers = model->layers();
    bool same = m_layers->count() == layers.size();
    for (qsizetype i = 0; same && i < layers.size(); ++i)
        same = m_layers->itemData(i).toULongLong() == layers[i].id && m_layers->itemText(i) == layers[i].name;
    if (same) return;
    const auto selected = m_layers->currentData();
    const QSignalBlocker blocker(m_layers);
    m_layers->clear();
    for (const auto &layer : layers) m_layers->addItem(layer.name, QVariant::fromValue(layer.id));
    const auto index = m_layers->findData(selected);
    m_layers->setCurrentIndex(index < 0 ? 0 : index);
    updateTool();
}

void KisGpuDocumentWindow::updateTool()
{
    if (!m_canvas || m_layers->currentIndex() < 0) return;
    KisGpuStrokeInput::Configuration config;
    config.clip = m_bounds;
    config.brush.diameter = {m_size->value(), m_size->value()};
    config.brush.spacing = .25;
    config.brush.rgba = m_color.rgba();
    if (m_eraser->isChecked()) config.brush.operation = KisGpuTileStore::CompositeOp::Erase;
    m_canvas->setPointerTool(m_layers->currentData().toULongLong(), std::move(config));
}
