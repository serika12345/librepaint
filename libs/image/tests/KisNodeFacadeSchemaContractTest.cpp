/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_node_facade.h"

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_NODE_FACADE_SIGNATURE(method, signature)                                                                \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisNodeFacade::method)), signature>)
} // namespace

class KisNodeFacadeSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeConstructionLifetimeAndRootSchemaRemainStable();
    void moveSignaturesRemainStable();
    void addSignaturesRemainStable();
    void removeSignatureRemainsStable();
};

void KisNodeFacadeSchemaContractTest::typeConstructionLifetimeAndRootSchemaRemainStable()
{
    using Facade = KisNodeFacade;

    static_assert(std::is_class_v<Facade>);
    static_assert(std::is_default_constructible_v<Facade>);
    static_assert(std::is_constructible_v<Facade, KisNodeSP>);
    static_assert(std::has_virtual_destructor_v<Facade>);
    ASSERT_NODE_FACADE_SIGNATURE(setRoot, void (Facade::*)(KisNodeSP));
    ASSERT_NODE_FACADE_SIGNATURE(root, const KisNodeSP (Facade::*)() const);

    QVERIFY(true);
}

void KisNodeFacadeSchemaContractTest::moveSignaturesRemainStable()
{
    using Facade = KisNodeFacade;

    ASSERT_NODE_FACADE_SIGNATURE(moveNode, bool (Facade::*)(KisNodeSP, KisNodeSP, KisNodeSP));
    ASSERT_NODE_FACADE_SIGNATURE(moveNode, bool (Facade::*)(KisNodeSP, KisNodeSP, quint32));
}

void KisNodeFacadeSchemaContractTest::addSignaturesRemainStable()
{
    using Facade = KisNodeFacade;

    ASSERT_NODE_FACADE_SIGNATURE(addNode, bool (Facade::*)(KisNodeSP, KisNodeSP, KisNodeAdditionFlags));
    ASSERT_NODE_FACADE_SIGNATURE(addNode, bool (Facade::*)(KisNodeSP, KisNodeSP, KisNodeSP, KisNodeAdditionFlags));
    ASSERT_NODE_FACADE_SIGNATURE(addNode, bool (Facade::*)(KisNodeSP, KisNodeSP, quint32, KisNodeAdditionFlags));
}

void KisNodeFacadeSchemaContractTest::removeSignatureRemainsStable()
{
    using Facade = KisNodeFacade;

    ASSERT_NODE_FACADE_SIGNATURE(removeNode, bool (Facade::*)(KisNodeSP));
}

#undef ASSERT_NODE_FACADE_SIGNATURE

QTEST_APPLESS_MAIN(KisNodeFacadeSchemaContractTest)

#include "KisNodeFacadeSchemaContractTest.moc"
