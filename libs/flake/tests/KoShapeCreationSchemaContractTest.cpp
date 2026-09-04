/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoBasicShapeFactory.h>
#include <KoPathShapeFactory.h>
#include <KoShapeRegistry.h>
#include <commands/KoShapeCreateCommand.h>

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
using ShapeList = QList<KoShape *>;

#define ASSERT_SHAPE_REGISTRY_SIGNATURE(method, signature)                                                             \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoShapeRegistry::method)), signature>)
#define ASSERT_PATH_SHAPE_FACTORY_SIGNATURE(method, signature)                                                         \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoPathShapeFactory::method)), signature>)
#define ASSERT_BASIC_SHAPE_FACTORY_SIGNATURE(method, signature)                                                        \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoBasicShapeFactory::method)), signature>)
#define ASSERT_SHAPE_CREATE_COMMAND_SIGNATURE(method, signature)                                                       \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoShapeCreateCommand::method)), signature>)
} // namespace

class KoShapeCreationSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void shapeRegistryTypeLifetimeAndAccessSchemaRemainStable();
    void pathShapeFactoryTypeLifetimeAndConstructionSchemaRemainStable();
    void pathShapeFactoryCreationAndSupportSignaturesRemainStable();
    void basicShapeFactoryCreationSignaturesRemainStable();
    void shapeCreateCommandSchemaRemainStable();
};

void KoShapeCreationSchemaContractTest::shapeRegistryTypeLifetimeAndAccessSchemaRemainStable()
{
    using Registry = KoShapeRegistry;

    static_assert(std::is_class_v<Registry>);
    static_assert(std::is_base_of_v<KoGenericRegistry<KoShapeFactoryBase *>, Registry>);
    static_assert(std::is_default_constructible_v<Registry>);
    static_assert(std::has_virtual_destructor_v<Registry>);
    ASSERT_SHAPE_REGISTRY_SIGNATURE(instance, Registry * (*)());
    ASSERT_SHAPE_REGISTRY_SIGNATURE(addFactory, void (Registry::*)(KoShapeFactoryBase *));
    ASSERT_SHAPE_REGISTRY_SIGNATURE(factoriesForElement,
                                    QList<KoShapeFactoryBase *> (Registry::*)(const QString &, const QString &));
}

void KoShapeCreationSchemaContractTest::pathShapeFactoryTypeLifetimeAndConstructionSchemaRemainStable()
{
    using Factory = KoPathShapeFactory;

    static_assert(std::is_class_v<Factory>);
    static_assert(std::is_base_of_v<KoShapeFactoryBase, Factory>);
    static_assert(std::is_constructible_v<Factory, const QStringList &>);
    static_assert(std::has_virtual_destructor_v<Factory>);
}

void KoShapeCreationSchemaContractTest::pathShapeFactoryCreationAndSupportSignaturesRemainStable()
{
    using Factory = KoPathShapeFactory;

    ASSERT_PATH_SHAPE_FACTORY_SIGNATURE(createDefaultShape, KoShape * (Factory::*)(KoDocumentResourceManager *) const);
    ASSERT_PATH_SHAPE_FACTORY_SIGNATURE(newDocumentResourceManager,
                                        void (Factory::*)(KoDocumentResourceManager *) const);
    ASSERT_PATH_SHAPE_FACTORY_SIGNATURE(supports,
                                        bool (Factory::*)(const QDomElement &, KoShapeLoadingContext &) const);
    static_assert(std::is_same_v<decltype(std::declval<const Factory &>().createDefaultShape()), KoShape *>);
}

void KoShapeCreationSchemaContractTest::basicShapeFactoryCreationSignaturesRemainStable()
{
    using Factory = KoBasicShapeFactory;

    static_assert(std::is_class_v<Factory>);
    ASSERT_BASIC_SHAPE_FACTORY_SIGNATURE(createEllipse, KoShape * (*)(const QRectF &));
    ASSERT_BASIC_SHAPE_FACTORY_SIGNATURE(createRectangle, KoShape * (*)(const QRectF &, qreal, qreal));
}

void KoShapeCreationSchemaContractTest::shapeCreateCommandSchemaRemainStable()
{
    using Command = KoShapeCreateCommand;

    static_assert(std::is_class_v<Command>);
    static_assert(std::is_base_of_v<KUndo2Command, Command>);
    static_assert(std::has_virtual_destructor_v<Command>);
    static_assert(std::is_constructible_v<Command, KoShapeControllerBase *, KoShape *>);
    static_assert(
        std::is_constructible_v<Command, KoShapeControllerBase *, KoShape *, KoShapeContainer *, KUndo2Command *>);
    static_assert(std::is_constructible_v<Command, KoShapeControllerBase *, ShapeList>);
    static_assert(
        std::is_constructible_v<Command, KoShapeControllerBase *, ShapeList, KoShapeContainer *, KUndo2Command *>);
    static_assert(
        std::is_same_v<decltype(Command(std::declval<KoShapeControllerBase *>(), std::declval<KoShape *>())), Command>);
    static_assert(
        std::is_same_v<decltype(Command(std::declval<KoShapeControllerBase *>(), std::declval<ShapeList>())), Command>);
    ASSERT_SHAPE_CREATE_COMMAND_SIGNATURE(redo, void (Command::*)());
    ASSERT_SHAPE_CREATE_COMMAND_SIGNATURE(undo, void (Command::*)());
}

QTEST_APPLESS_MAIN(KoShapeCreationSchemaContractTest)

#include "KoShapeCreationSchemaContractTest.moc"
