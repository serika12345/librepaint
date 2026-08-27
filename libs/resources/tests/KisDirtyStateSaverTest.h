/*
 * SPDX-FileCopyrightText: 2026 LibrePaint Contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_DIRTY_STATE_SAVER_TEST_H
#define KIS_DIRTY_STATE_SAVER_TEST_H

#include <QObject>

class KisDirtyStateSaverTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void restoresInitialDirtyState_data();
    void restoresInitialDirtyState();
};

#endif // KIS_DIRTY_STATE_SAVER_TEST_H
