/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisPaintOpOptionWidgetUtils.h"

#include <QTest>

#include <memory>
#include <optional>
#include <utility>

namespace
{
struct BaseData {
    int value = 0;
};

struct DerivedData : BaseData {
    int extra = 0;
};

struct PlainData {
    int value = 0;
};

struct LodData {
    int value = 0;

    KisPaintopLodLimitations lodLimitations() const
    {
        return {};
    }
};

struct PlainWidget {
    template<typename Cursor>
    PlainWidget(Cursor &&cursor, int forwardedArgument)
        : observedValue(cursor.get().value)
        , forwardedArgument(forwardedArgument)
    {
    }

    virtual ~PlainWidget() = default;

    int observedValue = 0;
    int forwardedArgument = 0;
};

struct BaseWidget {
    using data_type = BaseData;

    BaseWidget(lager::cursor<BaseData> cursor, int forwardedArgument)
        : observedValue(cursor.get().value)
        , forwardedArgument(forwardedArgument)
    {
    }

    virtual ~BaseWidget() = default;

    int observedValue = 0;
    int forwardedArgument = 0;
};

struct DefaultWidget {
    using data_type = PlainData;

    template<typename Cursor>
    explicit DefaultWidget(Cursor &&cursor)
        : observedValue(cursor.get().value)
    {
    }

    virtual ~DefaultWidget() = default;

    int observedValue = -1;
};

struct LodWidget {
    using data_type = LodData;

    template<typename Cursor>
    explicit LodWidget(Cursor &&cursor)
        : observedValue(cursor.get().value)
    {
    }

    template<typename Cursor>
    LodWidget(Cursor &&cursor, int forwardedArgument)
        : observedValue(cursor.get().value)
        , forwardedArgument(forwardedArgument)
    {
    }

    virtual ~LodWidget() = default;

    virtual KisPaintOpOption::OptionalLodLimitationsReader lodLimitationsReader() const
    {
        return std::nullopt;
    }

    int observedValue = 0;
    int forwardedArgument = 0;
};

void verifyEmptyLodLimitations(const KisPaintOpOption::OptionalLodLimitationsReader &reader)
{
    QVERIFY(reader);
    QVERIFY((**reader).limitations.isEmpty());
    QVERIFY((**reader).blockers.isEmpty());
}

} // namespace

class KisPaintOpOptionWidgetUtilsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void optionWidgetFactoriesPreserveInitialValues();
    void factoryAdaptsDerivedData();
};

void KisPaintOpOptionWidgetUtilsContractTest::optionWidgetFactoriesPreserveInitialValues()
{
    std::unique_ptr<PlainWidget> explicitWidget(
        KisPaintOpOptionWidgetUtils::createOptionWidget<PlainWidget>(PlainData{29}, 31));
    QCOMPARE(explicitWidget->observedValue, 29);
    QCOMPARE(explicitWidget->forwardedArgument, 31);

    std::unique_ptr<DefaultWidget> defaultWidget(KisPaintOpOptionWidgetUtils::createOptionWidget<DefaultWidget>());
    QCOMPARE(defaultWidget->observedValue, 0);

    std::unique_ptr<LodWidget> explicitLodWidget(
        KisPaintOpOptionWidgetUtils::createOptionWidgetWithLodLimitations<LodWidget>(LodData{37}, 41));
    QCOMPARE(explicitLodWidget->observedValue, 37);
    QCOMPARE(explicitLodWidget->forwardedArgument, 41);
    verifyEmptyLodLimitations(explicitLodWidget->lodLimitationsReader());

    std::unique_ptr<LodWidget> defaultLodWidget(
        KisPaintOpOptionWidgetUtils::createOptionWidgetWithLodLimitations<LodWidget>());
    QCOMPARE(defaultLodWidget->observedValue, 0);
    verifyEmptyLodLimitations(defaultLodWidget->lodLimitationsReader());
}

void KisPaintOpOptionWidgetUtilsContractTest::factoryAdaptsDerivedData()
{
    DerivedData data;
    data.value = 13;
    data.extra = 17;
    std::unique_ptr<BaseWidget> widget(
        KisPaintOpOptionWidgetUtils::createOptionWidget<BaseWidget>(std::move(data), 19));
    QCOMPARE(widget->observedValue, 13);
    QCOMPARE(widget->forwardedArgument, 19);
}

QTEST_GUILESS_MAIN(KisPaintOpOptionWidgetUtilsContractTest)

#include "KisPaintOpOptionWidgetUtilsContractTest.moc"
