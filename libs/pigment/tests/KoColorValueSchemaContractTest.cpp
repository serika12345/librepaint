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
    void colorSpaceConversionSignaturesRemainStable();
    void colorSerializationSignaturesRemainStable();
    void colorMetadataSignaturesRemainStable();
    void colorFormattingAndDiagnosticSignaturesRemainStable();
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

void KoColorValueSchemaContractTest::colorSpaceConversionSignaturesRemainStable()
{
    using Intent = KoColorConversionTransformation::Intent;
    using ConversionFlags = KoColorConversionTransformation::ConversionFlags;

    ASSERT_KO_COLOR_SIGNATURE(convertTo, void (KoColor::*)(const KoColorSpace *));
    ASSERT_KO_COLOR_SIGNATURE(convertTo, void (KoColor::*)(const KoColorSpace *, Intent, ConversionFlags));
    ASSERT_KO_COLOR_SIGNATURE(convertedTo, KoColor (KoColor::*)(const KoColorSpace *) const);
    ASSERT_KO_COLOR_SIGNATURE(convertedTo, KoColor (KoColor::*)(const KoColorSpace *, Intent, ConversionFlags) const);
}

void KoColorValueSchemaContractTest::colorSerializationSignaturesRemainStable()
{
    using ProfileMap = QHash<QString, const KoColorProfile *>;

    ASSERT_KO_COLOR_SIGNATURE(fromSVG11, KoColor (*)(QString, ProfileMap, KoColor));
    ASSERT_KO_COLOR_SIGNATURE(fromXML, KoColor (*)(const QDomElement &, const QString &));
    ASSERT_KO_COLOR_SIGNATURE(fromXML, KoColor (*)(const QDomElement &, const QString &, bool *));
    ASSERT_KO_COLOR_SIGNATURE(fromXML, KoColor (*)(const QString &));
    ASSERT_KO_COLOR_SIGNATURE(toSVG11, QString (KoColor::*)(ProfileMap *) const);
    ASSERT_KO_COLOR_SIGNATURE(toXML, QString (KoColor::*)() const);
    ASSERT_KO_COLOR_SIGNATURE(toXML, void (KoColor::*)(QDomDocument &, QDomElement &) const);

    static_assert(std::is_same_v<decltype(KoColor::fromSVG11(QString(), ProfileMap())), KoColor>);
}

void KoColorValueSchemaContractTest::colorMetadataSignaturesRemainStable()
{
    using Metadata = QMap<QString, QVariant>;

    ASSERT_KO_COLOR_SIGNATURE(addMetadata, void (KoColor::*)(QString, QVariant));
    ASSERT_KO_COLOR_SIGNATURE(clearMetadata, void (KoColor::*)());
    ASSERT_KO_COLOR_SIGNATURE(metadata, Metadata (KoColor::*)() const);
}

void KoColorValueSchemaContractTest::colorFormattingAndDiagnosticSignaturesRemainStable()
{
    ASSERT_KO_COLOR_SIGNATURE(dump, void (KoColor::*)() const);
    ASSERT_KO_COLOR_SIGNATURE(toQString, QString (*)(const KoColor &));
    static_assert(std::is_same_v<decltype(static_cast<QDebug (*)(QDebug, const KoColor &)>(&operator<<)),
                                 QDebug (*)(QDebug, const KoColor &)>);
}

QTEST_GUILESS_MAIN(KoColorValueSchemaContractTest)

#include "KoColorValueSchemaContractTest.moc"
