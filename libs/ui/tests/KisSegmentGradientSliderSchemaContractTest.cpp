/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <widgets/gradient/KisGenericGradientEditor.h>
#include <widgets/gradient/KisGradientChooser.h>
#include <widgets/gradient/KisSegmentGradientSlider.h>

#include <QTest>

#include <type_traits>
#include <utility>

class KisSegmentGradientSliderSchemaContractTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void segmentGradientSliderHandleSchemaRemainsStable();
    void segmentGradientSliderConstructionAndSelectionSchemaRemainsStable();
    void segmentGradientSliderNavigationAndNotificationSchemaRemainsStable();
    void segmentGradientSliderHandleMovementSchemaRemainsStable();
    void segmentGradientSliderMutationSchemaRemainsStable();
    void gradientChooserTypeAndEnumerationSchemaRemainStable();
    void gradientChooserResourceAndPresentationSignaturesRemainStable();
    void gradientChooserConfigurationAndNotificationSignaturesRemainStable();
    void genericGradientEditorTypeAndPresentationSchemaRemainStable();
    void genericGradientEditorConfigurationSignaturesRemainStable();
};

void KisSegmentGradientSliderSchemaContractTest::segmentGradientSliderHandleSchemaRemainsStable()
{
    using Slider = KisSegmentGradientSlider;
    using Handle = Slider::Handle;
    static_assert(std::is_class_v<Slider> && std::is_enum_v<Slider::HandleType> && std::is_class_v<Handle>);
    static_assert(Slider::HandleType_None == 0 && Slider::HandleType_Segment == 1);
    static_assert(Slider::HandleType_MidPoint == 2 && Slider::HandleType_Stop == 3);
    static_assert(std::is_same_v<decltype(&Handle::type), Slider::HandleType Handle::*>);
    static_assert(std::is_same_v<decltype(&Handle::index), int Handle::*>);
    static_assert(Slider::shrinkEpsilon == qreal(0.00001));
}

void KisSegmentGradientSliderSchemaContractTest::segmentGradientSliderConstructionAndSelectionSchemaRemainsStable()
{
    using Slider = KisSegmentGradientSlider;
    static_assert(std::is_constructible_v<Slider, QWidget *, const char *, Qt::WindowFlags>);
    static_assert(std::is_default_constructible_v<Slider>);
    static_assert(std::is_same_v<decltype(&Slider::selectedHandle), Slider::Handle (Slider::*)()>);
    static_assert(std::is_same_v<decltype(&Slider::sizeHint), QSize (Slider::*)() const>);
    static_assert(std::is_same_v<decltype(&Slider::minimumSizeHint), QSize (Slider::*)() const>);
    static_assert(std::is_same_v<decltype(&Slider::setGradientResource), void (Slider::*)(KoSegmentGradientSP)>);
}

void KisSegmentGradientSliderSchemaContractTest::segmentGradientSliderNavigationAndNotificationSchemaRemainsStable()
{
    using Slider = KisSegmentGradientSlider;
    using Action = void (Slider::*)();
    static_assert(std::is_same_v<decltype(&Slider::chooseSelectedStopColor), Action>);
    static_assert(std::is_same_v<decltype(&Slider::selectPreviousHandle), Action>);
    static_assert(std::is_same_v<decltype(&Slider::selectNextHandle), Action>);
    static_assert(std::is_same_v<decltype(&Slider::centerSelectedHandle), Action>);
    static_assert(std::is_same_v<decltype(&Slider::selectedHandleChanged), Action>);
    static_assert(std::is_same_v<decltype(&Slider::updateRequested), Action>);
}

void KisSegmentGradientSliderSchemaContractTest::segmentGradientSliderHandleMovementSchemaRemainsStable()
{
    using Slider = KisSegmentGradientSlider;
    using HandleMove = void (Slider::*)(Slider::Handle, qreal, bool);
    using SelectionMove = void (Slider::*)(qreal, bool);
    static_assert(std::is_same_v<decltype(&Slider::moveHandle), HandleMove>);
    static_assert(std::is_same_v<decltype(&Slider::moveHandleLeft), HandleMove>);
    static_assert(std::is_same_v<decltype(&Slider::moveHandleRight), HandleMove>);
    static_assert(std::is_same_v<decltype(&Slider::moveSelectedHandle), SelectionMove>);
    static_assert(std::is_same_v<decltype(&Slider::moveSelectedHandleLeft), SelectionMove>);
    static_assert(std::is_same_v<decltype(&Slider::moveSelectedHandleRight), SelectionMove>);
    static_assert(std::is_same_v<decltype(&Slider::deleteHandle), void (Slider::*)(Slider::Handle)>);
    static_assert(std::is_same_v<decltype(std::declval<Slider &>().moveHandle({}, qreal{})), void>);
    static_assert(std::is_same_v<decltype(std::declval<Slider &>().moveHandleLeft({}, qreal{})), void>);
    static_assert(std::is_same_v<decltype(std::declval<Slider &>().moveHandleRight({}, qreal{})), void>);
    static_assert(std::is_same_v<decltype(std::declval<Slider &>().moveSelectedHandle(qreal{})), void>);
    static_assert(std::is_same_v<decltype(std::declval<Slider &>().moveSelectedHandleLeft(qreal{})), void>);
    static_assert(std::is_same_v<decltype(std::declval<Slider &>().moveSelectedHandleRight(qreal{})), void>);
}

