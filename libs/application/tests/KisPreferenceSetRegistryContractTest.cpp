/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <application/ui/workspace/kis_preference_set_registry.h>

#include <memory>
#include <type_traits>

#include <QIcon>
#include <QTest>
#include <QWidget>

namespace
{

#define ASSERT_PREFERENCE_MEMBER(type, method, signature)                                                              \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&type::method)), signature>)

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
    static_assert(std::is_class_v<KisPreferenceSet>);
    static_assert(std::is_abstract_v<KisPreferenceSet>);
    static_assert(std::is_base_of_v<QWidget, KisPreferenceSet>);
    static_assert(std::is_constructible_v<PreferenceSetProbe, QWidget *>);

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
    ASSERT_PREFERENCE_MEMBER(KisPreferenceSet, id, QString (KisPreferenceSet::*)());
    ASSERT_PREFERENCE_MEMBER(KisPreferenceSet, name, QString (KisPreferenceSet::*)());
    ASSERT_PREFERENCE_MEMBER(KisPreferenceSet, header, QString (KisPreferenceSet::*)());
    ASSERT_PREFERENCE_MEMBER(KisPreferenceSet, icon, QIcon (KisPreferenceSet::*)());

    PreferenceSetProbe preferenceSet;
    QCOMPARE(preferenceSet.id(), QStringLiteral("probe.preference"));
    QCOMPARE(preferenceSet.name(), QStringLiteral("Probe Preferences"));
    QCOMPARE(preferenceSet.header(), QStringLiteral("Probe Header"));
    QVERIFY(preferenceSet.icon().isNull());
}

void KisPreferenceSetRegistryContractTest::preferenceSetPersistenceDispatchRemainsStable()
{
    ASSERT_PREFERENCE_MEMBER(KisPreferenceSet, savePreferences, void (KisPreferenceSet::*)() const);
    ASSERT_PREFERENCE_MEMBER(KisPreferenceSet, loadPreferences, void (KisPreferenceSet::*)());
    ASSERT_PREFERENCE_MEMBER(KisPreferenceSet, loadDefaultPreferences, void (KisPreferenceSet::*)());

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
    using Factory = KisAbstractPreferenceSetFactory;

    static_assert(std::is_class_v<Factory>);
    static_assert(std::is_abstract_v<Factory>);
    static_assert(std::has_virtual_destructor_v<Factory>);
    ASSERT_PREFERENCE_MEMBER(Factory, createPreferenceSet, KisPreferenceSet * (Factory::*)());
    ASSERT_PREFERENCE_MEMBER(Factory, id, QString (Factory::*)() const);

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
    using BaseRegistry = KoGenericRegistry<KisAbstractPreferenceSetFactory *>;

    static_assert(std::is_class_v<Registry>);
    static_assert(std::is_base_of_v<QObject, Registry>);
    static_assert(std::is_base_of_v<BaseRegistry, Registry>);
    static_assert(std::is_default_constructible_v<Registry>);
    static_assert(std::has_virtual_destructor_v<Registry>);
    static_assert(std::is_same_v<decltype(&Registry::instance), Registry *(*)()>);

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

#undef ASSERT_PREFERENCE_MEMBER

QTEST_MAIN(KisPreferenceSetRegistryContractTest)

#include "KisPreferenceSetRegistryContractTest.moc"
