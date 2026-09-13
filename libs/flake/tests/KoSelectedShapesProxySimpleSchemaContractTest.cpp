/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoSelectedShapesProxySimple.h"

#include <QTest>

#include <type_traits>

class KoSelectedShapesProxySimpleSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void schemaRemainsStable();
};

void KoSelectedShapesProxySimpleSchemaContractTest::schemaRemainsStable()
{
    using Proxy = KoSelectedShapesProxySimple;
    using SelectionSignature = KoSelection *(Proxy::*)();

    static_assert(std::is_class_v<Proxy>);
    static_assert(std::is_base_of_v<KoSelectedShapesProxy, Proxy>);
    static_assert(std::is_constructible_v<Proxy, KoShapeManager *>);
    static_assert(std::is_same_v<decltype(&Proxy::selection), SelectionSignature>);

    QVERIFY(true);
}

QTEST_GUILESS_MAIN(KoSelectedShapesProxySimpleSchemaContractTest)

#include "KoSelectedShapesProxySimpleSchemaContractTest.moc"
