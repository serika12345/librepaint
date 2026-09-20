/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QObject>

#if defined(KIS_NODE_MANAGER_LIFECYCLE_CONTRACT)
#pragma push_macro("Q_OBJECT")
#undef Q_OBJECT
#define Q_OBJECT
#endif
#include "kis_node_manager.h"
#if defined(KIS_NODE_MANAGER_LIFECYCLE_CONTRACT)
#pragma pop_macro("Q_OBJECT")
#endif

#include <QPointer>

KisNodeManager::KisNodeManager(KisViewManager *view)
    : m_d(static_cast<Private *>(LifecycleAccess::createPrivateState(this, view)))
{
    LifecycleAccess::connectReselectionOutput(this);
}

KisNodeManager::~KisNodeManager()
{
    LifecycleAccess::destroyPrivateState(m_d);
}

void KisNodeManager::setView(QPointer<KisView> imageView)
{
    LifecycleAccess::setMaskView(this, imageView);
    LifecycleAccess::setLayerView(this, imageView);

    if (LifecycleAccess::hasImageView(this)) {
        LifecycleAccess::disconnectNodeActivation(this);
        LifecycleAccess::disconnectImageSignals(this);
        LifecycleAccess::disconnectReselectionInput(this);
    }

    LifecycleAccess::assignImageView(this, imageView);
    LifecycleAccess::assignCommandImage(this);

    if (LifecycleAccess::hasImageView(this)) {
        LifecycleAccess::connectNodeActivation(this);

        if (!LifecycleAccess::currentNode(this)) {
            if (LifecycleAccess::lastActivatedNode(this) && !LifecycleAccess::currentNode(this)) {
                LifecycleAccess::activateNode(this, LifecycleAccess::lastActivatedNode(this));
            }
        } else if (!LifecycleAccess::hasGraphListener(LifecycleAccess::currentNode(this))) {
            LifecycleAccess::activateNode(this, LifecycleAccess::lastRootChild(this));
        }

        LifecycleAccess::connectReselectionInput(this);
        LifecycleAccess::notifyResourceProvider(this, LifecycleAccess::currentNode(this));
        LifecycleAccess::connectIsolation(this);
    }
}

void KisNodeManager::updateGUI()
{
    LifecycleAccess::updateLayerGui(this);
    LifecycleAccess::updateMaskGui(this);
}
