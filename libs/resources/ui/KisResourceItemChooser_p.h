/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KISRESOURCEITEMCHOOSER_P_H
#define KISRESOURCEITEMCHOOSER_P_H

#include "KisResourceItemChooser.h"

#include <QList>

#include <utility>

class QButtonGroup;
class QFrame;
class QHBoxLayout;
class QLabel;
class QScrollArea;
class QSplitter;
class QToolButton;
class KisResourceTaggingManager;
class KisStorageChooserWidget;

class Q_DECL_HIDDEN KisResourceItemChooser::Private
{
public:
    explicit Private(KisResourceUiDescriptor uiDescriptor)
        : descriptor(std::move(uiDescriptor))
        , resourceType(descriptor.resourceType())
        , usePreview(descriptor.previewEnabled())
    {
    }

    KisResourceUiDescriptor descriptor;
    QString resourceType;

    KisTagFilterResourceProxyModel *tagFilterProxyModel {nullptr};
    KisResourceTaggingManager *tagManager {nullptr};
    KisPopupButton *viewModeButton {nullptr};
    KisStorageChooserWidget *storagePopupButton {nullptr};

    KisResourceItemListView *view {nullptr};
    QSplitter *resourcesSplitter {nullptr};

    QScrollArea *previewScroller {nullptr};
    QLabel *previewLabel {nullptr};

    QFrame *importExportBtns {nullptr};
    QToolButton *importButton {nullptr};
    QToolButton *deleteButton {nullptr};
    QButtonGroup *buttonGroup {nullptr};

    QList<QAbstractButton *> customButtons;

    bool usePreview {false};
    bool tiledPreview {false};
    bool grayscalePreview {false};
    bool synced {false};
    bool updatesBlocked {false};

    KoResourceSP currentResource;
    Layout layout = Layout::NotSet;
    ListViewMode requestedViewMode = ListViewMode::IconGrid;
    bool isResponsive = false;
    bool showViewModeBtn = true;
    bool showStoragePopupBtn = true;

    QSplitter *horzSplitter {nullptr};
    QFrame *left {nullptr};
    QFrame *right {nullptr};

    QFrame *right2Rows {nullptr};
    QHBoxLayout *top {nullptr};
    QHBoxLayout *bot {nullptr};

    QToolButton *scroll_left {nullptr};
    QToolButton *scroll_right {nullptr};

    bool restoreSplitterState(Layout layout);
};

#endif // KISRESOURCEITEMCHOOSER_P_H
