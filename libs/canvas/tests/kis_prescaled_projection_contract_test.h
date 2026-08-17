/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_PRESCALED_PROJECTION_CONTRACT_TEST_H
#define KIS_PRESCALED_PROJECTION_CONTRACT_TEST_H

#include <QObject>

class KisPrescaledProjectionContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testDirtyRegionProducesProjectionNotification();
    void testEmptyUpdatePreservesLastValidFrame();
};

#endif
