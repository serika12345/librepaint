/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisResourceItemChooser.h"

#include "KisResourceItemChooser_p.h"

#include <QButtonGroup>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QScrollArea>
#include <QSplitter>
#include <QToolButton>

#include <klocalizedstring.h>

#include <KoIcon.h>
#include <KisKineticScroller.h>
#include <KisResourceTypes.h>
#include <KisTagFilterResourceProxyModel.h>

#include "KisPopupButton.h"
#include "KisResourceItemDelegate.h"
#include "KisResourceItemListView.h"
#include "KisResourceTaggingManager.h"
#include "KisStorageChooserWidget.h"

void KisResourceItemChooser::constructPresentation()
{
    d->tagFilterProxyModel =
        new KisTagFilterResourceProxyModel(d->resourceType, this);
    d->tagFilterProxyModel->sort(Qt::DisplayRole);
    d->tagManager = new KisResourceTaggingManager(
        d->resourceType,
        d->tagFilterProxyModel,
        this);

    d->viewModeButton = new KisPopupButton(this);
    d->viewModeButton->setToolTip(i18n("Display settings"));
    d->viewModeButton->setVisible(false);
    d->viewModeButton->setArrowVisible(false);
    d->viewModeButton->setAutoRaise(true);

    d->storagePopupButton = new KisStorageChooserWidget(d->resourceType, this);
    d->storagePopupButton->setToolTip(i18n("Storage Resources"));
    d->storagePopupButton->setAutoRaise(true);
    d->storagePopupButton->setArrowVisible(false);

    d->view = new KisResourceItemListView(this);
    d->view->setObjectName("ResourceItemview");
    d->view->setStrictSelectionMode(true);

    if (d->resourceType == ResourceType::Gradients) {
        d->view->setFixedToolTipThumbnailSize(QSize(256, 64));
        d->view->setToolTipShouldRenderCheckers(true);
    } else if (d->resourceType == ResourceType::PaintOpPresets) {
        d->view->setFixedToolTipThumbnailSize(QSize(128, 128));
    } else if (d->resourceType == ResourceType::Patterns
               || d->resourceType == ResourceType::Palettes) {
        d->view->setToolTipShouldRenderCheckers(false);
        d->view->setFixedToolTipThumbnailSize(QSize(256, 256));
    }

    d->view->setItemDelegate(new KisResourceItemDelegate(this));
    d->view->setSelectionMode(QAbstractItemView::SingleSelection);
    d->view->viewport()->installEventFilter(this);
    d->view->setModel(d->tagFilterProxyModel);

    connect(d->tagFilterProxyModel,
            SIGNAL(afterFilterChanged()),
            this,
            SLOT(afterFilterChanged()));
    connect(d->view,
            SIGNAL(currentResourceChanged(QModelIndex)),
            this,
            SLOT(activate(QModelIndex)));
    connect(d->view,
            SIGNAL(currentResourceClicked(QModelIndex)),
            this,
            SLOT(clicked(QModelIndex)));
    connect(d->view,
            SIGNAL(contextMenuRequested(QPoint)),
            this,
            SLOT(contextMenuRequested(QPoint)));
    connect(d->view,
            SIGNAL(sigSizeChanged()),
            this,
            SLOT(updateView()));

    d->resourcesSplitter = new QSplitter(this);
    d->resourcesSplitter->addWidget(d->view);
    d->resourcesSplitter->setStretchFactor(0, 1);

    if (d->usePreview) {
        d->previewScroller = new QScrollArea(this);
        d->previewScroller->setWidgetResizable(true);
        d->previewScroller->setBackgroundRole(QPalette::Dark);
        d->previewScroller->setVisible(true);
        d->previewScroller->setAlignment(Qt::AlignCenter);
        d->previewLabel = new QLabel(this);
        d->previewScroller->setWidget(d->previewLabel);
        d->resourcesSplitter->addWidget(d->previewScroller);

        if (d->resourcesSplitter->count() == 2) {
            d->resourcesSplitter->setSizes(QList<int>() << 280 << 160);
        }

        QScroller *scroller =
            KisKineticScroller::createPreconfiguredScroller(d->previewScroller);
        if (scroller) {
            connect(scroller,
                    SIGNAL(stateChanged(QScroller::State)),
                    this,
                    SLOT(slotScrollerStateChanged(QScroller::State)));
        }
    }

    d->importButton = new QToolButton(this);
    d->importButton->setToolTip(i18nc("@info:tooltip", "Import resource"));
    d->importButton->setAutoRaise(true);
    d->importButton->setEnabled(true);

    d->deleteButton = new QToolButton(this);
    d->deleteButton->setToolTip(i18nc("@info:tooltip", "Delete resource"));
    d->deleteButton->setEnabled(false);
    d->deleteButton->setAutoRaise(true);

    d->buttonGroup = new QButtonGroup(this);
    d->buttonGroup->setExclusive(false);
    d->buttonGroup->addButton(d->importButton, Button_Import);
    d->buttonGroup->addButton(d->deleteButton, Button_Remove);
    connect(d->buttonGroup,
            SIGNAL(idClicked(int)),
            this,
            SLOT(slotButtonClicked(int)));

    d->importExportBtns = new QFrame(this);
    QHBoxLayout *importExportLayout = new QHBoxLayout(d->importExportBtns);
    importExportLayout->setAlignment(Qt::AlignmentFlag::AlignLeft);
    importExportLayout->setContentsMargins(0, 0, 0, 0);
    importExportLayout->addWidget(d->importButton);
    importExportLayout->addWidget(d->deleteButton);

    auto removePadding = [](QLayout *layout) {
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
    };

    QGridLayout *thisLayout = new QGridLayout(this);
    thisLayout->setObjectName("ResourceChooser this");
    removePadding(thisLayout);

    d->horzSplitter = new QSplitter(this);
    d->horzSplitter->setOrientation(Qt::Orientation::Horizontal);
    connect(d->horzSplitter,
            SIGNAL(splitterMoved(int, int)),
            this,
            SLOT(slotSaveSplitterState()));

    d->left = new QFrame(this);
    QSizePolicy newPolicy = d->left->sizePolicy();
    newPolicy.setHorizontalStretch(2);
    d->left->setSizePolicy(newPolicy);
    QHBoxLayout *leftLayout = new QHBoxLayout(d->left);
    leftLayout->setObjectName("ResourceChooser left");

    d->right = new QFrame(this);
    newPolicy = d->right->sizePolicy();
    newPolicy.setHorizontalStretch(0);
    d->right->setSizePolicy(newPolicy);
    QHBoxLayout *rightLayout = new QHBoxLayout(d->right);
    rightLayout->setObjectName("ResourceChooser right");

    d->right2Rows = new QFrame(this);
    newPolicy = d->right2Rows->sizePolicy();
    newPolicy.setHorizontalStretch(0);
    d->right2Rows->setSizePolicy(newPolicy);
    QVBoxLayout *rightLayout2 = new QVBoxLayout(d->right2Rows);
    rightLayout2->setObjectName("ResourceChooser right 2 rows");

    d->top = new QHBoxLayout();
    d->bot = new QHBoxLayout();
    rightLayout2->addLayout(d->top);
    rightLayout2->addLayout(d->bot);

    removePadding(leftLayout);
    removePadding(rightLayout);
    removePadding(rightLayout2);
    removePadding(d->top);
    removePadding(d->bot);

    d->scroll_left = new QToolButton(this);
    d->scroll_left->setIcon(KisIconUtils::loadIcon("draw-arrow-back"));
    connect(d->scroll_left,
            &QToolButton::clicked,
            this,
            &KisResourceItemChooser::scrollBackwards);
    d->scroll_right = new QToolButton(this);
    d->scroll_right->setIcon(KisIconUtils::loadIcon("draw-arrow-forward"));
    connect(d->scroll_right,
            &QToolButton::clicked,
            this,
            &KisResourceItemChooser::scrollForwards);

    d->scroll_left->hide();
    d->scroll_right->hide();
    d->left->hide();
    d->right->hide();
    d->right2Rows->hide();
    d->horzSplitter->hide();

    updateView();
    updateButtonState();
    showViewModeBtn(false);
    showTaggingBar(false);
    showImportExportBtns(true);
    applyVerticalLayout();
}
