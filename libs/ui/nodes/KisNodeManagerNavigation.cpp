/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nodes/kis_node_manager.h"

void KisNodeManager::activateNextNode(bool siblingsOnly)
{
    const KisNodeSP active = NavigationAccess::activeNode(this);
    if (!active) {
        return;
    }

    KisNodeSP next = NavigationAccess::nextSibling(active);
    if (!siblingsOnly) {
        while (next && NavigationAccess::hasChildren(next)) {
            next = NavigationAccess::firstChild(next);
        }
        if (!next) {
            next = NavigationAccess::parentNode(active);
        }
    }

    while (next && NavigationAccess::isHidden(this, next)) {
        next = NavigationAccess::nextSibling(next);
    }

    if (next && NavigationAccess::parentNode(next)) {
        NavigationAccess::activateNode(this, next);
    }
}

void KisNodeManager::activatePreviousNode(bool siblingsOnly)
{
    const KisNodeSP active = NavigationAccess::activeNode(this);
    if (!active) {
        return;
    }

    KisNodeSP previous = NavigationAccess::previousSibling(active);
    if (!siblingsOnly) {
        if (NavigationAccess::hasChildren(active)) {
            previous = NavigationAccess::lastChild(active);
        }
        if (!previous) {
            const KisNodeSP parent = NavigationAccess::parentNode(active);
            if (parent) {
                previous = NavigationAccess::previousSibling(parent);
            }
        }
    }

    while (previous && NavigationAccess::isHidden(this, previous)) {
        previous = NavigationAccess::previousSibling(previous);
    }

    if (previous && NavigationAccess::parentNode(previous)) {
        NavigationAccess::activateNode(this, previous);
    }
}

void KisNodeManager::activateNextSiblingNode()
{
    NavigationAccess::activateNextNode(this, true);
}

void KisNodeManager::activatePreviousSiblingNode()
{
    NavigationAccess::activatePreviousNode(this, true);
}

void KisNodeManager::switchToPreviouslyActiveNode()
{
    const KisNodeSP node = NavigationAccess::previouslyActiveNode(this);
    if (node && NavigationAccess::hasParent(node)) {
        NavigationAccess::activateNode(this, node);
    }
}
