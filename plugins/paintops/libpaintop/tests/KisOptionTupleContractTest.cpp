/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QMap>
#include <QString>
#include <QTest>

class KisPropertiesConfiguration
{
public:
    QMap<QString, int> values;
};

#include "KisOptionTuple.h"

namespace
{

template<int Id>
struct PlainOptionData {
    PlainOptionData() = default;
    explicit PlainOptionData(int value)
        : value(value)
    {
    }

    bool read(const KisPropertiesConfiguration *setting)
    {
        const auto it = setting->values.constFind(key());
        if (it == setting->values.constEnd()) {
            return false;
        }
        value = *it;
        return true;
    }

    void write(KisPropertiesConfiguration *setting) const
    {
        setting->values.insert(key(), value);
    }

    friend bool operator==(const PlainOptionData &lhs, const PlainOptionData &rhs)
    {
        return lhs.value == rhs.value;
    }

    QString key() const
    {
        return prefix + QString::number(Id);
    }
    QString prefix;
    int value = 0;
};

template<int Id>
struct PrefixedOptionData : PlainOptionData<Id> {
    static constexpr bool supports_prefix = true;

    explicit PrefixedOptionData(const QString &prefix, int value = 0)
        : PlainOptionData<Id>(value)
    {
        this->prefix = prefix;
    }
};

using PlainTuple = KisOptionTuple<PlainOptionData<1>, PlainOptionData<2>, PlainOptionData<3>>;
using PrefixedTuple = KisOptionTuple<PrefixedOptionData<1>, PrefixedOptionData<2>, PrefixedOptionData<3>>;

} // namespace

class KisOptionTupleContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructorArgumentsInitializeFirstOption();
    void prefixAppliesToEveryOption();
    void readsEveryOption();
    void reportsFailureInAnyOption_data();
    void reportsFailureInAnyOption();
    void writesEveryOptionAndPreservesOtherSettings();
    void equalityIncludesEveryOption();
};

void KisOptionTupleContractTest::constructorArgumentsInitializeFirstOption()
{
    PlainTuple tuple(41);
    QCOMPARE(static_cast<const PlainOptionData<1> &>(tuple).value, 41);
    QCOMPARE(static_cast<const PlainOptionData<2> &>(tuple).value, 0);
    QCOMPARE(static_cast<const PlainOptionData<3> &>(tuple).value, 0);
}

void KisOptionTupleContractTest::prefixAppliesToEveryOption()
{
    const QString prefix = QString::fromUtf8("設定/β/");
    PrefixedTuple tuple(prefix, 73);
    KisPropertiesConfiguration setting;
    tuple.write(&setting);
    const QMap<QString, int> expected{{prefix + "1", 73}, {prefix + "2", 0}, {prefix + "3", 0}};
    QCOMPARE(setting.values, expected);

    setting.values = {{prefix + "1", 11}, {prefix + "2", 23}, {prefix + "3", 37}};
    QVERIFY(tuple.read(&setting));
    QCOMPARE(static_cast<const PrefixedOptionData<1> &>(tuple).value, 11);
    QCOMPARE(static_cast<const PrefixedOptionData<2> &>(tuple).value, 23);
    QCOMPARE(static_cast<const PrefixedOptionData<3> &>(tuple).value, 37);
}

void KisOptionTupleContractTest::readsEveryOption()
{
    KisPropertiesConfiguration setting;
    setting.values = {{"1", 11}, {"2", 23}, {"3", 37}};
    PlainTuple tuple;
    QVERIFY(tuple.read(&setting));
    QCOMPARE(static_cast<const PlainOptionData<1> &>(tuple).value, 11);
    QCOMPARE(static_cast<const PlainOptionData<2> &>(tuple).value, 23);
    QCOMPARE(static_cast<const PlainOptionData<3> &>(tuple).value, 37);
}

void KisOptionTupleContractTest::reportsFailureInAnyOption_data()
{
    QTest::addColumn<QString>("missingKey");
    for (int i = 1; i <= 3; ++i) {
        QTest::newRow(qPrintable(QString::number(i))) << QString::number(i);
    }
}

void KisOptionTupleContractTest::reportsFailureInAnyOption()
{
    QFETCH(QString, missingKey);
    KisPropertiesConfiguration setting;
    setting.values = {{"1", 11}, {"2", 23}, {"3", 37}};
    setting.values.remove(missingKey);
    PlainTuple plain;
    QVERIFY(!plain.read(&setting));
    PrefixedTuple prefixed(QString{});
    QVERIFY(!prefixed.read(&setting));
}

void KisOptionTupleContractTest::writesEveryOptionAndPreservesOtherSettings()
{
    PlainTuple tuple;
    static_cast<PlainOptionData<1> &>(tuple).value = 11;
    static_cast<PlainOptionData<2> &>(tuple).value = 23;
    static_cast<PlainOptionData<3> &>(tuple).value = 37;
    KisPropertiesConfiguration setting;
    setting.values.insert("other", 97);
    tuple.write(&setting);
    const QMap<QString, int> expected{{"1", 11}, {"2", 23}, {"3", 37}, {"other", 97}};
    QCOMPARE(setting.values, expected);
}

void KisOptionTupleContractTest::equalityIncludesEveryOption()
{
    const PlainTuple original;
    PlainTuple changed;
    QVERIFY(original == changed);
    static_cast<PlainOptionData<1> &>(changed).value = 11;
    QVERIFY(original != changed);
    changed = original;
    static_cast<PlainOptionData<2> &>(changed).value = 23;
    QVERIFY(original != changed);
    changed = original;
    static_cast<PlainOptionData<3> &>(changed).value = 37;
    QVERIFY(original != changed);

    const PrefixedTuple prefixed(QStringLiteral("prefix/"));
    PrefixedTuple other = prefixed;
    QVERIFY(prefixed == other);
    static_cast<PrefixedOptionData<1> &>(other).value = 11;
    QVERIFY(prefixed != other);
    other = prefixed;
    static_cast<PrefixedOptionData<2> &>(other).value = 23;
    QVERIFY(prefixed != other);
    other = prefixed;
    static_cast<PrefixedOptionData<3> &>(other).value = 37;
    QVERIFY(prefixed != other);
}

QTEST_GUILESS_MAIN(KisOptionTupleContractTest)

#include "KisOptionTupleContractTest.moc"
