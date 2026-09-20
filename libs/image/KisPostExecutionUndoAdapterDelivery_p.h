/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KISPOSTEXECUTIONUNDOADAPTERDELIVERY_P_H
#define KISPOSTEXECUTIONUNDOADAPTERDELIVERY_P_H

#include "kis_types.h"

class KUndo2Command;
class KUndo2MagicString;
class KisSavedMacroCommand;
class KisStrokesFacade;
class KisUndoStore;

namespace KisPostExecutionUndoAdapterDelivery
{
KUndo2Command *createSavedCommand(KUndo2CommandSP command, KisStrokesFacade *strokesFacade);
KisSavedMacroCommand *createSavedMacro(const KUndo2MagicString &macroName, KisStrokesFacade *strokesFacade);
void addCommand(KisUndoStore *undoStore, KUndo2Command *command);
void addMacro(KisUndoStore *undoStore, KisSavedMacroCommand *macro);
} // namespace KisPostExecutionUndoAdapterDelivery

#endif
