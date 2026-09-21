/*
 *  SPDX-FileCopyrightText: 2025 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "FontVariantEastAsianModel.h"
#include "KoSvgText.h"
#include <KisLager.h>
#include <lager/cursor.hpp>
#include <lager/extra/qt.hpp>

FontVariantEastAsianModel::FontVariantEastAsianModel(lager::cursor<KoSvgText::FontFeatureEastAsian> _data)
    : data(_data)
    , LAGER_QT(variant) {data[&KoSvgText::FontFeatureEastAsian::variant].zoom(kislager::lenses::do_static_cast<KoSvgText::EastAsianVariant, int>)}
    , LAGER_QT(width) {data[&KoSvgText::FontFeatureEastAsian::width].zoom(kislager::lenses::do_static_cast<KoSvgText::EastAsianWidth, int>)}
    , LAGER_QT(ruby) {data[&KoSvgText::FontFeatureEastAsian::ruby]}
{

}
