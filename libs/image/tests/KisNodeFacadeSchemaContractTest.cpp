/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_node_facade.h"
#include "kis_node_query_path.h"

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_NODE_FACADE_SIGNATURE(method, signature)                                                                \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisNodeFacade::method)), signature>)
#define ASSERT_NODE_QUERY_PATH_SIGNATURE(method, signature)                                                            \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisNodeQueryPath::method)), signature>)
} // namespace

class KisNodeFacadeSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeConstructionLifetimeAndRootSchemaRemainStable();
    void moveSignaturesRemainStable();
    void addSignaturesRemainStable();
    void removeSignatureRemainsStable();
    void queryPathTypeCopyAndLifetimeSchemaRemainStable();
    void queryPathNodeLookupSignaturesRemainStable();
    void queryPathObservationSignaturesRemainStable();
    void queryPathCreationSignaturesRemainStable();
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

void KisNodeFacadeSchemaContractTest::queryPathTypeCopyAndLifetimeSchemaRemainStable()
{
    using Path = KisNodeQueryPath;

    static_assert(std::is_class_v<Path>);
    static_assert(std::is_copy_constructible_v<Path>);
    static_assert(std::is_destructible_v<Path>);
    ASSERT_NODE_QUERY_PATH_SIGNATURE(operator=, Path & (Path::*)(const Path &));

    QVERIFY(true);
}

void KisNodeFacadeSchemaContractTest::queryPathNodeLookupSignaturesRemainStable()
{
    using Path = KisNodeQueryPath;

    ASSERT_NODE_QUERY_PATH_SIGNATURE(queryNodes, QList<KisNodeSP> (Path::*)(KisImageWSP, KisNodeSP) const);
    ASSERT_NODE_QUERY_PATH_SIGNATURE(queryUniqueNode, KisNodeSP (Path::*)(KisImageWSP, KisNodeSP) const);
}

void KisNodeFacadeSchemaContractTest::queryPathObservationSignaturesRemainStable()
{
    using Path = KisNodeQueryPath;

    ASSERT_NODE_QUERY_PATH_SIGNATURE(isRelative, bool (Path::*)() const);
    ASSERT_NODE_QUERY_PATH_SIGNATURE(toString, QString (Path::*)() const);
}

void KisNodeFacadeSchemaContractTest::queryPathCreationSignaturesRemainStable()
{
    using Path = KisNodeQueryPath;

    ASSERT_NODE_QUERY_PATH_SIGNATURE(fromString, Path (*)(const QString &));
    ASSERT_NODE_QUERY_PATH_SIGNATURE(absolutePath, Path (*)(KisNodeSP));
}

#undef ASSERT_NODE_QUERY_PATH_SIGNATURE
#undef ASSERT_NODE_FACADE_SIGNATURE

QTEST_APPLESS_MAIN(KisNodeFacadeSchemaContractTest)

#include "KisNodeFacadeSchemaContractTest.moc"
