/*
 * SPDX-FileCopyrightText: 2025 Dmitry Kazakov <dimula73@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_SURFACE_COLOR_SPACE_WRAPPER_TEST_H
#define KIS_SURFACE_COLOR_SPACE_WRAPPER_TEST_H

#include <QObject>

class KisSurfaceColorSpaceWrapperTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void enumValuesDefaultsAndFactories();
    void copyAndMovePreserveIndependentValues();
    void qtColorSpacesRoundTrip();
    void unsupportedQtColorSpaceFallsBackToDefault();
};

#endif
