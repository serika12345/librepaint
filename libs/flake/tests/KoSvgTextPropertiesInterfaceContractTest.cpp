/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QMap>
#include <QStringList>
#include <QTest>
#include <QVariant>

#include <text/KoSvgTextPropertiesInterface.h>

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

namespace
{

KoSvgTextProperties propertiesWithValue(KoSvgTextProperties::PropertyId id, const QVariant &value)
{
    KoSvgTextProperties properties;
    properties.setProperty(id, value);
    return properties;
}

class SvgTextPropertiesInterfaceProbe final : public KoSvgTextPropertiesInterface
{
public:
    explicit SvgTextPropertiesInterfaceProbe(QObject *parent = nullptr, int *destructionCount = nullptr)
        : KoSvgTextPropertiesInterface(parent)
        , m_destructionCount(destructionCount)
    {
    }

    ~SvgTextPropertiesInterfaceProbe() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    QList<KoSvgTextProperties> getSelectedProperties() override
    {
        getterEvents.append(QStringLiteral("selected"));
        return selectedProperties;
    }

    QList<KoSvgTextProperties> getCharacterProperties() override
    {
        getterEvents.append(QStringLiteral("character"));
        return characterProperties;
    }

    KoSvgTextProperties getInheritedProperties() override
    {
        getterEvents.append(QStringLiteral("inherited"));
        return inheritedProperties;
    }

    void setPropertiesOnSelected(KoSvgTextProperties properties,
                                 QSet<KoSvgTextProperties::PropertyId> removeProperties) override
    {
        setterEvents.append(QStringLiteral("selected"));
        selectedPropertiesSet = properties;
        selectedPropertiesRemoved = removeProperties;
    }

    void setCharacterPropertiesOnSelected(KoSvgTextProperties properties,
                                          QSet<KoSvgTextProperties::PropertyId> removeProperties) override
    {
        setterEvents.append(QStringLiteral("character"));
        characterPropertiesSet = properties;
        characterPropertiesRemoved = removeProperties;
    }

    bool spanSelection() override
    {
        return selectsSpan;
    }

    bool characterPropertiesEnabled() override
    {
        return characterSelectionEnabled;
    }

    void emitTextSelectionChanged()
    {
        Q_EMIT textSelectionChanged();
    }

    void emitTextCharacterSelectionChanged()
    {
        Q_EMIT textCharacterSelectionChanged();
    }

    QList<KoSvgTextProperties> selectedProperties;
    QList<KoSvgTextProperties> characterProperties;
    KoSvgTextProperties inheritedProperties;
    KoSvgTextProperties selectedPropertiesSet;
    KoSvgTextProperties characterPropertiesSet;
    QSet<KoSvgTextProperties::PropertyId> selectedPropertiesRemoved;
    QSet<KoSvgTextProperties::PropertyId> characterPropertiesRemoved;
    QStringList getterEvents;
    QStringList setterEvents;
    bool selectsSpan{false};
    bool characterSelectionEnabled{false};

private:
    int *m_destructionCount;
};

} // namespace

class KoSvgTextPropertiesInterfaceContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parentOwnsDerivedInterface();
    void gettersDispatchConfiguredPropertyValues();
    void settersDispatchCopiedValuesAndRemovalSets();
    void selectionFlagsDispatchIndependently();
    void selectionSignalsPreserveEmissionOrder();
};

void KoSvgTextPropertiesInterfaceContractTest::parentOwnsDerivedInterface()
{
    int destructionCount = 0;

    {
        QObject parent;
        auto *probe = new SvgTextPropertiesInterfaceProbe(&parent, &destructionCount);
        KoSvgTextPropertiesInterface *interface = probe;

        QCOMPARE(interface->parent(), &parent);
        QCOMPARE(destructionCount, 0);
    }

    QCOMPARE(destructionCount, 1);
}

