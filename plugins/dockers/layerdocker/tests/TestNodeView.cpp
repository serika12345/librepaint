/*
 *  SPDX-FileCopyrightText: 2015 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "TestNodeView.h"

#include <QTest>
#include <QDialog>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QItemSelectionModel>
#include <QSignalSpy>
#include <QStandardItemModel>

#include <NodeView.h>

#include "document/KisDocument.h"
#include "application/ui/orchestration/KisPart.h"
#include "kis_name_server.h"
#include "flake/kis_shape_controller.h"
#include "nodes/kis_node_model.h"
#include "kis_color_filter_combo.h"

#include <testresources.h>

void NodeViewTest::init()
{
    m_doc = KisPart::instance()->createDocument();

    m_nameServer = new KisNameServer();
    m_shapeController = new KisShapeController(m_nameServer, m_doc->undoStack());

    initBase();
}

void NodeViewTest::cleanup()
{
    cleanupBase();

    delete m_shapeController;
    delete m_nameServer;
    delete m_doc;
}

void NodeViewTest::testModelActiveNodePreservesExtendedSelection_data()
{
    QTest::addColumn<bool>("preserveSelection");
    QTest::newRow("view-owned-selection") << false;
    QTest::newRow("document-owned-selection") << true;
}

void NodeViewTest::testModelActiveNodePreservesExtendedSelection()
{
    QFETCH(bool, preserveSelection);
    QStandardItemModel model;
    model.setColumnCount(3);
    auto *group = new QStandardItem(QStringLiteral("Group"));
    group->setColumnCount(3);
    group->appendRow(new QStandardItem(QStringLiteral("Child")));
    model.appendRow(group);
    model.appendRow(new QStandardItem(QStringLiteral("Top")));
    NodeView view;
    view.setActiveChangePreservesSelection(preserveSelection);
    view.setModel(&model);
    view.setSelectionMode(QAbstractItemView::ExtendedSelection);
    view.expandAll();
    const auto groupIndex = model.index(0, 0);
    const auto childIndex = model.index(0, 0, groupIndex);
    const auto topIndex = model.index(1, 0);
    auto *selection = view.selectionModel();
    QVERIFY(model.setData(groupIndex, true, KisNodeModel::ActiveRole));
    QCOMPARE(view.currentIndex(), groupIndex);
    QCOMPARE(selection->selectedRows(), QModelIndexList{groupIndex});
    selection->select(groupIndex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    selection->select(topIndex, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    model.setData(groupIndex, false, KisNodeModel::ActiveRole);
    QSignalSpy changes(selection, &QItemSelectionModel::selectionChanged);

    QVERIFY(model.setData(childIndex, true, KisNodeModel::ActiveRole));

    QCOMPARE(view.currentIndex(), childIndex);
    if (preserveSelection) {
        QCOMPARE(selection->selectedRows().size(), 2);
        QVERIFY(selection->selectedRows().contains(groupIndex));
        QVERIFY(selection->selectedRows().contains(topIndex));
        QCOMPARE(changes.count(), 0);
    } else {
        QCOMPARE(selection->selectedRows(), QModelIndexList{childIndex});
        QCOMPARE(changes.count(), 1);
    }
}


void NodeViewTest::testLayers()
{
#ifndef ENABLE_GUI_TESTS
    return;
#endif

    QDialog dlg;

    QFont font;
    font.setPointSizeF(8);
    dlg.setFont(font);

    KisNodeModel *model = new KisNodeModel(0);
    NodeView *view = new NodeView(&dlg);

    view->setModel(model);

    constructImage();
    addSelectionMasks();
    m_shapeController->setImage(m_image);

    model->setDummiesFacade(m_shapeController, m_image, m_shapeController, 0, 0);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    KisColorFilterCombo *cb = new KisColorFilterCombo(&dlg);

    QSet<int> labels;
    for (int i = 0; i < 6; i++) {
        labels.insert(i);
    }
    cb->updateAvailableLabels(labels);

    QHBoxLayout *hbox = new QHBoxLayout(&dlg);
    hbox->addStretch(1);
    hbox->addWidget(cb);
    layout->addLayout(hbox);
    layout->addWidget(view);

    dlg.resize(280, 400);
    view->expandAll();

    dlg.exec();
}

#include "kis_color_label_selector_widget.h"

void NodeViewTest::testColorLabels()
{
#ifndef ENABLE_GUI_TESTS
    return;
#endif


    QDialog dlg;

    QFont font;
    font.setPointSizeF(8);
    dlg.setFont(font);

    KisColorLabelSelectorWidget *widget = new KisColorLabelSelectorWidget(&dlg);
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    widget->setSizePolicy(policy);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    layout->addWidget(widget);
    layout->addStretch(1);

    dlg.resize(280, 400);
    dlg.exec();
}

KISTEST_MAIN(NodeViewTest)
