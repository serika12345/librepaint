/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

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

#undef ASSERT_TOOL_SELECT_UI_SIGNATURE

QTEST_APPLESS_MAIN(KisToolSelectUiBaseSchemaContractTest)

#include "KisToolSelectUiBaseSchemaContractTest.moc"
