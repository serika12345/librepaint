/*
 *  SPDX-FileCopyrightText: 2008 Lukas Tvrdy <lukast.dev@gmail.com>
 *  SPDX-FileCopyrightText: 2010 José Luis Vergara <pentalis@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisHatchingPreferencesModel.h"
#include "KisHatchingPreferencesData.h"
#include <lager/cursor.hpp>
#include <lager/extra/qt.hpp>

using namespace KisWidgetConnectionUtils;

KisHatchingPreferencesModel::KisHatchingPreferencesModel(lager::cursor<KisHatchingPreferencesData> _optionData)
    : optionData(_optionData)
    , LAGER_QT(useAntialias) {_optionData[&KisHatchingPreferencesData::useAntialias]}
    , LAGER_QT(useOpaqueBackground) {_optionData[&KisHatchingPreferencesData::useOpaqueBackground]}
    , LAGER_QT(useSubpixelPrecision) {_optionData[&KisHatchingPreferencesData::useSubpixelPrecision]}
{
}
