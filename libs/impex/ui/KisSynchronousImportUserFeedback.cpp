/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisSynchronousImportUserFeedback.h"

#include <QWidget>

KisSynchronousImportUserFeedback::KisSynchronousImportUserFeedback(QWidget *parent,
                                                                   bool batchMode)
    : m_parent(parent)
    , m_batchMode(batchMode)
{
}

KisImportUserFeedbackInterface::Result
KisSynchronousImportUserFeedback::askUser(AskCallback callback)
{
    if (m_batchMode || !m_parent) {
        return SuppressedByBatchMode;
    }

    return callback(m_parent) ? Success : UserCancelled;
}
