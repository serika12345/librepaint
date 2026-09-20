/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisTextureOptionData.h"

#include <kis_properties_configuration.h>

#include <QHash>
#include <QMap>
#include <QTest>

namespace
{
using PropertyStore = QMap<QString, QVariant>;

QHash<const KisPropertiesConfiguration *, PropertyStore> &configurationStores()
{
    static QHash<const KisPropertiesConfiguration *, PropertyStore> stores;
    return stores;
}

PropertyStore &properties(KisPropertiesConfiguration *configuration)
{
    return configurationStores()[configuration];
}

const PropertyStore &properties(const KisPropertiesConfiguration *configuration)
{
    return configurationStores()[configuration];
}
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
    configurationStores().insert(this, {});
}

KisPropertiesConfiguration::~KisPropertiesConfiguration()
{
    configurationStores().remove(this);
    delete d;
}

KisPropertiesConfiguration::KisPropertiesConfiguration(const KisPropertiesConfiguration &rhs)
    : KisSerializableConfiguration(rhs)
    , d(new Private)
{
    configurationStores().insert(this, properties(&rhs));
}

KisPropertiesConfiguration &KisPropertiesConfiguration::operator=(const KisPropertiesConfiguration &rhs)
{
    if (this != &rhs) {
        properties(this) = properties(&rhs);
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
    return properties(this).contains(name);
}

void KisPropertiesConfiguration::setProperty(const QString &name, const QVariant &value)
{
    properties(this).insert(name, value);
}

bool KisPropertiesConfiguration::getProperty(const QString &name, QVariant &value) const
{
    const auto item = properties(this).constFind(name);
    if (item == properties(this).constEnd()) {
        return false;
    }
    value = *item;
    return true;
}

QVariant KisPropertiesConfiguration::getProperty(const QString &name) const
{
    return properties(this).value(name);
}

int KisPropertiesConfiguration::getInt(const QString &name, int defaultValue) const
{
    const QVariant value = getProperty(name);
    return value.isValid() ? value.toInt() : defaultValue;
}

double KisPropertiesConfiguration::getDouble(const QString &name, double defaultValue) const
{
    const QVariant value = getProperty(name);
    return value.isValid() ? value.toDouble() : defaultValue;
}

bool KisPropertiesConfiguration::getBool(const QString &name, bool defaultValue) const
{
    const QVariant value = getProperty(name);
    return value.isValid() ? value.toBool() : defaultValue;
}

QString KisPropertiesConfiguration::getString(const QString &name, const QString &defaultValue) const
{
    const QVariant value = getProperty(name);
    return value.isValid() ? value.toString() : defaultValue;
}

QMap<QString, QVariant> KisPropertiesConfiguration::getProperties() const
{
    return properties(this);
}

QList<QString> KisPropertiesConfiguration::getPropertiesKeys() const
{
    return properties(this).keys();
}

bool KisPropertiesConfiguration::compareTo(const KisPropertiesConfiguration *rhs) const
{
    return properties(this) == properties(rhs);
}

void KisPropertiesConfiguration::dump() const
{
}

class KisTextureOptionDataIOContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void enabledWriteRoundTripsEveryPersistedProperty();
    void disabledWriteLeavesConfigurationUntouched();
    void missingPropertiesRestoreDocumentedDefaults();
    void readLoadsEmbeddedPayloadAndNormalizesFileName();
};

