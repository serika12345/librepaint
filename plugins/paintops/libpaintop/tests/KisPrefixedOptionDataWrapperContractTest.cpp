/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisPrefixedOptionDataWrapper.h"

#include <QHash>
#include <QMap>
#include <QTest>

namespace
{

using PropertyStore = QMap<QString, QVariant>;

struct ConfigurationTrace {
    QStringList events;
    const KisPropertiesConfiguration *extractSource = nullptr;
    KisPropertiesConfiguration *extractDestination = nullptr;
    QString extractPrefix;
    KisPropertiesConfiguration *reflectDestination = nullptr;
    const KisPropertiesConfiguration *reflectSource = nullptr;
    QString reflectPrefix;
};

struct ConfigurationState {
    PropertyStore properties;
    ConfigurationTrace *trace = nullptr;
};

QHash<const KisPropertiesConfiguration *, ConfigurationState> &configurationStates()
{
    static QHash<const KisPropertiesConfiguration *, ConfigurationState> states;
    return states;
}

ConfigurationState &state(KisPropertiesConfiguration *configuration)
{
    return configurationStates()[configuration];
}

const ConfigurationState &state(const KisPropertiesConfiguration *configuration)
{
    return configurationStates()[configuration];
}

void attachTrace(KisPropertiesConfiguration *configuration, ConfigurationTrace *trace)
{
    state(configuration).trace = trace;
}

int baseConstructionCount = 0;

struct RecordingOptionData {
    RecordingOptionData()
    {
        ++baseConstructionCount;
    }

    bool read(const KisPropertiesConfiguration *setting)
    {
        ++readCalls;
        lastReadSetting = setting;
        observedPayload = setting->getProperty(QStringLiteral("payload")).toInt();
        observedUnrelated = setting->hasProperty(QStringLiteral("unrelated"));
        if (events) {
            events->append(QStringLiteral("option-read"));
        }
        return readResult;
    }

    void write(KisPropertiesConfiguration *setting) const
    {
        ++writeCalls;
        lastWriteSetting = setting;
        if (events) {
            events->append(QStringLiteral("option-write"));
        }
        setting->setProperty(QStringLiteral("written"), writeValue);
    }

    QStringList *events = nullptr;
    bool readResult = true;
    int readCalls = 0;
    const KisPropertiesConfiguration *lastReadSetting = nullptr;
    int observedPayload = 0;
    bool observedUnrelated = false;
    mutable int writeCalls = 0;
    mutable KisPropertiesConfiguration *lastWriteSetting = nullptr;
    int writeValue = 0;
};

} // namespace

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
    configurationStates().insert(this, {});
}

KisPropertiesConfiguration::~KisPropertiesConfiguration()
{
    configurationStates().remove(this);
    delete d;
}

KisPropertiesConfiguration::KisPropertiesConfiguration(const KisPropertiesConfiguration &rhs)
    : KisSerializableConfiguration(rhs)
    , d(new Private)
{
    configurationStates().insert(this, state(&rhs));
}

KisPropertiesConfiguration &KisPropertiesConfiguration::operator=(const KisPropertiesConfiguration &rhs)
{
    if (this != &rhs) {
        state(this) = state(&rhs);
    }
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
    return state(this).properties.contains(name);
}

void KisPropertiesConfiguration::setProperty(const QString &name, const QVariant &value)
{
    state(this).properties.insert(name, value);
}

bool KisPropertiesConfiguration::getProperty(const QString &name, QVariant &value) const
{
    const auto item = state(this).properties.constFind(name);
    if (item == state(this).properties.constEnd()) {
        return false;
    }
    value = *item;
    return true;
}

QVariant KisPropertiesConfiguration::getProperty(const QString &name) const
{
    return state(this).properties.value(name);
}

QMap<QString, QVariant> KisPropertiesConfiguration::getProperties() const
{
    return state(this).properties;
}

QList<QString> KisPropertiesConfiguration::getPropertiesKeys() const
{
    return state(this).properties.keys();
}

void KisPropertiesConfiguration::getPrefixedProperties(const QString &prefix,
                                                       KisPropertiesConfiguration *configuration) const
{
    ConfigurationTrace *trace = state(this).trace;
    if (trace) {
        trace->events.append(QStringLiteral("extract-prefix"));
        trace->extractSource = this;
        trace->extractDestination = configuration;
        trace->extractPrefix = prefix;
    }

    const qsizetype prefixSize = prefix.size();
    for (auto item = state(this).properties.constBegin(); item != state(this).properties.constEnd(); ++item) {
        if (item.key().startsWith(prefix)) {
            configuration->setProperty(item.key().mid(prefixSize), item.value());
        }
    }
}

void KisPropertiesConfiguration::setPrefixedProperties(const QString &prefix,
                                                       const KisPropertiesConfiguration *configuration)
{
    ConfigurationTrace *trace = state(this).trace;
    if (trace) {
        trace->events.append(QStringLiteral("reflect-prefix"));
        trace->reflectDestination = this;
        trace->reflectSource = configuration;
        trace->reflectPrefix = prefix;
    }

    for (auto item = state(configuration).properties.constBegin(); item != state(configuration).properties.constEnd();
         ++item) {
        setProperty(prefix + item.key(), item.value());
    }
}

