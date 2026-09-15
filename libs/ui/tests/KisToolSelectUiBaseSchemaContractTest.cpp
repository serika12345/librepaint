/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <tool/kis_selection_tool_helper.h>
#include <tool/kis_tool_freehand.h>
#include <tool/kis_tool_freehand_helper.h>
#include <tool/kis_tool_select_ui_base.h>

#include <QTest>

#include <type_traits>

namespace
{
using Subject = KisToolSelectUiBase<FakeBaseTool>;

class FakeBaseToolProbe final : public FakeBaseTool
{
public:
    using FakeBaseTool::FakeBaseTool;

    void paint(QPainter &, const KoViewConverter &) override;
};

class SubjectProbe final : public Subject
{
public:
    using Subject::Subject;

    void paint(QPainter &, const KoViewConverter &) override;
};

#define ASSERT_TOOL_SELECT_UI_SIGNATURE(method, signature)                                                             \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&Subject::method)), signature>)
#define ASSERT_SELECTION_HELPER_SIGNATURE(method, ...)                                                                 \
    static_assert(std::is_same_v<decltype(static_cast<__VA_ARGS__>(&KisSelectionToolHelper::method)), __VA_ARGS__>)
#define ASSERT_FREEHAND_HELPER_SIGNATURE(method, ...)                                                                  \
    static_assert(std::is_same_v<decltype(static_cast<__VA_ARGS__>(&KisToolFreehandHelper::method)), __VA_ARGS__>)
} // namespace

class KisToolSelectUiBaseSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void toolSelectUiTypeAliasAndConstructionSchemaRemainStable();
    void toolSelectUiSampleLayersModeSchemaRemainsStable();
    void toolSelectUiActivationAndPresentationSignaturesRemainStable();
    void toolSelectUiSelectionConfigurationSignaturesRemainStable();
    void toolSelectUiAdjustmentAndOptionSignaturesRemainStable();
    void selectionHelperTypeConstructionAndLifetimeSchemaRemainStable();
    void selectionHelperPixelAndShapeApplicationSignaturesRemainStable();
    void selectionHelperShortcutMenuAndModeSignaturesRemainStable();
    void freehandHelperTypeSmoothingAndRunningSchemaRemainStable();
    void freehandHelperPaintLifecycleAndOutlineSignaturesRemainStable();
    void freehandToolPublicSchemaRemainsStable();
};

void KisToolSelectUiBaseSchemaContractTest::toolSelectUiTypeAliasAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<Subject>);
    static_assert(std::is_same_v<KisToolSelect, Subject>);
    static_assert(std::is_base_of_v<KisToolSelectBase<FakeBaseTool>, Subject>);
    static_assert(std::is_class_v<FakeBaseTool>);
    static_assert(std::is_base_of_v<KisTool, FakeBaseTool>);
    static_assert(std::is_constructible_v<FakeBaseToolProbe, KoCanvasBase *>);
    static_assert(std::is_constructible_v<FakeBaseToolProbe, KoCanvasBase *, const QCursor &>);
    static_assert(std::is_constructible_v<SubjectProbe, KoCanvasBase *, const QString &>);
    static_assert(std::is_constructible_v<SubjectProbe, KoCanvasBase *, const QCursor &, const QString &>);
    static_assert(
        std::is_constructible_v<SubjectProbe, KoCanvasBase *, const QCursor &, const QString &, KoToolBase *>);

    QVERIFY(true);
}

void KisToolSelectUiBaseSchemaContractTest::toolSelectUiSampleLayersModeSchemaRemainsStable()
{
    using Mode = Subject::SampleLayersMode;

    static_assert(std::is_enum_v<Mode>);
    static_assert(Subject::SampleAllLayers != Subject::SampleCurrentLayer);
    static_assert(Subject::SampleCurrentLayer != Subject::SampleColorLabeledLayers);
    ASSERT_TOOL_SELECT_UI_SIGNATURE(sampleLayersMode, Mode (Subject::*)() const);

    QCOMPARE(static_cast<int>(Subject::SampleAllLayers), 0);
    QCOMPARE(static_cast<int>(Subject::SampleCurrentLayer), 1);
    QCOMPARE(static_cast<int>(Subject::SampleColorLabeledLayers), 2);
}

