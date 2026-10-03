/*
 *  SPDX-FileCopyrightText: 2022 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KISDYNAMICSENSORFACTORYFADE_H
#define KISDYNAMICSENSORFACTORYFADE_H

#include <QString>

#include <KisCurveOptionDataCommon.h>
#include <KisSimpleDynamicSensorFactory.h>
#include <lager/cursor.hpp>

#include <kritapaintop_export.h>

class QWidget;

class PAINTOP_EXPORT KisDynamicSensorFactoryFade : public KisSimpleDynamicSensorFactory
{
public:
    KisDynamicSensorFactoryFade();
    QWidget* createConfigWidget(lager::cursor<KisCurveOptionDataCommon> data, QWidget*parent) override;

    int maximumValue(int length) override;
    QString maximumLabel(int length) override;
};

#endif // KISDYNAMICSENSORFACTORYFADE_H
