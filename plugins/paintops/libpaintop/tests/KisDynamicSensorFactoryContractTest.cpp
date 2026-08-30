/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <functional>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

#include <KisDynamicSensorFactory.h>

#include <QTest>

namespace
{

struct FactoryObserver {
    int destructionCount = 0;
    int maximumValueLength = 0;
    int maximumLabelLength = 0;
    int widgetCallCount = 0;
    bool cursorWasInitialized = true;
    QWidget *parent = nullptr;
};

class DynamicSensorFactoryProbe final : public KisDynamicSensorFactory
{
public:
    DynamicSensorFactoryProbe(FactoryObserver *observer, QWidget *widgetResult)
        : m_observer(observer)
        , m_widgetResult(widgetResult)
    {
    }

    ~DynamicSensorFactoryProbe() override
    {
        ++m_observer->destructionCount;
    }

    QString id() const override
    {
        return QStringLiteral("筆圧/Δ");
    }

    int minimumValue() override
    {
        return -17;
    }

    int maximumValue(int length) override
    {
        m_observer->maximumValueLength = length;
        return 109;
    }

    QString minimumLabel() override
    {
        return QStringLiteral("最小-α");
    }

    QString maximumLabel(int length) override
    {
        m_observer->maximumLabelLength = length;
        return QStringLiteral("最大-β");
    }

    QString valueSuffix() override
    {
        return QStringLiteral("°/秒");
    }

    QWidget *createConfigWidget(lager::cursor<KisCurveOptionDataCommon> data, QWidget *parent) override
    {
        ++m_observer->widgetCallCount;
        m_observer->cursorWasInitialized = static_cast<bool>(data);
        m_observer->parent = parent;
        return m_widgetResult;
    }

private:
    FactoryObserver *m_observer;
    QWidget *m_widgetResult;
};

} // namespace

class KisDynamicSensorFactoryContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void accessorsAndWidgetFactoryPreserveConfiguredValuesAndInputs();
    void baseLifetimeIsVirtual();
};

void KisDynamicSensorFactoryContractTest::accessorsAndWidgetFactoryPreserveConfiguredValuesAndInputs()
{
    FactoryObserver observer;
    int parentToken = 41;
    int resultToken = 53;
    auto *parent = reinterpret_cast<QWidget *>(&parentToken);
    auto *widgetResult = reinterpret_cast<QWidget *>(&resultToken);
    DynamicSensorFactoryProbe factory(&observer, widgetResult);

    QCOMPARE(factory.id(), QStringLiteral("筆圧/Δ"));
    QCOMPARE(factory.minimumValue(), -17);
    QCOMPARE(factory.maximumValue(83), 109);
    QCOMPARE(observer.maximumValueLength, 83);
    QCOMPARE(factory.minimumLabel(), QStringLiteral("最小-α"));
    QCOMPARE(factory.maximumLabel(127), QStringLiteral("最大-β"));
    QCOMPARE(observer.maximumLabelLength, 127);
    QCOMPARE(factory.valueSuffix(), QStringLiteral("°/秒"));

    lager::cursor<KisCurveOptionDataCommon> cursor;
    QVERIFY(!cursor);
    QCOMPARE(factory.createConfigWidget(cursor, parent), widgetResult);
    QCOMPARE(observer.widgetCallCount, 1);
    QVERIFY(!observer.cursorWasInitialized);
    QCOMPARE(observer.parent, parent);
    QCOMPARE(parentToken, 41);
    QCOMPARE(resultToken, 53);
}

void KisDynamicSensorFactoryContractTest::baseLifetimeIsVirtual()
{
    FactoryObserver observer;
    KisDynamicSensorFactory *factory = new DynamicSensorFactoryProbe(&observer, nullptr);

    delete factory;

    QCOMPARE(observer.destructionCount, 1);
}

QTEST_GUILESS_MAIN(KisDynamicSensorFactoryContractTest)

#include "KisDynamicSensorFactoryContractTest.moc"
