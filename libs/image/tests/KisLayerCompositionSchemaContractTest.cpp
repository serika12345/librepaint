/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_layer_composition.h"

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_LAYER_COMPOSITION_SIGNATURE(method, signature)                                                          \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisLayerComposition::method)), signature>)
} // namespace

class KisLayerCompositionSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeConstructionAndLifetimeSchemaRemainStable();
    void identityAndExportStateSignaturesRemainStable();
    void storedNodeStateSignaturesRemainStable();
    void applicationAndSerializationSignaturesRemainStable();
};

void KisLayerCompositionSchemaContractTest::typeConstructionAndLifetimeSchemaRemainStable()
{
    using Composition = KisLayerComposition;

    static_assert(std::is_class_v<Composition>);
    static_assert(std::is_constructible_v<Composition, KisImageWSP, const QString &>);
    static_assert(std::is_constructible_v<Composition, const Composition &, KisImageWSP>);
    static_assert(std::is_destructible_v<Composition>);

    QVERIFY(true);
}

void KisLayerCompositionSchemaContractTest::identityAndExportStateSignaturesRemainStable()
{
    using Composition = KisLayerComposition;

    ASSERT_LAYER_COMPOSITION_SIGNATURE(setName, void (Composition::*)(const QString &));
    ASSERT_LAYER_COMPOSITION_SIGNATURE(name, QString (Composition::*)());
    ASSERT_LAYER_COMPOSITION_SIGNATURE(setExportEnabled, void (Composition::*)(bool));
    ASSERT_LAYER_COMPOSITION_SIGNATURE(isExportEnabled, bool (Composition::*)());
}

void KisLayerCompositionSchemaContractTest::storedNodeStateSignaturesRemainStable()
{
    using Composition = KisLayerComposition;

    ASSERT_LAYER_COMPOSITION_SIGNATURE(setCollapsed, void (Composition::*)(QUuid, bool));
    ASSERT_LAYER_COMPOSITION_SIGNATURE(setVisible, void (Composition::*)(QUuid, bool));
    ASSERT_LAYER_COMPOSITION_SIGNATURE(store, void (Composition::*)());
}

void KisLayerCompositionSchemaContractTest::applicationAndSerializationSignaturesRemainStable()
{
    using Composition = KisLayerComposition;

    ASSERT_LAYER_COMPOSITION_SIGNATURE(apply, void (Composition::*)());
    ASSERT_LAYER_COMPOSITION_SIGNATURE(save, void (Composition::*)(QDomDocument &, QDomElement &));
}

#undef ASSERT_LAYER_COMPOSITION_SIGNATURE

QTEST_GUILESS_MAIN(KisLayerCompositionSchemaContractTest)

#include "KisLayerCompositionSchemaContractTest.moc"