void KisToolSelectUiBaseSchemaContractTest::toolSelectUiActivationAndPresentationSignaturesRemainStable()
{
    ASSERT_TOOL_SELECT_UI_SIGNATURE(updateActionShortcutToolTips, void (Subject::*)());
    ASSERT_TOOL_SELECT_UI_SIGNATURE(activate, void (Subject::*)(const QSet<KoShape *> &));
    ASSERT_TOOL_SELECT_UI_SIGNATURE(deactivate, void (Subject::*)());
    ASSERT_TOOL_SELECT_UI_SIGNATURE(createOptionWidget, QWidget * (Subject::*)());
    ASSERT_TOOL_SELECT_UI_SIGNATURE(popupActionsMenu, QMenu * (Subject::*)());

    QVERIFY(true);
}

void KisToolSelectUiBaseSchemaContractTest::toolSelectUiSelectionConfigurationSignaturesRemainStable()
{
    ASSERT_TOOL_SELECT_UI_SIGNATURE(selectionMode, SelectionMode (Subject::*)() const);
    ASSERT_TOOL_SELECT_UI_SIGNATURE(antiAliasSelection, bool (Subject::*)() const);
    ASSERT_TOOL_SELECT_UI_SIGNATURE(growSelection, int (Subject::*)() const);
    ASSERT_TOOL_SELECT_UI_SIGNATURE(stopGrowingAtDarkestPixel, bool (Subject::*)() const);

    QVERIFY(true);
}

void KisToolSelectUiBaseSchemaContractTest::toolSelectUiAdjustmentAndOptionSignaturesRemainStable()
{
    ASSERT_TOOL_SELECT_UI_SIGNATURE(featherSelection, int (Subject::*)() const);
    ASSERT_TOOL_SELECT_UI_SIGNATURE(colorLabelsSelected, QList<int> (Subject::*)() const);
    ASSERT_TOOL_SELECT_UI_SIGNATURE(selectionOptionWidget, KisSelectionOptions * (Subject::*)());

    QVERIFY(true);
}

void KisToolSelectUiBaseSchemaContractTest::selectionHelperTypeConstructionAndLifetimeSchemaRemainStable()
{
    using Helper = KisSelectionToolHelper;

    static_assert(std::is_class_v<Helper>);
    static_assert(std::is_constructible_v<Helper, KisCanvas2 *, const KUndo2MagicString &>);
    static_assert(std::has_virtual_destructor_v<Helper>);

    QVERIFY(true);
}

void KisToolSelectUiBaseSchemaContractTest::selectionHelperPixelAndShapeApplicationSignaturesRemainStable()
{
    using Helper = KisSelectionToolHelper;

    ASSERT_SELECTION_HELPER_SIGNATURE(
        selectPixelSelection,
        void (Helper::*)(KisProcessingApplicator &, KisPixelSelectionSP, SelectionAction));
    ASSERT_SELECTION_HELPER_SIGNATURE(selectPixelSelection, void (Helper::*)(KisPixelSelectionSP, SelectionAction));
    ASSERT_SELECTION_HELPER_SIGNATURE(addSelectionShape, void (Helper::*)(KoShape *, SelectionAction));
    ASSERT_SELECTION_HELPER_SIGNATURE(addSelectionShapes, void (Helper::*)(QList<KoShape *>, SelectionAction));
}

void KisToolSelectUiBaseSchemaContractTest::selectionHelperShortcutMenuAndModeSignaturesRemainStable()
{
    using Helper = KisSelectionToolHelper;

    ASSERT_SELECTION_HELPER_SIGNATURE(canShortcutToDeselect, bool (Helper::*)(const QRect &, SelectionAction));
    ASSERT_SELECTION_HELPER_SIGNATURE(canShortcutToNoop, bool (Helper::*)(const QRect &, SelectionAction));
    ASSERT_SELECTION_HELPER_SIGNATURE(tryDeselectCurrentSelection, bool (Helper::*)(QRectF, SelectionAction));
    ASSERT_SELECTION_HELPER_SIGNATURE(getSelectionContextMenu, QMenu * (*)(KoCanvasBase *));
    ASSERT_SELECTION_HELPER_SIGNATURE(tryOverrideSelectionMode,
                                      SelectionMode (Helper::*)(KisSelectionSP, SelectionMode, SelectionAction) const);
}

