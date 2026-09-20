/*
 * SPDX-FileCopyrightText: 2006, 2010 Boudewijn Rempt <boud@valdyas.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_tool.h"

KisTool::AlternateAction KisTool::actionToAlternateAction(ToolAction action)
{
    KIS_ASSERT_RECOVER_RETURN_VALUE(action != Primary, Secondary);
    return (AlternateAction)action;
}
