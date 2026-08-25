/*
 *  SPDX-FileCopyrightText: 2009 Cyrille Berger <cberger@cberger.net>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "commands/kis_node_commands_adapter.h"

#include <KoCompositeOp.h>
#include "kis_undo_adapter.h"
#include "kis_image.h"
#include "commands/kis_image_layer_move_command.h"
#include "commands/kis_image_layer_remove_command.h"
#include "commands/KisNodeRenameCommand.h"
#include "kis_node_compositeop_command.h"
#include "kis_node_opacity_command.h"
#include "kis_processing_applicator.h"
#include "kis_layer.h"
#include "kis_selection_mask.h"

namespace {

bool prepareNodeMove(KisNodeSP node, KisNodeSP parent)
{
    if (!parent->allowAsChild(node)) {
        return false;
    }

    KisSelectionMask *selectionMask = dynamic_cast<KisSelectionMask *>(node.data());
    KisLayer *parentLayer = qobject_cast<KisLayer *>(parent.data());
    if (selectionMask && selectionMask->active() && parentLayer && parentLayer->selectionMask()) {
        parentLayer->selectionMask()->setActive(false);
    }

    return true;
}

}

KisNodeCommandsAdapter::KisNodeCommandsAdapter(
    KisImageWSP image,
    QObject *parent)
    : QObject(parent)
    , m_image(image)
{
}

KisNodeCommandsAdapter::~KisNodeCommandsAdapter()
{
}

void KisNodeCommandsAdapter::setImage(KisImageWSP image)
{
    m_image = image;
}

KisImageSP KisNodeCommandsAdapter::image() const
{
    KisImageSP image = m_image.toStrongRef();
    Q_ASSERT(image);
    return image;
}

void KisNodeCommandsAdapter::applyOneCommandAsync(KUndo2Command *cmd, KisProcessingApplicator *applicator)
{
    if (applicator) {
        applicator->applyCommand(cmd, KisStrokeJobData::SEQUENTIAL, KisStrokeJobData::EXCLUSIVE);
    } else {
        KisImageSP currentImage = image();
        QScopedPointer<KisProcessingApplicator> localApplicator(
            new KisProcessingApplicator(currentImage, 0, KisProcessingApplicator::NONE,
                                        KisImageSignalVector(),
                                        cmd->text(),
                                        0, cmd->id()));
        localApplicator->applyCommand(cmd);
        localApplicator->end();
    }
}

void KisNodeCommandsAdapter::addNodeAsync(KisNodeSP node, KisNodeSP parent, KisNodeSP aboveThis, bool doRedoUpdates, bool doUndoUpdates, KisProcessingApplicator *applicator)
{
    KUndo2Command *cmd = new KisImageLayerAddCommand(image(), node, parent, aboveThis, doRedoUpdates, doUndoUpdates);
    applyOneCommandAsync(cmd, applicator);
}

void KisNodeCommandsAdapter::addNodeAsync(KisNodeSP node, KisNodeSP parent, quint32 index, bool doRedoUpdates, bool doUndoUpdates, KisProcessingApplicator *applicator)
{
    KUndo2Command *cmd = new KisImageLayerAddCommand(image(), node, parent, index, doRedoUpdates, doUndoUpdates);
    applyOneCommandAsync(cmd, applicator);
}

void KisNodeCommandsAdapter::beginMacro(const KUndo2MagicString& macroName)
{
    KisImageSP currentImage = image();
    Q_ASSERT(currentImage->undoAdapter());
    currentImage->undoAdapter()->beginMacro(macroName);
}

void KisNodeCommandsAdapter::addExtraCommand(KUndo2Command *command)
{
    KisImageSP currentImage = image();
    Q_ASSERT(currentImage->undoAdapter());
    currentImage->undoAdapter()->addCommand(command);
}

void KisNodeCommandsAdapter::endMacro()
{
    KisImageSP currentImage = image();
    Q_ASSERT(currentImage->undoAdapter());
    currentImage->undoAdapter()->endMacro();
}

void KisNodeCommandsAdapter::addNode(KisNodeSP node, KisNodeSP parent, KisNodeSP aboveThis, KisImageLayerAddCommand::Flags flags)
{
    KisImageSP currentImage = image();
    Q_ASSERT(currentImage->undoAdapter());
    currentImage->undoAdapter()->addCommand(new KisImageLayerAddCommand(currentImage, node, parent, aboveThis, flags));
}

void KisNodeCommandsAdapter::addNode(KisNodeSP node, KisNodeSP parent, quint32 index, KisImageLayerAddCommand::Flags flags)
{
    KisImageSP currentImage = image();
    Q_ASSERT(currentImage->undoAdapter());
    currentImage->undoAdapter()->addCommand(new KisImageLayerAddCommand(currentImage, node, parent, index, flags));
}

void KisNodeCommandsAdapter::moveNode(KisNodeSP node, KisNodeSP parent, KisNodeSP aboveThis)
{
    if (!prepareNodeMove(node, parent)) {
        return;
    }

    KisImageSP currentImage = image();
    Q_ASSERT(currentImage->undoAdapter());
    currentImage->undoAdapter()->addCommand(new KisImageLayerMoveCommand(currentImage, node, parent, aboveThis));
}

void KisNodeCommandsAdapter::moveNode(KisNodeSP node, KisNodeSP parent, quint32 indexaboveThis)
{
    if (!prepareNodeMove(node, parent)) {
        return;
    }

    KisImageSP currentImage = image();
    Q_ASSERT(currentImage->undoAdapter());
    currentImage->undoAdapter()->addCommand(new KisImageLayerMoveCommand(currentImage, node, parent, indexaboveThis));
}

void KisNodeCommandsAdapter::removeNode(KisNodeSP node)
{
    KisImageSP currentImage = image();
    Q_ASSERT(currentImage->undoAdapter());
    currentImage->undoAdapter()->addCommand(new KisImageLayerRemoveCommand(currentImage, node));
}

void KisNodeCommandsAdapter::setOpacity(KisNodeSP node, qint32 opacity)
{
    KUndo2Command *cmd = new KisNodeOpacityCommand(node, opacity);
    applyOneCommandAsync(cmd);
}

void KisNodeCommandsAdapter::setCompositeOp(KisNodeSP node,
                                            const KoCompositeOp* compositeOp)
{
    KUndo2Command *cmd = new KisNodeCompositeOpCommand(node,
                                                       compositeOp->id());
    applyOneCommandAsync(cmd);
}

void KisNodeCommandsAdapter::setNodeName(KisNodeSP node, const QString &name)
{
    KUndo2Command *cmd = new KisNodeRenameCommand(node, node->name(), name);
    applyOneCommandAsync(cmd);
}

void KisNodeCommandsAdapter::undoLastCommand()
{
    KisImageSP currentImage = image();
    Q_ASSERT(currentImage->undoAdapter());
    currentImage->undoAdapter()->undoLastCommand();
}
