/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisPaintOpOptionWidgetUtils.h"

#include <QTest>

#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

namespace
{

namespace detail = KisPaintOpOptionWidgetUtils::detail;

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
    void stateStorageRemainsStable();
    void wrapperSelectionAndConstructionRemainStable();
    void lodWrapperCompositionRemainsStable();
    void optionWidgetFactoriesRemainStable();
    void curveWidgetFactorySignatureRemainsStable();
};

void KisPaintOpOptionWidgetUtilsContractTest::stateStorageRemainsStable()
{
    using Storage = detail::DataStorage<PlainData>;

    static_assert(std::is_class_v<Storage>);
    static_assert(std::is_constructible_v<Storage, PlainData &&>);
    static_assert(std::is_same_v<decltype(Storage::m_data), lager::state<PlainData, lager::automatic_tag>>);

    Storage storage(PlainData{17});
    QCOMPARE(storage.m_data.get().value, 17);
}

void KisPaintOpOptionWidgetUtilsContractTest::wrapperSelectionAndConstructionRemainStable()
{
    using ConversionWrapper = detail::WidgetWrapperConversionChecker<false, PlainWidget, PlainData, int>;
    using DataTypeWrapper = detail::WidgetWrapperDataTypeChecker<false, PlainWidget, PlainData, int>;
    using PlainWrapper = detail::WidgetWrapper<PlainWidget, PlainData, int>;
    using ConvertedWrapper = detail::WidgetWrapper<BaseWidget, DerivedData, int>;

    static_assert(std::is_same_v<typename DataTypeWrapper::BaseClass, ConversionWrapper>);
    static_assert(std::is_same_v<typename PlainWrapper::BaseClass, DataTypeWrapper>);
    static_assert(std::is_base_of_v<PlainWidget, ConversionWrapper>);
    static_assert(std::is_base_of_v<DataTypeWrapper, PlainWrapper>);

    ConversionWrapper conversion(PlainData{3}, 5);
    QCOMPARE(conversion.observedValue, 3);
    QCOMPARE(conversion.forwardedArgument, 5);

    PlainWrapper plain(PlainData{7}, 11);
    QCOMPARE(plain.observedValue, 7);
    QCOMPARE(plain.forwardedArgument, 11);

    DerivedData derived;
    derived.value = 13;
    derived.extra = 17;
    ConvertedWrapper converted(std::move(derived), 19);
    QCOMPARE(converted.observedValue, 13);
    QCOMPARE(converted.forwardedArgument, 19);
    QCOMPARE(converted.m_data.get().extra, 17);
}

void KisPaintOpOptionWidgetUtilsContractTest::lodWrapperCompositionRemainsStable()
{
    using BaseWrapper = detail::WidgetWrapper<LodWidget, LodData>;
    using LodWrapper = detail::WidgetWrapperWithLodLimitations<LodWidget, LodData>;

    static_assert(std::is_same_v<typename LodWrapper::BaseClass, BaseWrapper>);
    static_assert(std::is_base_of_v<BaseWrapper, LodWrapper>);

    LodWrapper wrapper(LodData{23});
    QCOMPARE(wrapper.observedValue, 23);
    verifyEmptyLodLimitations(wrapper.lodLimitationsReader());
}

void KisPaintOpOptionWidgetUtilsContractTest::optionWidgetFactoriesRemainStable()
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

void KisPaintOpOptionWidgetUtilsContractTest::curveWidgetFactorySignatureRemainsStable()
{
    using Factory = KisCurveOptionWidget *(*)(PlainData &&);

    static_assert(
        std::is_same_v<decltype(static_cast<Factory>(&KisPaintOpOptionWidgetUtils::createCurveOptionWidget<PlainData>)),
                       Factory>);
}

QTEST_GUILESS_MAIN(KisPaintOpOptionWidgetUtilsContractTest)

#include "KisPaintOpOptionWidgetUtilsContractTest.moc"
