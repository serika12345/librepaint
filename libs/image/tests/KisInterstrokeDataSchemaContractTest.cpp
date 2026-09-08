/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisInterstrokeData.h"

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_INTERSTROKE_DATA_SIGNATURE(method, signature)                                                           \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisInterstrokeData::method)), signature>)

class InterstrokeDataProbe final : public KisInterstrokeData
{
public:
    using KisInterstrokeData::KisInterstrokeData;

    void beginTransaction() override;
    KUndo2Command *endTransaction() override;
};
} // namespace

class KisInterstrokeDataSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeConstructionAndLifetimeSchemaRemainStable();
    void transactionSignaturesRemainStable();
    void compatibilitySignatureRemainsStable();
};

void KisInterstrokeDataSchemaContractTest::typeConstructionAndLifetimeSchemaRemainStable()
{
    using Data = KisInterstrokeData;

    static_assert(std::is_class_v<Data>);
    static_assert(std::is_abstract_v<Data>);
    static_assert(std::is_same_v<KisInterstrokeDataSP, QSharedPointer<Data>>);
    static_assert(std::is_constructible_v<InterstrokeDataProbe, KisPaintDeviceSP>);
    static_assert(std::has_virtual_destructor_v<Data>);

    QVERIFY(true);
}

void KisInterstrokeDataSchemaContractTest::transactionSignaturesRemainStable()
{
    using Data = KisInterstrokeData;

    ASSERT_INTERSTROKE_DATA_SIGNATURE(beginTransaction, void (Data::*)());
    ASSERT_INTERSTROKE_DATA_SIGNATURE(endTransaction, KUndo2Command * (Data::*)());
}

void KisInterstrokeDataSchemaContractTest::compatibilitySignatureRemainsStable()
{
    using Data = KisInterstrokeData;

    ASSERT_INTERSTROKE_DATA_SIGNATURE(isStillCompatible, bool (Data::*)() const);
}

#undef ASSERT_INTERSTROKE_DATA_SIGNATURE

QTEST_APPLESS_MAIN(KisInterstrokeDataSchemaContractTest)

#include "KisInterstrokeDataSchemaContractTest.moc"
