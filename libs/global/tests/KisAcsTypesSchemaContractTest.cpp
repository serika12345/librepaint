/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_acs_types.h>

#include <QTest>

#include <type_traits>

namespace
{
class ResourceProviderProbe;
class PaintDeviceHandleProbe;
class IteratorProbe;
} // namespace

class KisAcsTypesSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void colorRoleAndMouseButtonMappingRemainStable();
    void currentColorDispatchSchemaRemainsStable();
    void paintDeviceColorAccessSchemaRemainsStable();
    void iteratorColorWriteSchemaRemainsStable();
};

void KisAcsTypesSchemaContractTest::colorRoleAndMouseButtonMappingRemainStable()
{
    using ButtonToRoleSignature = Acs::ColorRole (*)(Qt::MouseButton);
    using ButtonsToRoleSignature = Acs::ColorRole (*)(Qt::MouseButton, Qt::MouseButtons);

    static_assert(std::is_enum_v<Acs::ColorRole>);
    static_assert(Acs::Foreground == 0);
    static_assert(Acs::Background == 1);
    static_assert(std::is_same_v<decltype(&Acs::buttonToRole), ButtonToRoleSignature>);
    static_assert(std::is_same_v<decltype(&Acs::buttonsToRole), ButtonsToRoleSignature>);

    QCOMPARE(Acs::buttonToRole(Qt::LeftButton), Acs::Foreground);
    QCOMPARE(Acs::buttonToRole(Qt::RightButton), Acs::Background);
    QCOMPARE(Acs::buttonsToRole(Qt::RightButton, Qt::LeftButton), Acs::Foreground);
    QCOMPARE(Acs::buttonsToRole(Qt::MiddleButton, Qt::RightButton), Acs::Background);
}

void KisAcsTypesSchemaContractTest::currentColorDispatchSchemaRemainsStable()
{
    using CurrentColorSignature = KoColor (*)(ResourceProviderProbe *, Acs::ColorRole);
    using SetCurrentColorSignature = void (*)(ResourceProviderProbe *, Acs::ColorRole, const KoColor &);

    static_assert(
        std::is_same_v<decltype(static_cast<CurrentColorSignature>(&Acs::currentColor<ResourceProviderProbe>)),
                       CurrentColorSignature>);
    static_assert(
        std::is_same_v<decltype(static_cast<SetCurrentColorSignature>(&Acs::setCurrentColor<ResourceProviderProbe>)),
                       SetCurrentColorSignature>);

    QVERIFY(true);
}

void KisAcsTypesSchemaContractTest::paintDeviceColorAccessSchemaRemainsStable()
{
    using SampleColorSignature = KoColor (*)(PaintDeviceHandleProbe, const QPoint &);
    using SetColorSignature = void (*)(PaintDeviceHandleProbe, const QPoint &, const KoColor &);

    static_assert(std::is_same_v<decltype(static_cast<SampleColorSignature>(&Acs::sampleColor<PaintDeviceHandleProbe>)),
                                 SampleColorSignature>);
    static_assert(std::is_same_v<decltype(static_cast<SetColorSignature>(&Acs::setColor<PaintDeviceHandleProbe>)),
                                 SetColorSignature>);

    QVERIFY(true);
}

void KisAcsTypesSchemaContractTest::iteratorColorWriteSchemaRemainsStable()
{
    using SetColorWithIteratorSignature = void (*)(IteratorProbe &, const KoColor &, int);

    static_assert(
        std::is_same_v<decltype(static_cast<SetColorWithIteratorSignature>(&Acs::setColorWithIterator<IteratorProbe>)),
                       SetColorWithIteratorSignature>);

    QVERIFY(true);
}

QTEST_GUILESS_MAIN(KisAcsTypesSchemaContractTest)

#include "KisAcsTypesSchemaContractTest.moc"
