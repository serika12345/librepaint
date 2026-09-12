/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisSelectionToolFactoryBase.h"

#include <QTest>

#include <type_traits>

namespace
{

class SelectionToolFactoryProbe : public KisSelectionToolFactoryBase
{
public:
    explicit SelectionToolFactoryProbe(const QString &id)
        : KisSelectionToolFactoryBase(id)
    {
    }

    KoToolBase *createTool(KoCanvasBase *) override
    {
        return nullptr;
    }
};

class PolyLineToolFactoryProbe : public KisToolPolyLineFactoryBase
{
public:
    explicit PolyLineToolFactoryProbe(const QString &id)
        : KisToolPolyLineFactoryBase(id)
    {
    }

    KoToolBase *createTool(KoCanvasBase *) override
    {
        return nullptr;
    }
};

} // namespace

class KisSelectionToolFactoryBaseSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void paintToolFactoryBaseLifetimeSchemaRemainsStable();
    void selectionToolFactoryTypeConstructionAndLifetimeSchemaRemainStable();
    void polyLineToolFactoryTypeConstructionAndLifetimeSchemaRemainStable();
};

void KisSelectionToolFactoryBaseSchemaContractTest::paintToolFactoryBaseLifetimeSchemaRemainsStable()
{
    static_assert(std::has_virtual_destructor_v<KisToolPaintFactoryBase>);
}

void KisSelectionToolFactoryBaseSchemaContractTest::selectionToolFactoryTypeConstructionAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisSelectionToolFactoryBase>);
    static_assert(std::is_base_of_v<KisToolPaintFactoryBase, KisSelectionToolFactoryBase>);
    static_assert(std::is_constructible_v<SelectionToolFactoryProbe, const QString &>);
    static_assert(std::has_virtual_destructor_v<KisSelectionToolFactoryBase>);
}

void KisSelectionToolFactoryBaseSchemaContractTest::polyLineToolFactoryTypeConstructionAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisToolPolyLineFactoryBase>);
    static_assert(std::is_base_of_v<KisToolPaintFactoryBase, KisToolPolyLineFactoryBase>);
    static_assert(std::is_constructible_v<PolyLineToolFactoryProbe, const QString &>);
    static_assert(std::has_virtual_destructor_v<KisToolPolyLineFactoryBase>);
}

QTEST_GUILESS_MAIN(KisSelectionToolFactoryBaseSchemaContractTest)

#include "KisSelectionToolFactoryBaseSchemaContractTest.moc"
