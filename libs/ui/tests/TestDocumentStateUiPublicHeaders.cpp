/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <type_traits>

#include <QAbstractItemModel>
#include <QObject>
#include <QTest>

#include <document/KisDocument.h>
#include <document/KisTextPropertiesManager.h>
#include <document/StoryboardItem.h>
#include <document/kis_file_layer.h>
#include <document/kis_filter_manager.h>
#include <document/kis_image_manager.h>
#include <nodes/kis_model_index_converter.h>
#include <nodes/kis_model_index_converter_base.h>
#include <nodes/kis_model_index_converter_show_all.h>
#include <nodes/kis_multinode_property.h>
#include <nodes/kis_node_filter_proxy_model.h>
#include <nodes/kis_node_manager.h>
#include <nodes/kis_node_model.h>
#include <nodes/kis_node_selection_adapter.h>
#include <nodes/kis_node_view_color_scheme.h>
#include <selection/KisSelectionActionsAdapter.h>
#include <selection/kis_selection_actions_panel.h>
#include <selection/kis_selection_manager.h>

static_assert(std::is_base_of_v<QObject, KisDocument>);
static_assert(std::is_base_of_v<QAbstractItemModel, KisNodeModel>);
static_assert(std::is_base_of_v<QObject, KisSelectionActionsPanel>);

class TestDocumentStateUiPublicHeaders : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void headersCompileAsOnePublicSurface();
};

void TestDocumentStateUiPublicHeaders::headersCompileAsOnePublicSurface()
{
    QVERIFY(true);
}

QTEST_MAIN(TestDocumentStateUiPublicHeaders)

#include "TestDocumentStateUiPublicHeaders.moc"
