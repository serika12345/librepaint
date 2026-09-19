/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <tool/kis_selection_tool_helper.h>
#include <tool/kis_tool_ellipse_base.h>
#include <tool/kis_tool_freehand.h>
#include <tool/kis_tool_freehand_helper.h>
#include <tool/kis_tool_select_ui_base.h>
#include <tool/kis_tool_shape.h>

#include <QTest>


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

class ToolShapeProbe final : public KisToolShape
{
public:
    using KisToolShape::KisToolShape;

    void paint(QPainter &, const KoViewConverter &) override;
};

class EllipseToolProbe final : public KisToolEllipseBase
{
public:
    using KisToolEllipseBase::KisToolEllipseBase;

protected:
    void finishRect(const QRectF &, qreal, qreal) override;
};

} // namespace

class KisToolSelectUiBaseSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void toolSelectUiSampleLayersModeSchemaRemainsStable();
};

void KisToolSelectUiBaseSchemaContractTest::toolSelectUiSampleLayersModeSchemaRemainsStable()
{


    QCOMPARE(static_cast<int>(Subject::SampleAllLayers), 0);
    QCOMPARE(static_cast<int>(Subject::SampleCurrentLayer), 1);
    QCOMPARE(static_cast<int>(Subject::SampleColorLabeledLayers), 2);
}

QTEST_APPLESS_MAIN(KisToolSelectUiBaseSchemaContractTest)

#include "KisToolSelectUiBaseSchemaContractTest.moc"
