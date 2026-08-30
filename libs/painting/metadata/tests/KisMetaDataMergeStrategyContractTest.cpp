/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_meta_data_merge_strategy.h"

#include <QTest>

#include <memory>
#include <type_traits>

namespace
{
class RecordingMergeStrategy final : public KisMetaData::MergeStrategy
{
public:
    explicit RecordingMergeStrategy(int *destructionCount = nullptr)
        : m_destructionCount(destructionCount)
    {
    }

    ~RecordingMergeStrategy() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    QString id() const override
    {
        ++idCalls;
        return idValue;
    }

    QString name() const override
    {
        ++nameCalls;
        return nameValue;
    }

    QString description() const override
    {
        ++descriptionCalls;
        return descriptionValue;
    }

    void merge(KisMetaData::Store *destination,
               QList<const KisMetaData::Store *> sources,
               QList<double> scores) const override
    {
        ++mergeCalls;
        mergedDestination = destination;
        mergedSources = sources;
        mergedScores = scores;
    }

    QString idValue{QStringLiteral("merge/保持-β")};
    QString nameValue{QStringLiteral("優先統合・日本語")};
    QString descriptionValue{QStringLiteral("入力列と重みを順番どおり統合")};

    mutable int idCalls{0};
    mutable int nameCalls{0};
    mutable int descriptionCalls{0};
    mutable int mergeCalls{0};
    mutable KisMetaData::Store *mergedDestination{nullptr};
    mutable QList<const KisMetaData::Store *> mergedSources;
    mutable QList<double> mergedScores;

private:
    int *m_destructionCount;
};

class KisMetaDataMergeStrategyContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void identityMethodsDispatchUnicodeValues();
    void mergePreservesPointersOrderDuplicatesAndScores();
    void baseOwnershipHasVirtualLifetime();
};

void KisMetaDataMergeStrategyContractTest::identityMethodsDispatchUnicodeValues()
{
    static_assert(std::is_abstract_v<KisMetaData::MergeStrategy>);
    static_assert(std::is_polymorphic_v<KisMetaData::MergeStrategy>);
    static_assert(std::has_virtual_destructor_v<KisMetaData::MergeStrategy>);

    RecordingMergeStrategy strategy;
    const KisMetaData::MergeStrategy &interface = strategy;

    QCOMPARE(interface.id(), QStringLiteral("merge/保持-β"));
    QCOMPARE(interface.name(), QStringLiteral("優先統合・日本語"));
    QCOMPARE(interface.description(), QStringLiteral("入力列と重みを順番どおり統合"));
    QCOMPARE(strategy.idCalls, 1);
    QCOMPARE(strategy.nameCalls, 1);
    QCOMPARE(strategy.descriptionCalls, 1);

    strategy.idValue = QStringLiteral("merge/変更-γ");
    strategy.nameValue = QStringLiteral("別名");
    strategy.descriptionValue = QStringLiteral("別説明");

    QCOMPARE(interface.id(), QStringLiteral("merge/変更-γ"));
    QCOMPARE(interface.name(), QStringLiteral("別名"));
    QCOMPARE(interface.description(), QStringLiteral("別説明"));
    QCOMPARE(strategy.idCalls, 2);
    QCOMPARE(strategy.nameCalls, 2);
    QCOMPARE(strategy.descriptionCalls, 2);
}

void KisMetaDataMergeStrategyContractTest::mergePreservesPointersOrderDuplicatesAndScores()
{
    char destinationToken = 0;
    char firstSourceToken = 0;
    char secondSourceToken = 0;
    auto *destination = reinterpret_cast<KisMetaData::Store *>(&destinationToken);
    const auto *firstSource = reinterpret_cast<const KisMetaData::Store *>(&firstSourceToken);
    const auto *secondSource = reinterpret_cast<const KisMetaData::Store *>(&secondSourceToken);
    const QList<const KisMetaData::Store *> sources{firstSource, secondSource, firstSource};
    const QList<double> scores{0.625, 0.125, 0.25};

    RecordingMergeStrategy strategy;
    const KisMetaData::MergeStrategy &interface = strategy;

    interface.merge(destination, sources, scores);

    QCOMPARE(strategy.mergeCalls, 1);
    QCOMPARE(strategy.mergedDestination, destination);
    QCOMPARE(strategy.mergedSources, sources);
    QCOMPARE(strategy.mergedSources.at(0), firstSource);
    QCOMPARE(strategy.mergedSources.at(1), secondSource);
    QCOMPARE(strategy.mergedSources.at(2), firstSource);
    QCOMPARE(strategy.mergedScores, scores);
}

void KisMetaDataMergeStrategyContractTest::baseOwnershipHasVirtualLifetime()
{
    int destructionCount = 0;
    {
        std::unique_ptr<KisMetaData::MergeStrategy> strategy =
            std::make_unique<RecordingMergeStrategy>(&destructionCount);
        QCOMPARE(destructionCount, 0);
    }
    QCOMPARE(destructionCount, 1);
}
} // namespace

QTEST_GUILESS_MAIN(KisMetaDataMergeStrategyContractTest)

#include "KisMetaDataMergeStrategyContractTest.moc"
