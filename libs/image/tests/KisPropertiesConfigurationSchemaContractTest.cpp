/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "filter/kis_filter_configuration.h"
#include "kis_properties_configuration.h"

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{

#define ASSERT_PROPERTIES_CONFIGURATION_SIGNATURE(method, ...)                                                         \
    static_assert(std::is_same_v<decltype(static_cast<__VA_ARGS__>(&KisPropertiesConfiguration::method)), __VA_ARGS__>)
#define ASSERT_FILTER_CONFIGURATION_SIGNATURE(method, ...)                                                             \
    static_assert(std::is_same_v<decltype(static_cast<__VA_ARGS__>(&KisFilterConfiguration::method)), __VA_ARGS__>)

} // namespace

class KisPropertiesConfigurationSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void propertyMapSurfaceAndMutationSignaturesRemainStable();
    void typedPropertyConversionAndDefaultSignaturesRemainStable();
    void prefixedPropertyTransferSignaturesRemainStable();
    void propertyXmlSerializationSignaturesRemainStable();
    void propertyKeyAndStringEncodingSignaturesRemainStable();
    void filterConfigurationTypeAndLifetimeSchemaRemainStable();
    void filterConfigurationIdentityAndChannelSignaturesRemainStable();
    void filterConfigurationSerializationSignaturesRemainStable();
    void filterConfigurationResourceSnapshotSignaturesRemainStable();
    void filterConfigurationRequiredResourceSignaturesRemainStable();
};

void KisPropertiesConfigurationSchemaContractTest::propertyMapSurfaceAndMutationSignaturesRemainStable()
{
    static_assert(std::is_class_v<KisPropertiesConfiguration>);
    ASSERT_PROPERTIES_CONFIGURATION_SIGNATURE(clearProperties, void (KisPropertiesConfiguration::*)());
    ASSERT_PROPERTIES_CONFIGURATION_SIGNATURE(compareTo,
                                              bool (KisPropertiesConfiguration::*)(const KisPropertiesConfiguration *)
                                                  const);
    ASSERT_PROPERTIES_CONFIGURATION_SIGNATURE(getProperties,
                                              QMap<QString, QVariant> (KisPropertiesConfiguration::*)() const);
    ASSERT_PROPERTIES_CONFIGURATION_SIGNATURE(getPropertiesKeys,
                                              QList<QString> (KisPropertiesConfiguration::*)() const);
    ASSERT_PROPERTIES_CONFIGURATION_SIGNATURE(getProperty,
                                              QVariant (KisPropertiesConfiguration::*)(const QString &) const);
    ASSERT_PROPERTIES_CONFIGURATION_SIGNATURE(getProperty,
                                              bool (KisPropertiesConfiguration::*)(const QString &, QVariant &) const);
    ASSERT_PROPERTIES_CONFIGURATION_SIGNATURE(hasProperty, bool (KisPropertiesConfiguration::*)(const QString &) const);
    ASSERT_PROPERTIES_CONFIGURATION_SIGNATURE(removeProperty, void (KisPropertiesConfiguration::*)(const QString &));
    ASSERT_PROPERTIES_CONFIGURATION_SIGNATURE(setProperty,
                                              void (KisPropertiesConfiguration::*)(const QString &, const QVariant &));
    ASSERT_PROPERTIES_CONFIGURATION_SIGNATURE(setPropertyNotSaved,
                                              void (KisPropertiesConfiguration::*)(const QString &));

    QVERIFY(true);
}

