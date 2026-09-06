/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoBgrColorSpaceTraits.h>
#include <KoColorSpaceAbstract.h>

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
using Subject = KoColorSpaceAbstract<KoBgrU8Traits>;

#define ASSERT_SUBJECT_SIGNATURE(method, signature)                                                                    \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&Subject::method)), signature>)

class ConstructionProbe : public Subject
{
public:
    ConstructionProbe(const QString &id, const QString &name)
        : Subject(id, name)
    {
    }
};
} // namespace

class KoColorSpaceAbstractSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeAndConstructionSchemaRemainStable();
    void channelTopologyAndTextSchemaRemainStable();
    void opacityAndAlphaMaskSchemaRemainStable();
    void channelVisualizationAndBrushSchemaRemainStable();
    void intensityAndTransformationSchemaRemainStable();
};

void KoColorSpaceAbstractSchemaContractTest::typeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<Subject>);
    static_assert(std::is_abstract_v<Subject>);
    static_assert(std::is_base_of_v<KoColorSpace, Subject>);
    static_assert(std::is_same_v<typename Subject::ColorSpaceTraits, KoBgrU8Traits>);
    static_assert(std::is_base_of_v<Subject, ConstructionProbe>);
}

void KoColorSpaceAbstractSchemaContractTest::channelTopologyAndTextSchemaRemainStable()
{
    ASSERT_SUBJECT_SIGNATURE(alphaPos, quint32 (Subject::*)() const);
    ASSERT_SUBJECT_SIGNATURE(channelCount, quint32 (Subject::*)() const);
    ASSERT_SUBJECT_SIGNATURE(colorChannelCount, quint32 (Subject::*)() const);
    ASSERT_SUBJECT_SIGNATURE(pixelSize, quint32 (Subject::*)() const);
    ASSERT_SUBJECT_SIGNATURE(channelValueText, QString (Subject::*)(const quint8 *, quint32) const);
    ASSERT_SUBJECT_SIGNATURE(normalisedChannelValueText, QString (Subject::*)(const quint8 *, quint32) const);
    ASSERT_SUBJECT_SIGNATURE(normalisedChannelsValue, void (Subject::*)(const quint8 *, QVector<float> &) const);
    ASSERT_SUBJECT_SIGNATURE(fromNormalisedChannelsValue, void (Subject::*)(quint8 *, const QVector<float> &) const);
}

void KoColorSpaceAbstractSchemaContractTest::opacityAndAlphaMaskSchemaRemainStable()
{
    ASSERT_SUBJECT_SIGNATURE(opacityF, qreal (Subject::*)(const quint8 *) const);
    ASSERT_SUBJECT_SIGNATURE(opacityU8, quint8 (Subject::*)(const quint8 *) const);
    ASSERT_SUBJECT_SIGNATURE(setOpacity, void (Subject::*)(quint8 *, qreal, qint32) const);
    ASSERT_SUBJECT_SIGNATURE(setOpacity, void (Subject::*)(quint8 *, quint8, qint32) const);
    ASSERT_SUBJECT_SIGNATURE(copyOpacityU8, void (Subject::*)(quint8 *, quint8 *, qint32) const);
    ASSERT_SUBJECT_SIGNATURE(multiplyAlpha, void (Subject::*)(quint8 *, quint8, qint32) const);
    ASSERT_SUBJECT_SIGNATURE(applyAlphaNormedFloatMask, void (Subject::*)(quint8 *, const float *, qint32) const);
    ASSERT_SUBJECT_SIGNATURE(applyAlphaU8Mask, void (Subject::*)(quint8 *, const quint8 *, qint32) const);
    ASSERT_SUBJECT_SIGNATURE(applyInverseAlphaU8Mask, void (Subject::*)(quint8 *, const quint8 *, qint32) const);
    ASSERT_SUBJECT_SIGNATURE(applyInverseNormedFloatMask, void (Subject::*)(quint8 *, const float *, qint32) const);
}

void KoColorSpaceAbstractSchemaContractTest::channelVisualizationAndBrushSchemaRemainStable()
{
    ASSERT_SUBJECT_SIGNATURE(scaleToU8, quint8 (Subject::*)(const quint8 *, qint32) const);
    ASSERT_SUBJECT_SIGNATURE(singleChannelPixel, void (Subject::*)(quint8 *, const quint8 *, quint32) const);
    ASSERT_SUBJECT_SIGNATURE(fillInverseAlphaNormedFloatMaskWithColor,
                             void (Subject::*)(quint8 *, const float *, const quint8 *, qint32) const);
    ASSERT_SUBJECT_SIGNATURE(fillGrayBrushWithColor, void (Subject::*)(quint8 *, const QRgb *, quint8 *, qint32) const);
    ASSERT_SUBJECT_SIGNATURE(convertChannelToVisualRepresentation,
                             void (Subject::*)(const quint8 *, quint8 *, quint32, qint32) const);
    ASSERT_SUBJECT_SIGNATURE(convertChannelToVisualRepresentation,
                             void (Subject::*)(const quint8 *, quint8 *, quint32, QBitArray) const);
    ASSERT_SUBJECT_SIGNATURE(toQColor16, void (Subject::*)(const quint8 *, QColor *) const);
}

void KoColorSpaceAbstractSchemaContractTest::intensityAndTransformationSchemaRemainStable()
{
    ASSERT_SUBJECT_SIGNATURE(intensity8, quint8 (Subject::*)(const quint8 *) const);
    ASSERT_SUBJECT_SIGNATURE(intensityF, qreal (Subject::*)(const quint8 *) const);
    ASSERT_SUBJECT_SIGNATURE(createInvertTransformation, KoColorTransformation * (Subject::*)() const);
    ASSERT_SUBJECT_SIGNATURE(createDarkenAdjustment, KoColorTransformation * (Subject::*)(qint32, bool, qreal) const);
}

QTEST_GUILESS_MAIN(KoColorSpaceAbstractSchemaContractTest)

#include "KoColorSpaceAbstractSchemaContractTest.moc"
