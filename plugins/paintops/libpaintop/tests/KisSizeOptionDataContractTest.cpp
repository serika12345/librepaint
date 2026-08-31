/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include "KisSizeOptionData.h"

#include <kis_properties_configuration.h>

#include <QHash>
#include <QMap>
#include <QTest>

namespace
{
using Store = QMap<QString, QVariant>;
QHash<const KisPropertiesConfiguration *, Store> &stores()
{
    static QHash<const KisPropertiesConfiguration *, Store> value;
    return value;
}
Store &properties(KisPropertiesConfiguration *value)
{
    return stores()[value];
}
const Store &properties(const KisPropertiesConfiguration *value)
{
    return stores()[value];
}
} // namespace

void kis_assert_exception(const char *, const char *, int)
{
}
void kis_safe_assert_recoverable(const char *, const char *, int)
{
}
KisShared::KisShared()
    : _ref(0)
    , _sharedWeakReference(nullptr)
{
}
KisShared::~KisShared()
{
    delete _sharedWeakReference;
}
KisSerializableConfiguration::KisSerializableConfiguration() = default;
KisSerializableConfiguration::KisSerializableConfiguration(const KisSerializableConfiguration &)
    : KisShared()
{
}
bool KisSerializableConfiguration::fromXML(const QString &, bool)
{
    return false;
}
QString KisSerializableConfiguration::toXML() const
{
    return {};
}
struct KisPropertiesConfiguration::Private {
};
KisPropertiesConfiguration::KisPropertiesConfiguration()
    : d(new Private)
{
    stores().insert(this, {});
}
KisPropertiesConfiguration::~KisPropertiesConfiguration()
{
    stores().remove(this);
    delete d;
}
KisPropertiesConfiguration::KisPropertiesConfiguration(const KisPropertiesConfiguration &rhs)
    : KisSerializableConfiguration(rhs)
    , d(new Private)
{
    stores().insert(this, properties(&rhs));
}
KisPropertiesConfiguration &KisPropertiesConfiguration::operator=(const KisPropertiesConfiguration &rhs)
{
    if (this != &rhs)
        properties(this) = properties(&rhs);
    return *this;
}
bool KisPropertiesConfiguration::fromXML(const QString &, bool)
{
    return false;
}
void KisPropertiesConfiguration::fromXML(const QDomElement &)
{
}
void KisPropertiesConfiguration::toXML(QDomDocument &, QDomElement &) const
{
}
QString KisPropertiesConfiguration::toXML() const
{
    return {};
}
bool KisPropertiesConfiguration::hasProperty(const QString &name) const
{
    return properties(this).contains(name);
}
void KisPropertiesConfiguration::setProperty(const QString &name, const QVariant &value)
{
    properties(this).insert(name, value);
}
bool KisPropertiesConfiguration::getProperty(const QString &name, QVariant &value) const
{
    const auto it = properties(this).constFind(name);
    if (it == properties(this).constEnd())
        return false;
    value = *it;
    return true;
}
QVariant KisPropertiesConfiguration::getProperty(const QString &name) const
{
    return properties(this).value(name);
}
int KisPropertiesConfiguration::getInt(const QString &name, int value) const
{
    const QVariant result = getProperty(name);
    return result.isValid() ? result.toInt() : value;
}
double KisPropertiesConfiguration::getDouble(const QString &name, double value) const
{
    const QVariant result = getProperty(name);
    return result.isValid() ? result.toDouble() : value;
}
bool KisPropertiesConfiguration::getBool(const QString &name, bool value) const
{
    const QVariant result = getProperty(name);
    return result.isValid() ? result.toBool() : value;
}
QString KisPropertiesConfiguration::getString(const QString &name, const QString &value) const
{
    const QVariant result = getProperty(name);
    return result.isValid() ? result.toString() : value;
}
QMap<QString, QVariant> KisPropertiesConfiguration::getProperties() const
{
    return properties(this);
}
QList<QString> KisPropertiesConfiguration::getPropertiesKeys() const
{
    return properties(this).keys();
}
void KisPropertiesConfiguration::getPrefixedProperties(const QString &prefix,
                                                       KisPropertiesConfiguration *configuration) const
{
    for (auto it = properties(this).cbegin(); it != properties(this).cend(); ++it) {
        if (it.key().startsWith(prefix))
            configuration->setProperty(it.key().mid(prefix.size()), it.value());
    }
}
void KisPropertiesConfiguration::setPrefixedProperties(const QString &prefix,
                                                       const KisPropertiesConfiguration *configuration)
{
    for (auto it = properties(configuration).cbegin(); it != properties(configuration).cend(); ++it) {
        setProperty(prefix + it.key(), it.value());
    }
}
QString KisPropertiesConfiguration::extractedPrefixKey()
{
    return QStringLiteral("__extractedFromPrefix");
}
bool KisPropertiesConfiguration::compareTo(const KisPropertiesConfiguration *rhs) const
{
    return properties(this) == properties(rhs);
}
void KisPropertiesConfiguration::dump() const
{
}

class KisSizeOptionDataContractTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void constructorPreservesSizeIdentityAndPrefix();
    void uncheckedSizeHasNoPreviewRestrictions();
    void activeSensorsReportCurrentRestrictionIds();
};

void KisSizeOptionDataContractTest::constructorPreservesSizeIdentityAndPrefix()
{
    const KisSizeOptionData data(QStringLiteral("寸法/"));
    QCOMPARE(data.id.id(), QStringLiteral("Size"));
    QVERIFY(!data.id.name().isEmpty());
    QCOMPARE(data.prefix, QStringLiteral("寸法/"));
    QVERIFY(data.isCheckable);
    QVERIFY(!data.isChecked);
    QCOMPARE(data.strengthMinValue, 0.0);
    QCOMPARE(data.strengthMaxValue, 1.0);
}

void KisSizeOptionDataContractTest::uncheckedSizeHasNoPreviewRestrictions()
{
    const KisSizeOptionData data;
    const KisPaintopLodLimitations result = data.lodLimitations();
    QVERIFY(result.limitations.isEmpty());
    QVERIFY(result.blockers.isEmpty());
}

void KisSizeOptionDataContractTest::activeSensorsReportCurrentRestrictionIds()
{
    KisSizeOptionData data;
    data.isChecked = true;
    data.sensorStruct().sensorFuzzyPerDab.isActive = true;

    KisPaintopLodLimitations result = data.lodLimitations();
    QCOMPARE(result.limitations.size(), 1);
    QCOMPARE(result.limitations.constBegin()->id(), QStringLiteral("size-fade"));
    QVERIFY(result.blockers.isEmpty());

    data.sensorStruct().sensorFuzzyPerDab.isActive = false;
    data.sensorStruct().sensorFade.isActive = true;
    result = data.lodLimitations();
    QVERIFY(result.limitations.isEmpty());
    QCOMPARE(result.blockers.size(), 1);
    QCOMPARE(result.blockers.constBegin()->id(), QStringLiteral("size-fuzzy"));

    data.sensorStruct().sensorFuzzyPerDab.isActive = true;
    result = data.lodLimitations();
    QCOMPARE(result.limitations.size(), 1);
    QCOMPARE(result.blockers.size(), 1);

    data.isCheckable = false;
    data.isChecked = false;
    result = data.lodLimitations();
    QCOMPARE(result.limitations.size(), 1);
    QCOMPARE(result.blockers.size(), 1);
}

QTEST_MAIN(KisSizeOptionDataContractTest)
#include "KisSizeOptionDataContractTest.moc"
