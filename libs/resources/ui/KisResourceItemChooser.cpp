/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2002 Patrick Julien <freak@codepimps.org>
   SPDX-FileCopyrightText: 2007 Jan Hambrecht <jaham@gmx.net>
   SPDX-FileCopyrightText: 2007 Sven Langkamp <sven.langkamp@gmail.com>
   SPDX-FileCopyrightText: 2011 Srikanth Tiyyagura <srikanth.tulasiram@gmail.com>
   SPDX-FileCopyrightText: 2011 José Luis Vergara <pentalis@gmail.com>
   SPDX-FileCopyrightText: 2013 Sascha Suelzer <s.suelzer@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "KisResourceItemChooser.h"

#include "KisResourceItemChooser_p.h"

#include <math.h>

#include <QGridLayout>
#include <QButtonGroup>
#include <QHeaderView>
#include <QAbstractProxyModel>
#include <QLabel>
#include <QScrollArea>
#include <QScrollBar>
#include <QImage>
#include <QPixmap>
#include <QPainter>
#include <QSplitter>
#include <QToolButton>
#include <QWheelEvent>
#include <QLineEdit>
#include <QStandardPaths>

#include "ksharedconfig.h"
#include "kconfiggroup.h"
#include <klocalizedstring.h>

#include <KoIcon.h>
#include <KoFileDialog.h>
#include <KisKineticScroller.h>
#include "KisPopupButton.h"

#include <KisResourceModel.h>
#include <KisTagFilterResourceProxyModel.h>
#include <KisResourceLoaderRegistry.h>

#include "KisResourceItemListView.h"
#include "KisResourceItemDelegate.h"
#include "KisTagChooserWidget.h"
#include "KisResourceItemChooserSync.h"
#include "KisResourceTaggingManager.h"
#include <KisResourceUserOperations.h>

#include "KisStorageChooserWidget.h"


void KisResourceItemChooser::setResponsiveness(bool isResponsive)
{
    if (isResponsive) {

        if (d->isResponsive == false) {
            d->isResponsive = true;
            changeLayoutBasedOnSize();
        }
        else {
            d->isResponsive = true;
        }
    }
    else {
        d->isResponsive = false;
    }
}

void KisResourceItemChooser::setListViewMode(ListViewMode newViewMode)
{
    d->requestedViewMode = newViewMode;
    if (d->layout == Layout::NotSet || d->layout == Layout::Vertical) {
        d->view->setListViewMode(newViewMode);
    }
}

KisTagFilterResourceProxyModel *KisResourceItemChooser::tagFilterModel() const
{
    return d->tagFilterProxyModel;
}

void KisResourceItemChooser::slotButtonClicked(int button)
{
    if (button == Button_Import) {
        QStringList mimeTypes = KisResourceLoaderRegistry::instance()->mimeTypes(d->resourceType);
        KoFileDialog dialog(0, KoFileDialog::OpenFiles, "OpenDocument");
        dialog.setMimeTypeFilters(mimeTypes);
        dialog.setDefaultDir(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));
        dialog.setCaption(i18nc("@title:window", "Choose File to Add"));
        Q_FOREACH(const QString &filename, dialog.filenames()) {
            if (QFileInfo(filename).exists() && QFileInfo(filename).isReadable()) {

                KoResourceSP previousResource = this->currentResource();
                KoResourceSP newResource = KisResourceUserOperations::importResourceFileWithUserInput(this, "", d->resourceType, filename);

                if (previousResource && newResource && !currentResource()) {
                    /// We have overridden the currently selected resource and
                    /// nothing is selected now
                    setCurrentResource(newResource);
                } else if (currentResource() == newResource) {
                    /// We have overridden the currently selected resource and
                    /// the model has managed to track the selection under it
                    /// (that is not possible right now, but can theoretically
                    /// happen under some circumstances)
                    const QModelIndex index = d->tagFilterProxyModel->indexForResource(newResource);
                    updatePreview(index);
                }
            }
        }
        tagFilterModel()->sort(Qt::DisplayRole);
    }
    else if (button == Button_Remove) {
        QModelIndex index = d->view->currentIndex();
        if (index.isValid()) {
            d->tagFilterProxyModel->setResourceInactive(index);
        }
        int row = index.row();
        int rowMin = --row;
        row = qBound(0, rowMin, row);
        setCurrentItem(row);
        activate(d->tagFilterProxyModel->index(row, index.column()));
    }
    updateButtonState();
}