void KoSvgTextPropertiesInterfaceContractTest::gettersDispatchConfiguredPropertyValues()
{
    SvgTextPropertiesInterfaceProbe probe;
    probe.selectedProperties = {
        propertiesWithValue(KoSvgTextProperties::WritingModeId, QStringLiteral("horizontal")),
        propertiesWithValue(KoSvgTextProperties::DirectionId, QStringLiteral("rtl")),
    };
    probe.characterProperties = {
        propertiesWithValue(KoSvgTextProperties::FontWeightId, 650),
    };
    probe.inheritedProperties = propertiesWithValue(KoSvgTextProperties::FontSizeId, 17.5);
    KoSvgTextPropertiesInterface *interface = &probe;

    QVERIFY(interface->getSelectedProperties() == probe.selectedProperties);
    QVERIFY(interface->getCharacterProperties() == probe.characterProperties);
    QVERIFY(interface->getInheritedProperties() == probe.inheritedProperties);
    QCOMPARE(probe.getterEvents,
             QStringList({QStringLiteral("selected"), QStringLiteral("character"), QStringLiteral("inherited")}));
}

void KoSvgTextPropertiesInterfaceContractTest::settersDispatchCopiedValuesAndRemovalSets()
{
    SvgTextPropertiesInterfaceProbe probe;
    KoSvgTextPropertiesInterface *interface = &probe;
    KoSvgTextProperties selected = propertiesWithValue(KoSvgTextProperties::TextAnchorId, QStringLiteral("middle"));
    const KoSvgTextProperties selectedExpected = selected;
    const QSet<KoSvgTextProperties::PropertyId> removed{
        KoSvgTextProperties::DirectionId,
        KoSvgTextProperties::UnicodeBidiId,
    };

    interface->setPropertiesOnSelected(selected, removed);
    selected.setProperty(KoSvgTextProperties::TextAnchorId, QStringLiteral("end"));

    QVERIFY(probe.selectedPropertiesSet == selectedExpected);
    QCOMPARE(probe.selectedPropertiesRemoved, removed);

    const KoSvgTextProperties character = propertiesWithValue(KoSvgTextProperties::FontWeightId, 720);
    interface->setCharacterPropertiesOnSelected(character);

    QVERIFY(probe.characterPropertiesSet == character);
    QVERIFY(probe.characterPropertiesRemoved.isEmpty());
    QCOMPARE(probe.setterEvents, QStringList({QStringLiteral("selected"), QStringLiteral("character")}));
}

void KoSvgTextPropertiesInterfaceContractTest::selectionFlagsDispatchIndependently()
{
    SvgTextPropertiesInterfaceProbe probe;
    KoSvgTextPropertiesInterface *interface = &probe;

    QVERIFY(!interface->spanSelection());
    QVERIFY(!interface->characterPropertiesEnabled());

    probe.selectsSpan = true;
    QVERIFY(interface->spanSelection());
    QVERIFY(!interface->characterPropertiesEnabled());

    probe.selectsSpan = false;
    probe.characterSelectionEnabled = true;
    QVERIFY(!interface->spanSelection());
    QVERIFY(interface->characterPropertiesEnabled());
}

void KoSvgTextPropertiesInterfaceContractTest::selectionSignalsPreserveEmissionOrder()
{
    SvgTextPropertiesInterfaceProbe probe;
    KoSvgTextPropertiesInterface *interface = &probe;
    QStringList events;

    connect(interface, &KoSvgTextPropertiesInterface::textSelectionChanged, this, [&events] {
        events.append(QStringLiteral("textSelectionChanged"));
    });
    connect(interface, &KoSvgTextPropertiesInterface::textCharacterSelectionChanged, this, [&events] {
        events.append(QStringLiteral("textCharacterSelectionChanged"));
    });

    probe.emitTextSelectionChanged();
    probe.emitTextCharacterSelectionChanged();
    probe.emitTextSelectionChanged();

    QCOMPARE(events,
             QStringList({QStringLiteral("textSelectionChanged"),
                          QStringLiteral("textCharacterSelectionChanged"),
                          QStringLiteral("textSelectionChanged")}));
}

QTEST_GUILESS_MAIN(KoSvgTextPropertiesInterfaceContractTest)

#include "KoSvgTextPropertiesInterfaceContractTest.moc"
