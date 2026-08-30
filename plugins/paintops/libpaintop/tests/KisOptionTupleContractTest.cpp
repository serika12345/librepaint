/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QString>

class KisPropertiesConfiguration
{
};

#include "KisOptionTuple.h"

#include <QTest>

#include <array>
#include <type_traits>

namespace
{

struct CallTrace {
    QStringList events;
};

std::array<int, 4> plainDefaultConstructions{};
std::array<int, 4> plainArgumentConstructions{};
std::array<int, 4> prefixedPrefixConstructions{};
std::array<int, 4> prefixedArgumentConstructions{};

template<int Id>
struct PlainOptionData {
    PlainOptionData()
    {
        ++plainDefaultConstructions[Id];
    }

    PlainOptionData(int value, const QString &label)
        : constructorValue(value)
        , constructorLabel(label)
    {
        ++plainArgumentConstructions[Id];
    }

    bool read(const KisPropertiesConfiguration *setting)
    {
        ++readCalls;
        lastReadSetting = setting;
        if (trace) {
            trace->events.append(QStringLiteral("plain-%1-read").arg(Id));
        }
        return readResult;
    }

    void write(KisPropertiesConfiguration *setting) const
    {
        ++writeCalls;
        lastWriteSetting = setting;
        if (trace) {
            trace->events.append(QStringLiteral("plain-%1-write").arg(Id));
        }
    }

    friend bool operator==(const PlainOptionData &lhs, const PlainOptionData &rhs)
    {
        return lhs.equalityValue == rhs.equalityValue;
    }

    CallTrace *trace = nullptr;
    bool readResult = true;
    int readCalls = 0;
    const KisPropertiesConfiguration *lastReadSetting = nullptr;
    mutable int writeCalls = 0;
    mutable KisPropertiesConfiguration *lastWriteSetting = nullptr;
    int constructorValue = -1;
    QString constructorLabel;
    int equalityValue = 0;
};

template<int Id>
struct PrefixedOptionData {
    static constexpr bool supports_prefix = true;

    explicit PrefixedOptionData(const QString &prefix)
        : constructorPrefix(prefix)
    {
        ++prefixedPrefixConstructions[Id];
    }

    PrefixedOptionData(const QString &prefix, int value)
        : constructorPrefix(prefix)
        , constructorValue(value)
    {
        ++prefixedArgumentConstructions[Id];
    }

    bool read(const KisPropertiesConfiguration *setting)
    {
        ++readCalls;
        lastReadSetting = setting;
        if (trace) {
            trace->events.append(QStringLiteral("prefixed-%1-read").arg(Id));
        }
        return readResult;
    }

    void write(KisPropertiesConfiguration *setting) const
    {
        ++writeCalls;
        lastWriteSetting = setting;
        if (trace) {
            trace->events.append(QStringLiteral("prefixed-%1-write").arg(Id));
        }
    }

    friend bool operator==(const PrefixedOptionData &lhs, const PrefixedOptionData &rhs)
    {
        return lhs.equalityValue == rhs.equalityValue;
    }

    CallTrace *trace = nullptr;
    bool readResult = true;
    int readCalls = 0;
    const KisPropertiesConfiguration *lastReadSetting = nullptr;
    mutable int writeCalls = 0;
    mutable KisPropertiesConfiguration *lastWriteSetting = nullptr;
    QString constructorPrefix;
    int constructorValue = -1;
    int equalityValue = 0;
};

struct ExplicitlyUnprefixedOptionData {
    static constexpr bool supports_prefix = false;
};

using PlainTuple = KisOptionTuple<PlainOptionData<1>, PlainOptionData<2>, PlainOptionData<3>>;
using PrefixedTuple = KisOptionTuple<PrefixedOptionData<1>, PrefixedOptionData<2>, PrefixedOptionData<3>>;

void resetConstructionCounters()
{
    plainDefaultConstructions.fill(0);
    plainArgumentConstructions.fill(0);
    prefixedPrefixConstructions.fill(0);
    prefixedArgumentConstructions.fill(0);
}

template<template<int> class OptionData, typename Tuple>
void attachTraceToEveryBase(Tuple &tuple, CallTrace *trace)
{
    static_cast<OptionData<1> &>(tuple).trace = trace;
    static_cast<OptionData<2> &>(tuple).trace = trace;
    static_cast<OptionData<3> &>(tuple).trace = trace;
}

} // namespace

class KisOptionTupleContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void traitsAndAliasSelectPrefixMode();
    void nonPrefixConstructionRoutesArgumentsOnlyToFirstBase();
    void prefixedConstructionRoutesUnicodePrefixToEveryBase();
    void readPreservesPointerOrderShortCircuitAndReturn();
    void writePreservesPointerAndVisitsEveryBaseInOrder();
    void equalityComparesEveryBaseValue();
};

