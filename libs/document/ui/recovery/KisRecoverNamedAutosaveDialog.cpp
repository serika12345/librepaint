/*
 *  SPDX-FileCopyrightText: 2020 Agata Cacko <cacko.azh@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisRecoverNamedAutosaveDialog.h"
#include "ui_KisRecoverNamedAutosaveDialog.h"

#include <KLocalizedString>

KisRecoverNamedAutosaveDialog::KisRecoverNamedAutosaveDialog(QWidget *parent)
    : QDialog(parent),
      ui(new Ui::KisRecoverNamedAutosaveDialog)
{

    ui->setupUi(this);
    m_filePreviewIconSize = ui->rbOpenAutosave->iconSize() * 4;

    connect(ui->btOk, SIGNAL(clicked()), this, SLOT(slotOkRequested()));
    connect(ui->btCancel, SIGNAL(clicked()), this, SLOT(slotCancelRequested()));

    ui->lblExplanation->setText(i18nc("Recover an autosave for an already existing file: explanation in the recovery dialog",
                                      "An autosave for this file exists. How do you want to proceed?\n"
                                      "Warning: if you discard the autosave now, it will be removed."));

    ui->rbOpenAutosave->setChecked(true); // it should be selected by default
}

KisRecoverNamedAutosaveDialog::~KisRecoverNamedAutosaveDialog()
{
    delete ui;
}

QSize KisRecoverNamedAutosaveDialog::filePreviewIconSize() const
{
    return m_filePreviewIconSize;
}

void KisRecoverNamedAutosaveDialog::setMainFileIcon(const QIcon &icon)
{
    ui->rbDiscardAutosave->setIcon(icon);
    ui->rbDiscardAutosave->setIconSize(filePreviewIconSize());
}

void KisRecoverNamedAutosaveDialog::setAutosaveFileIcon(const QIcon &icon)
{
    ui->rbOpenAutosave->setIcon(icon);
    ui->rbOpenAutosave->setIconSize(filePreviewIconSize());
}

void KisRecoverNamedAutosaveDialog::slotOkRequested()
{
    close();
    setResult(ui->rbOpenAutosave->isChecked() ? OpenAutosave : OpenMainFile);
}

void KisRecoverNamedAutosaveDialog::slotCancelRequested()
{
    close();
    setResult(Cancel);
}
