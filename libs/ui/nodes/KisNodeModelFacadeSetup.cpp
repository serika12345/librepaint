/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_node_model.h"

void KisNodeModel::setDummiesFacade(KisDummiesFacadeBase *dummiesFacade,
                                    KisImageWSP image,
                                    KisShapeController *shapeController,
                                    KisSelectionActionsAdapter *selectionActionsAdapter,
                                    KisNodeManager *nodeManager)
{
    KisDummiesFacadeBase *oldDummiesFacade = FacadeSetupAccess::currentFacade(this);
    KisShapeController *oldShapeController = FacadeSetupAccess::currentShapeController(this);

    FacadeSetupAccess::configureCollaborators(this, shapeController, selectionActionsAdapter, nodeManager);
    FacadeSetupAccess::configureDisplayMode(this, nodeManager);

    if (oldDummiesFacade && FacadeSetupAccess::hasImage(this)) {
        FacadeSetupAccess::disconnectCurrentTree(this, oldDummiesFacade);
    }

    FacadeSetupAccess::replaceTree(this, dummiesFacade, image);

    if (FacadeSetupAccess::currentFacade(this)) {
        FacadeSetupAccess::connectCurrentTree(this);
    }

    if (FacadeSetupAccess::currentFacade(this) != oldDummiesFacade
        || FacadeSetupAccess::currentShapeController(this) != oldShapeController) {
        beginResetModel();
        endResetModel();
    }
}