void KisToolSelectUiBaseSchemaContractTest::freehandHelperTypeSmoothingAndRunningSchemaRemainStable()
{
    using Helper = KisToolFreehandHelper;

    static_assert(std::is_class_v<Helper>);
    static_assert(std::is_constructible_v<Helper, KisPaintingInformationBuilder *, KoCanvasResourceProvider *>);
    static_assert(std::is_constructible_v<Helper,
                                          KisPaintingInformationBuilder *,
                                          KoCanvasResourceProvider *,
                                          const KUndo2MagicString &,
                                          KisSmoothingOptions *>);
    static_assert(std::has_virtual_destructor_v<Helper>);
    ASSERT_FREEHAND_HELPER_SIGNATURE(setSmoothness, void (Helper::*)(KisSmoothingOptionsSP));
    ASSERT_FREEHAND_HELPER_SIGNATURE(smoothingOptions, KisSmoothingOptionsSP (Helper::*)() const);
    ASSERT_FREEHAND_HELPER_SIGNATURE(isRunning, bool (Helper::*)() const);
}

void KisToolSelectUiBaseSchemaContractTest::freehandHelperPaintLifecycleAndOutlineSignaturesRemainStable()
{
    using Helper = KisToolFreehandHelper;

    ASSERT_FREEHAND_HELPER_SIGNATURE(cursorMoved, void (Helper::*)(const QPointF &));
    ASSERT_FREEHAND_HELPER_SIGNATURE(initPaint,
                                     void (Helper::*)(KoPointerEvent *,
                                                      const QPointF &,
                                                      KisImageWSP,
                                                      KisNodeSP,
                                                      KisStrokesFacade *,
                                                      KisNodeSP,
                                                      KisDefaultBoundsBaseSP));
    ASSERT_FREEHAND_HELPER_SIGNATURE(paintEvent, void (Helper::*)(KoPointerEvent *));
    ASSERT_FREEHAND_HELPER_SIGNATURE(endPaint, void (Helper::*)());
    ASSERT_FREEHAND_HELPER_SIGNATURE(paintOpOutline,
                                     KisOptimizedBrushOutline (Helper::*)(const QPointF &,
                                                                          const KoPointerEvent *,
                                                                          KisPaintOpSettingsSP,
                                                                          KisPaintOpSettings::OutlineMode) const);
    ASSERT_FREEHAND_HELPER_SIGNATURE(requestExplicitUpdateOutline, void (Helper::*)());
}

void KisToolSelectUiBaseSchemaContractTest::freehandToolPublicSchemaRemainsStable()
{
    using Tool = KisToolFreehand;

    static_assert(std::is_base_of_v<KisToolPaint, Tool>);
    static_assert(std::is_constructible_v<Tool, KoCanvasBase *, const QCursor &, const KUndo2MagicString &, bool>);
    static_assert(std::has_virtual_destructor_v<Tool>);
    static_assert(std::is_same_v<decltype(&Tool::flags), int (Tool::*)() const>);
    static_assert(std::is_same_v<decltype(&Tool::mouseMoveEvent), void (Tool::*)(KoPointerEvent *)>);
    static_assert(std::is_same_v<decltype(&Tool::activate), void (Tool::*)(const QSet<KoShape *> &)>);
    static_assert(std::is_same_v<decltype(&Tool::deactivate), void (Tool::*)()>);

    QVERIFY(true);
}

#undef ASSERT_FREEHAND_HELPER_SIGNATURE
#undef ASSERT_SELECTION_HELPER_SIGNATURE
#undef ASSERT_TOOL_SELECT_UI_SIGNATURE

QTEST_APPLESS_MAIN(KisToolSelectUiBaseSchemaContractTest)

#include "KisToolSelectUiBaseSchemaContractTest.moc"
