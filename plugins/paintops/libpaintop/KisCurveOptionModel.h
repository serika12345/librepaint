/*
 *  SPDX-FileCopyrightText: 2022 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KISCURVEOPTIONMODEL_H
#define KISCURVEOPTIONMODEL_H

#include <memory>
#include <optional>
#include <tuple>

#include <QObject>
#include <QString>
#include <QtCore/qtypes.h>

#include <lager/constant.hpp>
#include <lager/cursor.hpp>
#include <lager/extra/qt.hpp>
#include <lager/reader.hpp>
#include <lager/state.hpp>

#include "KisCurveOptionData.h"
#include "KisCurveRangeModelInterface.h"
#include "kritapaintop_export.h"

using RangeState = std::tuple<qreal, qreal>;
using StrengthState = std::tuple<qreal, qreal, qreal>;
using LabelsState = std::tuple<QString, int>;


class PAINTOP_EXPORT KisCurveOptionModel : public QObject
{
    Q_OBJECT
public:
    KisCurveOptionModel(lager::cursor<KisCurveOptionDataCommon> optionData,
                        lager::reader<bool> externallyEnabled,
                        std::optional<lager::reader<RangeState>> strengthRangeOverride,
                        qreal strengthDisplayMultiplier,
                        KisCurveRangeModelFactory rangeModelFactory);
    ~KisCurveOptionModel();

    // the state must be declared **before** any cursors or readers
    lager::cursor<KisCurveOptionDataCommon> optionData;
    lager::reader<RangeState> strengthRangeNorm;
    lager::state<QString, lager::automatic_tag> activeSensorIdData;
    LAGER_QT_READER(bool, isCheckable);
    LAGER_QT_CURSOR(bool, isChecked);
    LAGER_QT_READER(bool, effectiveIsChecked);
    LAGER_QT_READER(qreal, effectiveStrengthValueNorm);
    LAGER_QT_CURSOR(qreal, strengthValueDenorm);
    LAGER_QT_READER(StrengthState, effectiveStrengthStateDenorm);
    LAGER_QT_CURSOR(bool, useCurve);
    LAGER_QT_CURSOR(bool, useSameCurve);
    LAGER_QT_CURSOR(int, curveMode);
    LAGER_QT_CURSOR(QString, activeSensorId);
    LAGER_QT_READER(int, activeSensorLength);
    LAGER_QT_READER(LabelsState, labelsState);
    LAGER_QT_CURSOR(QString, activeCurve);
    std::unique_ptr<KisCurveRangeModelInterface> rangeModel;
    LAGER_QT_CURSOR(QString, displayedCurve);
    LAGER_QT_READER(QString, curveXMinLabel);
    LAGER_QT_READER(QString, curveXMaxLabel);
    LAGER_QT_READER(QString, curveYMinLabel);
    LAGER_QT_READER(QString, curveYMaxLabel);

    KisCurveOptionDataCommon bakedOptionData() const;
};

#endif // KISCURVEOPTIONMODEL_H