void KisResourceItemChooser::showImportExportBtns(bool show)
{
    // assert(show == false);
    if (show) {
        d->importExportBtns->show();
    }
    else {
        d->importExportBtns->hide();
    }
}

void KisResourceItemChooser::showTaggingBar(bool show)
{
    d->tagManager->showTaggingBar(show);
}

void KisResourceItemChooser::setRowHeight(int rowHeight)
{
    d->view->setItemSize(QSize(d->view->gridSize().width(), rowHeight));
}

void KisResourceItemChooser::setColumnWidth(int columnWidth)
{
    d->view->setItemSize(QSize(columnWidth, d->view->gridSize().height()));
}

void KisResourceItemChooser::setItemDelegate(QAbstractItemDelegate *delegate)
{
    d->view->setItemDelegate(delegate);
}

KoResourceSP KisResourceItemChooser::currentResource(bool includeHidden) const
{
    if (includeHidden || d->view->selectionModel()->isSelected(d->view->currentIndex())) {
        return d->currentResource;
    }

    return nullptr;
}

void KisResourceItemChooser::setCurrentResource(KoResourceSP resource)
{
    // don't update if the change came from the same chooser
    if (d->updatesBlocked) {
        return;
    }
    QModelIndex index = d->tagFilterProxyModel->indexForResource(resource);
    d->view->setCurrentIndex(index);

    // The resource may currently be filtered out, but we want to be able
    // to select it if the filter changes and includes the resource.
    // Otherwise, activate() already took care of setting the current resource.
    if (!index.isValid()) {
        d->currentResource = resource;
    }
    updatePreview(index);
}

void KisResourceItemChooser::setCurrentResource(QString resourceName)
{
    // don't update if the change came from the same chooser
    if (d->updatesBlocked) {
        return;
    }

    for (int row = 0; row < d->tagFilterProxyModel->rowCount(); row++) {
        for (int col = 0; col < d->tagFilterProxyModel->columnCount(); col++) {
            QModelIndex index = d->tagFilterProxyModel->index(row, col);
            KoResourceSP resource = d->tagFilterProxyModel->resourceForIndex(index);

            if (resource->name() == resourceName) {
                d->view->setCurrentIndex(index);

                // The resource may currently be filtered out, but we want to be able
                // to select it if the filter changes and includes the resource.
                // Otherwise, activate() already took care of setting the current resource.
                if (!index.isValid()) {
                    d->currentResource = resource;
                }
                updatePreview(index);
            }
        }
    }
}

void KisResourceItemChooser::setPreviewOrientation(Qt::Orientation orientation)
{
    d->resourcesSplitter->setOrientation(orientation);
}

void KisResourceItemChooser::setPreviewTiled(bool tiled)
{
    d->tiledPreview = tiled;
}

void KisResourceItemChooser::setGrayscalePreview(bool grayscale)
{
    d->grayscalePreview = grayscale;
}

void KisResourceItemChooser::setCurrentItem(int row)
{
    QModelIndex index = d->view->model()->index(row, 0);
    if (!index.isValid())
        return;

    d->view->setCurrentIndex(index);
    if (index.isValid()) {
        updatePreview(index);
    }
}

void KisResourceItemChooser::scrollBackwards()
{
    QScrollBar* bar = d->view->horizontalScrollBar();
    bar->setValue(bar->value() - d->view->gridSize().width());
}

void KisResourceItemChooser::scrollForwards()
{
    QScrollBar* bar = d->view->horizontalScrollBar();
    bar->setValue(bar->value() + d->view->gridSize().width());
}

void KisResourceItemChooser::activate(const QModelIndex &index)
{
    if (!index.isValid())
    {
        updateButtonState();
        return;
    }

    KoResourceSP resource = resourceFromModelIndex(index);

    if (resource && resource->valid()) {
        if (resource != d->currentResource) {
            d->currentResource = resource;
            d->updatesBlocked = true;
            Q_EMIT resourceSelected(resource);
            d->updatesBlocked = false;
        }
        updatePreview(index);
        updateButtonState();
    }
}

