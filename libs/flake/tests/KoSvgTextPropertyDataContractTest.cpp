/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QMap>
#include <QMetaType>
#include <QTest>
#include <QVariant>

#include <text/KoSvgTextPropertyData.h>

namespace
{
QMap<QString, QString> paragraphPropertiesForDebug;
QMap<QString, QString> textAttributesForDebug;
} // namespace

struct KoSvgTextProperties::Private {
    QMap<PropertyId, QVariant> properties;
};

KoSvgTextProperties::KoSvgTextProperties()
    : m_d(new Private)
{
}

KoSvgTextProperties::~KoSvgTextProperties() = default;

KoSvgTextProperties::KoSvgTextProperties(const KoSvgTextProperties &rhs)
    : m_d(new Private(*rhs.m_d))
{
}

KoSvgTextProperties &KoSvgTextProperties::operator=(const KoSvgTextProperties &rhs)
{
    if (this != &rhs) {
        m_d->properties = rhs.m_d->properties;
    }
    return *this;
}

bool KoSvgTextProperties::operator==(const KoSvgTextProperties &rhs) const
{
    return m_d->properties == rhs.m_d->properties;
}

void KoSvgTextProperties::setProperty(PropertyId id, const QVariant &value)
{
    m_d->properties.insert(id, value);
}

QMap<QString, QString> KoSvgTextProperties::convertToSvgTextAttributes() const
{
    return textAttributesForDebug;
}

QMap<QString, QString> KoSvgTextProperties::convertParagraphProperties() const
{
    return paragraphPropertiesForDebug;
}

const KoSvgTextProperties &KoSvgTextProperties::defaultProperties()
{
    static const KoSvgTextProperties properties;
    return properties;
}

class KoSvgTextPropertyDataContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultsToInheritedDefaultsAndDisabledSpanState();
    void eachFieldIndependentlyControlsEquality();
    void registersMetaTypeAndPrintsOrderedState();
};

void KoSvgTextPropertyDataContractTest::defaultsToInheritedDefaultsAndDisabledSpanState()
{
    const KoSvgTextPropertyData data;

    QCOMPARE(data.commonProperties, KoSvgTextProperties());
    QCOMPARE(data.inheritedProperties, KoSvgTextProperties::defaultProperties());
    QVERIFY(data.tristate.isEmpty());
    QVERIFY(!data.spanSelection);
    QVERIFY(!data.enabled);
}

void KoSvgTextPropertyDataContractTest::eachFieldIndependentlyControlsEquality()
{
    const KoSvgTextPropertyData baseline;
    QCOMPARE(KoSvgTextPropertyData(baseline), baseline);

    KoSvgTextPropertyData changed = baseline;
    changed.commonProperties.setProperty(KoSvgTextProperties::WritingModeId, 1);
    QVERIFY(changed != baseline);

    changed = baseline;
    changed.inheritedProperties.setProperty(KoSvgTextProperties::DirectionId, 2);
    QVERIFY(changed != baseline);

    changed = baseline;
    changed.tristate.insert(KoSvgTextProperties::FontWeightId);
    QVERIFY(changed != baseline);

    changed = baseline;
    changed.spanSelection = true;
    QVERIFY(changed != baseline);

    changed = baseline;
    changed.enabled = true;
    QVERIFY(changed != baseline);
}

void KoSvgTextPropertyDataContractTest::registersMetaTypeAndPrintsOrderedState()
{
    const QMetaType type = QMetaType::fromName("KoSvgTextPropertyData");
    QVERIFY(type.isValid());
    QCOMPARE(type.name(), "KoSvgTextPropertyData");

    paragraphPropertiesForDebug = {{QStringLiteral("paragraph-key"), QStringLiteral("paragraph-value")}};
    textAttributesForDebug = {{QStringLiteral("attribute-key"), QStringLiteral("attribute-value")}};

    KoSvgTextPropertyData data;
    data.tristate.insert(KoSvgTextProperties::FontWeightId);
    data.spanSelection = true;
    data.enabled = true;

    QString output;
    {
        QDebug debug(&output);
        debug << data;
    }

    const QStringList orderedFragments{
        QStringLiteral("TextPropertyData("),
        QStringLiteral(" Common properties:"),
        QStringLiteral("paragraph-key"),
        QStringLiteral("paragraph-value"),
        QStringLiteral("attribute-key"),
        QStringLiteral("attribute-value"),
        QStringLiteral(" Tristate:"),
        QStringLiteral(" SpanSelection:true"),
        QStringLiteral(" Enabled:true"),
        QStringLiteral(" )"),
    };

    qsizetype previousPosition = -1;
    for (const QString &fragment : orderedFragments) {
        const qsizetype position = output.indexOf(fragment, previousPosition + 1);
        QVERIFY2(position > previousPosition, qPrintable(output));
        previousPosition = position;
    }
}

QTEST_GUILESS_MAIN(KoSvgTextPropertyDataContractTest)

#include "KoSvgTextPropertyDataContractTest.moc"
