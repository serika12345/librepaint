/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_SYNCHRONOUS_IMPORT_USER_FEEDBACK_H
#define KIS_SYNCHRONOUS_IMPORT_USER_FEEDBACK_H

#include <QPointer>

#include <kritaui_export.h>

#include "KisImportUserFeedbackInterface.h"

class QWidget;

class KRITAUI_EXPORT KisSynchronousImportUserFeedback final : public KisImportUserFeedbackInterface
{
public:
    KisSynchronousImportUserFeedback(QWidget *parent, bool batchMode);

    Result askUser(AskCallback callback) override;

private:
    QPointer<QWidget> m_parent;
    bool m_batchMode;
};

#endif