void KisSegmentGradientSliderSchemaContractTest::segmentGradientSliderMutationSchemaRemainsStable()
{
    using Slider = KisSegmentGradientSlider;
    using Action = void (Slider::*)();
    static_assert(std::is_same_v<decltype(&Slider::deleteSelectedHandle), Action>);
    static_assert(std::is_same_v<decltype(&Slider::collapseSelectedSegment), Action>);
    static_assert(std::is_same_v<decltype(&Slider::splitSelectedSegment), Action>);
    static_assert(std::is_same_v<decltype(&Slider::duplicateSelectedSegment), Action>);
    static_assert(std::is_same_v<decltype(&Slider::mirrorSelectedSegment), Action>);
    static_assert(std::is_same_v<decltype(&Slider::flipGradient), Action>);
    static_assert(std::is_same_v<decltype(&Slider::distributeStopsEvenly), Action>);
}

void KisSegmentGradientSliderSchemaContractTest::gradientChooserTypeAndEnumerationSchemaRemainStable()
{
    using Chooser = KisGradientChooser;
    static_assert(std::is_class_v<Chooser> && std::has_virtual_destructor_v<Chooser>);
    static_assert(std::is_base_of_v<QFrame, Chooser>);
    static_assert(std::is_enum_v<Chooser::ViewMode> && std::is_enum_v<Chooser::ItemSize>);
    static_assert(Chooser::ViewMode_Icon == 0 && Chooser::ViewMode_List == 1);
    static_assert(Chooser::ItemSize_Small == 0 && Chooser::ItemSize_Medium == 1);
    static_assert(Chooser::ItemSize_Large == 2 && Chooser::ItemSize_Custom == 3);
    static_assert(std::is_default_constructible_v<Chooser>);
    static_assert(std::is_constructible_v<Chooser, QWidget *, const char *, bool>);
}

void KisSegmentGradientSliderSchemaContractTest::gradientChooserResourceAndPresentationSignaturesRemainStable()
{
    using Chooser = KisGradientChooser;
    static_assert(std::is_same_v<decltype(&Chooser::canvasResourcesInterface),
                                 KoCanvasResourcesInterfaceSP (Chooser::*)() const>);
    static_assert(std::is_same_v<decltype(&Chooser::currentResource), KoResourceSP (Chooser::*)(bool)>);
    static_assert(
        std::is_same_v<decltype(&Chooser::resourceItemChooser), KisResourceItemChooser *(Chooser::*)() const>);
    static_assert(std::is_same_v<decltype(&Chooser::viewMode), Chooser::ViewMode (Chooser::*)() const>);
    static_assert(std::is_same_v<decltype(&Chooser::itemSize), Chooser::ItemSize (Chooser::*)() const>);
    static_assert(std::is_same_v<decltype(&Chooser::itemSizeCustom), int (Chooser::*)() const>);
    static_assert(std::is_same_v<decltype(&Chooser::isNameLabelVisible), bool (Chooser::*)() const>);
    static_assert(std::is_same_v<decltype(&Chooser::areEditOptionsVisible), bool (Chooser::*)() const>);
    static_assert(std::is_same_v<decltype(std::declval<Chooser &>().currentResource()), KoResourceSP>);
}

void KisSegmentGradientSliderSchemaContractTest::gradientChooserConfigurationAndNotificationSignaturesRemainStable()
{
    using Chooser = KisGradientChooser;
    static_assert(std::is_same_v<decltype(&Chooser::setCanvasResourcesInterface),
                                 void (Chooser::*)(KoCanvasResourcesInterfaceSP)>);
    static_assert(std::is_same_v<decltype(&Chooser::setCurrentResource), void (Chooser::*)(KoResourceSP)>);
    static_assert(std::is_same_v<decltype(&Chooser::setCurrentItem), void (Chooser::*)(int)>);
    static_assert(std::is_same_v<decltype(&Chooser::loadViewSettings), void (Chooser::*)(const QString &)>);
    static_assert(std::is_same_v<decltype(&Chooser::saveViewSettings), void (Chooser::*)(const QString &)>);
    static_assert(std::is_same_v<decltype(&Chooser::slotUpdateIcons), void (Chooser::*)()>);
    static_assert(std::is_same_v<decltype(&Chooser::setViewMode), void (Chooser::*)(Chooser::ViewMode)>);
    static_assert(std::is_same_v<decltype(&Chooser::setItemSize), void (Chooser::*)(Chooser::ItemSize)>);
    static_assert(std::is_same_v<decltype(&Chooser::setItemSizeCustom), void (Chooser::*)(int)>);
    static_assert(std::is_same_v<decltype(&Chooser::setNameLabelVisible), void (Chooser::*)(bool)>);
    static_assert(std::is_same_v<decltype(&Chooser::setEditOptionsVisible), void (Chooser::*)(bool)>);
    static_assert(std::is_same_v<decltype(&Chooser::resourceSelected), void (Chooser::*)(KoResourceSP)>);
    static_assert(std::is_same_v<decltype(&Chooser::resourceClicked), void (Chooser::*)(KoResourceSP)>);
    static_assert(std::is_same_v<decltype(&Chooser::gradientEdited), void (Chooser::*)(KoAbstractGradientSP)>);
    static_assert(std::is_same_v<decltype(std::declval<Chooser &>().loadViewSettings()), void>);
    static_assert(std::is_same_v<decltype(std::declval<Chooser &>().saveViewSettings()), void>);
}