void KisPropertiesConfigurationSchemaContractTest::typedPropertyConversionAndDefaultSignaturesRemainStable()
{
    ASSERT_PROPERTIES_CONFIGURATION_SIGNATURE(getBool,
                                              bool (KisPropertiesConfiguration::*)(const QString &, bool) const);
    ASSERT_PROPERTIES_CONFIGURATION_SIGNATURE(getColor,
                                              KoColor (KisPropertiesConfiguration::*)(const QString &, const KoColor &)
                                                  const);
    ASSERT_PROPERTIES_CONFIGURATION_SIGNATURE(
        getCubicCurve,
        KisCubicCurve (KisPropertiesConfiguration::*)(const QString &, const KisCubicCurve &) const);
    ASSERT_PROPERTIES_CONFIGURATION_SIGNATURE(getDouble,
                                              double (KisPropertiesConfiguration::*)(const QString &, double) const);
    ASSERT_PROPERTIES_CONFIGURATION_SIGNATURE(getFloat,
                                              float (KisPropertiesConfiguration::*)(const QString &, float) const);
    ASSERT_PROPERTIES_CONFIGURATION_SIGNATURE(getInt, int (KisPropertiesConfiguration::*)(const QString &, int) const);
    ASSERT_PROPERTIES_CONFIGURATION_SIGNATURE(
        getPropertyLazy,
        QStringList (KisPropertiesConfiguration::*)(const QString &, const QStringList &) const);
    ASSERT_PROPERTIES_CONFIGURATION_SIGNATURE(getPropertyLazy<int>,
                                              int (KisPropertiesConfiguration::*)(const QString &, const int &) const);
    ASSERT_PROPERTIES_CONFIGURATION_SIGNATURE(getPropertyLazy,
                                              QString (KisPropertiesConfiguration::*)(const QString &, const char *)
                                                  const);
    ASSERT_PROPERTIES_CONFIGURATION_SIGNATURE(getString,
                                              QString (KisPropertiesConfiguration::*)(const QString &, const QString &)
                                                  const);
    ASSERT_PROPERTIES_CONFIGURATION_SIGNATURE(
        getStringList,
        QStringList (KisPropertiesConfiguration::*)(const QString &, const QStringList &) const);
    ASSERT_PROPERTIES_CONFIGURATION_SIGNATURE(
        setProperty,
        void (KisPropertiesConfiguration::*)(const QString &, const QStringList &));

    static_assert(std::is_same_v<decltype(std::declval<const KisPropertiesConfiguration &>().getBool(
                                     std::declval<const QString &>())),
                                 bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisPropertiesConfiguration &>().getColor(
                                     std::declval<const QString &>())),
                                 KoColor>);
    static_assert(std::is_same_v<decltype(std::declval<const KisPropertiesConfiguration &>().getCubicCurve(
                                     std::declval<const QString &>())),
                                 KisCubicCurve>);
    static_assert(std::is_same_v<decltype(std::declval<const KisPropertiesConfiguration &>().getDouble(
                                     std::declval<const QString &>())),
                                 double>);
    static_assert(std::is_same_v<decltype(std::declval<const KisPropertiesConfiguration &>().getFloat(
                                     std::declval<const QString &>())),
                                 float>);
    static_assert(std::is_same_v<decltype(std::declval<const KisPropertiesConfiguration &>().getInt(
                                     std::declval<const QString &>())),
                                 int>);
    static_assert(std::is_same_v<decltype(std::declval<const KisPropertiesConfiguration &>().getString(
                                     std::declval<const QString &>())),
                                 QString>);
    static_assert(std::is_same_v<decltype(std::declval<const KisPropertiesConfiguration &>().getStringList(
                                     std::declval<const QString &>())),
                                 QStringList>);

    QVERIFY(true);
}

void KisPropertiesConfigurationSchemaContractTest::prefixedPropertyTransferSignaturesRemainStable()
{
    ASSERT_PROPERTIES_CONFIGURATION_SIGNATURE(
        getPrefixedProperties,
        void (KisPropertiesConfiguration::*)(const QString &, KisPropertiesConfiguration *) const);
    ASSERT_PROPERTIES_CONFIGURATION_SIGNATURE(
        getPrefixedProperties,
        void (KisPropertiesConfiguration::*)(const QString &, KisPropertiesConfigurationSP) const);
    ASSERT_PROPERTIES_CONFIGURATION_SIGNATURE(
        setPrefixedProperties,
        void (KisPropertiesConfiguration::*)(const QString &, const KisPropertiesConfiguration *));
    ASSERT_PROPERTIES_CONFIGURATION_SIGNATURE(
        setPrefixedProperties,
        void (KisPropertiesConfiguration::*)(const QString &, KisPropertiesConfigurationSP));

    QVERIFY(true);
}

void KisPropertiesConfigurationSchemaContractTest::propertyXmlSerializationSignaturesRemainStable()
{
    ASSERT_PROPERTIES_CONFIGURATION_SIGNATURE(fromXML, void (KisPropertiesConfiguration::*)(const QDomElement &));
    ASSERT_PROPERTIES_CONFIGURATION_SIGNATURE(fromXML, bool (KisPropertiesConfiguration::*)(const QString &, bool));
    ASSERT_PROPERTIES_CONFIGURATION_SIGNATURE(toXML, QString (KisPropertiesConfiguration::*)() const);
    ASSERT_PROPERTIES_CONFIGURATION_SIGNATURE(toXML,
                                              void (KisPropertiesConfiguration::*)(QDomDocument &, QDomElement &)
                                                  const);

    static_assert(
        std::is_same_v<decltype(std::declval<KisPropertiesConfiguration &>().fromXML(std::declval<const QString &>())),
                       bool>);

    QVERIFY(true);
}

void KisPropertiesConfigurationSchemaContractTest::propertyKeyAndStringEncodingSignaturesRemainStable()
{
    ASSERT_PROPERTIES_CONFIGURATION_SIGNATURE(escapeString, QString (*)(const QString &));
    ASSERT_PROPERTIES_CONFIGURATION_SIGNATURE(extractedPrefixKey, QString (*)());
    ASSERT_PROPERTIES_CONFIGURATION_SIGNATURE(unescapeString, QString (*)(const QString &));

    QVERIFY(true);
}

