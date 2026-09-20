/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_node_manager.h"

#include <QStringList>

namespace
{

struct ActionRegistration {
    const char *id;
    const char *signal;
    const char *slot;
    bool checkable{false};
    bool shapeLayerOnly{false};
    bool storePinAction{false};
};

struct TypeRegistration {
    const char *id;
    const char *type;
};

constexpr ActionRegistration initialActions[]{
    {"mirrorNodeX", "triggered()", "mirrorNodeX()"},
    {"mirrorNodeY", "triggered()", "mirrorNodeY()"},
    {"mirrorAllNodesX", "triggered()", "mirrorAllNodesX()"},
    {"mirrorAllNodesY", "triggered()", "mirrorAllNodesY()"},
    {"activateNextLayer", "triggered()", "activateNextNode()"},
    {"activateNextSiblingLayer", "triggered()", "activateNextSiblingNode()"},
    {"activatePreviousLayer", "triggered()", "activatePreviousNode()"},
    {"activatePreviousSiblingLayer", "triggered()", "activatePreviousSiblingNode()"},
    {"switchToPreviouslyActiveNode", "triggered()", "switchToPreviouslyActiveNode()"},
    {"save_node_as_image", "triggered()", "saveNodeAsImage()"},
    {"save_vector_node_to_svg", "triggered()", "saveVectorLayerAsImage()", false, true},
    {"duplicatelayer", "triggered()", "duplicateActiveNode()"},
    {"copy_layer_clipboard", "triggered()", "copyLayersToClipboard()"},
    {"cut_layer_clipboard", "triggered()", "cutLayersToClipboard()"},
    {"paste_layer_from_clipboard", "triggered()", "pasteLayersFromClipboard()"},
    {"create_quick_group", "triggered()", "createQuickGroup()"},
    {"create_quick_clipping_group", "triggered()", "createQuickClippingGroup()"},
    {"quick_ungroup", "triggered()", "quickUngroup()"},
    {"select_all_layers", "triggered()", "selectAllNodes()"},
    {"select_visible_layers", "triggered()", "selectVisibleNodes()"},
    {"select_locked_layers", "triggered()", "selectLockedNodes()"},
    {"select_invisible_layers", "triggered()", "selectInvisibleNodes()"},
    {"select_unlocked_layers", "triggered()", "selectUnlockedNodes()"},
    {"new_from_visible", "triggered()", "createFromVisible()"},
    {"create_reference_image_from_active_layer", "triggered()", "createReferenceImageFromLayer()"},
    {"create_reference_image_from_visible_canvas", "triggered()", "createReferenceImageFromVisible()"},
    {"pin_to_timeline", "toggled(bool)", "slotPinToTimeline(bool)", true, false, true},
};

constexpr TypeRegistration creationActions[]{
    {"add_new_paint_layer", "KisPaintLayer"},
    {"add_new_group_layer", "KisGroupLayer"},
    {"add_new_clone_layer", "KisCloneLayer"},
    {"add_new_shape_layer", "KisShapeLayer"},
    {"add_new_adjustment_layer", "KisAdjustmentLayer"},
    {"add_new_fill_layer", "KisGeneratorLayer"},
    {"add_new_file_layer", "KisFileLayer"},
    {"add_new_transparency_mask", "KisTransparencyMask"},
    {"add_new_filter_mask", "KisFilterMask"},
    {"add_new_fast_color_overlay_mask", "FastColorOverlayFilterMask"},
    {"add_new_colorize_mask", "KisColorizeMask"},
    {"add_new_transform_mask", "KisTransformMask"},
    {"add_new_selection_mask", "KisSelectionMask"},
};

constexpr ActionRegistration isolationActions[]{
    {"isolate_active_layer", "toggled(bool)", "setIsolateActiveLayerMode(bool)"},
    {"isolate_active_group", "triggered(bool)", "setIsolateActiveGroupMode(bool)"},
};

constexpr ActionRegistration trailingActions[]{
    {"toggle_layer_visibility", "triggered()", "toggleVisibility()"},
    {"toggle_layer_lock", "triggered()", "toggleLock()"},
    {"toggle_layer_inherit_alpha", "triggered()", "toggleInheritAlpha()"},
    {"toggle_layer_alpha_lock", "triggered()", "toggleAlphaLock()"},
    {"split_alpha_into_mask", "triggered()", "slotSplitAlphaIntoMask()"},
    {"split_alpha_write", "triggered()", "slotSplitAlphaWrite()"},
    {"split_alpha_save_merged", "triggered()", "slotSplitAlphaSaveMerged()"},
};

} // namespace

void KisNodeManager::setup(KisKActionCollection *actionCollection, KisActionManager *actionManager)
{
    SetupAccess::setupLayerManager(this, actionManager);
    SetupAccess::setupMaskManager(this, actionCollection, actionManager);

    const auto registerActions = [this, actionManager](const auto &registrations) {
        for (const ActionRegistration &registration : registrations) {
            SetupAccess::registerAction(this,
                                        actionManager,
                                        registration.id,
                                        registration.signal,
                                        registration.slot,
                                        registration.checkable,
                                        registration.shapeLayerOnly,
                                        registration.storePinAction);
        }
    };
    registerActions(initialActions);

    for (const TypeRegistration &registration : creationActions) {
        SetupAccess::registerNodeCreation(this, actionManager, registration.id, registration.type);
    }
    SetupAccess::connectNodeCreation(this, SetupAccess::deferNodeCreation());

    SetupAccess::registerNodeConversion(this,
                                        actionManager,
                                        "convert_to_paint_layer",
                                        "KisPaintLayer",
                                        {QStringLiteral("KisPaintLayer")});
    SetupAccess::registerNodeConversion(this,
                                        actionManager,
                                        "convert_to_selection_mask",
                                        "KisSelectionMask",
                                        {QStringLiteral("KisSelectionMask"), QStringLiteral("KisColorizeMask")});
    SetupAccess::registerNodeConversion(this,
                                        actionManager,
                                        "convert_to_filter_mask",
                                        "KisFilterMask",
                                        {QStringLiteral("KisFilterMask"), QStringLiteral("KisColorizeMask")});
    SetupAccess::registerNodeConversion(this,
                                        actionManager,
                                        "convert_to_transparency_mask",
                                        "KisTransparencyMask",
                                        {QStringLiteral("KisTransparencyMask"), QStringLiteral("KisColorizeMask")});
    SetupAccess::registerNodeConversion(this,
                                        actionManager,
                                        "convert_to_file_layer",
                                        "KisFileLayer",
                                        {QStringLiteral("KisFileLayer"), QStringLiteral("KisCloneLayer")});
    SetupAccess::connectNodeConversion(this);

    registerActions(isolationActions);
    SetupAccess::connectNodeActivationToIsolation(this);
    registerActions(trailingActions);
}
