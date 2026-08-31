/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later */
#include "KisSpacingOptionData.h"
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

class KisSpacingOptionDataContractTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void typesAndDefaults();
    void constructorForwardsPrefixesAndId();
    void readMapsCurrentAndEmptySettings();
    void writeMapsBothKeys();
    void equalityUsesBothMembers();
};
void KisSpacingOptionDataContractTest::typesAndDefaults()
{
    static_assert(std::is_same_v<KisSpacingOptionMixIn, KisPrefixedOptionDataWrapper<KisSpacingOptionMixInImpl>>);
    KisSpacingOptionData defaults;
    QVERIFY(!defaults.isotropicSpacing);
    QVERIFY(!defaults.useSpacingUpdates);
    QVERIFY(static_cast<const KisCurveOptionData &>(defaults).prefix.isEmpty());
    QVERIFY(static_cast<const KisSpacingOptionMixIn &>(defaults).prefix.isEmpty());
}

void KisSpacingOptionDataContractTest::constructorForwardsPrefixesAndId()
{
    KisSpacingOptionData d(QStringLiteral("間隔/"));
    QCOMPARE(static_cast<const KisCurveOptionData &>(d).prefix, QStringLiteral("間隔/"));
    QCOMPARE(static_cast<const KisSpacingOptionMixIn &>(d).prefix, QStringLiteral("間隔/"));
    QCOMPARE(d.id.id(), QStringLiteral("Spacing"));
}
void KisSpacingOptionDataContractTest::readMapsCurrentAndEmptySettings()
{
    KisPropertiesConfiguration c;
    c.setProperty(QStringLiteral("Spacing/Isotropic"), true);
    c.setProperty(QStringLiteral("PaintOpSettings/updateSpacingBetweenDabs"), false);
    KisSpacingOptionMixInImpl d;
    QVERIFY(d.read(&c));
    QVERIFY(d.isotropicSpacing);
    QCOMPARE(d.useSpacingUpdates, false);

    KisPropertiesConfiguration empty;
    d.isotropicSpacing = true;
    d.useSpacingUpdates = false;
    QVERIFY(d.read(&empty));
    QVERIFY(!d.isotropicSpacing);
    QVERIFY(!d.useSpacingUpdates);
}
void KisSpacingOptionDataContractTest::writeMapsBothKeys()
{
    KisPropertiesConfiguration c;
    c.setProperty(QStringLiteral("other"), 137);
    KisSpacingOptionMixInImpl d;
    d.isotropicSpacing = true;
    d.useSpacingUpdates = false;
    d.write(&c);
    QVERIFY(c.getBool(QStringLiteral("Spacing/Isotropic")));
    QVERIFY(!c.getBool(QStringLiteral("PaintOpSettings/updateSpacingBetweenDabs"), true));
    QCOMPARE(c.getInt(QStringLiteral("other")), 137);
}
void KisSpacingOptionDataContractTest::equalityUsesBothMembers()
{
    KisSpacingOptionMixInImpl a, b;
    QVERIFY(a == b);
    a.isotropicSpacing = true;
    QVERIFY(!(a == b));
    a = b;
    a.useSpacingUpdates = true;
    QVERIFY(!(a == b));
}
QTEST_MAIN(KisSpacingOptionDataContractTest)
#include "KisSpacingOptionDataContractTest.moc"
