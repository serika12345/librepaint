/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <type_traits>

#include <colorspaces/KoAlphaColorSpace.h>

class KoAlphaColorSpaceTypeContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void aliasesSelectTheExpectedTraitsAndImplementations();
    void channelTypesProvideStableAlphaIdentifiers();
};

void KoAlphaColorSpaceTypeContractTest::aliasesSelectTheExpectedTraitsAndImplementations()
{
    QVERIFY((std::is_same_v<AlphaU8Traits, KoColorSpaceTrait<quint8, 1, 0>>));
    QVERIFY((std::is_same_v<AlphaU16Traits, KoColorSpaceTrait<quint16, 1, 0>>));
    QVERIFY((std::is_same_v<AlphaF32Traits, KoColorSpaceTrait<float, 1, 0>>));
#ifdef HAVE_OPENEXR
    QVERIFY((std::is_same_v<AlphaF16Traits, KoColorSpaceTrait<half, 1, 0>>));
#endif

    QVERIFY((std::is_same_v<KoAlphaColorSpace, KoAlphaColorSpaceImpl<AlphaU8Traits>>));
    QVERIFY((std::is_same_v<KoAlphaU16ColorSpace, KoAlphaColorSpaceImpl<AlphaU16Traits>>));
    QVERIFY((std::is_same_v<KoAlphaF32ColorSpace, KoAlphaColorSpaceImpl<AlphaF32Traits>>));
#ifdef HAVE_OPENEXR
    QVERIFY((std::is_same_v<KoAlphaF16ColorSpace, KoAlphaColorSpaceImpl<AlphaF16Traits>>));
#endif

    QVERIFY((std::is_same_v<KoAlphaColorSpaceFactory, KoAlphaColorSpaceFactoryImpl<AlphaU8Traits>>));
    QVERIFY((std::is_same_v<KoAlphaU16ColorSpaceFactory, KoAlphaColorSpaceFactoryImpl<AlphaU16Traits>>));
    QVERIFY((std::is_same_v<KoAlphaF32ColorSpaceFactory, KoAlphaColorSpaceFactoryImpl<AlphaF32Traits>>));
#ifdef HAVE_OPENEXR
    QVERIFY((std::is_same_v<KoAlphaF16ColorSpaceFactory, KoAlphaColorSpaceFactoryImpl<AlphaF16Traits>>));
#endif

    QVERIFY((std::is_base_of_v<KoColorSpace, KoAlphaColorSpaceImpl<AlphaU8Traits>>));
    QVERIFY((std::is_base_of_v<KoSimpleColorSpaceFactory, KoAlphaColorSpaceFactoryImpl<AlphaU8Traits>>));
}

void KoAlphaColorSpaceTypeContractTest::channelTypesProvideStableAlphaIdentifiers()
{
    QCOMPARE(alphaIdFromChannelType<quint8>().id(), QStringLiteral("ALPHA"));
    QCOMPARE(alphaIdFromChannelType<quint16>().id(), QStringLiteral("ALPHAU16"));
    QCOMPARE(alphaIdFromChannelType<float>().id(), QStringLiteral("ALPHAF32"));
#ifdef HAVE_OPENEXR
    QCOMPARE(alphaIdFromChannelType<half>().id(), QStringLiteral("ALPHAF16"));
#endif
}

QTEST_GUILESS_MAIN(KoAlphaColorSpaceTypeContractTest)

#include "KoAlphaColorSpaceTypeContractTest.moc"
