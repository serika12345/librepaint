/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoColorSpace.h>

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_KO_COLOR_SPACE_SIGNATURE(method, signature)                                                             \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoColorSpace::method)), signature>)
} // namespace

class KoColorSpaceSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void colorSpaceIdentitySignaturesRemainStable();
    void channelTopologySignaturesRemainStable();
    void channelRepresentationSignaturesRemainStable();
    void profileCompatibilityAndIndependencePolicyRemainStable();
    void gamutMetadataSignaturesRemainStable();
};

void KoColorSpaceSchemaContractTest::colorSpaceIdentitySignaturesRemainStable()
{
    static_assert(std::is_class_v<KoColorSpace>);
    static_assert(std::is_abstract_v<KoColorSpace>);
    static_assert(!std::is_destructible_v<KoColorSpace>);

    ASSERT_KO_COLOR_SPACE_SIGNATURE(id, QString (KoColorSpace::*)() const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(name, QString (KoColorSpace::*)() const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(colorModelId, KoID (KoColorSpace::*)() const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(colorDepthId, KoID (KoColorSpace::*)() const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(operator!=, bool (KoColorSpace::*)(const KoColorSpace &) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(operator==, bool (KoColorSpace::*)(const KoColorSpace &) const);
}

void KoColorSpaceSchemaContractTest::channelTopologySignaturesRemainStable()
{
    ASSERT_KO_COLOR_SPACE_SIGNATURE(alphaPos, quint32 (KoColorSpace::*)() const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(channelCount, quint32 (KoColorSpace::*)() const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(channelFlags, QBitArray (KoColorSpace::*)(bool, bool) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(channels, QList<KoChannelInfo *> (KoColorSpace::*)() const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(colorChannelCount, quint32 (KoColorSpace::*)() const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(pixelSize, quint32 (KoColorSpace::*)() const);

    static_assert(std::is_same_v<decltype(std::declval<const KoColorSpace &>().channelFlags()), QBitArray>);
}

void KoColorSpaceSchemaContractTest::channelRepresentationSignaturesRemainStable()
{
    ASSERT_KO_COLOR_SPACE_SIGNATURE(channelValueText, QString (KoColorSpace::*)(const quint8 *, quint32) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(fromNormalisedChannelsValue,
                                    void (KoColorSpace::*)(quint8 *, const QVector<float> &) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(normalisedChannelValueText,
                                    QString (KoColorSpace::*)(const quint8 *, quint32) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(normalisedChannelsValue,
                                    void (KoColorSpace::*)(const quint8 *, QVector<float> &) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(scaleToU8, quint8 (KoColorSpace::*)(const quint8 *, qint32) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(singleChannelPixel,
                                    void (KoColorSpace::*)(quint8 *, const quint8 *, quint32) const);
}

void KoColorSpaceSchemaContractTest::profileCompatibilityAndIndependencePolicyRemainStable()
{
    static_assert(std::is_enum_v<ColorSpaceIndependence>);
    static_assert(FULLY_INDEPENDENT == 0);
    static_assert(TO_LAB16 == 1);
    static_assert(TO_RGBA8 == 2);
    static_assert(TO_RGBA16 == 3);

    ASSERT_KO_COLOR_SPACE_SIGNATURE(hasHighDynamicRange, bool (KoColorSpace::*)() const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(profile, const KoColorProfile *(KoColorSpace::*)() const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(profileIsCompatible, bool (KoColorSpace::*)(const KoColorProfile *) const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(willDegrade, bool (KoColorSpace::*)(ColorSpaceIndependence) const);
}

void KoColorSpaceSchemaContractTest::gamutMetadataSignaturesRemainStable()
{
    ASSERT_KO_COLOR_SPACE_SIGNATURE(estimatedTRCXYY, QPolygonF (KoColorSpace::*)() const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(gamutXYY, QPolygonF (KoColorSpace::*)() const);
    ASSERT_KO_COLOR_SPACE_SIGNATURE(lumaCoefficients, QVector<qreal> (KoColorSpace::*)() const);
}

QTEST_GUILESS_MAIN(KoColorSpaceSchemaContractTest)

#include "KoColorSpaceSchemaContractTest.moc"
