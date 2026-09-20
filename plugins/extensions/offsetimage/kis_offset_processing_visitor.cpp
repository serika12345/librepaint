/*
 *  SPDX-FileCopyrightText: 2013 Lukáš Tvrdý <lukast.dev@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_offset_processing_visitor.h"

#include <klocalizedstring.h>



#include "kis_transparency_mask.h"

#include "kis_external_layer_iface.h"

#include "kis_paint_device.h"
#include "kis_transaction.h"
#include "kis_undo_adapter.h"
#include <kis_transform_worker.h>
#include "lazybrush/kis_colorize_mask.h"


KisOffsetProcessingVisitor::KisOffsetProcessingVisitor(const QPoint &offsetPoint, const QRect &rect)
    :   m_offset(offsetPoint),
        m_wrapRect(rect)

{
}

void KisOffsetProcessingVisitor::offsetPaintDevice(KisPaintDeviceSP device, KisUndoAdapter *undoAdapter)
{
    KisTransaction transaction(device);
    KisTransformWorker::offset(device, m_offset, m_wrapRect);
    transaction.commit(undoAdapter);
}

void KisOffsetProcessingVisitor::visitNodeWithPaintDevice(KisNode *node, KisUndoAdapter *undoAdapter)
{
    offsetPaintDevice(node->paintDevice(), undoAdapter);
}

void KisOffsetProcessingVisitor::visitExternalLayer(KisExternalLayer *layer, KisUndoAdapter *undoAdapter)
{
    Q_UNUSED(layer);
    Q_UNUSED(undoAdapter);
}

void KisOffsetProcessingVisitor::visitColorizeMask(KisColorizeMask *node, KisUndoAdapter *undoAdapter)
{
    QVector<KisPaintDeviceSP> devices = node->allPaintDevices();

    Q_FOREACH (KisPaintDeviceSP device, devices) {
        offsetPaintDevice(device, undoAdapter);
    }
}