void KisTextureOptionDataIOContractTest::enabledWriteRoundTripsEveryPersistedProperty()
{
    KisTextureOptionData source;
    source.textureData.md5Base64 = QStringLiteral("bWQ1");
    source.textureData.md5sum = QStringLiteral("0123456789abcdef");
    source.textureData.fileName = QStringLiteral("texture.pat");
    source.textureData.name = QStringLiteral("Texture Name");
    source.textureData.patternBase64 = QStringLiteral("read-only-embedded-payload");
    source.isEnabled = true;
    source.scale = 1.75;
    source.brightness = -0.25;
    source.contrast = 0.75;
    source.neutralPoint = 0.25;
    source.offsetX = -17;
    source.offsetY = 23;
    source.maximumOffsetX = 101;
    source.maximumOffsetY = 202;
    source.isRandomOffsetX = true;
    source.isRandomOffsetY = true;
    source.texturingMode = KisTextureOptionData::LINEAR_HEIGHT;
    source.useSoftTexturing = true;
    source.cutOffPolicy = 2;
    source.cutOffLeft = 12;
    source.cutOffRight = 230;
    source.invert = true;
    source.autoInvertOnErase = true;

    KisPropertiesConfiguration setting;
    source.write(&setting);

    QCOMPARE(setting.getProperties().size(), 20);
    QCOMPARE(setting.getString(QStringLiteral("Texture/Pattern/PatternMD5")), source.textureData.md5Base64);
    QCOMPARE(setting.getString(QStringLiteral("Texture/Pattern/PatternMD5Sum")), source.textureData.md5sum);
    QCOMPARE(setting.getString(QStringLiteral("Texture/Pattern/PatternFileName")), source.textureData.fileName);
    QCOMPARE(setting.getString(QStringLiteral("Texture/Pattern/Name")), source.textureData.name);
    QVERIFY(!setting.hasProperty(QStringLiteral("Texture/Pattern/Pattern")));
    QCOMPARE(setting.getBool(QStringLiteral("Texture/Pattern/Enabled")), source.isEnabled);
    QCOMPARE(setting.getDouble(QStringLiteral("Texture/Pattern/Scale")), source.scale);
    QCOMPARE(setting.getDouble(QStringLiteral("Texture/Pattern/Brightness")), source.brightness);
    QCOMPARE(setting.getDouble(QStringLiteral("Texture/Pattern/Contrast")), source.contrast);
    QCOMPARE(setting.getDouble(QStringLiteral("Texture/Pattern/NeutralPoint")), source.neutralPoint);
    QCOMPARE(setting.getInt(QStringLiteral("Texture/Pattern/OffsetX")), source.offsetX);
    QCOMPARE(setting.getInt(QStringLiteral("Texture/Pattern/OffsetY")), source.offsetY);
    QCOMPARE(setting.getBool(QStringLiteral("Texture/Pattern/isRandomOffsetX")), source.isRandomOffsetX);
    QCOMPARE(setting.getBool(QStringLiteral("Texture/Pattern/isRandomOffsetY")), source.isRandomOffsetY);
    QCOMPARE(setting.getInt(QStringLiteral("Texture/Pattern/TexturingMode")), int(source.texturingMode));
    QCOMPARE(setting.getBool(QStringLiteral("Texture/Pattern/UseSoftTexturing")), source.useSoftTexturing);
    QCOMPARE(setting.getInt(QStringLiteral("Texture/Pattern/CutoffPolicy")), source.cutOffPolicy);
    QCOMPARE(setting.getInt(QStringLiteral("Texture/Pattern/CutoffLeft")), source.cutOffLeft);
    QCOMPARE(setting.getInt(QStringLiteral("Texture/Pattern/CutoffRight")), source.cutOffRight);
    QCOMPARE(setting.getBool(QStringLiteral("Texture/Pattern/Invert")), source.invert);
    QCOMPARE(setting.getBool(QStringLiteral("Texture/Pattern/AutoInvertOnErase")), source.autoInvertOnErase);
    QVERIFY(!setting.hasProperty(QStringLiteral("Texture/Pattern/MaximumOffsetX")));
    QVERIFY(!setting.hasProperty(QStringLiteral("Texture/Pattern/MaximumOffsetY")));

    KisTextureOptionData restored;
    restored.maximumOffsetX = -101;
    restored.maximumOffsetY = -202;
    QVERIFY(restored.read(&setting));

    QCOMPARE(restored.textureData.md5Base64, source.textureData.md5Base64);
    QCOMPARE(restored.textureData.md5sum, source.textureData.md5sum);
    QCOMPARE(restored.textureData.fileName, source.textureData.fileName);
    QCOMPARE(restored.textureData.name, source.textureData.name);
    QVERIFY(restored.textureData.patternBase64.isEmpty());
    QCOMPARE(restored.isEnabled, source.isEnabled);
    QCOMPARE(restored.scale, source.scale);
    QCOMPARE(restored.brightness, source.brightness);
    QCOMPARE(restored.contrast, source.contrast);
    QCOMPARE(restored.neutralPoint, source.neutralPoint);
    QCOMPARE(restored.offsetX, source.offsetX);
    QCOMPARE(restored.offsetY, source.offsetY);
    QCOMPARE(restored.isRandomOffsetX, source.isRandomOffsetX);
    QCOMPARE(restored.isRandomOffsetY, source.isRandomOffsetY);
    QCOMPARE(restored.texturingMode, source.texturingMode);
    QCOMPARE(restored.useSoftTexturing, source.useSoftTexturing);
    QCOMPARE(restored.cutOffPolicy, source.cutOffPolicy);
    QCOMPARE(restored.cutOffLeft, source.cutOffLeft);
    QCOMPARE(restored.cutOffRight, source.cutOffRight);
    QCOMPARE(restored.invert, source.invert);
    QCOMPARE(restored.autoInvertOnErase, source.autoInvertOnErase);
    QCOMPARE(restored.maximumOffsetX, -101);
    QCOMPARE(restored.maximumOffsetY, -202);
}

