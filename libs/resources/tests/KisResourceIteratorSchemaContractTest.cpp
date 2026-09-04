/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisResourceIterator.h"

#include <QTest>

#include <type_traits>

class KisResourceIteratorSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void resourceIteratorAndItemTypeLifetimeSchemaRemainStable();
    void resourceItemIdentityAndMetadataSignaturesRemainStable();
    void resourceItemPayloadSignaturesRemainStable();
    void resourceIteratorForwardTraversalSignaturesRemainStable();
    void resourceIteratorBackwardTraversalSignaturesRemainStable();
};

void KisResourceIteratorSchemaContractTest::resourceIteratorAndItemTypeLifetimeSchemaRemainStable()
{
    static_assert(std::is_same_v<KisResourceItemSP, QSharedPointer<KisResourceItem>>);
    static_assert(std::is_class_v<KisResourceItem>);
    static_assert(std::is_class_v<KisResourceIterator>);
    static_assert(std::is_constructible_v<KisResourceIterator, KisResourceModel *>);
    static_assert(std::is_destructible_v<KisResourceIterator>);
}

void KisResourceIteratorSchemaContractTest::resourceItemIdentityAndMetadataSignaturesRemainStable()
{
    using IdSignature = int (KisResourceItem::*)();
    using TextSignature = QString (KisResourceItem::*)();

    static_assert(std::is_same_v<decltype(&KisResourceItem::filename), TextSignature>);
    static_assert(std::is_same_v<decltype(&KisResourceItem::id), IdSignature>);
    static_assert(std::is_same_v<decltype(&KisResourceItem::md5sum), TextSignature>);
    static_assert(std::is_same_v<decltype(&KisResourceItem::name), TextSignature>);
    static_assert(std::is_same_v<decltype(&KisResourceItem::resourceType), TextSignature>);
    static_assert(std::is_same_v<decltype(&KisResourceItem::tooltip), TextSignature>);
}

void KisResourceIteratorSchemaContractTest::resourceItemPayloadSignaturesRemainStable()
{
    static_assert(std::is_same_v<decltype(&KisResourceItem::resource), KoResourceSP (KisResourceItem::*)()>);
    static_assert(std::is_same_v<decltype(&KisResourceItem::thumbnail), QImage (KisResourceItem::*)()>);
}

void KisResourceIteratorSchemaContractTest::resourceIteratorForwardTraversalSignaturesRemainStable()
{
    using ItemSignature = const KisResourceItemSP (KisResourceIterator::*)();
    using PeekSignature = const KisResourceItemSP (KisResourceIterator::*)() const;

    static_assert(std::is_same_v<decltype(&KisResourceIterator::hasNext), bool (KisResourceIterator::*)() const>);
    static_assert(std::is_same_v<decltype(&KisResourceIterator::next), ItemSignature>);
    static_assert(std::is_same_v<decltype(&KisResourceIterator::peekNext), PeekSignature>);
    static_assert(std::is_same_v<decltype(&KisResourceIterator::toFront), void (KisResourceIterator::*)()>);
}

void KisResourceIteratorSchemaContractTest::resourceIteratorBackwardTraversalSignaturesRemainStable()
{
    using ItemSignature = const KisResourceItemSP (KisResourceIterator::*)();
    using PeekSignature = const KisResourceItemSP (KisResourceIterator::*)() const;

    static_assert(std::is_same_v<decltype(&KisResourceIterator::hasPrevious), bool (KisResourceIterator::*)() const>);
    static_assert(std::is_same_v<decltype(&KisResourceIterator::peekPrevious), PeekSignature>);
    static_assert(std::is_same_v<decltype(&KisResourceIterator::previous), ItemSignature>);
    static_assert(std::is_same_v<decltype(&KisResourceIterator::toBack), void (KisResourceIterator::*)()>);
}

QTEST_GUILESS_MAIN(KisResourceIteratorSchemaContractTest)

#include "KisResourceIteratorSchemaContractTest.moc"
