/*
 *  SPDX-FileCopyrightText: 2025 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "FontVariantLigaturesModel.h"
#include "KoSvgText.h"
#include <lager/cursor.hpp>
#include <lager/extra/qt.hpp>

FontVariantLigaturesModel::FontVariantLigaturesModel(lager::cursor<KoSvgText::FontFeatureLigatures> _data)
    : data(_data)
    , LAGER_QT(commonLigatures) {data[&KoSvgText::FontFeatureLigatures::commonLigatures]}
    , LAGER_QT(discretionaryLigatures) {data[&KoSvgText::FontFeatureLigatures::discretionaryLigatures]}
    , LAGER_QT(historicalLigatures) {data[&KoSvgText::FontFeatureLigatures::historicalLigatures]}
    , LAGER_QT(contextualAlternates) {data[&KoSvgText::FontFeatureLigatures::contextualAlternates]}
{

}