void KisTextureOptionDataIOContractTest::disabledWriteLeavesConfigurationUntouched()
{
    KisPropertiesConfiguration setting;
    setting.setProperty(QStringLiteral("sentinel"), 17);
    setting.setProperty(QStringLiteral("Texture/Pattern/Enabled"), true);

    const KisTextureOptionData disabled;
    disabled.write(&setting);

    QCOMPARE(setting.getProperties().size(), 2);
    QCOMPARE(setting.getInt(QStringLiteral("sentinel")), 17);
    QVERIFY(setting.getBool(QStringLiteral("Texture/Pattern/Enabled")));
}

void KisTextureOptionDataIOContractTest::missingPropertiesRestoreDocumentedDefaults()
{
    KisTextureOptionData data;
    data.textureData.md5Base64 = QStringLiteral("old-md5");
    data.textureData.md5sum = QStringLiteral("old-sum");
    data.textureData.fileName = QStringLiteral("old-file");
    data.textureData.name = QStringLiteral("old-name");
    data.textureData.patternBase64 = QStringLiteral("old-payload");
    data.isEnabled = true;
    data.scale = 7.0;
    data.brightness = 7.0;
    data.contrast = 7.0;
    data.neutralPoint = 7.0;
    data.offsetX = 7;
    data.offsetY = 7;
    data.maximumOffsetX = 71;
    data.maximumOffsetY = 72;
    data.isRandomOffsetX = true;
    data.isRandomOffsetY = true;
    data.texturingMode = KisTextureOptionData::OVERLAY;
    data.useSoftTexturing = true;
    data.cutOffPolicy = 7;
    data.cutOffLeft = 7;
    data.cutOffRight = 7;
    data.invert = true;
    data.autoInvertOnErase = true;
    const KisPropertiesConfiguration emptySetting;

    QVERIFY(data.read(&emptySetting));

    QCOMPARE(data.textureData, KisEmbeddedTextureData());
    QVERIFY(!data.isEnabled);
    QCOMPARE(data.scale, 1.0);
    QCOMPARE(data.brightness, 0.0);
    QCOMPARE(data.contrast, 1.0);
    QCOMPARE(data.neutralPoint, 0.5);
    QCOMPARE(data.offsetX, 0);
    QCOMPARE(data.offsetY, 0);
    QCOMPARE(data.maximumOffsetX, 71);
    QCOMPARE(data.maximumOffsetY, 72);
    QVERIFY(!data.isRandomOffsetX);
    QVERIFY(!data.isRandomOffsetY);
    QCOMPARE(data.texturingMode, KisTextureOptionData::MULTIPLY);
    QVERIFY(!data.useSoftTexturing);
    QCOMPARE(data.cutOffPolicy, 0);
    QCOMPARE(data.cutOffLeft, 0);
    QCOMPARE(data.cutOffRight, 255);
    QVERIFY(!data.invert);
    QVERIFY(!data.autoInvertOnErase);
}

void KisTextureOptionDataIOContractTest::readLoadsEmbeddedPayloadAndNormalizesFileName()
{
    KisPropertiesConfiguration setting;
    setting.setProperty(QStringLiteral("Texture/Pattern/Pattern"), QStringLiteral("cGF5bG9hZA=="));
    setting.setProperty(QStringLiteral("Texture/Pattern/PatternFileName"), QStringLiteral("/patterns/texture.pat"));
    KisTextureOptionData data;

    QVERIFY(data.read(&setting));

    QCOMPARE(data.textureData.patternBase64, QStringLiteral("cGF5bG9hZA=="));
    QCOMPARE(data.textureData.fileName, QStringLiteral("texture.pat"));
}

QTEST_GUILESS_MAIN(KisTextureOptionDataIOContractTest)

#include "KisTextureOptionDataIOContractTest.moc"
