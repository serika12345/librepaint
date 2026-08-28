/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KIS_PAINTOP_OPTIONS_MODEL_SOURCE_P_H
#define KIS_PAINTOP_OPTIONS_MODEL_SOURCE_P_H

#include <QString>

#include "kis_paintop_option.h"

class QSignalMapper;

namespace KisPaintOpOptionsModelSource
{

struct OptionState
{
    QString objectName;
    KisPaintOpOption::PaintopCategory category {KisPaintOpOption::GENERAL};
    bool checkable {false};
    bool checked {false};
    bool enabled {false};
};

OptionState optionState(const KisPaintOpOption *option);
void setChecked(KisPaintOpOption *option, bool checked);
void connectStateChanges(KisPaintOpOption *option,
                         QSignalMapper *mapper,
                         int row,
                         bool connectCheckedState);

}

#endif
