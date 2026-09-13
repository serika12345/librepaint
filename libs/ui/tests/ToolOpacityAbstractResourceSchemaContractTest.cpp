/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "resources/kis_abstract_resources.h"

#include <QTest>
#include <QVariant>

#include <type_traits>

class ToolOpacityAbstractResourceSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void resourceTypeConstructionAndValueSchemaRemainStable();
};

void ToolOpacityAbstractResourceSchemaContractTest::resourceTypeConstructionAndValueSchemaRemainStable()
{
    using Resource = ToolOpacityAbstractResource;
    using Value = QVariant (Resource::*)() const;
    using SetValue = void (Resource::*)(QVariant);

    static_assert(std::is_base_of_v<KoAbstractCanvasResourceInterface, Resource>);
    static_assert(std::is_constructible_v<Resource, int, qreal>);
    static_assert(std::has_virtual_destructor_v<Resource>);
    static_assert(std::is_same_v<decltype(&Resource::value), Value>);
    static_assert(std::is_same_v<decltype(&Resource::setValue), SetValue>);
}

QTEST_GUILESS_MAIN(ToolOpacityAbstractResourceSchemaContractTest)

#include "ToolOpacityAbstractResourceSchemaContractTest.moc"
