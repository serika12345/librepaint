/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <selection/kis_selection_manager.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_SELECTION_MANAGER_SIGNATURE(method, signature)                                                          \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisSelectionManager::method)), signature>)
} // namespace

class KisSelectionManagerSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeConstructionViewAndDisplaySchemaRemainStable();
    void clipboardTransferSignaturesRemainStable();
    void basicSelectionActionSignaturesRemainStable();
    void fillActionSignaturesRemainStable();
    void conversionAndShapeActionSignaturesRemainStable();
    void uiAndConfigurationActionSignaturesRemainStable();
    void selectionStateQuerySignaturesRemainStable();
    void notificationSignaturesRemainStable();
};

void KisSelectionManagerSchemaContractTest::typeConstructionViewAndDisplaySchemaRemainStable()
{
    static_assert(std::is_class_v<KisSelectionManager>);
    static_assert(std::is_constructible_v<KisSelectionManager, KisViewManager *>);
    static_assert(std::has_virtual_destructor_v<KisSelectionManager>);
    ASSERT_SELECTION_MANAGER_SIGNATURE(setup, void (KisSelectionManager::*)(KisActionManager *));
    ASSERT_SELECTION_MANAGER_SIGNATURE(setView, void (KisSelectionManager::*)(QPointer<KisView>));
    ASSERT_SELECTION_MANAGER_SIGNATURE(displaySelection, bool (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(showSelectionAsMask, bool (KisSelectionManager::*)() const);
}

void KisSelectionManagerSchemaContractTest::clipboardTransferSignaturesRemainStable()
{
    ASSERT_SELECTION_MANAGER_SIGNATURE(clipboardDataChanged, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(cut, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(copy, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(cutSharp, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(copySharp, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(copyMerged, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(paste, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(pasteNew, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(pasteAt, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(pasteInto, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(pasteAsReference, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(pasteShapeStyle, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(cutToNewLayer, void (KisSelectionManager::*)());
}

void KisSelectionManagerSchemaContractTest::basicSelectionActionSignaturesRemainStable()
{
    ASSERT_SELECTION_MANAGER_SIGNATURE(selectAll, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(deselect, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(invert, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(clear, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(reselect, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(editSelection, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(copySelectionToNewLayer, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(imageResizeToSelection, void (KisSelectionManager::*)());
}

void KisSelectionManagerSchemaContractTest::fillActionSignaturesRemainStable()
{
    ASSERT_SELECTION_MANAGER_SIGNATURE(fillForegroundColor, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(fillBackgroundColor, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(fillPattern, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(fillForegroundColorOpacity, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(fillBackgroundColorOpacity, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(fillPatternOpacity, void (KisSelectionManager::*)());
}

void KisSelectionManagerSchemaContractTest::conversionAndShapeActionSignaturesRemainStable()
{
    ASSERT_SELECTION_MANAGER_SIGNATURE(convertToVectorSelection, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(convertToRasterSelection, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(convertShapesToVectorSelection, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(convertToShape, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(shapeSelectionChanged, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(paintSelectedShapes, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(slotStrokeSelection, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(selectOpaqueOnNode, void (KisSelectionManager::*)(KisNodeSP, SelectionAction));
}

void KisSelectionManagerSchemaContractTest::uiAndConfigurationActionSignaturesRemainStable()
{
    ASSERT_SELECTION_MANAGER_SIGNATURE(updateGUI, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(selectionChanged, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(enableSelectionActionsPanel, void (KisSelectionManager::*)(bool));
    ASSERT_SELECTION_MANAGER_SIGNATURE(configChanged, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(toggleDisplaySelection, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(slotToggleSelectionDecoration, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(toggleSAPpin, void (KisSelectionManager::*)());
}

void KisSelectionManagerSchemaContractTest::selectionStateQuerySignaturesRemainStable()
{
    ASSERT_SELECTION_MANAGER_SIGNATURE(havePixelsSelected, bool (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(havePixelsInClipboard, bool (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(haveShapesSelected, bool (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(haveShapesInClipboard, bool (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(haveAnySelectionWithPixels, bool (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(haveShapeSelectionWithShapes, bool (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(haveRasterSelectionWithPixels, bool (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(canReselectDeactivatedSelection, bool (KisSelectionManager::*)());
}

void KisSelectionManagerSchemaContractTest::notificationSignaturesRemainStable()
{
    ASSERT_SELECTION_MANAGER_SIGNATURE(currentSelectionChanged, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(signalUpdateGUI, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(displaySelectionChanged, void (KisSelectionManager::*)());
    ASSERT_SELECTION_MANAGER_SIGNATURE(strokeSelected, void (KisSelectionManager::*)());
}

#undef ASSERT_SELECTION_MANAGER_SIGNATURE

QTEST_GUILESS_MAIN(KisSelectionManagerSchemaContractTest)

#include "KisSelectionManagerSchemaContractTest.moc"
