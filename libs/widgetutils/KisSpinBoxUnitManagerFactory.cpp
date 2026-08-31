/*
 *  SPDX-FileCopyrightText: 2017 Laurent Valentin Jospin <laurent.valentin@famillejospin.ch>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_spin_box_unit_manager.h"

KisSpinBoxUnitManagerBuilder* KisSpinBoxUnitManagerFactory::builder = nullptr;

KisSpinBoxUnitManager* KisSpinBoxUnitManagerFactory::buildDefaultUnitManager(QObject* parent)
{
    if (builder == nullptr) {
        return new KisSpinBoxUnitManager(parent);
    }

    return builder->buildUnitManager(parent);
}

void KisSpinBoxUnitManagerFactory::setDefaultUnitManagerBuilder(KisSpinBoxUnitManagerBuilder* pBuilder)
{
    if (builder != nullptr) {
        delete builder; //The factory took over the lifecycle of the builder, so it delete it when replaced.
    }

    builder = pBuilder;
}

void KisSpinBoxUnitManagerFactory::clearUnitManagerBuilder()
{
    if (builder != nullptr) {
        delete builder; //The factory took over the lifecycle of the builder, so it delete it when replaced.
    }

    builder = nullptr;
}
