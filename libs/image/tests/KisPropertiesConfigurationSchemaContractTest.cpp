/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_properties_configuration.h"

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{

#define ASSERT_PROPERTIES_CONFIGURATION_SIGNATURE(method, ...)                                                         \
    static_assert(std::is_same_v<decltype(static_cast<__VA_ARGS__>(&KisPropertiesConfiguration::method)), __VA_ARGS__>)

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

QTEST_MAIN(KisPropertiesConfigurationSchemaContractTest)

#include "KisPropertiesConfigurationSchemaContractTest.moc"