void KisOptionTupleContractTest::traitsAndAliasSelectPrefixMode()
{
    static_assert(!detail::supports_prefix<PlainOptionData<1>>::value);
    static_assert(!detail::supports_prefix<ExplicitlyUnprefixedOptionData>::value);
    static_assert(detail::supports_prefix<PrefixedOptionData<1>>::value);
    static_assert(detail::all_support_prefix<PrefixedOptionData<1>, PrefixedOptionData<2>>::value);
    static_assert(!detail::all_support_prefix<PrefixedOptionData<1>, PlainOptionData<1>>::value);
    static_assert(detail::none_support_prefix<PlainOptionData<1>, ExplicitlyUnprefixedOptionData>::value);
    static_assert(!detail::none_support_prefix<PlainOptionData<1>, PrefixedOptionData<1>>::value);
    static_assert(std::is_same_v<
                  PlainTuple,
                  detail::KisOptionTupleImpl<false, true, PlainOptionData<1>, PlainOptionData<2>, PlainOptionData<3>>>);
    static_assert(
        std::is_same_v<
            PrefixedTuple,
            detail::
                KisOptionTupleImpl<true, false, PrefixedOptionData<1>, PrefixedOptionData<2>, PrefixedOptionData<3>>>);

    QVERIFY(!detail::supports_prefix<PlainOptionData<1>>::value);
    QVERIFY(detail::supports_prefix<PrefixedOptionData<1>>::value);
    QVERIFY((detail::all_support_prefix<PrefixedOptionData<1>, PrefixedOptionData<2>>::value));
    QVERIFY((detail::none_support_prefix<PlainOptionData<1>, ExplicitlyUnprefixedOptionData>::value));
}

void KisOptionTupleContractTest::nonPrefixConstructionRoutesArgumentsOnlyToFirstBase()
{
    resetConstructionCounters();
    const QString label = QString::fromUtf8("先頭・α");

    PlainTuple tuple(41, label);

    const auto &first = static_cast<const PlainOptionData<1> &>(tuple);
    const auto &second = static_cast<const PlainOptionData<2> &>(tuple);
    const auto &third = static_cast<const PlainOptionData<3> &>(tuple);
    QCOMPARE(first.constructorValue, 41);
    QCOMPARE(first.constructorLabel, label);
    QCOMPARE(second.constructorValue, -1);
    QVERIFY(second.constructorLabel.isEmpty());
    QCOMPARE(third.constructorValue, -1);
    QVERIFY(third.constructorLabel.isEmpty());
    QCOMPARE(plainArgumentConstructions[1], 1);
    QCOMPARE(plainArgumentConstructions[2], 0);
    QCOMPARE(plainArgumentConstructions[3], 0);
    QCOMPARE(plainDefaultConstructions[1], 0);
    QCOMPARE(plainDefaultConstructions[2], 1);
    QCOMPARE(plainDefaultConstructions[3], 1);
}

void KisOptionTupleContractTest::prefixedConstructionRoutesUnicodePrefixToEveryBase()
{
    resetConstructionCounters();
    const QString prefix = QString::fromUtf8("覆面/β・設定/");

    PrefixedTuple tuple(prefix, 73);

    const auto &first = static_cast<const PrefixedOptionData<1> &>(tuple);
    const auto &second = static_cast<const PrefixedOptionData<2> &>(tuple);
    const auto &third = static_cast<const PrefixedOptionData<3> &>(tuple);
    QCOMPARE(first.constructorPrefix, prefix);
    QCOMPARE(second.constructorPrefix, prefix);
    QCOMPARE(third.constructorPrefix, prefix);
    QCOMPARE(first.constructorValue, 73);
    QCOMPARE(second.constructorValue, -1);
    QCOMPARE(third.constructorValue, -1);
    QCOMPARE(prefixedArgumentConstructions[1], 1);
    QCOMPARE(prefixedArgumentConstructions[2], 0);
    QCOMPARE(prefixedArgumentConstructions[3], 0);
    QCOMPARE(prefixedPrefixConstructions[1], 0);
    QCOMPARE(prefixedPrefixConstructions[2], 1);
    QCOMPARE(prefixedPrefixConstructions[3], 1);
}

