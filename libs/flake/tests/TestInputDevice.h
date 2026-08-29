/*
 * SPDX-FileCopyrightText: 2011 Hanna Skott <hannaetscott@gmail.com>
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef TESTINPUTDEVICE_H
#define TESTINPUTDEVICE_H

#include <QObject>

class TestInputDevice : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void enumValuesMatchQtInputClassifications();
    void tabletEventsMapQtDeviceAndPointerTypes();
    void constructionAndMouseFallbackPreserveState();
    void copyAssignmentEqualityAndLifetime();
    void factoriesExposeCanonicalDevices();
    void equalDevicesProduceEqualHashes();
    void debugOutputKeepsCurrentLabels();
    void concreteMouseDeviceIsNotReportedAsMouse();
    void qt6AdditionalTypesUseUnsupportedDebugLabels();
};

#endif // TESTINPUTDEVICE_H
