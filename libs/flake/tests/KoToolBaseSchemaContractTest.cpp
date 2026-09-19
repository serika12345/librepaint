/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoInteractionStrategy.h>
#include <KoInteractionTool.h>
#include <KoShapeRubberSelectStrategy.h>
#include <KoToolBase.h>
#include <KoToolFactoryBase.h>
#include <KoToolRegistry.h>

#include <QTest>


namespace
{

class ToolConstructorProbe : public KoToolBase
{
public:
    explicit ToolConstructorProbe(KoCanvasBase *canvas)
        : KoToolBase(canvas)
    {
    }
};

class ToolFactoryConstructorProbe final : public KoToolFactoryBase
{
public:
    using KoToolFactoryBase::KoToolFactoryBase;

    KoToolBase *createTool(KoCanvasBase *canvas) override;
};

class InteractionStrategyConstructorProbe final : public KoInteractionStrategy
{
public:
    using KoInteractionStrategy::KoInteractionStrategy;

    void handleMouseMove(const QPointF &mouseLocation, Qt::KeyboardModifiers modifiers) override;
    KUndo2Command *createCommand() override;
    void finishInteraction(Qt::KeyboardModifiers modifiers) override;
};

class InteractionToolConstructorProbe final : public KoInteractionTool
{
public:
    using KoInteractionTool::KoInteractionTool;

    KoInteractionStrategy *createStrategy(KoPointerEvent *event) override;
};

class RubberSelectStrategyConstructorProbe final : public KoShapeRubberSelectStrategy
{
public:
    using KoShapeRubberSelectStrategy::KoShapeRubberSelectStrategy;

    void finishInteraction(Qt::KeyboardModifiers modifiers) override;
};
} // namespace

class KoToolBaseSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void toolFactoryTypeAndSectionValuesRemainStable();
};

void KoToolBaseSchemaContractTest::toolFactoryTypeAndSectionValuesRemainStable()
{

    QCOMPARE(ToolBoxSection::Main, QStringLiteral("main"));
    QCOMPARE(ToolBoxSection::Shape, QStringLiteral("0 Krita/Shape"));
    QCOMPARE(ToolBoxSection::Transform, QStringLiteral("2 Krita/Transform"));
    QCOMPARE(ToolBoxSection::Fill, QStringLiteral("3 Krita/Fill"));
    QCOMPARE(ToolBoxSection::View, QStringLiteral("4 Krita/View"));
    QCOMPARE(ToolBoxSection::Select, QStringLiteral("5 Krita/Select"));
    QCOMPARE(ToolBoxSection::Navigation, QStringLiteral("navigation"));
}

QTEST_APPLESS_MAIN(KoToolBaseSchemaContractTest)

#include "KoToolBaseSchemaContractTest.moc"
