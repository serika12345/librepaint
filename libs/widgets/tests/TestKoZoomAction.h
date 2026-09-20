/*
 *  SPDX-FileCopyrightText: 2020 Agata Cacko <cacko.azh@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef __TEST_KO_ZOOM_ACTION_H
#define __TEST_KO_ZOOM_ACTION_H

#include <QObject>

class TestKoZoomAction : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void testTypeAndConstruction();
    void testStateTransitions();
    void testNearestStandardLevels();
    void testDebugOutput();
};

#endif /* __TEST_KO_ZOOM_ACTION_H */
