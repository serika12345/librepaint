/*
 *  SPDX-FileCopyrightText: 2011 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_post_execution_undo_adapter.h"

#include "KisPostExecutionUndoAdapterDelivery_p.h"


KisPostExecutionUndoAdapter::KisPostExecutionUndoAdapter(KisUndoStore *undoStore,
                                                         KisStrokesFacade *strokesFacade)
    : m_undoStore(undoStore),
      m_strokesFacade(strokesFacade)
{
}

void KisPostExecutionUndoAdapter::addCommand(KUndo2CommandSP command)
{
    if(!command) return;
    KisPostExecutionUndoAdapterDelivery::addCommand(
        m_undoStore,
        KisPostExecutionUndoAdapterDelivery::createSavedCommand(command, m_strokesFacade));
}

KisSavedMacroCommand* KisPostExecutionUndoAdapter::createMacro(const KUndo2MagicString& macroName)
{
    return KisPostExecutionUndoAdapterDelivery::createSavedMacro(macroName, m_strokesFacade);
}

void KisPostExecutionUndoAdapter::addMacro(KisSavedMacroCommand *macro)
{
    KisPostExecutionUndoAdapterDelivery::addMacro(m_undoStore, macro);
}

void KisPostExecutionUndoAdapter::setUndoStore(KisUndoStore *undoStore)
{
    m_undoStore = undoStore;
}

KisStrokesFacade* KisPostExecutionUndoAdapter::strokesFacade() const
{
    return m_strokesFacade;
}
