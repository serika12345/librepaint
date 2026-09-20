/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoColorTransformation.h>
#include <KoCompositeColorTransformation.h>

#include <QHash>
#include <QTest>
#include <QVariant>
#include <QVector>

#include <array>

namespace
{

class ParameterTransformation : public KoColorTransformation
{
public:
    explicit ParameterTransformation(int *destructionCount = nullptr)
        : m_destructionCount(destructionCount)
    {
    }

    ~ParameterTransformation() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    void transform(const quint8 *src, quint8 *dst, qint32 nPixels) const override
    {
        ++transformCount;
        transformSource = src;
        transformDestination = dst;
        transformLength = nPixels;
        for (qint32 i = 0; i < nPixels; ++i) {
            dst[i] = src[i];
        }
    }

    int parameterId(const QString &name) const override
    {
        requestedNames.append(name);
        if (name == QStringLiteral("gain")) {
            return 11;
        }
        if (name == QStringLiteral("offset")) {
            return 12;
        }
        return -1;
    }

    void setParameter(int id, const QVariant &parameter) override
    {
        assignedParameters.insert(id, parameter);
    }

    mutable int transformCount = 0;
    mutable const quint8 *transformSource = nullptr;
    mutable quint8 *transformDestination = nullptr;
    mutable qint32 transformLength = -1;
    mutable QList<QString> requestedNames;
    QHash<int, QVariant> assignedParameters;

private:
    int *m_destructionCount;
};

struct StageCall {
    int stage = 0;
    const quint8 *source = nullptr;
    quint8 *destination = nullptr;
    qint32 length = -1;
    QVector<quint8> input;
};

class StageTransformation : public KoColorTransformation
{
public:
    StageTransformation(int stage, quint8 increment, QVector<StageCall> *calls, int *destructionCount)
        : m_stage(stage)
        , m_increment(increment)
        , m_calls(calls)
        , m_destructionCount(destructionCount)
    {
    }

    ~StageTransformation() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    void transform(const quint8 *src, quint8 *dst, qint32 nPixels) const override
    {
        StageCall call;
        call.stage = m_stage;
        call.source = src;
        call.destination = dst;
        call.length = nPixels;
        call.input.reserve(nPixels);
        for (qint32 i = 0; i < nPixels; ++i) {
            call.input.append(src[i]);
            dst[i] = static_cast<quint8>(src[i] + m_increment);
        }
        m_calls->append(call);
    }

private:
    int m_stage;
    quint8 m_increment;
    QVector<StageCall> *m_calls;
    int *m_destructionCount;
};

} // namespace

class KoColorTransformationContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void baseDefaultsRouteParametersAndOwnVirtualLifetime();
    void modeIdentifiersConstructAnEmptyInPlaceComposite();
    void compositeAppliesOwnedTransformsInSequence();
    void optimizedFactoryHandlesEmptySingleAndMultipleTransforms();
};

void KoColorTransformationContractTest::baseDefaultsRouteParametersAndOwnVirtualLifetime()
{
    int destructionCount = 0;
    auto *transformation = new ParameterTransformation(&destructionCount);
    KoColorTransformation *base = transformation;

    QVERIFY(base->parameters().isEmpty());
    QVERIFY(base->isValid());

    const std::array<quint8, 3> source{2, 5, 8};
    std::array<quint8, 3> destination{0, 0, 0};
    base->transform(source.data(), destination.data(), 3);
    QCOMPARE(transformation->transformCount, 1);
    QCOMPARE(transformation->transformSource, source.data());
    QCOMPARE(transformation->transformDestination, destination.data());
    QCOMPARE(transformation->transformLength, 3);
    QCOMPARE(destination[0], source[0]);
    QCOMPARE(destination[1], source[1]);
    QCOMPARE(destination[2], source[2]);

    QHash<QString, QVariant> parameters;
    parameters.insert(QStringLiteral("gain"), 3);
    parameters.insert(QStringLiteral("offset"), -2);
    base->setParameters(parameters);

    QCOMPARE(transformation->requestedNames.size(), 2);
    QVERIFY(transformation->requestedNames.contains(QStringLiteral("gain")));
    QVERIFY(transformation->requestedNames.contains(QStringLiteral("offset")));
    QCOMPARE(transformation->assignedParameters.value(11).toInt(), 3);
    QCOMPARE(transformation->assignedParameters.value(12).toInt(), -2);

    delete base;
    QCOMPARE(destructionCount, 1);
}