bool KisPropertiesConfiguration::compareTo(const KisPropertiesConfiguration *rhs) const
{
    return state(this).properties == state(rhs).properties;
}

void KisPropertiesConfiguration::dump() const
{
}

class KisPrefixedOptionDataWrapperContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructionPreservesCapabilityPrefixAndBaseInitialization();
    void nullReadReturnsFalseWithoutDelegation();
    void emptyPrefixDelegatesDirectly();
    void nonEmptyPrefixRoutesThroughTemporaryConfiguration();
};

void KisPrefixedOptionDataWrapperContractTest::constructionPreservesCapabilityPrefixAndBaseInitialization()
{
    baseConstructionCount = 0;
    const QString prefix = QString::fromUtf8("覆面/α・設定/");

    KisPrefixedOptionDataWrapper<RecordingOptionData> wrapper(prefix);

    static_assert(decltype(wrapper)::supports_prefix);
    QVERIFY(wrapper.supports_prefix);
    QCOMPARE(wrapper.prefix, prefix);
    QCOMPARE(baseConstructionCount, 1);
}

void KisPrefixedOptionDataWrapperContractTest::nullReadReturnsFalseWithoutDelegation()
{
    KisPrefixedOptionDataWrapper<RecordingOptionData> wrapper(QStringLiteral("masked/"));

    QVERIFY(!wrapper.read(nullptr));
    QCOMPARE(wrapper.readCalls, 0);
    QCOMPARE(wrapper.lastReadSetting, nullptr);
}

void KisPrefixedOptionDataWrapperContractTest::emptyPrefixDelegatesDirectly()
{
    KisPropertiesConfiguration setting;
    ConfigurationTrace trace;
    attachTrace(&setting, &trace);
    setting.setProperty(QStringLiteral("payload"), 17);

    KisPrefixedOptionDataWrapper<RecordingOptionData> wrapper(QString{});
    wrapper.events = &trace.events;
    wrapper.readResult = false;
    wrapper.writeValue = 29;

    QVERIFY(!wrapper.read(&setting));
    QCOMPARE(wrapper.readCalls, 1);
    QCOMPARE(wrapper.lastReadSetting, &setting);
    QCOMPARE(wrapper.observedPayload, 17);

    wrapper.write(&setting);
    QCOMPARE(wrapper.writeCalls, 1);
    QCOMPARE(wrapper.lastWriteSetting, &setting);
    QCOMPARE(setting.getProperty(QStringLiteral("written")).toInt(), 29);
    QCOMPARE(trace.events, QStringList({QStringLiteral("option-read"), QStringLiteral("option-write")}));
    QCOMPARE(trace.extractSource, nullptr);
    QCOMPARE(trace.reflectDestination, nullptr);
}

void KisPrefixedOptionDataWrapperContractTest::nonEmptyPrefixRoutesThroughTemporaryConfiguration()
{
    const QString prefix = QString::fromUtf8("覆面/β・設定/");
    KisPropertiesConfiguration setting;
    ConfigurationTrace trace;
    attachTrace(&setting, &trace);
    setting.setProperty(prefix + QStringLiteral("payload"), 43);
    setting.setProperty(QStringLiteral("unrelated"), 71);

    KisPrefixedOptionDataWrapper<RecordingOptionData> wrapper(prefix);
    wrapper.events = &trace.events;
    wrapper.readResult = false;
    wrapper.writeValue = 83;

    QVERIFY(!wrapper.read(&setting));
    QCOMPARE(trace.events, QStringList({QStringLiteral("extract-prefix"), QStringLiteral("option-read")}));
    QCOMPARE(trace.extractSource, &setting);
    QCOMPARE(trace.extractPrefix, prefix);
    QVERIFY(trace.extractDestination);
    QVERIFY(trace.extractDestination != &setting);
    QCOMPARE(wrapper.lastReadSetting, trace.extractDestination);
    QCOMPARE(wrapper.observedPayload, 43);
    QVERIFY(!wrapper.observedUnrelated);

    trace.events.clear();
    wrapper.write(&setting);

    QCOMPARE(trace.events, QStringList({QStringLiteral("option-write"), QStringLiteral("reflect-prefix")}));
    QCOMPARE(wrapper.writeCalls, 1);
    QCOMPARE(trace.reflectDestination, &setting);
    QCOMPARE(trace.reflectPrefix, prefix);
    QVERIFY(trace.reflectSource);
    QVERIFY(trace.reflectSource != &setting);
    QCOMPARE(wrapper.lastWriteSetting, trace.reflectSource);
    QCOMPARE(setting.getProperty(prefix + QStringLiteral("written")).toInt(), 83);
    QCOMPARE(setting.getProperty(QStringLiteral("unrelated")).toInt(), 71);
}

QTEST_GUILESS_MAIN(KisPrefixedOptionDataWrapperContractTest)

#include "KisPrefixedOptionDataWrapperContractTest.moc"
