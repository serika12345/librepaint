/*
 *  SPDX-FileCopyrightText: 2024 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "CssLengthPercentageModel.h"
#include "KoSvgText.h"
#include <KisLager.h>
#include <lager/cursor.hpp>
#include <lager/extra/qt.hpp>
#include <lager/lenses.hpp>
#include <QtGlobal>

auto getValue =  lager::lenses::getset (
            [] (const KoSvgText::CssLengthPercentage &data) -> qreal {
    if (data.unit == KoSvgText::CssLengthPercentage::Percentage) {
        return data.value * 100;
    } else {
        return data.value;
    }
}, [] (KoSvgText::CssLengthPercentage data, const qreal &val) -> KoSvgText::CssLengthPercentage {
        if (data.unit == KoSvgText::CssLengthPercentage::Percentage) {
        data.value = val * 0.01;
        } else {
            data.value = val;
        }
        return data;
});

CssLengthPercentageModel::CssLengthPercentageModel(lager::cursor<KoSvgText::CssLengthPercentage> _data)
    : length(_data)
    , LAGER_QT(value) {length.zoom(getValue)}
, LAGER_QT(unitType) {length[&KoSvgText::CssLengthPercentage::unit].zoom(kislager::lenses::do_static_cast<KoSvgText::CssLengthPercentage::UnitType, int>)}
{}
