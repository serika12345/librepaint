/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later */
#include "KisSharpnessOptionData.h"
#include <QHash>
#include <QMap>
#include <QTest>
#include <kis_properties_configuration.h>
#include <type_traits>

namespace
{
using Store = QMap<QString, QVariant>;
QHash<const KisPropertiesConfiguration *, Store> &stores()
{
    static QHash<const KisPropertiesConfiguration *, Store> s;
    return s;
}
Store &props(KisPropertiesConfiguration *p)
{
    return stores()[p];
}
const Store &props(const KisPropertiesConfiguration *p)
{
    return stores()[p];
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
KisPropertiesConfiguration::KisPropertiesConfiguration(const KisPropertiesConfiguration &r)
    : KisSerializableConfiguration(r)
    , d(new Private)
{
    stores().insert(this, props(&r));
}
KisPropertiesConfiguration &KisPropertiesConfiguration::operator=(const KisPropertiesConfiguration &r)
{
    if (this != &r)
        props(this) = props(&r);
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
bool KisPropertiesConfiguration::hasProperty(const QString &n) const
{
    return props(this).contains(n);
}
void KisPropertiesConfiguration::setProperty(const QString &n, const QVariant &v)
{
    props(this).insert(n, v);
}
bool KisPropertiesConfiguration::getProperty(const QString &n, QVariant &v) const
{
    auto i = props(this).constFind(n);
    if (i == props(this).constEnd())
        return false;
    v = *i;
    return true;
}
QVariant KisPropertiesConfiguration::getProperty(const QString &n) const
{
    return props(this).value(n);
}
int KisPropertiesConfiguration::getInt(const QString &n, int d) const
{
    auto v = getProperty(n);
    return v.isValid() ? v.toInt() : d;
}
double KisPropertiesConfiguration::getDouble(const QString &n, double d) const
{
    auto v = getProperty(n);
    return v.isValid() ? v.toDouble() : d;
}
bool KisPropertiesConfiguration::getBool(const QString &n, bool d) const
{
    auto v = getProperty(n);
    return v.isValid() ? v.toBool() : d;
}
QString KisPropertiesConfiguration::getString(const QString &n, const QString &d) const
{
    auto v = getProperty(n);
    return v.isValid() ? v.toString() : d;
}
QMap<QString, QVariant> KisPropertiesConfiguration::getProperties() const
{
    return props(this);
}
QList<QString> KisPropertiesConfiguration::getPropertiesKeys() const
{
    return props(this).keys();
}
void KisPropertiesConfiguration::getPrefixedProperties(const QString &p, KisPropertiesConfiguration *c) const
{
    for (auto i = props(this).cbegin(); i != props(this).cend(); ++i)
        if (i.key().startsWith(p))
            c->setProperty(i.key().mid(p.size()), i.value());
}
void KisPropertiesConfiguration::setPrefixedProperties(const QString &p, const KisPropertiesConfiguration *c)
{
    for (auto i = props(c).cbegin(); i != props(c).cend(); ++i)
        setProperty(p + i.key(), i.value());
}
QString KisPropertiesConfiguration::extractedPrefixKey()
{
    return QStringLiteral("__extractedFromPrefix");
}
bool KisPropertiesConfiguration::compareTo(const KisPropertiesConfiguration *r) const
{
    return props(this) == props(r);
}
void KisPropertiesConfiguration::dump() const
{
}

class KisSharpnessOptionDataContractTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void typesDefaultsAndConstructor();
    void readMapsCurrentAndEmptySettings();
    void legacyFactorAndModernValuePrecedence();
    void writeMapsBothKeys();
    void equalityUsesBothMembers();
};
void KisSharpnessOptionDataContractTest::typesDefaultsAndConstructor()
{
    static_assert(std::is_same_v<KisSharpnessOptionMixIn, KisPrefixedOptionDataWrapper<KisSharpnessOptionMixInImpl>>);
    KisSharpnessOptionData defaults;
    QVERIFY(!defaults.alignOutlinePixels);
    QCOMPARE(defaults.softness, 0);
    QVERIFY(static_cast<const KisCurveOptionData &>(defaults).prefix.isEmpty());
    QVERIFY(static_cast<const KisSharpnessOptionMixIn &>(defaults).prefix.isEmpty());

    KisSharpnessOptionData d(QStringLiteral("鋭さ/"));
    QCOMPARE(static_cast<const KisCurveOptionData &>(d).prefix, QStringLiteral("鋭さ/"));
    QCOMPARE(static_cast<const KisSharpnessOptionMixIn &>(d).prefix, QStringLiteral("鋭さ/"));
    QCOMPARE(d.id.id(), QStringLiteral("Sharpness"));
}
void KisSharpnessOptionDataContractTest::readMapsCurrentAndEmptySettings()
{
    KisPropertiesConfiguration c;
    c.setProperty(QStringLiteral("Sharpness/alignoutline"), true);
    c.setProperty(QStringLiteral("Sharpness/softness"), 73);
    KisSharpnessOptionMixInImpl d;
    QVERIFY(d.read(&c));
    QVERIFY(d.alignOutlinePixels);
    QCOMPARE(d.softness, 73);

    KisPropertiesConfiguration empty;
    d.alignOutlinePixels = true;
    d.softness = 99;
    QVERIFY(d.read(&empty));
    QVERIFY(!d.alignOutlinePixels);
    QCOMPARE(d.softness, 0);
}
void KisSharpnessOptionDataContractTest::legacyFactorAndModernValuePrecedence()
{
    KisPropertiesConfiguration legacy;
    legacy.setProperty(QStringLiteral("Sharpness/factor"), 0.375);
    KisSharpnessOptionData data;
    QVERIFY(data.read(&legacy));
    QCOMPARE(data.softness, 37);
    QCOMPARE(data.strengthValue, 0.375);

    KisPropertiesConfiguration modern;
    modern.setProperty(QStringLiteral("Sharpness/factor"), 0.375);
    modern.setProperty(QStringLiteral("SharpnessValue"), 0.8);
    KisSharpnessOptionMixInImpl mixin;
    mixin.softness = 61;
    QVERIFY(mixin.read(&modern));
    QCOMPARE(mixin.softness, 0);
}
void KisSharpnessOptionDataContractTest::writeMapsBothKeys()
{
    KisPropertiesConfiguration c;
    c.setProperty(QStringLiteral("other"), 137);
    KisSharpnessOptionMixInImpl d;
    d.alignOutlinePixels = true;
    d.softness = 41;
    d.write(&c);
    QVERIFY(c.getBool(QStringLiteral("Sharpness/alignoutline")));
    QCOMPARE(c.getInt(QStringLiteral("Sharpness/softness")), 41);
    QCOMPARE(c.getInt(QStringLiteral("other")), 137);
}
void KisSharpnessOptionDataContractTest::equalityUsesBothMembers()
{
    KisSharpnessOptionMixInImpl a, b;
    QVERIFY(a == b);
    a.alignOutlinePixels = true;
    QVERIFY(!(a == b));
    a = b;
    a.softness = 1;
    QVERIFY(!(a == b));
}
QTEST_MAIN(KisSharpnessOptionDataContractTest)
#include "KisSharpnessOptionDataContractTest.moc"
