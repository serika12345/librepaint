/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoCompositeOp.h>

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_COMPOSITE_MEMBER(method, signature)                                                                     \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoCompositeOp::method)), signature>)
#define ASSERT_PARAMETER_MEMBER(method, signature)                                                                     \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoCompositeOp::ParameterInfo::method)), signature>)
#define ASSERT_COMPOSITE_CATEGORY(method) static_assert(std::is_same_v<decltype(&KoCompositeOp::method), QString (*)()>)
} // namespace

class KoCompositeOpSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void compositeParameterBufferDefaultsRemainStable();
    void compositeParameterOpacityAndCopyBehaviorRemainStable();
    void compositeCategorySignaturesRemainStable();
    void compositeIdentityAndLifecycleSignaturesRemainStable();
    void compositeInvocationSignaturesRemainStable();
};

void KoCompositeOpSchemaContractTest::compositeParameterBufferDefaultsRemainStable()
{
    using ParameterInfo = KoCompositeOp::ParameterInfo;

    static_assert(std::is_class_v<ParameterInfo>);
    static_assert(std::is_default_constructible_v<ParameterInfo>);
    static_assert(std::is_same_v<decltype(ParameterInfo::dstRowStart), quint8 *>);
    static_assert(std::is_same_v<decltype(ParameterInfo::dstRowStride), qint32>);
    static_assert(std::is_same_v<decltype(ParameterInfo::srcRowStart), const quint8 *>);
    static_assert(std::is_same_v<decltype(ParameterInfo::srcRowStride), qint32>);
    static_assert(std::is_same_v<decltype(ParameterInfo::maskRowStart), const quint8 *>);
    static_assert(std::is_same_v<decltype(ParameterInfo::maskRowStride), qint32>);

    const ParameterInfo parameters;

    QCOMPARE(parameters.dstRowStart, nullptr);
    QCOMPARE(parameters.dstRowStride, 0);
    QCOMPARE(parameters.srcRowStart, nullptr);
    QCOMPARE(parameters.srcRowStride, 0);
    QCOMPARE(parameters.maskRowStart, nullptr);
    QCOMPARE(parameters.maskRowStride, 0);
    QCOMPARE(parameters.rows, 0);
    QCOMPARE(parameters.cols, 0);
    QCOMPARE(parameters.opacity, 1.0f);
    QCOMPARE(parameters.flow, 1.0f);
    QCOMPARE(parameters._lastOpacityData, 0.0f);
    QCOMPARE(parameters.lastOpacity, &parameters.opacity);
    QVERIFY(parameters.channelFlags.isEmpty());
}

void KoCompositeOpSchemaContractTest::compositeParameterOpacityAndCopyBehaviorRemainStable()
{
    using ParameterInfo = KoCompositeOp::ParameterInfo;

    static_assert(std::is_same_v<decltype(ParameterInfo::rows), qint32>);
    static_assert(std::is_same_v<decltype(ParameterInfo::cols), qint32>);
    static_assert(std::is_same_v<decltype(ParameterInfo::opacity), float>);
    static_assert(std::is_same_v<decltype(ParameterInfo::flow), float>);
    static_assert(std::is_same_v<decltype(ParameterInfo::_lastOpacityData), float>);
    static_assert(std::is_same_v<decltype(ParameterInfo::lastOpacity), float *>);
    static_assert(std::is_same_v<decltype(ParameterInfo::channelFlags), QBitArray>);
    static_assert(std::is_copy_constructible_v<ParameterInfo>);
    static_assert(std::is_copy_assignable_v<ParameterInfo>);
    ASSERT_PARAMETER_MEMBER(operator=, ParameterInfo & (ParameterInfo::*)(const ParameterInfo &));
    ASSERT_PARAMETER_MEMBER(setOpacityAndAverage, void (ParameterInfo::*)(float, float));
    ASSERT_PARAMETER_MEMBER(updateOpacityAndAverage, void (ParameterInfo::*)(float));

    quint8 destinationByte = 0;
    const quint8 sourceByte = 0;
    const quint8 maskByte = 0;

    ParameterInfo source;
    source.dstRowStart = &destinationByte;
    source.dstRowStride = 13;
    source.srcRowStart = &sourceByte;
    source.srcRowStride = 17;
    source.maskRowStart = &maskByte;
    source.maskRowStride = 19;
    source.rows = 23;
    source.cols = 29;
    source.flow = 0.75f;
    source.channelFlags = QBitArray(3, true);
    source.setOpacityAndAverage(0.7f, 0.4f);

    const ParameterInfo copied(source);
    QCOMPARE(copied.dstRowStart, source.dstRowStart);
    QCOMPARE(copied.dstRowStride, source.dstRowStride);
    QCOMPARE(copied.srcRowStart, source.srcRowStart);
    QCOMPARE(copied.srcRowStride, source.srcRowStride);
    QCOMPARE(copied.maskRowStart, source.maskRowStart);
    QCOMPARE(copied.maskRowStride, source.maskRowStride);
    QCOMPARE(copied.rows, source.rows);
    QCOMPARE(copied.cols, source.cols);
    QCOMPARE(copied.opacity, source.opacity);
    QCOMPARE(copied.flow, source.flow);
    QCOMPARE(copied._lastOpacityData, source._lastOpacityData);
    QCOMPARE(copied.channelFlags, source.channelFlags);
    QCOMPARE(copied.lastOpacity, &copied._lastOpacityData);
    QVERIFY(copied.lastOpacity != source.lastOpacity);

    ParameterInfo assigned;
    assigned = source;
    QCOMPARE(assigned.dstRowStart, source.dstRowStart);
    QCOMPARE(assigned.dstRowStride, source.dstRowStride);
    QCOMPARE(assigned.srcRowStart, source.srcRowStart);
    QCOMPARE(assigned.srcRowStride, source.srcRowStride);
    QCOMPARE(assigned.maskRowStart, source.maskRowStart);
    QCOMPARE(assigned.maskRowStride, source.maskRowStride);
    QCOMPARE(assigned.rows, source.rows);
    QCOMPARE(assigned.cols, source.cols);
    QCOMPARE(assigned.opacity, source.opacity);
    QCOMPARE(assigned.flow, source.flow);
    QCOMPARE(assigned._lastOpacityData, source._lastOpacityData);
    QCOMPARE(assigned.channelFlags, source.channelFlags);
    QCOMPARE(assigned.lastOpacity, &assigned._lastOpacityData);
    QVERIFY(assigned.lastOpacity != source.lastOpacity);
    assigned.channelFlags.clearBit(0);
    QVERIFY(source.channelFlags.testBit(0));

    ParameterInfo matchingAverage;
    matchingAverage.setOpacityAndAverage(0.6f, 0.6f);
    QCOMPARE(matchingAverage.opacity, 0.6f);
    QCOMPARE(matchingAverage.lastOpacity, &matchingAverage.opacity);
    const ParameterInfo copiedMatchingAverage(matchingAverage);
    QCOMPARE(copiedMatchingAverage.lastOpacity, &copiedMatchingAverage.opacity);
    QVERIFY(copiedMatchingAverage.lastOpacity != matchingAverage.lastOpacity);

    ParameterInfo increasingOpacity;
    increasingOpacity.setOpacityAndAverage(0.7f, 0.4f);
    increasingOpacity.updateOpacityAndAverage(0.8f);
    QCOMPARE(increasingOpacity.opacity, 0.8f);
    QCOMPARE(increasingOpacity.lastOpacity, &increasingOpacity.opacity);

    ParameterInfo averagedOpacity;
    averagedOpacity.setOpacityAndAverage(0.6f, 0.8f);
    averagedOpacity.updateOpacityAndAverage(0.2f);
    QCOMPARE(averagedOpacity.opacity, 0.2f);
    QCOMPARE(averagedOpacity.lastOpacity, &averagedOpacity._lastOpacityData);
    QVERIFY(qFuzzyCompare(averagedOpacity._lastOpacityData, 0.74f));
}

