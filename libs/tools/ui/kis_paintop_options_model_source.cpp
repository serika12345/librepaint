/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "kis_paintop_options_model_source_p.h"

#include <QSignalMapper>

namespace KisPaintOpOptionsModelSource
{

OptionState optionState(const KisPaintOpOption *option)
{
    return {option->objectName(),
            option->category(),
            option->isCheckable(),
            option->isChecked(),
            option->isEnabled()};
}

void setChecked(KisPaintOpOption *option, bool checked)
{
    option->setChecked(checked);
}

void connectStateChanges(KisPaintOpOption *option,
                         QSignalMapper *mapper,
                         int row,
                         bool connectCheckedState)
{
    if (connectCheckedState) {
        QObject::connect(option, &KisPaintOpOption::sigCheckedChanged,
                         mapper, qOverload<>(&QSignalMapper::map));
    }

    QObject::connect(option, &KisPaintOpOption::sigEnabledChanged,
                     mapper, qOverload<>(&QSignalMapper::map));
    mapper->setMapping(option, row);
}

}