void KisOptionTupleContractTest::readPreservesPointerOrderShortCircuitAndReturn()
{
    KisPropertiesConfiguration setting;
    CallTrace trace;
    PlainTuple plainTuple(1, QStringLiteral("plain"));
    attachTraceToEveryBase<PlainOptionData>(plainTuple, &trace);
    static_cast<PlainOptionData<2> &>(plainTuple).readResult = false;

    QVERIFY(!plainTuple.read(&setting));
    QCOMPARE(trace.events, QStringList({QStringLiteral("plain-1-read"), QStringLiteral("plain-2-read")}));
    QCOMPARE(static_cast<PlainOptionData<1> &>(plainTuple).lastReadSetting, &setting);
    QCOMPARE(static_cast<PlainOptionData<2> &>(plainTuple).lastReadSetting, &setting);
    QCOMPARE(static_cast<PlainOptionData<3> &>(plainTuple).readCalls, 0);

    trace.events.clear();
    const QString prefix = QString::fromUtf8("接頭/γ/");
    PrefixedTuple prefixedTuple(prefix, 2);
    attachTraceToEveryBase<PrefixedOptionData>(prefixedTuple, &trace);

    QVERIFY(prefixedTuple.read(&setting));
    QCOMPARE(
        trace.events,
        QStringList(
            {QStringLiteral("prefixed-1-read"), QStringLiteral("prefixed-2-read"), QStringLiteral("prefixed-3-read")}));
    QCOMPARE(static_cast<PrefixedOptionData<1> &>(prefixedTuple).lastReadSetting, &setting);
    QCOMPARE(static_cast<PrefixedOptionData<2> &>(prefixedTuple).lastReadSetting, &setting);
    QCOMPARE(static_cast<PrefixedOptionData<3> &>(prefixedTuple).lastReadSetting, &setting);
}

void KisOptionTupleContractTest::writePreservesPointerAndVisitsEveryBaseInOrder()
{
    KisPropertiesConfiguration setting;
    CallTrace trace;
    PlainTuple plainTuple(1, QStringLiteral("plain"));
    attachTraceToEveryBase<PlainOptionData>(plainTuple, &trace);

    plainTuple.write(&setting);
    QCOMPARE(trace.events,
             QStringList(
                 {QStringLiteral("plain-1-write"), QStringLiteral("plain-2-write"), QStringLiteral("plain-3-write")}));
    QCOMPARE(static_cast<const PlainOptionData<1> &>(plainTuple).lastWriteSetting, &setting);
    QCOMPARE(static_cast<const PlainOptionData<2> &>(plainTuple).lastWriteSetting, &setting);
    QCOMPARE(static_cast<const PlainOptionData<3> &>(plainTuple).lastWriteSetting, &setting);

    trace.events.clear();
    PrefixedTuple prefixedTuple(QString::fromUtf8("接頭/δ/"), 2);
    attachTraceToEveryBase<PrefixedOptionData>(prefixedTuple, &trace);

    prefixedTuple.write(&setting);
    QCOMPARE(trace.events,
             QStringList({QStringLiteral("prefixed-1-write"),
                          QStringLiteral("prefixed-2-write"),
                          QStringLiteral("prefixed-3-write")}));
    QCOMPARE(static_cast<const PrefixedOptionData<1> &>(prefixedTuple).lastWriteSetting, &setting);
    QCOMPARE(static_cast<const PrefixedOptionData<2> &>(prefixedTuple).lastWriteSetting, &setting);
    QCOMPARE(static_cast<const PrefixedOptionData<3> &>(prefixedTuple).lastWriteSetting, &setting);
}

void KisOptionTupleContractTest::equalityComparesEveryBaseValue()
{
    PlainTuple plainLeft(1, QStringLiteral("left"));
    PlainTuple plainRight(2, QStringLiteral("right"));
    QVERIFY(plainLeft == plainRight);

    static_cast<PlainOptionData<1> &>(plainRight).equalityValue = 1;
    QVERIFY(!(plainLeft == plainRight));
    static_cast<PlainOptionData<1> &>(plainRight).equalityValue = 0;
    static_cast<PlainOptionData<2> &>(plainRight).equalityValue = 2;
    QVERIFY(!(plainLeft == plainRight));
    static_cast<PlainOptionData<2> &>(plainRight).equalityValue = 0;
    static_cast<PlainOptionData<3> &>(plainRight).equalityValue = 3;
    QVERIFY(!(plainLeft == plainRight));

    PrefixedTuple prefixedLeft(QStringLiteral("left/"), 1);
    PrefixedTuple prefixedRight(QStringLiteral("right/"), 2);
    QVERIFY(prefixedLeft == prefixedRight);

    static_cast<PrefixedOptionData<1> &>(prefixedRight).equalityValue = 1;
    QVERIFY(!(prefixedLeft == prefixedRight));
    static_cast<PrefixedOptionData<1> &>(prefixedRight).equalityValue = 0;
    static_cast<PrefixedOptionData<2> &>(prefixedRight).equalityValue = 2;
    QVERIFY(!(prefixedLeft == prefixedRight));
    static_cast<PrefixedOptionData<2> &>(prefixedRight).equalityValue = 0;
    static_cast<PrefixedOptionData<3> &>(prefixedRight).equalityValue = 3;
    QVERIFY(!(prefixedLeft == prefixedRight));
}

QTEST_GUILESS_MAIN(KisOptionTupleContractTest)

#include "KisOptionTupleContractTest.moc"
