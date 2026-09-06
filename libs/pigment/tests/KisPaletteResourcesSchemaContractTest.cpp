/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <resources/KisSwatch.h>
#include <resources/KisUniqueColorSet.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_SWATCH_SIGNATURE(method, signature)                                                                     \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisSwatch::method)), signature>)
#define ASSERT_UNIQUE_COLOR_SET_SIGNATURE(method, signature)                                                           \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisUniqueColorSet::method)), signature>)
} // namespace

class KisPaletteResourcesSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void swatchTypeConstructionAndEqualitySchemaRemainStable();
    void swatchStateSignaturesRemainStable();
    void swatchSerializationSignaturesRemainStable();
    void uniqueColorSetTypeConstructionAndLifetimeSchemaRemainStable();
    void uniqueColorSetContentsAndNotificationSignaturesRemainStable();
};

void KisPaletteResourcesSchemaContractTest::swatchTypeConstructionAndEqualitySchemaRemainStable()
{
    static_assert(std::is_class_v<KisSwatch>);
    static_assert(std::is_default_constructible_v<KisSwatch>);
    static_assert(std::is_constructible_v<KisSwatch, const KoColor &, const QString &>);
    static_assert(std::is_constructible_v<KisSwatch, const KoColor &>);
    ASSERT_SWATCH_SIGNATURE(operator==, bool (KisSwatch::*)(const KisSwatch &) const);
}

void KisPaletteResourcesSchemaContractTest::swatchStateSignaturesRemainStable()
{
    ASSERT_SWATCH_SIGNATURE(name, QString (KisSwatch::*)() const);
    ASSERT_SWATCH_SIGNATURE(setName, void (KisSwatch::*)(const QString &));
    ASSERT_SWATCH_SIGNATURE(id, QString (KisSwatch::*)() const);
    ASSERT_SWATCH_SIGNATURE(setId, void (KisSwatch::*)(const QString &));
    ASSERT_SWATCH_SIGNATURE(color, KoColor (KisSwatch::*)() const);
    ASSERT_SWATCH_SIGNATURE(setColor, void (KisSwatch::*)(const KoColor &));
    ASSERT_SWATCH_SIGNATURE(spotColor, bool (KisSwatch::*)() const);
    ASSERT_SWATCH_SIGNATURE(setSpotColor, void (KisSwatch::*)(bool));
    ASSERT_SWATCH_SIGNATURE(isValid, bool (KisSwatch::*)() const);
}

void KisPaletteResourcesSchemaContractTest::swatchSerializationSignaturesRemainStable()
{
    ASSERT_SWATCH_SIGNATURE(writeToStream, void (KisSwatch::*)(QDataStream &, const QString &, int, int));
    ASSERT_SWATCH_SIGNATURE(fromByteArray, KisSwatch (*)(QByteArray &, QString &, int &, int &));
    ASSERT_SWATCH_SIGNATURE(fromByteArray, KisSwatch (*)(QByteArray &));
}

void KisPaletteResourcesSchemaContractTest::uniqueColorSetTypeConstructionAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisUniqueColorSet>);
    static_assert(std::is_base_of_v<QObject, KisUniqueColorSet>);
    static_assert(std::is_default_constructible_v<KisUniqueColorSet>);
    static_assert(std::is_constructible_v<KisUniqueColorSet, QObject *>);
    static_assert(std::is_constructible_v<KisUniqueColorSet, const KisUniqueColorSet &>);
    static_assert(std::is_constructible_v<KisUniqueColorSet, const KisUniqueColorSet &, QObject *>);
    static_assert(std::has_virtual_destructor_v<KisUniqueColorSet>);
}

void KisPaletteResourcesSchemaContractTest::uniqueColorSetContentsAndNotificationSignaturesRemainStable()
{
    ASSERT_UNIQUE_COLOR_SET_SIGNATURE(addColor, void (KisUniqueColorSet::*)(const KoColor &));
    ASSERT_UNIQUE_COLOR_SET_SIGNATURE(clear, void (KisUniqueColorSet::*)());
    ASSERT_UNIQUE_COLOR_SET_SIGNATURE(color, KoColor (KisUniqueColorSet::*)(int) const);
    ASSERT_UNIQUE_COLOR_SET_SIGNATURE(colorList, QList<KoColor> (KisUniqueColorSet::*)() const);
    ASSERT_UNIQUE_COLOR_SET_SIGNATURE(setColorList, void (KisUniqueColorSet::*)(const QList<KoColor> &));
    ASSERT_UNIQUE_COLOR_SET_SIGNATURE(size, int (KisUniqueColorSet::*)() const);
    ASSERT_UNIQUE_COLOR_SET_SIGNATURE(sigReset, void (KisUniqueColorSet::*)());
    ASSERT_UNIQUE_COLOR_SET_SIGNATURE(sigColorAdded, void (KisUniqueColorSet::*)(int));
    ASSERT_UNIQUE_COLOR_SET_SIGNATURE(sigColorMoved, void (KisUniqueColorSet::*)(int, int));
    ASSERT_UNIQUE_COLOR_SET_SIGNATURE(sigColorRemoved, void (KisUniqueColorSet::*)(int));
}

#undef ASSERT_SWATCH_SIGNATURE
#undef ASSERT_UNIQUE_COLOR_SET_SIGNATURE

QTEST_GUILESS_MAIN(KisPaletteResourcesSchemaContractTest)

#include "KisPaletteResourcesSchemaContractTest.moc"
