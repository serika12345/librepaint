/*
 *  SPDX-FileCopyrightText: 2022 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisMirrorOptionModel.h"

#include <lager/cursor.hpp>
#include <lager/extra/qt.hpp>

#include "KisMirrorOptionData.h"

KisMirrorOptionModel::KisMirrorOptionModel(lager::cursor<KisMirrorOptionMixIn> optionData)
    : mirrorOptionData(optionData)
    , LAGER_QT(enableVerticalMirror) {mirrorOptionData[&KisMirrorOptionMixIn::enableVerticalMirror]}
    , LAGER_QT(enableHorizontalMirror) {mirrorOptionData[&KisMirrorOptionMixIn::enableHorizontalMirror]}
{
}
