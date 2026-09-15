/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <toolbox/KoToolBoxFactory.h>

#include <QTest>

#include <type_traits>

class KoToolBoxFactorySchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void toolBoxFactorySchemaRemainStable();
};

void KoToolBoxFactorySchemaContractTest::toolBoxFactorySchemaRemainStable()
{
    using Factory = KoToolBoxFactory;

    static_assert(std::is_class_v<Factory>);
    static_assert(std::is_base_of_v<KoDockFactoryBase, Factory>);
    static_assert(std::is_default_constructible_v<Factory>);
    static_assert(std::has_virtual_destructor_v<Factory>);
    static_assert(std::is_same_v<decltype(&Factory::id), QString (Factory::*)() const>);
    static_assert(
        std::is_same_v<decltype(&Factory::defaultDockPosition), KoDockFactoryBase::DockPosition (Factory::*)() const>);
    static_assert(std::is_same_v<decltype(&Factory::createDockWidget), QDockWidget *(Factory::*)()>);
}

QTEST_APPLESS_MAIN(KoToolBoxFactorySchemaContractTest)

#include "KoToolBoxFactorySchemaContractTest.moc"