void KoColorTransformationContractTest::modeIdentifiersConstructAnEmptyInPlaceComposite()
{
    QCOMPARE(static_cast<int>(KoCompositeColorTransformation::INPLACE), 0);
    QCOMPARE(static_cast<int>(KoCompositeColorTransformation::BUFFERED), 1);

    KoCompositeColorTransformation composite(KoCompositeColorTransformation::INPLACE);
    const quint8 source = 17;
    quint8 destination = 91;
    composite.transform(&source, &destination, 1);
    QCOMPARE(destination, quint8(91));
}

void KoColorTransformationContractTest::compositeAppliesOwnedTransformsInSequence()
{
    QVector<StageCall> calls;
    int destructionCount = 0;
    {
        KoCompositeColorTransformation composite(KoCompositeColorTransformation::INPLACE);
        composite.appendTransform(nullptr);
        composite.appendTransform(new StageTransformation(1, 1, &calls, &destructionCount));
        composite.appendTransform(new StageTransformation(2, 2, &calls, &destructionCount));

        const std::array<quint8, 3> source{1, 4, 7};
        std::array<quint8, 3> destination{0, 0, 0};
        composite.transform(source.data(), destination.data(), 3);

        QCOMPARE(calls.size(), 2);
        QCOMPARE(calls[0].stage, 1);
        QCOMPARE(calls[0].source, source.data());
        QCOMPARE(calls[0].destination, destination.data());
        QCOMPARE(calls[0].length, 3);
        QCOMPARE(calls[0].input, QVector<quint8>({1, 4, 7}));
        QCOMPARE(calls[1].stage, 2);
        QCOMPARE(calls[1].source, destination.data());
        QCOMPARE(calls[1].destination, destination.data());
        QCOMPARE(calls[1].length, 3);
        QCOMPARE(calls[1].input, QVector<quint8>({2, 5, 8}));
        QCOMPARE(destination[0], quint8(4));
        QCOMPARE(destination[1], quint8(7));
        QCOMPARE(destination[2], quint8(10));
    }
    QCOMPARE(destructionCount, 2);
}

void KoColorTransformationContractTest::optimizedFactoryHandlesEmptySingleAndMultipleTransforms()
{
    QVERIFY(!KoCompositeColorTransformation::createOptimizedCompositeTransform({nullptr, nullptr}));

    QVector<StageCall> singleCalls;
    int singleDestructionCount = 0;
    auto *single = new StageTransformation(1, 1, &singleCalls, &singleDestructionCount);
    KoColorTransformation *singleResult =
        KoCompositeColorTransformation::createOptimizedCompositeTransform({nullptr, single});
    QCOMPARE(singleResult, single);
    delete singleResult;
    QCOMPARE(singleDestructionCount, 1);

    QVector<StageCall> multipleCalls;
    int multipleDestructionCount = 0;
    auto *first = new StageTransformation(1, 1, &multipleCalls, &multipleDestructionCount);
    auto *second = new StageTransformation(2, 2, &multipleCalls, &multipleDestructionCount);
    KoColorTransformation *multipleResult =
        KoCompositeColorTransformation::createOptimizedCompositeTransform({first, nullptr, second});
    QVERIFY(multipleResult);
    QVERIFY(multipleResult != first);
    QVERIFY(multipleResult != second);

    const quint8 source = 3;
    quint8 destination = 0;
    multipleResult->transform(&source, &destination, 1);
    QCOMPARE(destination, quint8(6));
    QCOMPARE(multipleCalls.size(), 2);

    delete multipleResult;
    QCOMPARE(multipleDestructionCount, 2);
}

QTEST_MAIN(KoColorTransformationContractTest)

#include "KoColorTransformationContractTest.moc"
