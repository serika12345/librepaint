/*
 *  SPDX-FileCopyrightText: 2022 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisSmudgeLengthOptionModel.h"
#include "KisSmudgeLengthOptionData.h"
#include "KisWidgetConnectionUtils.h"

#include <KisLager.h>
#include <lager/cursor.hpp>
#include <lager/extra/qt.hpp>
#include <lager/reader.hpp>
#include <lager/with.hpp>

namespace {
CheckBoxState calcUseNewEngineState(bool useNewEngine, bool forceUseNewEngine) {
    return {useNewEngine || forceUseNewEngine, !forceUseNewEngine};
}
}

KisSmudgeLengthOptionModel::KisSmudgeLengthOptionModel(lager::cursor<KisSmudgeLengthOptionMixIn> optionData,
                                                       lager::reader<bool> forceUseNewEngine)
    : optionData(optionData)
    , LAGER_QT(mode) {optionData[&KisSmudgeLengthOptionMixIn::mode].zoom(kislager::lenses::do_static_cast<KisSmudgeLengthOptionMixIn::Mode, int>)}
    , LAGER_QT(smearAlpha) {optionData[&KisSmudgeLengthOptionMixIn::smearAlpha]}
    , LAGER_QT(useNewEngine) {optionData[&KisSmudgeLengthOptionMixIn::useNewEngine]}
    , LAGER_QT(useNewEngineState) {
        lager::with(optionData[&KisSmudgeLengthOptionMixIn::useNewEngine],
                    forceUseNewEngine)
                .map(&calcUseNewEngineState)}
{
}

KisSmudgeLengthOptionMixIn KisSmudgeLengthOptionModel::backedOptionData() const
{
    KisSmudgeLengthOptionMixIn data = optionData.get();
    data.useNewEngine = useNewEngineState().value;
    return data;
}
