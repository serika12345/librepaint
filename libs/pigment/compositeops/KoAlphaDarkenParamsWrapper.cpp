/*
 *  SPDX-FileCopyrightText: 2019 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoAlphaDarkenParamsWrapper.h"

#include <QDebug>

#include <ksharedconfig.h>
#include <kconfiggroup.h>


bool useCreamyAlphaDarken()
{
    static bool isConfigInitialized = false;
    static bool useCreamyAlphaDarken = true;

    if (!isConfigInitialized) {
        KConfigGroup cfg = KSharedConfig::openConfig()->group("");
        useCreamyAlphaDarken = cfg.readEntry("useCreamyAlphaDarken", true);
        isConfigInitialized = true;
    }

    if (!useCreamyAlphaDarken) {
        qInfo() << "INFO: requested old version of AlphaDarken composite op. Switching...";
    }

    return useCreamyAlphaDarken;
}
