/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoParameterShape.h>
#include <commands/KoParameterToPathCommand.h>
#include <tools/KoParameterChangeStrategy.h>

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_PARAMETER_SHAPE_SIGNATURE(method, signature)                                                            \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoParameterShape::method)), signature>)
#define ASSERT_PARAMETER_STRATEGY_SIGNATURE(method, signature)                                                         \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoParameterChangeStrategy::method)), signature>)
#define ASSERT_PARAMETER_COMMAND_SIGNATURE(method, signature)                                                          \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoParameterToPathCommand::method)), signature>)

class ParameterShapeConstructorProbe final : public KoParameterShape
{
public:
    using KoParameterShape::KoParameterShape;

private:
    void moveHandleAction(int handleId, const QPointF &point, Qt::KeyboardModifiers modifiers) override;
    void updatePath(const QSizeF &size) override;
};
} // namespace

class KoParameterShapeSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parameterShapeTypeAndLifetimeSchemaRemainStable();
    void parameterShapeHandleAndStateSignaturesRemainStable();
    void parameterChangeStrategySchemaRemainStable();
    void parameterToPathConstructionAndLifetimeSchemaRemainStable();
    void parameterToPathExecutionSignaturesRemainStable();
};

void KoParameterShapeSchemaContractTest::parameterShapeTypeAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KoParameterShape>);
    static_assert(std::is_abstract_v<KoParameterShape>);
    static_assert(std::is_base_of_v<KoPathShape, KoParameterShape>);
    static_assert(std::has_virtual_destructor_v<KoParameterShape>);
    static_assert(std::is_default_constructible_v<ParameterShapeConstructorProbe>);
    static_assert(std::is_same_v<decltype(ParameterShapeConstructorProbe()), ParameterShapeConstructorProbe>);
}

void KoParameterShapeSchemaContractTest::parameterShapeHandleAndStateSignaturesRemainStable()
{
    ASSERT_PARAMETER_SHAPE_SIGNATURE(handleCount, int (KoParameterShape::*)() const);
    ASSERT_PARAMETER_SHAPE_SIGNATURE(handleIdAt, int (KoParameterShape::*)(const QRectF &) const);
    ASSERT_PARAMETER_SHAPE_SIGNATURE(handlePosition, QPointF (KoParameterShape::*)(int) const);
    ASSERT_PARAMETER_SHAPE_SIGNATURE(isParametricShape, bool (KoParameterShape::*)() const);
    ASSERT_PARAMETER_SHAPE_SIGNATURE(moveHandle,
                                     void (KoParameterShape::*)(int, const QPointF &, Qt::KeyboardModifiers));
    ASSERT_PARAMETER_SHAPE_SIGNATURE(normalize, QPointF (KoParameterShape::*)());
    ASSERT_PARAMETER_SHAPE_SIGNATURE(paintHandle, void (KoParameterShape::*)(KisHandlePainterHelper &, int));
    ASSERT_PARAMETER_SHAPE_SIGNATURE(paintHandles, void (KoParameterShape::*)(KisHandlePainterHelper &));
    ASSERT_PARAMETER_SHAPE_SIGNATURE(setParametricShape, void (KoParameterShape::*)(bool));
    ASSERT_PARAMETER_SHAPE_SIGNATURE(setSize, void (KoParameterShape::*)(const QSizeF &));
    static_assert(
        std::is_same_v<decltype(std::declval<KoParameterShape &>().moveHandle(std::declval<int>(),
                                                                              std::declval<const QPointF &>())),
                       void>);
}

void KoParameterShapeSchemaContractTest::parameterChangeStrategySchemaRemainStable()
{
    static_assert(std::is_class_v<KoParameterChangeStrategy>);
    static_assert(std::is_base_of_v<KoInteractionStrategy, KoParameterChangeStrategy>);
    static_assert(std::has_virtual_destructor_v<KoParameterChangeStrategy>);
    static_assert(std::is_constructible_v<KoParameterChangeStrategy, KoToolBase *, KoParameterShape *, int>);
    ASSERT_PARAMETER_STRATEGY_SIGNATURE(createCommand, KUndo2Command * (KoParameterChangeStrategy::*)());
    ASSERT_PARAMETER_STRATEGY_SIGNATURE(finishInteraction, void (KoParameterChangeStrategy::*)(Qt::KeyboardModifiers));
    ASSERT_PARAMETER_STRATEGY_SIGNATURE(handleMouseMove,
                                        void (KoParameterChangeStrategy::*)(const QPointF &, Qt::KeyboardModifiers));
}

void KoParameterShapeSchemaContractTest::parameterToPathConstructionAndLifetimeSchemaRemainStable()
{
    using ParameterShapes = QList<KoParameterShape *>;

    static_assert(std::is_class_v<KoParameterToPathCommand>);
    static_assert(std::is_base_of_v<KUndo2Command, KoParameterToPathCommand>);
    static_assert(std::has_virtual_destructor_v<KoParameterToPathCommand>);
    static_assert(std::is_constructible_v<KoParameterToPathCommand, KoParameterShape *>);
    static_assert(std::is_constructible_v<KoParameterToPathCommand, KoParameterShape *, KUndo2Command *>);
    static_assert(std::is_constructible_v<KoParameterToPathCommand, const ParameterShapes &>);
    static_assert(std::is_constructible_v<KoParameterToPathCommand, const ParameterShapes &, KUndo2Command *>);
    static_assert(std::is_same_v<decltype(KoParameterToPathCommand(std::declval<KoParameterShape *>())),
                                 KoParameterToPathCommand>);
    static_assert(std::is_same_v<decltype(KoParameterToPathCommand(std::declval<const ParameterShapes &>())),
                                 KoParameterToPathCommand>);
}

void KoParameterShapeSchemaContractTest::parameterToPathExecutionSignaturesRemainStable()
{
    ASSERT_PARAMETER_COMMAND_SIGNATURE(redo, void (KoParameterToPathCommand::*)());
    ASSERT_PARAMETER_COMMAND_SIGNATURE(undo, void (KoParameterToPathCommand::*)());
}

QTEST_APPLESS_MAIN(KoParameterShapeSchemaContractTest)

#include "KoParameterShapeSchemaContractTest.moc"