void KoCompositeOpSchemaContractTest::compositeCategorySignaturesRemainStable()
{
    ASSERT_COMPOSITE_CATEGORY(categoryArithmetic);
    ASSERT_COMPOSITE_CATEGORY(categoryBinary);
    ASSERT_COMPOSITE_CATEGORY(categoryModulo);
    ASSERT_COMPOSITE_CATEGORY(categoryNegative);
    ASSERT_COMPOSITE_CATEGORY(categoryLight);
    ASSERT_COMPOSITE_CATEGORY(categoryDark);
    ASSERT_COMPOSITE_CATEGORY(categoryHSY);
    ASSERT_COMPOSITE_CATEGORY(categoryHSI);
    ASSERT_COMPOSITE_CATEGORY(categoryHSL);
    ASSERT_COMPOSITE_CATEGORY(categoryHSV);
    ASSERT_COMPOSITE_CATEGORY(categoryMix);
    ASSERT_COMPOSITE_CATEGORY(categoryMisc);
    ASSERT_COMPOSITE_CATEGORY(categoryQuadratic);
}

void KoCompositeOpSchemaContractTest::compositeIdentityAndLifecycleSignaturesRemainStable()
{
    static_assert(std::is_class_v<KoCompositeOp>);
    static_assert(std::is_constructible_v<KoCompositeOp, const KoColorSpace *, const QString &, const QString &>);
    static_assert(std::is_constructible_v<KoCompositeOp, const KoColorSpace *, const QString &>);
    static_assert(std::is_destructible_v<KoCompositeOp>);
    static_assert(std::has_virtual_destructor_v<KoCompositeOp>);

    ASSERT_COMPOSITE_MEMBER(id, QString (KoCompositeOp::*)() const);
    ASSERT_COMPOSITE_MEMBER(description, QString (KoCompositeOp::*)() const);
    ASSERT_COMPOSITE_MEMBER(colorSpace, const KoColorSpace *(KoCompositeOp::*)() const);
    ASSERT_COMPOSITE_MEMBER(category, QString (KoCompositeOp::*)() const);
}

void KoCompositeOpSchemaContractTest::compositeInvocationSignaturesRemainStable()
{
    using ParameterComposite = void (KoCompositeOp::*)(const KoCompositeOp::ParameterInfo &) const;
    using BufferComposite = void (KoCompositeOp::*)(quint8 *,
                                                    qint32,
                                                    const quint8 *,
                                                    qint32,
                                                    const quint8 *,
                                                    qint32,
                                                    qint32,
                                                    qint32,
                                                    float,
                                                    const QBitArray &) const;

    ASSERT_COMPOSITE_MEMBER(composite, ParameterComposite);
    ASSERT_COMPOSITE_MEMBER(composite, BufferComposite);
    static_assert(
        std::is_same_v<decltype(std::declval<const KoCompositeOp &>().composite(static_cast<quint8 *>(nullptr),
                                                                                qint32{},
                                                                                static_cast<const quint8 *>(nullptr),
                                                                                qint32{},
                                                                                static_cast<const quint8 *>(nullptr),
                                                                                qint32{},
                                                                                qint32{},
                                                                                qint32{},
                                                                                float{})),
                       void>);
}

QTEST_APPLESS_MAIN(KoCompositeOpSchemaContractTest)

#include "KoCompositeOpSchemaContractTest.moc"