void KisResourceItemChooser::clicked(const QModelIndex &index)
{
    Q_UNUSED(index);

    KoResourceSP resource = currentResource();
    if (resource) {
        Q_EMIT resourceClicked(resource);
    }
}

void KisResourceItemChooser::updateButtonState()
{
    QAbstractButton *removeButton = d->buttonGroup->button(Button_Remove);
    if (! removeButton)
        return;

    KoResourceSP resource = currentResource();
    if (resource) {
        removeButton->setEnabled(!resource->permanent());
        return;
    }
    removeButton->setEnabled(false);
}

void KisResourceItemChooser::updatePreview(const QModelIndex &idx)
{
    if (!d->usePreview) return;

    if (!idx.isValid()) {
        d->previewLabel->setPixmap(QPixmap());
        return;
    }

    QImage image = idx.data(Qt::UserRole + KisAbstractResourceModel::Thumbnail).value<QImage>();

    if (image.format() != QImage::Format_RGB32 &&
        image.format() != QImage::Format_ARGB32 &&
        image.format() != QImage::Format_ARGB32_Premultiplied) {

        image.convertTo(QImage::Format_ARGB32_Premultiplied);
    }

    if (d->tiledPreview) {
        int width = d->previewScroller->width() * 4 * devicePixelRatioF();
        int height = d->previewScroller->height() * 4 * devicePixelRatioF();
        QImage img(width, height, image.format());
        QPainter gc(&img);
        gc.fillRect(img.rect(), Qt::white);
        gc.setPen(Qt::NoPen);
        gc.setBrush(QBrush(image));
        gc.drawRect(img.rect());
        image = img;
    }

    // Only convert to grayscale if it is rgb. Otherwise, it's gray already.
    if (d->grayscalePreview && !image.isGrayscale()) {
        QRgb *pixel = reinterpret_cast<QRgb *>(image.bits());
        for (int row = 0; row < image.height(); ++row) {
            for (int col = 0; col < image.width(); ++col) {
                const QRgb currentPixel = pixel[row * image.width() + col];
                const int red = qRed(currentPixel);
                const int green = qGreen(currentPixel);
                const int blue = qBlue(currentPixel);
                const int grayValue = (red * 11 + green * 16 + blue * 5) / 32;
                pixel[row * image.width() + col] = qRgb(grayValue, grayValue, grayValue);
            }
        }
    }
    image.setDevicePixelRatio(devicePixelRatioF());
    d->previewLabel->setPixmap(QPixmap::fromImage(image));
}

KoResourceSP KisResourceItemChooser::resourceFromModelIndex(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return 0;
    }
    KoResourceSP r = d->tagFilterProxyModel->resourceForIndex(index);
    return r;
}

QSize KisResourceItemChooser::viewSize() const
{
    return d->view->size();
}

KisResourceItemListView *KisResourceItemChooser::itemView() const
{
    return d->view;
}

void KisResourceItemChooser::contextMenuRequested(const QPoint &pos)
{
    d->tagManager->contextMenuRequested(currentResource(), pos);
}

void KisResourceItemChooser::showStorageBtn(bool visible)
{
    d->storagePopupButton->setVisible(visible);
    d->showStoragePopupBtn = visible;
}

void KisResourceItemChooser::showViewModeBtn(bool visible)
{
    d->viewModeButton->setVisible(visible);
    d->showViewModeBtn = visible;
}

KisPopupButton *KisResourceItemChooser::viewModeButton() const
{
    return d->viewModeButton;
}

void KisResourceItemChooser::setSynced(bool sync)
{
    if (d->synced == sync)
        return;

    d->synced = sync;
    KisResourceItemChooserSync *chooserSync = KisResourceItemChooserSync::instance();
    if (sync) {
        connect(chooserSync, SIGNAL(baseLengthChanged(int)), SLOT(baseLengthChanged(int)));
        baseLengthChanged(chooserSync->baseLength());
    } else {
        chooserSync->disconnect(this);
    }
}

void KisResourceItemChooser::slotScrollerStateChanged(QScroller::State state)
{
    KisKineticScroller::updateCursor(this, state);
}

void KisResourceItemChooser::baseLengthChanged(int length)
{
    if (d->synced) {
        d->view->setItemSize(QSize(length, length));
    }
}

