/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2012 Boudewijn Rempt <boud@valdyas.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef KISAUTOSAVERECOVERYDIALOG_H
#define KISAUTOSAVERECOVERYDIALOG_H

#include <KoDialog.h>
#include <QStringList>
#include <QModelIndex>

#include "kritadocumentui_export.h"
#include <files/kis_document_autosave_files.h>

class QListView;

Q_DECLARE_METATYPE(QModelIndex)

class KRITADOCUMENTUI_EXPORT KisAutoSaveRecoveryDialog : public KoDialog
{
    Q_OBJECT
public:

    explicit KisAutoSaveRecoveryDialog(
        const QList<Krita::Document::KisDocumentAutoSaveFile> &files,
        QWidget *parent = 0);
    ~KisAutoSaveRecoveryDialog() override;
    QStringList recoverableFiles();

public Q_SLOTS:

    void toggleFileItem(bool toggle);
    void slotDeleteAll();

private:

    QListView *m_listView;

    class FileItemModel;
    FileItemModel *m_model;
};


#endif // KISAUTOSAVERECOVERYDIALOG_H
