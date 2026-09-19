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

struct ConfigurationState {
    PropertyStore properties;
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

struct RecordingOptionData {
    bool read(const KisPropertiesConfiguration *setting)
    {
        observedPayload = setting->getProperty(QStringLiteral("payload")).toInt();
        observedUnrelated = setting->hasProperty(QStringLiteral("unrelated"));
        return readResult;
    }

    void write(KisPropertiesConfiguration *setting) const
    {
        setting->setProperty(QStringLiteral("written"), writeValue);
    }

    bool readResult = true;
    int observedPayload = 0;
    bool observedUnrelated = false;
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
    void constructionPreservesPrefix();
    void nullReadPreservesValuesAndReturnsFalse();
    void emptyPrefixReadsAndWritesUnprefixedValues();
    void prefixSelectsValuesAndPreservesUnrelatedSettings();
};

void KisPrefixedOptionDataWrapperContractTest::constructionPreservesPrefix()
{
    const QString prefix = QString::fromUtf8("覆面/α・設定/");

    KisPrefixedOptionDataWrapper<RecordingOptionData> wrapper(prefix);

    QCOMPARE(wrapper.prefix, prefix);
}

void KisPrefixedOptionDataWrapperContractTest::nullReadPreservesValuesAndReturnsFalse()
{
    KisPrefixedOptionDataWrapper<RecordingOptionData> wrapper(QStringLiteral("masked/"));

    wrapper.observedPayload = 123;
    QVERIFY(!wrapper.read(nullptr));
    QCOMPARE(wrapper.observedPayload, 123);
}

void KisPrefixedOptionDataWrapperContractTest::emptyPrefixReadsAndWritesUnprefixedValues()
{
    KisPropertiesConfiguration setting;
    setting.setProperty(QStringLiteral("payload"), 17);

    KisPrefixedOptionDataWrapper<RecordingOptionData> wrapper(QString{});
    wrapper.readResult = false;
    wrapper.writeValue = 29;

    QVERIFY(!wrapper.read(&setting));
    QCOMPARE(wrapper.observedPayload, 17);

    wrapper.write(&setting);
    QCOMPARE(setting.getProperty(QStringLiteral("written")).toInt(), 29);
}

void KisPrefixedOptionDataWrapperContractTest::prefixSelectsValuesAndPreservesUnrelatedSettings()
{
    const QString prefix = QString::fromUtf8("覆面/β・設定/");
    KisPropertiesConfiguration setting;
    setting.setProperty(prefix + QStringLiteral("payload"), 43);
    setting.setProperty(QStringLiteral("unrelated"), 71);

    KisPrefixedOptionDataWrapper<RecordingOptionData> wrapper(prefix);
    wrapper.readResult = false;
    wrapper.writeValue = 83;

    QVERIFY(!wrapper.read(&setting));
    QCOMPARE(wrapper.observedPayload, 43);
    QVERIFY(!wrapper.observedUnrelated);

    wrapper.write(&setting);

    QCOMPARE(setting.getProperty(prefix + QStringLiteral("written")).toInt(), 83);
    QCOMPARE(setting.getProperty(QStringLiteral("unrelated")).toInt(), 71);
}

QTEST_GUILESS_MAIN(KisPrefixedOptionDataWrapperContractTest)

#include "KisPrefixedOptionDataWrapperContractTest.moc"
