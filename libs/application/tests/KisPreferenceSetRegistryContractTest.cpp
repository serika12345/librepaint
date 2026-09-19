/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <application/ui/workspace/kis_preference_set_registry.h>

#include <memory>

#include <QIcon>
#include <QTest>
#include <QWidget>

namespace
{

class PreferenceSetProbe final : public KisPreferenceSet
{
public:
    explicit PreferenceSetProbe(QWidget *parent = nullptr, int *destructionCount = nullptr)
        : KisPreferenceSet(parent)
        , m_destructionCount(destructionCount)
    {
    }

    ~PreferenceSetProbe() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    QString id() override
    {
        return QStringLiteral("probe.preference");
    }

    QString name() override
    {
        return QStringLiteral("Probe Preferences");
    }

    QString header() override
    {
        return QStringLiteral("Probe Header");
    }

    QIcon icon() override
    {
        return QIcon();
    }

    void savePreferences() const override
    {
        ++saveCount;
    }

    void loadPreferences() override
    {
        ++loadCount;
    }

    void loadDefaultPreferences() override
    {
        ++loadDefaultCount;
    }

    mutable int saveCount = 0;
    int loadCount = 0;
    int loadDefaultCount = 0;

private:
    int *m_destructionCount;
};

class PreferenceSetFactoryProbe final : public KisAbstractPreferenceSetFactory
{
public:
    explicit PreferenceSetFactoryProbe(int *destructionCount = nullptr)
        : m_destructionCount(destructionCount)
    {
    }

    ~PreferenceSetFactoryProbe() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    KisPreferenceSet *createPreferenceSet() override
    {
        return new PreferenceSetProbe;
    }

    QString id() const override
    {
        return QStringLiteral("probe.factory");
    }

private:
    int *m_destructionCount;
};

} // namespace

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected safe assertion %s at %s:%d", assertion, file, line);
}

class KisPreferenceSetRegistryContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void preferenceSetTypeConstructionAndLifetimeRemainStable();
    void preferenceSetIdentityAndPresentationRemainStable();
    void preferenceSetPersistenceDispatchRemainsStable();
    void preferenceSetFactoryLifecycleAndCreationRemainStable();
    void preferenceSetRegistryOwnershipAndSingletonRemainStable();
};

void KisPreferenceSetRegistryContractTest::preferenceSetTypeConstructionAndLifetimeRemainStable()
{
    int destructionCount = 0;
    {
        QWidget parent;
        auto *preferenceSet = new PreferenceSetProbe(&parent, &destructionCount);
        QCOMPARE(preferenceSet->parentWidget(), &parent);
    }
    QCOMPARE(destructionCount, 1);
}

void KisPreferenceSetRegistryContractTest::preferenceSetIdentityAndPresentationRemainStable()
{
    PreferenceSetProbe preferenceSet;
    QCOMPARE(preferenceSet.id(), QStringLiteral("probe.preference"));
    QCOMPARE(preferenceSet.name(), QStringLiteral("Probe Preferences"));
    QCOMPARE(preferenceSet.header(), QStringLiteral("Probe Header"));
    QVERIFY(preferenceSet.icon().isNull());
}

void KisPreferenceSetRegistryContractTest::preferenceSetPersistenceDispatchRemainsStable()
{
    PreferenceSetProbe preferenceSet;
    const KisPreferenceSet &constPreferenceSet = preferenceSet;
    constPreferenceSet.savePreferences();
    preferenceSet.loadPreferences();
    preferenceSet.loadDefaultPreferences();

    QCOMPARE(preferenceSet.saveCount, 1);
    QCOMPARE(preferenceSet.loadCount, 1);
    QCOMPARE(preferenceSet.loadDefaultCount, 1);
}

void KisPreferenceSetRegistryContractTest::preferenceSetFactoryLifecycleAndCreationRemainStable()
{
    int destructionCount = 0;
    auto factory = std::make_unique<PreferenceSetFactoryProbe>(&destructionCount);
    QCOMPARE(factory->id(), QStringLiteral("probe.factory"));

    std::unique_ptr<KisPreferenceSet> preferenceSet(factory->createPreferenceSet());
    QVERIFY(dynamic_cast<PreferenceSetProbe *>(preferenceSet.get()));
    QCOMPARE(preferenceSet->id(), QStringLiteral("probe.preference"));

    factory.reset();
    QCOMPARE(destructionCount, 1);
}

void KisPreferenceSetRegistryContractTest::preferenceSetRegistryOwnershipAndSingletonRemainStable()
{
    using Registry = KisPreferenceSetRegistry;
    Registry *firstInstance = Registry::instance();
    QVERIFY(firstInstance);
    QCOMPARE(Registry::instance(), firstInstance);

    int destructionCount = 0;
    {
        Registry registry;
        QCOMPARE(registry.count(), 0);

        auto *factory = new PreferenceSetFactoryProbe(&destructionCount);
        registry.add(factory);
        QCOMPARE(registry.count(), 1);
        QCOMPARE(registry.value(QStringLiteral("probe.factory")), factory);
    }
    QCOMPARE(destructionCount, 1);
}

QTEST_MAIN(KisPreferenceSetRegistryContractTest)

#include "KisPreferenceSetRegistryContractTest.moc"