void KisResourceItemChooser::afterFilterChanged()
{
    // Note: Item model reset events silently reset the view's selection model too.
    // This currently only covers models resets as part of filter changes.
    QModelIndex idx = d->tagFilterProxyModel->indexForResource(d->currentResource);

    if (idx.isValid()) {
        d->view->setCurrentIndex(idx);
    }

    updateButtonState();
}

bool KisResourceItemChooser::eventFilter(QObject *object, QEvent *event)
{
    if (d->synced && event->type() == QEvent::Wheel) {
        KisResourceItemChooserSync *chooserSync = KisResourceItemChooserSync::instance();
        QWheelEvent *qwheel = static_cast<QWheelEvent *>(event);
        if (qwheel->modifiers() & Qt::ControlModifier) {

            int degrees = qwheel->angleDelta().y() / 8;
            int newBaseLength = chooserSync->baseLength() + degrees / 15 * 10;
            chooserSync->setBaseLength(newBaseLength);
            return true;
        }
    }
    return QObject::eventFilter(object, event);
}


void KisResourceItemChooser::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    changeLayoutBasedOnSize();
    updateView();
    updatePreview(d->view->currentIndex());
}

void KisResourceItemChooser::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    updateView();
    updatePreview(d->view->currentIndex());
}

void KisResourceItemChooser::hideEverything()
{
    d->horzSplitter->hide();
    d->left->hide();
    d->right->hide();
    d->right2Rows->hide();
    d->scroll_left->hide();
    d->scroll_right->hide();

    d->viewModeButton->hide();
}

void KisResourceItemChooser::applyVerticalLayout()
{
    if (d->layout == Layout::Vertical) {
        return;
    }

    hideEverything();

    d->view->setListViewMode(d->requestedViewMode);
    Q_EMIT listViewModeChanged(d->requestedViewMode);

    // The horizontal layouts size the items based widget height not user configured base length
    // so it needs to be restored when switching back to vertical layout
    KisResourceItemChooserSync *chooserSync = KisResourceItemChooserSync::instance();
    d->view->setItemSize(QSize(chooserSync->baseLength(), chooserSync->baseLength()));

    QGridLayout* thisLayout = dynamic_cast<QGridLayout*>(layout());
    thisLayout->addWidget(d->tagManager->tagChooserWidget(), 0, 0);
    thisLayout->addWidget(d->viewModeButton, 0, 1);
    thisLayout->addWidget(d->storagePopupButton, 0, 2);
    thisLayout->addWidget(d->resourcesSplitter, 1, 0, 1, 3);
    thisLayout->setRowStretch(1, 1);
    thisLayout->addWidget(d->tagManager->tagFilterWidget(), 2, 0, 1, 3);
    thisLayout->addWidget(d->importExportBtns, 3, 0, 1, 3);

    d->viewModeButton->setVisible(d->showViewModeBtn);
    d->storagePopupButton->setVisible(d->showStoragePopupBtn);

    d->viewModeButton->setVisible(d->showViewModeBtn);
    d->layout = Layout::Vertical;
}

