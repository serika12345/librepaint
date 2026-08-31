/*
 *  SPDX-FileCopyrightText: 2014 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_do_nothing_processing_visitor.h"

#include <QtGlobal>

KisDoNothingProcessingVisitor::~KisDoNothingProcessingVisitor()
{
}

void KisDoNothingProcessingVisitor::visit(KisNode *node, KisUndoAdapter *undoAdapter)
{
    Q_UNUSED(node);
    Q_UNUSED(undoAdapter);
}

void KisDoNothingProcessingVisitor::visit(KisCloneLayer *layer, KisUndoAdapter *undoAdapter)
{
    Q_UNUSED(layer);
    Q_UNUSED(undoAdapter);
}

void KisDoNothingProcessingVisitor::visit(KisExternalLayer *layer, KisUndoAdapter *undoAdapter)
{
    Q_UNUSED(layer);
    Q_UNUSED(undoAdapter);
}

void KisDoNothingProcessingVisitor::visit(KisPaintLayer *layer, KisUndoAdapter *undoAdapter)
{
    Q_UNUSED(layer);
    Q_UNUSED(undoAdapter);
}

void KisDoNothingProcessingVisitor::visit(KisGroupLayer *layer, KisUndoAdapter *undoAdapter)
{
    Q_UNUSED(layer);
    Q_UNUSED(undoAdapter);
}

void KisDoNothingProcessingVisitor::visit(KisAdjustmentLayer *layer, KisUndoAdapter *undoAdapter)
{
    Q_UNUSED(layer);
    Q_UNUSED(undoAdapter);
}

void KisDoNothingProcessingVisitor::visit(KisGeneratorLayer *layer, KisUndoAdapter *undoAdapter)
{
    Q_UNUSED(layer);
    Q_UNUSED(undoAdapter);
}

void KisDoNothingProcessingVisitor::visit(KisFilterMask *mask, KisUndoAdapter *undoAdapter)
{
    Q_UNUSED(mask);
    Q_UNUSED(undoAdapter);
}

void KisDoNothingProcessingVisitor::visit(KisTransformMask *mask, KisUndoAdapter *undoAdapter)
{
    Q_UNUSED(mask);
    Q_UNUSED(undoAdapter);
}

void KisDoNothingProcessingVisitor::visit(KisTransparencyMask *mask, KisUndoAdapter *undoAdapter)
{
    Q_UNUSED(mask);
    Q_UNUSED(undoAdapter);
}

void KisDoNothingProcessingVisitor::visit(KisSelectionMask *mask, KisUndoAdapter *undoAdapter)
{
    Q_UNUSED(mask);
    Q_UNUSED(undoAdapter);
}

void KisDoNothingProcessingVisitor::visit(KisColorizeMask *mask, KisUndoAdapter *undoAdapter)
{
    Q_UNUSED(mask);
    Q_UNUSED(undoAdapter);
}
