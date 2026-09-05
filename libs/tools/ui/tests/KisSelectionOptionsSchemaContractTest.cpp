/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_selection_options.h"

#include <QTest>

#include <type_traits>

namespace
{
using Options = KisSelectionOptions;

#define ASSERT_SELECTION_OPTIONS_SIGNATURE(method, signature)                                                          \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&Options::method)), signature>)
} // namespace

class KisSelectionOptionsSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void selectionOptionsTypeLifetimeAndReferenceLayerSchemaRemainStable();
    void selectionOptionsModeActionAndAntialiasSignaturesRemainStable();
    void selectionOptionsAdjustmentSignaturesRemainStable();
    void selectionOptionsReferenceAndColorLabelSignaturesRemainStable();
    void selectionOptionsPresentationSignaturesRemainStable();
};

void KisSelectionOptionsSchemaContractTest::selectionOptionsTypeLifetimeAndReferenceLayerSchemaRemainStable()
{
    static_assert(std::is_class_v<Options>);
    static_assert(std::is_base_of_v<KisOptionCollectionWidget, Options>);
    static_assert(std::is_enum_v<Options::ReferenceLayers>);
    static_assert(Options::CurrentLayer == 0);
    static_assert(Options::AllLayers == 1);
    static_assert(Options::ColorLabeledLayers == 2);
    static_assert(std::is_default_constructible_v<Options>);
    static_assert(std::is_constructible_v<Options, QWidget *>);
    static_assert(std::has_virtual_destructor_v<Options>);
}

void KisSelectionOptionsSchemaContractTest::selectionOptionsModeActionAndAntialiasSignaturesRemainStable()
{
    ASSERT_SELECTION_OPTIONS_SIGNATURE(mode, SelectionMode (Options::*)() const);
    ASSERT_SELECTION_OPTIONS_SIGNATURE(setMode, void (Options::*)(SelectionMode));
    ASSERT_SELECTION_OPTIONS_SIGNATURE(modeChanged, void (Options::*)(SelectionMode));
    ASSERT_SELECTION_OPTIONS_SIGNATURE(action, SelectionAction (Options::*)() const);
    ASSERT_SELECTION_OPTIONS_SIGNATURE(setAction, void (Options::*)(SelectionAction));
    ASSERT_SELECTION_OPTIONS_SIGNATURE(actionChanged, void (Options::*)(SelectionAction));
    ASSERT_SELECTION_OPTIONS_SIGNATURE(antiAliasSelection, bool (Options::*)() const);
    ASSERT_SELECTION_OPTIONS_SIGNATURE(setAntiAliasSelection, void (Options::*)(bool));
    ASSERT_SELECTION_OPTIONS_SIGNATURE(antiAliasSelectionChanged, void (Options::*)(bool));
}

void KisSelectionOptionsSchemaContractTest::selectionOptionsAdjustmentSignaturesRemainStable()
{
    ASSERT_SELECTION_OPTIONS_SIGNATURE(growSelection, int (Options::*)() const);
    ASSERT_SELECTION_OPTIONS_SIGNATURE(setGrowSelection, void (Options::*)(int));
    ASSERT_SELECTION_OPTIONS_SIGNATURE(growSelectionChanged, void (Options::*)(int));
    ASSERT_SELECTION_OPTIONS_SIGNATURE(featherSelection, int (Options::*)() const);
    ASSERT_SELECTION_OPTIONS_SIGNATURE(setFeatherSelection, void (Options::*)(int));
    ASSERT_SELECTION_OPTIONS_SIGNATURE(featherSelectionChanged, void (Options::*)(int));
    ASSERT_SELECTION_OPTIONS_SIGNATURE(stopGrowingAtDarkestPixel, bool (Options::*)() const);
    ASSERT_SELECTION_OPTIONS_SIGNATURE(setStopGrowingAtDarkestPixel, void (Options::*)(bool));
    ASSERT_SELECTION_OPTIONS_SIGNATURE(stopGrowingAtDarkestPixelChanged, void (Options::*)(bool));
}

void KisSelectionOptionsSchemaContractTest::selectionOptionsReferenceAndColorLabelSignaturesRemainStable()
{
    ASSERT_SELECTION_OPTIONS_SIGNATURE(referenceLayers, Options::ReferenceLayers (Options::*)() const);
    ASSERT_SELECTION_OPTIONS_SIGNATURE(setReferenceLayers, void (Options::*)(Options::ReferenceLayers));
    ASSERT_SELECTION_OPTIONS_SIGNATURE(referenceLayersChanged, void (Options::*)(Options::ReferenceLayers));
    ASSERT_SELECTION_OPTIONS_SIGNATURE(selectedColorLabels, QList<int> (Options::*)() const);
    ASSERT_SELECTION_OPTIONS_SIGNATURE(setSelectedColorLabels, void (Options::*)(const QList<int> &));
    ASSERT_SELECTION_OPTIONS_SIGNATURE(selectedColorLabelsChanged, void (Options::*)());
}

void KisSelectionOptionsSchemaContractTest::selectionOptionsPresentationSignaturesRemainStable()
{
    ASSERT_SELECTION_OPTIONS_SIGNATURE(setActionSectionVisible, void (Options::*)(bool));
    ASSERT_SELECTION_OPTIONS_SIGNATURE(setAdjustmentsSectionVisible, void (Options::*)(bool));
    ASSERT_SELECTION_OPTIONS_SIGNATURE(setModeSectionVisible, void (Options::*)(bool));
    ASSERT_SELECTION_OPTIONS_SIGNATURE(setReferenceSectionVisible, void (Options::*)(bool));
    ASSERT_SELECTION_OPTIONS_SIGNATURE(setStopGrowingAtDarkestPixelButtonVisible, void (Options::*)(bool));
    ASSERT_SELECTION_OPTIONS_SIGNATURE(updateActionButtonToolTip,
                                       void (Options::*)(SelectionAction, const QKeySequence &));
}

QTEST_GUILESS_MAIN(KisSelectionOptionsSchemaContractTest)

#include "KisSelectionOptionsSchemaContractTest.moc"