// clang-format off
void KisPropertiesConfigurationSchemaContractTest::filterConfigurationTypeAndLifetimeSchemaRemainStable()
// clang-format on
{
    static_assert(std::is_class_v<KisFilterConfiguration>);
    static_assert(std::is_base_of_v<KisPropertiesConfiguration, KisFilterConfiguration>);
    static_assert(std::is_constructible_v<KisFilterConfiguration, const QString &, qint32, KisResourcesInterfaceSP>);
    static_assert(std::is_destructible_v<KisFilterConfiguration>);
    static_assert(std::has_virtual_destructor_v<KisFilterConfiguration>);

    QVERIFY(true);
}

// clang-format off
void KisPropertiesConfigurationSchemaContractTest::filterConfigurationIdentityAndChannelSignaturesRemainStable()
// clang-format on
{
    ASSERT_FILTER_CONFIGURATION_SIGNATURE(channelFlags, QBitArray (KisFilterConfiguration::*)() const);
    ASSERT_FILTER_CONFIGURATION_SIGNATURE(isCompatible, bool (KisFilterConfiguration::*)(const KisPaintDeviceSP) const);
    ASSERT_FILTER_CONFIGURATION_SIGNATURE(name, const QString &(KisFilterConfiguration::*)() const);
    ASSERT_FILTER_CONFIGURATION_SIGNATURE(setChannelFlags, void (KisFilterConfiguration::*)(QBitArray));
    ASSERT_FILTER_CONFIGURATION_SIGNATURE(version, qint32 (KisFilterConfiguration::*)() const);

    QVERIFY(true);
}

// clang-format off
void KisPropertiesConfigurationSchemaContractTest::filterConfigurationSerializationSignaturesRemainStable()
// clang-format on
{
    ASSERT_FILTER_CONFIGURATION_SIGNATURE(clone, KisFilterConfigurationSP (KisFilterConfiguration::*)() const);
    ASSERT_FILTER_CONFIGURATION_SIGNATURE(compareTo,
                                          bool (KisFilterConfiguration::*)(const KisPropertiesConfiguration *) const);
    ASSERT_FILTER_CONFIGURATION_SIGNATURE(fromLegacyXML, void (KisFilterConfiguration::*)(const QDomElement &));
    ASSERT_FILTER_CONFIGURATION_SIGNATURE(fromXML, void (KisFilterConfiguration::*)(const QDomElement &));
    ASSERT_FILTER_CONFIGURATION_SIGNATURE(toXML, void (KisFilterConfiguration::*)(QDomDocument &, QDomElement &) const);

    QVERIFY(true);
}

// clang-format off
void KisPropertiesConfigurationSchemaContractTest::filterConfigurationResourceSnapshotSignaturesRemainStable()
// clang-format on
{
    ASSERT_FILTER_CONFIGURATION_SIGNATURE(cloneWithResourcesSnapshot,
                                          KisFilterConfigurationSP (KisFilterConfiguration::*)(KisResourcesInterfaceSP)
                                              const);
    ASSERT_FILTER_CONFIGURATION_SIGNATURE(createLocalResourcesSnapshot,
                                          void (KisFilterConfiguration::*)(KisResourcesInterfaceSP));
    ASSERT_FILTER_CONFIGURATION_SIGNATURE(hasLocalResourcesSnapshot, bool (KisFilterConfiguration::*)() const);
    ASSERT_FILTER_CONFIGURATION_SIGNATURE(resourcesInterface,
                                          KisResourcesInterfaceSP (KisFilterConfiguration::*)() const);
    ASSERT_FILTER_CONFIGURATION_SIGNATURE(setResourcesInterface,
                                          void (KisFilterConfiguration::*)(KisResourcesInterfaceSP));

    static_assert(std::is_same_v<decltype(std::declval<const KisFilterConfiguration &>().cloneWithResourcesSnapshot()),
                                 KisFilterConfigurationSP>);
    static_assert(
        std::is_same_v<decltype(std::declval<KisFilterConfiguration &>().createLocalResourcesSnapshot()), void>);

    QVERIFY(true);
}

// clang-format off
void KisPropertiesConfigurationSchemaContractTest::filterConfigurationRequiredResourceSignaturesRemainStable()
// clang-format on
{
    using ResourceResults = QList<KoResourceLoadResult>;

    ASSERT_FILTER_CONFIGURATION_SIGNATURE(embeddedResources,
                                          ResourceResults (KisFilterConfiguration::*)(KisResourcesInterfaceSP) const);
    ASSERT_FILTER_CONFIGURATION_SIGNATURE(linkedResources,
                                          ResourceResults (KisFilterConfiguration::*)(KisResourcesInterfaceSP) const);
    ASSERT_FILTER_CONFIGURATION_SIGNATURE(requiredResources,
                                          ResourceResults (KisFilterConfiguration::*)(KisResourcesInterfaceSP) const);

    QVERIFY(true);
}

QTEST_MAIN(KisPropertiesConfigurationSchemaContractTest)

#include "KisPropertiesConfigurationSchemaContractTest.moc"
