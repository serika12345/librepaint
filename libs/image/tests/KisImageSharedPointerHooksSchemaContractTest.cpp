/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_types.h"

#include <QTest>

#include <cstddef>
#include <functional>
#include <type_traits>

class KisImageSharedPointerHooksSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void imageSharedPointerNodeAndLayerHooksRemainStable();
    void imageSharedPointerConfigurationHooksRemainStable();
    void imageSharedPointerSelectionHooksRemainStable();
    void imageSharedPointerQtHashSignaturesRemainStable();
    void imageSharedPointerStandardHashSchemaRemainStable();
};

void KisImageSharedPointerHooksSchemaContractTest::imageSharedPointerNodeAndLayerHooksRemainStable()
{
    static_assert(
        std::is_same_v<decltype(static_cast<void (*)(KisNode *)>(&kisSharedPtrAddReference)), void (*)(KisNode *)>);
    static_assert(
        std::is_same_v<decltype(static_cast<bool (*)(KisNode *)>(&kisSharedPtrRelease)), bool (*)(KisNode *)>);
    static_assert(std::is_same_v<decltype(static_cast<void (*)(KisGroupLayer *)>(&kisSharedPtrAddReference)),
                                 void (*)(KisGroupLayer *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (*)(KisGroupLayer *)>(&kisSharedPtrRelease)),
                                 bool (*)(KisGroupLayer *)>);
    static_assert(std::is_same_v<decltype(static_cast<void (*)(KisFilterMask *)>(&kisSharedPtrAddReference)),
                                 void (*)(KisFilterMask *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (*)(KisFilterMask *)>(&kisSharedPtrRelease)),
                                 bool (*)(KisFilterMask *)>);
}

void KisImageSharedPointerHooksSchemaContractTest::imageSharedPointerConfigurationHooksRemainStable()
{
    static_assert(std::is_same_v<decltype(static_cast<void (*)(KisPaintOpSettings *)>(&kisSharedPtrAddReference)),
                                 void (*)(KisPaintOpSettings *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (*)(KisPaintOpSettings *)>(&kisSharedPtrRelease)),
                                 bool (*)(KisPaintOpSettings *)>);
    static_assert(
        std::is_same_v<decltype(static_cast<void (*)(KisPropertiesConfiguration *)>(&kisSharedPtrAddReference)),
                       void (*)(KisPropertiesConfiguration *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (*)(KisPropertiesConfiguration *)>(&kisSharedPtrRelease)),
                                 bool (*)(KisPropertiesConfiguration *)>);
}

void KisImageSharedPointerHooksSchemaContractTest::imageSharedPointerSelectionHooksRemainStable()
{
    static_assert(std::is_same_v<decltype(static_cast<void (*)(KisSelection *)>(&kisSharedPtrAddReference)),
                                 void (*)(KisSelection *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (*)(KisSelection *)>(&kisSharedPtrRelease)),
                                 bool (*)(KisSelection *)>);
    static_assert(std::is_same_v<decltype(static_cast<void (*)(KisSelectionMask *)>(&kisSharedPtrAddReference)),
                                 void (*)(KisSelectionMask *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (*)(KisSelectionMask *)>(&kisSharedPtrRelease)),
                                 bool (*)(KisSelectionMask *)>);
}

void KisImageSharedPointerHooksSchemaContractTest::imageSharedPointerQtHashSignaturesRemainStable()
{
    using StrongHashSignature = uint (*)(KisSharedPtr<KisNode>);
    using WeakHashSignature = uint (*)(KisWeakSharedPtr<KisNode>);

    static_assert(std::is_same_v<decltype(static_cast<StrongHashSignature>(&qHash<KisNode>)), StrongHashSignature>);
    static_assert(std::is_same_v<decltype(static_cast<WeakHashSignature>(&qHash<KisNode>)), WeakHashSignature>);
}

void KisImageSharedPointerHooksSchemaContractTest::imageSharedPointerStandardHashSchemaRemainStable()
{
    using StrongHasher = std::hash<KisSharedPtr<KisNode>>;
    using WeakHasher = std::hash<KisWeakSharedPtr<KisNode>>;
    using StrongHashSignature = std::size_t (StrongHasher::*)(const KisSharedPtr<KisNode> &) const;
    using WeakHashSignature = std::size_t (WeakHasher::*)(const KisWeakSharedPtr<KisNode> &) const;

    static_assert(std::is_class_v<StrongHasher>);
    static_assert(
        std::is_same_v<decltype(static_cast<StrongHashSignature>(&StrongHasher::operator())), StrongHashSignature>);
    static_assert(std::is_same_v<decltype(static_cast<WeakHashSignature>(&WeakHasher::operator())), WeakHashSignature>);
}

QTEST_APPLESS_MAIN(KisImageSharedPointerHooksSchemaContractTest)

#include "KisImageSharedPointerHooksSchemaContractTest.moc"
