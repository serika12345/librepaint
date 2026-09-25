/*
 * This file is part of the KDE project
 * SPDX-FileCopyrightText: 2013 Arjen Hiemstra <ahiemstra@heimr.nl>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_input_configuration_page_item.h"

#include "kis_icon_utils.h"

#include "input/ui/kis_abstract_input_action.h"
#include "input/ui/kis_input_profile_manager.h"
#include "kis_action_shortcuts_model.h"
#include "kis_input_type_delegate.h"
#include "kis_input_mode_delegate.h"
#include "kis_input_editor_delegate.h"
#include "ui_kis_input_configuration_page_item.h"

#include <QAction>
#include <QItemSelectionModel>
#include <QMessageBox>
#include <QToolTip>
#include <QtGlobal>
#include <qcursor.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qobjectdefs.h>

KisInputConfigurationPageItem::KisInputConfigurationPageItem(QWidget *parent, Qt::WindowFlags f)
    : QWidget(parent, f)
    , m_defaultToolTipText(i18n("Conflict found for a Shortcut in this Action."))
{
    ui = new Ui::KisInputConfigurationPageItem;
    this->setContentsMargins(0,0,0,0);
    ui->setupUi(this);

    m_shortcutsModel = new KisActionShortcutsModel(this);
    ui->shortcutsView->setModel(m_shortcutsModel);
    ui->shortcutsView->setItemDelegateForColumn(0, new KisInputTypeDelegate(ui->shortcutsView));
    ui->shortcutsView->setItemDelegateForColumn(1, new KisInputEditorDelegate(ui->shortcutsView));
    ui->shortcutsView->setItemDelegateForColumn(2, new KisInputModeDelegate(ui->shortcutsView));
    ui->shortcutsView->header()->setSectionResizeMode(QHeaderView::Stretch);
    setExpanded(false);

    ui->warningConflictButton->setIcon(KisIconUtils::loadIcon("warning"));
    ui->warningConflictButton->setFlat(true);
    ui->warningConflictButton->hide();
    connect(ui->warningConflictButton, &QPushButton::clicked, [&]() {
        QToolTip::showText(QCursor::pos(), ui->warningConflictButton->toolTip());
    });

    connect(m_shortcutsModel, &KisActionShortcutsModel::dataChanged, this, [&]() {
        Q_EMIT inputConfigurationChanged();
    });

    m_deleteShortcutAction =
        new QAction(KisIconUtils::loadIcon("edit-delete"), i18n("Delete Shortcut"), ui->shortcutsView);
    connect(m_deleteShortcutAction, &QAction::triggered, this, &KisInputConfigurationPageItem::deleteShortcut);
    ui->shortcutsView->addAction(m_deleteShortcutAction);
    ui->shortcutsView->setContextMenuPolicy(Qt::ActionsContextMenu);
    ui->deleteShortcutButton->setIcon(m_deleteShortcutAction->icon());
    connect(ui->deleteShortcutButton, &QPushButton::clicked, this, &KisInputConfigurationPageItem::deleteShortcut);
    connect(ui->shortcutsView->selectionModel(),
            &QItemSelectionModel::currentChanged,
            this,
            &KisInputConfigurationPageItem::updateDeleteShortcutActions);
    updateDeleteShortcutActions();

    connect(ui->collapseButton, SIGNAL(clicked(bool)), SLOT(setExpanded(bool)));
}

KisInputConfigurationPageItem::~KisInputConfigurationPageItem()
{
    delete ui;
}

void KisInputConfigurationPageItem::setAction(KisAbstractInputAction *action)
{
    m_action = action;
    ui->collapseButton->setText(action->name());
    ui->descriptionLabel->setText(action->description());
    m_shortcutsModel->setProfile(KisInputProfileManager::instance()->currentProfile());
    m_shortcutsModel->setAction(action);
    qobject_cast<KisInputModeDelegate *>(ui->shortcutsView->itemDelegateForColumn(2))->setAction(action);
}

void KisInputConfigurationPageItem::setWarningEnabled(bool enabled, QString additionalToolTipText)
{
    ui->warningConflictButton->setVisible(enabled);
    if (!enabled) {
        ui->warningConflictButton->setToolTip(m_defaultToolTipText);
    } else if (!additionalToolTipText.isEmpty()) {
        const QString toolTipText =
            "<html>" +
            ui->warningConflictButton->toolTip() + "<br>" + additionalToolTipText +
            "</html>";
        ui->warningConflictButton->setToolTip(toolTipText);
    }
}

void KisInputConfigurationPageItem::setExpanded(bool expand)
{
    if (expand) {
        ui->descriptionLabel->setVisible(true);
        ui->shortcutsView->setVisible(true);
        ui->deleteShortcutButton->setVisible(true);
        ui->collapseButton->setArrowType(Qt::DownArrow);
    }
    else {
        ui->descriptionLabel->setVisible(false);
        ui->shortcutsView->setVisible(false);
        ui->deleteShortcutButton->setVisible(false);
        ui->collapseButton->setArrowType(Qt::RightArrow);
    }
}

void KisInputConfigurationPageItem::deleteShortcut()
{
    int row = ui->shortcutsView->selectionModel()->currentIndex().row();

    if (m_shortcutsModel->canRemoveRow(row)) {
        m_shortcutsModel->removeRow(row, QModelIndex());
        updateDeleteShortcutActions();
        Q_EMIT inputConfigurationChanged();
    } else {
        QMessageBox shortcutMessage;
        shortcutMessage.setText(i18n("Deleting last shortcut for this action!"));
        shortcutMessage.setInformativeText(i18n("It is not allowed to erase some default shortcuts. Modify it instead."));
        shortcutMessage.exec();
    }
}

void KisInputConfigurationPageItem::updateDeleteShortcutActions()
{
    const QModelIndex currentIndex = ui->shortcutsView->selectionModel()->currentIndex();
    const bool canDelete = currentIndex.isValid() && m_shortcutsModel->canRemoveRow(currentIndex.row());
    m_deleteShortcutAction->setEnabled(canDelete);
    ui->deleteShortcutButton->setEnabled(canDelete);
}
