/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nodes/kis_node_manager.h"

#include "nodes/KisNodeActivationActionCreatorVisitor.h"

#include <kis_image.h>

void KisNodeManager::updateImageNodeSettings(KisImageWSP image)
{
    if (image) {
        image->rootLayer()->updateSettings();
    }
}

void KisNodeManager::createNodeActivationActions(KisImageWSP image, KisKActionCollection *collection)
{
    if (image) {
        KisNodeActivationActionCreatorVisitor visitor(collection, this);
        image->rootLayer()->accept(visitor);
    }
}