void KisResourceItemChooser::changeLayoutBasedOnSize()
{
    if (d->isResponsive == false) {
        return;
    }
    // Responsive Layout
    else {
        // Vertical
        if (height() > 100) {
            applyVerticalLayout();
        }
        // Horizontal 2 rows
        else if (height() > 60) {

            if (d->layout == Layout::Horizontal2Rows) {
                return;
            }

            hideEverything();

            d->view->setListViewMode(ListViewMode::IconStripHorizontal);
            Q_EMIT listViewModeChanged(ListViewMode::IconStripHorizontal);

            // Left
            QLayout* leftLayout = d->left->layout();
            leftLayout->addWidget(d->resourcesSplitter);

            // Right Top
            d->top->addWidget(d->scroll_left);
            d->top->addWidget(d->scroll_right);
            d->top->addWidget(d->tagManager->tagChooserWidget());
            d->top->addWidget(d->importExportBtns);

            // Right Bot
            d->bot->addWidget(d->viewModeButton);
            d->bot->addWidget(d->storagePopupButton);
            d->bot->addWidget(d->tagManager->tagFilterWidget());

            d->horzSplitter->addWidget(d->left);
            d->horzSplitter->addWidget(d->right2Rows);

            QGridLayout* thisLayout = dynamic_cast<QGridLayout*>(layout());
            thisLayout->addWidget(d->horzSplitter, 0, 0);
            thisLayout->setRowStretch(0, 2);
            thisLayout->setColumnStretch(0, 2);
            thisLayout->setRowStretch(1, 0);

            d->viewModeButton->setVisible(false);
            d->storagePopupButton->setVisible(d->showStoragePopupBtn);

            const bool splitterRestored = d->restoreSplitterState(Layout::Horizontal2Rows);

            d->horzSplitter->show();
            d->left->show();
            d->scroll_left->show();
            d->scroll_right->show();
            d->right2Rows->show();

            d->layout = Layout::Horizontal2Rows;

            if (!splitterRestored) {
                slotSaveSplitterState();
            }
        }
        // Horizontal 1 row
        else {
            if (d->layout == Layout::Horizontal1Row) {
                return;
            }

            hideEverything();

            d->view->setListViewMode(ListViewMode::IconStripHorizontal);
            Q_EMIT listViewModeChanged(ListViewMode::IconStripHorizontal);

            QLayout* leftLayout = d->left->layout();
            leftLayout->addWidget(d->resourcesSplitter);
            leftLayout->addWidget(d->scroll_left);
            leftLayout->addWidget(d->scroll_right);

            QLayout* rightLayout = d->right->layout();
            rightLayout->addWidget(d->tagManager->tagChooserWidget());
            rightLayout->addWidget(d->viewModeButton);
            rightLayout->addWidget(d->storagePopupButton);
            rightLayout->addWidget(d->tagManager->tagFilterWidget());
            rightLayout->addWidget(d->importExportBtns);

            d->horzSplitter->addWidget(d->left);
            d->horzSplitter->addWidget(d->right);

            QGridLayout* thisLayout = dynamic_cast<QGridLayout*>(layout());
            thisLayout->addWidget(d->horzSplitter, 0, 0);
            thisLayout->setRowStretch(0, 2);
            thisLayout->setColumnStretch(0, 2);
            thisLayout->setRowStretch(1, 0);

            d->viewModeButton->setVisible(false);
            d->storagePopupButton->setVisible(d->showStoragePopupBtn);

            const bool splitterRestored = d->restoreSplitterState(Layout::Horizontal1Row);

            d->horzSplitter->show();
            d->left->show();
            d->scroll_left->show();
            d->scroll_right->show();
            d->right->show();

            d->layout = Layout::Horizontal1Row;

            if (!splitterRestored) {
                slotSaveSplitterState();
            }
        }
    }
}

bool KisResourceItemChooser::Private::restoreSplitterState(Layout layout)
{
    KIS_SAFE_ASSERT_RECOVER_RETURN_VALUE(layout > Layout::Vertical, false);

    KConfigGroup group = KSharedConfig::openConfig()->group(QString("KisResourceItemChooser_%1").arg(resourceType));

    const QLatin1String key(
        layout == Layout::Horizontal1Row ? "splitterState_1row" : "splitterState_2row");

    bool splitterRestored = false;
    QByteArray state = group.readEntry(key, QByteArray());
    if (!state.isEmpty()) {
        splitterRestored = horzSplitter->restoreState(state);
        KIS_SAFE_ASSERT_RECOVER_NOOP(splitterRestored);
    }

    return splitterRestored;
}

void KisResourceItemChooser::slotSaveSplitterState()
{
    if (d->layout > Layout::Vertical) {
        KConfigGroup group = KSharedConfig::openConfig()->group(QString("KisResourceItemChooser_%1").arg(d->resourceType));
        const QByteArray state = d->horzSplitter->saveState();

        const QLatin1String key(
            d->layout == Layout::Horizontal1Row ? "splitterState_1row" : "splitterState_2row");

        group.writeEntry(key, state);
    }
}

void KisResourceItemChooser::updateView()
{
    /// helps to set icons here in case the theme is changed
    d->viewModeButton->setIcon(KisIconUtils::loadIcon("view-choose"));
    d->importButton->setIcon(koIcon("document-import-16"));
    d->deleteButton->setIcon(koIcon("edit-delete"));
    d->storagePopupButton->setIcon(koIcon("bundle_archive"));
    d->tagManager->tagChooserWidget()->updateIcons();
}
