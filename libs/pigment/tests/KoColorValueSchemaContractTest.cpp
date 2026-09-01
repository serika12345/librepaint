/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoColor.h>

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_KO_COLOR_SIGNATURE(method, signature)                                                                   \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoColor::method)), signature>)
} // namespace

class KoColorValueSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void colorValueConstructionAndCopySchemaRemainsStable();
    void colorValueStorageAndAssociationSignaturesRemainStable();
    void colorValueQColorInterchangeSignaturesRemainStable();
    void colorValueOpacitySignaturesRemainStable();
    void colorValueChannelArithmeticSignaturesRemainStable();
};

void KoColorValueSchemaContractTest::colorValueConstructionAndCopySchemaRemainsStable()
{
    static_assert(std::is_class_v<KoColor>);
    static_assert(std::is_default_constructible_v<KoColor>);
    static_assert(std::is_copy_constructible_v<KoColor>);
    static_assert(std::is_constructible_v<KoColor, const KoColorSpace *>);
    static_assert(std::is_constructible_v<KoColor, const QColor &, const KoColorSpace *>);
    static_assert(std::is_constructible_v<KoColor, const quint8 *, const KoColorSpace *>);
    static_assert(std::is_constructible_v<KoColor, const KoColor &, const KoColorSpace *>);

    ASSERT_KO_COLOR_SIGNATURE(fromKoColor, void (KoColor::*)(const KoColor &));
    ASSERT_KO_COLOR_SIGNATURE(operator=, KoColor & (KoColor::*)(const KoColor &));
    ASSERT_KO_COLOR_SIGNATURE(operator==, bool (KoColor::*)(const KoColor &) const);
}

void KoColorValueSchemaContractTest::colorValueStorageAndAssociationSignaturesRemainStable()
{
    ASSERT_KO_COLOR_SIGNATURE(colorSpace, const KoColorSpace *(KoColor::*)() const);
    ASSERT_KO_COLOR_SIGNATURE(createTransparent, KoColor (*)(const KoColorSpace *));
    ASSERT_KO_COLOR_SIGNATURE(data, quint8 * (KoColor::*)());
    ASSERT_KO_COLOR_SIGNATURE(data, const quint8 *(KoColor::*)() const);
    ASSERT_KO_COLOR_SIGNATURE(profile, const KoColorProfile *(KoColor::*)() const);
    ASSERT_KO_COLOR_SIGNATURE(setColor, void (KoColor::*)(const quint8 *, const KoColorSpace *));
    ASSERT_KO_COLOR_SIGNATURE(setProfile, void (KoColor::*)(const KoColorProfile *));

    static_assert(std::is_same_v<decltype(std::declval<KoColor &>().setColor(std::declval<const quint8 *>())), void>);
}

void KoColorValueSchemaContractTest::colorValueQColorInterchangeSignaturesRemainStable()
{
    ASSERT_KO_COLOR_SIGNATURE(fromQColor, void (KoColor::*)(const QColor &));
    ASSERT_KO_COLOR_SIGNATURE(toQColor, QColor (KoColor::*)() const);
    ASSERT_KO_COLOR_SIGNATURE(toQColor, void (KoColor::*)(QColor *) const);
}

void KoColorValueSchemaContractTest::colorValueOpacitySignaturesRemainStable()
{
    ASSERT_KO_COLOR_SIGNATURE(opacityF, qreal (KoColor::*)() const);
    ASSERT_KO_COLOR_SIGNATURE(opacityU8, quint8 (KoColor::*)() const);
    ASSERT_KO_COLOR_SIGNATURE(setOpacity, void (KoColor::*)(qreal));
    ASSERT_KO_COLOR_SIGNATURE(setOpacity, void (KoColor::*)(quint8));
}

void KoColorValueSchemaContractTest::colorValueChannelArithmeticSignaturesRemainStable()
{
    ASSERT_KO_COLOR_SIGNATURE(add, void (KoColor::*)(const KoColor &));
    ASSERT_KO_COLOR_SIGNATURE(added, KoColor (KoColor::*)(const KoColor &) const);
    ASSERT_KO_COLOR_SIGNATURE(subtract, void (KoColor::*)(const KoColor &));
    ASSERT_KO_COLOR_SIGNATURE(subtracted, KoColor (KoColor::*)(const KoColor &) const);
}

QTEST_GUILESS_MAIN(KoColorValueSchemaContractTest)

#include "KoColorValueSchemaContractTest.moc"