void KisSegmentGradientSliderSchemaContractTest::genericGradientEditorTypeAndPresentationSchemaRemainStable()
{
    using Editor = KisGenericGradientEditor;
    static_assert(std::is_class_v<Editor> && std::has_virtual_destructor_v<Editor>);
    static_assert(std::is_base_of_v<QWidget, Editor> && std::is_default_constructible_v<Editor>);
    static_assert(std::is_constructible_v<Editor, QWidget *>);
    static_assert(std::is_same_v<decltype(&Editor::event), bool (Editor::*)(QEvent *)>);
    static_assert(std::is_same_v<decltype(&Editor::sizeHint), QSize (Editor::*)() const>);
    static_assert(std::is_same_v<decltype(&Editor::minimumSizeHint), QSize (Editor::*)() const>);
    static_assert(std::is_same_v<decltype(&Editor::gradient), KoAbstractGradientSP (Editor::*)() const>);
    static_assert(
        std::is_same_v<decltype(&Editor::canvasResourcesInterface), KoCanvasResourcesInterfaceSP (Editor::*)() const>);
    static_assert(std::is_same_v<decltype(&Editor::compactMode), bool (Editor::*)() const>);
    static_assert(std::is_same_v<decltype(&Editor::isConvertGradientButtonVisible), bool (Editor::*)() const>);
    static_assert(std::is_same_v<decltype(&Editor::isUpdateGradientButtonVisible), bool (Editor::*)() const>);
}

void KisSegmentGradientSliderSchemaContractTest::genericGradientEditorConfigurationSignaturesRemainStable()
{
    using Editor = KisGenericGradientEditor;
    using BooleanSetter = void (Editor::*)(bool);
    using BooleanGetter = bool (Editor::*)() const;
    static_assert(std::is_same_v<decltype(&Editor::isAddGradientButtonVisible), BooleanGetter>);
    static_assert(std::is_same_v<decltype(&Editor::isGradientPresetChooserVisible), BooleanGetter>);
    static_assert(std::is_same_v<decltype(&Editor::isGradientPresetChooserOptionsButtonVisible), BooleanGetter>);
    static_assert(std::is_same_v<decltype(&Editor::useGradientPresetChooserPopUp), BooleanGetter>);
    static_assert(std::is_same_v<decltype(&Editor::compactGradientPresetChooserMode), BooleanGetter>);
    static_assert(std::is_same_v<decltype(&Editor::compactGradientEditorMode), BooleanGetter>);
    static_assert(std::is_same_v<decltype(&Editor::loadUISettings), void (Editor::*)(const QString &)>);
    static_assert(std::is_same_v<decltype(&Editor::saveUISettings), void (Editor::*)(const QString &)>);
    static_assert(std::is_same_v<decltype(&Editor::setGradient), void (Editor::*)(KoAbstractGradientSP)>);
    static_assert(
        std::is_same_v<decltype(&Editor::setCanvasResourcesInterface), void (Editor::*)(KoCanvasResourcesInterfaceSP)>);
    static_assert(std::is_same_v<decltype(&Editor::setCompactMode), BooleanSetter>);
    static_assert(std::is_same_v<decltype(&Editor::setConvertGradientButtonVisible), BooleanSetter>);
    static_assert(std::is_same_v<decltype(&Editor::setUpdateGradientButtonVisible), BooleanSetter>);
    static_assert(std::is_same_v<decltype(&Editor::setAddGradientButtonVisible), BooleanSetter>);
    static_assert(std::is_same_v<decltype(&Editor::setGradientPresetChooserVisible), BooleanSetter>);
    static_assert(std::is_same_v<decltype(&Editor::setGradientPresetChooserOptionsButtonVisible), BooleanSetter>);
    static_assert(std::is_same_v<decltype(&Editor::setUseGradientPresetChooserPopUp), BooleanSetter>);
    static_assert(std::is_same_v<decltype(&Editor::setCompactGradientPresetChooserMode), BooleanSetter>);
    static_assert(std::is_same_v<decltype(&Editor::setCompactGradientEditorMode), BooleanSetter>);
    static_assert(std::is_same_v<decltype(&Editor::sigGradientChanged), void (Editor::*)()>);
    static_assert(std::is_same_v<decltype(std::declval<Editor &>().loadUISettings()), void>);
    static_assert(std::is_same_v<decltype(std::declval<Editor &>().saveUISettings()), void>);
}

QTEST_GUILESS_MAIN(KisSegmentGradientSliderSchemaContractTest)
#include "KisSegmentGradientSliderSchemaContractTest.moc"
