/*
 *  SPDX-FileCopyrightText: 2022 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "MyPaintCurveOptionData.h"
#include "KisCurveOptionDataCommon.h"
#include "KoID.h"
#include "MyPaintSensorPack.h"
#include <qhashfunctions.h>
#include <qtypes.h>

MyPaintCurveOptionData::MyPaintCurveOptionData(const QString &prefix,
                                       const KoID &id,
                                       bool isCheckable,
                                       bool isChecked,
                                       qreal minValue,
                                       qreal maxValue)
    : MyPaintCurveOptionData(prefix,
                             id,
                             isCheckable,
                             isChecked,
                             minValue,
                             maxValue,
                             new MyPaintSensorPack())
{
}

MyPaintCurveOptionData::MyPaintCurveOptionData(const KoID &id,
                                       bool isCheckable,
                                       bool isChecked,
                                       qreal minValue,
                                       qreal maxValue)
    : MyPaintCurveOptionData("", id, isCheckable, isChecked, minValue, maxValue)
{
}

MyPaintCurveOptionData::MyPaintCurveOptionData(const QString &prefix, const KoID &id, bool isCheckable, bool isChecked, qreal minValue, qreal maxValue, MyPaintSensorPack *sensorInterface)
    : KisCurveOptionDataCommon(prefix,
                               id,
                               isCheckable,
                               isChecked,
                               minValue,
                               maxValue,
                               sensorInterface)
{
}

MyPaintSensorData &MyPaintCurveOptionData::sensorStruct()
{
    return dynamic_cast<MyPaintSensorPack *>(sensorData.data())->sensorsStruct();
}

const MyPaintSensorData &MyPaintCurveOptionData::sensorStruct() const
{
    return dynamic_cast<const MyPaintSensorPack*>(sensorData.constData())->constSensorsStruct();
}
