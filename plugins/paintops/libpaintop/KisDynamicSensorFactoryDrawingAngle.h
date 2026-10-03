/*
 *  SPDX-FileCopyrightText: 2022 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KISDYNAMICSENSORFACTORYDRAWINGANGLE_H
#define KISDYNAMICSENSORFACTORYDRAWINGANGLE_H

#include <KisCurveOptionDataCommon.h>
#include <KisSimpleDynamicSensorFactory.h>
#include <lager/cursor.hpp>

#include <kritapaintop_export.h>

class QWidget;

class PAINTOP_EXPORT KisDynamicSensorFactoryDrawingAngle : public KisSimpleDynamicSensorFactory
{
public:
    KisDynamicSensorFactoryDrawingAngle();
    QWidget* createConfigWidget(lager::cursor<KisCurveOptionDataCommon> data, QWidget*parent) override;
};

#endif // KISDYNAMICSENSORFACTORYDRAWINGANGLE_H
