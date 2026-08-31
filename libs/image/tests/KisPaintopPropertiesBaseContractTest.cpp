/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "brushengine/KisPaintopPropertiesBase.h"

#include <QTest>

#include <memory>

class KisPropertiesConfiguration
{
public:
    explicit KisPropertiesConfiguration(int token)
        : token(token)
    {
    }

    int token;
};

class KisResourcesInterface
{
public:
    explicit KisResourcesInterface(int token)
        : token(token)
    {
    }

    int token;
};

class KoCanvasResourcesInterface
{
public:
    explicit KoCanvasResourcesInterface(int token)
        : token(token)
    {
    }

    int token;
};

class KoResourceLoadResult
{
public:
    explicit KoResourceLoadResult(int token)
        : token(token)
    {
    }

    int token;
};

namespace
{

class BasicProbe final : public KisPaintopPropertiesBase
{
public:
    explicit BasicProbe(int *destructionCount = nullptr)
        : m_destructionCount(destructionCount)
    {
    }

    ~BasicProbe() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    const KisPropertiesConfiguration *readSettings = nullptr;
    mutable KisPropertiesConfiguration *writtenSettings = nullptr;
    int readCount = 0;
    mutable int writeCount = 0;

protected:
    void readOptionSettingImpl(const KisPropertiesConfiguration *settings) override
    {
        readSettings = settings;
        ++readCount;
    }

    void writeOptionSettingImpl(KisPropertiesConfiguration *settings) const override
    {
        writtenSettings = settings;
        ++writeCount;
    }

private:
    int *m_destructionCount;
};

class ResourcesProbe final : public KisPaintopPropertiesResourcesBase
{
public:
    explicit ResourcesProbe(int *destructionCount = nullptr)
        : m_destructionCount(destructionCount)
    {
    }

    ~ResourcesProbe() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    const KisPropertiesConfiguration *readSettings = nullptr;
    const KisResourcesInterface *readResources = nullptr;
    mutable KisPropertiesConfiguration *writtenSettings = nullptr;
    mutable const KisPropertiesConfiguration *linkedSettings = nullptr;
    mutable const KisResourcesInterface *linkedResources = nullptr;
    mutable const KisPropertiesConfiguration *embeddedSettings = nullptr;
    mutable const KisResourcesInterface *embeddedResources = nullptr;

protected:
    void readOptionSettingResourceImpl(const KisPropertiesConfiguration *settings,
                                       KisResourcesInterfaceSP resourcesInterface) override
    {
        readSettings = settings;
        readResources = resourcesInterface.data();
    }

    void writeOptionSettingImpl(KisPropertiesConfiguration *settings) const override
    {
        writtenSettings = settings;
    }

    QList<KoResourceLoadResult> prepareLinkedResourcesImpl(const KisPropertiesConfiguration *settings,
                                                           KisResourcesInterfaceSP resourcesInterface) const override
    {
        linkedSettings = settings;
        linkedResources = resourcesInterface.data();
        return {KoResourceLoadResult(41)};
    }

    QList<KoResourceLoadResult> prepareEmbeddedResourcesImpl(const KisPropertiesConfiguration *settings,
                                                             KisResourcesInterfaceSP resourcesInterface) const override
    {
        embeddedSettings = settings;
        embeddedResources = resourcesInterface.data();
        return {KoResourceLoadResult(42)};
    }

private:
    int *m_destructionCount;
};

class CanvasResourcesProbe final : public KisPaintopPropertiesCanvasResourcesBase
{
public:
    explicit CanvasResourcesProbe(int *destructionCount = nullptr)
        : m_destructionCount(destructionCount)
    {
    }

    ~CanvasResourcesProbe() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    const KisPropertiesConfiguration *readSettings = nullptr;
    const KisResourcesInterface *readResources = nullptr;
    const KoCanvasResourcesInterface *readCanvasResources = nullptr;
    mutable KisPropertiesConfiguration *writtenSettings = nullptr;
    mutable const KisPropertiesConfiguration *linkedSettings = nullptr;
    mutable const KisResourcesInterface *linkedResources = nullptr;
    mutable const KisPropertiesConfiguration *embeddedSettings = nullptr;
    mutable const KisResourcesInterface *embeddedResources = nullptr;

protected:
    void readOptionSettingResourceImpl(const KisPropertiesConfiguration *settings,
                                       KisResourcesInterfaceSP resourcesInterface,
                                       KoCanvasResourcesInterfaceSP canvasResourcesInterface) override
    {
        readSettings = settings;
        readResources = resourcesInterface.data();
        readCanvasResources = canvasResourcesInterface.data();
    }

    void writeOptionSettingImpl(KisPropertiesConfiguration *settings) const override
    {
        writtenSettings = settings;
    }

    QList<KoResourceLoadResult> prepareLinkedResourcesImpl(const KisPropertiesConfiguration *settings,
                                                           KisResourcesInterfaceSP resourcesInterface) const override
    {
        linkedSettings = settings;
        linkedResources = resourcesInterface.data();
        return {KoResourceLoadResult(51)};
    }

    QList<KoResourceLoadResult> prepareEmbeddedResourcesImpl(const KisPropertiesConfiguration *settings,
                                                             KisResourcesInterfaceSP resourcesInterface) const override
    {
        embeddedSettings = settings;
        embeddedResources = resourcesInterface.data();
        return {KoResourceLoadResult(52)};
    }

private:
    int *m_destructionCount;
};

} // namespace

class KisPaintopPropertiesBaseContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void basicWrapperForwardsRawAndSharedSettings();
    void resourcesWrapperForwardsSettingsOwnerAndResults();
    void canvasWrapperForwardsCollaboratorsAndResults();
    void basePointersPreserveVirtualLifetimes();
};

void KisPaintopPropertiesBaseContractTest::basicWrapperForwardsRawAndSharedSettings()
{
    BasicProbe probe;
    KisPropertiesConfiguration rawSettings(11);
    QSharedPointer<KisPropertiesConfiguration> sharedSettings(new KisPropertiesConfiguration(12));

    probe.readOptionSetting(&rawSettings);
    probe.writeOptionSetting(sharedSettings);

    QCOMPARE(probe.readSettings, &rawSettings);
    QCOMPARE(probe.readSettings->token, 11);
    QCOMPARE(probe.writtenSettings, sharedSettings.data());
    QCOMPARE(probe.writtenSettings->token, 12);
    QCOMPARE(probe.readCount, 1);
    QCOMPARE(probe.writeCount, 1);
}

void KisPaintopPropertiesBaseContractTest::resourcesWrapperForwardsSettingsOwnerAndResults()
{
    ResourcesProbe probe;
    QSharedPointer<KisPropertiesConfiguration> settings(new KisPropertiesConfiguration(21));
    KisResourcesInterfaceSP resources(new KisResourcesInterface(22));

    probe.readOptionSetting(settings, resources);
    probe.writeOptionSetting(settings);
    const QList<KoResourceLoadResult> linked = probe.prepareLinkedResources(settings, resources);
    const QList<KoResourceLoadResult> embedded = probe.prepareEmbeddedResources(settings, resources);

    QCOMPARE(probe.readSettings, settings.data());
    QCOMPARE(probe.readResources, resources.data());
    QCOMPARE(probe.writtenSettings, settings.data());
    QCOMPARE(probe.linkedSettings, settings.data());
    QCOMPARE(probe.linkedResources, resources.data());
    QCOMPARE(linked.size(), 1);
    QCOMPARE(linked.constFirst().token, 41);
    QCOMPARE(probe.embeddedSettings, settings.data());
    QCOMPARE(probe.embeddedResources, resources.data());
    QCOMPARE(embedded.size(), 1);
    QCOMPARE(embedded.constFirst().token, 42);
}

void KisPaintopPropertiesBaseContractTest::canvasWrapperForwardsCollaboratorsAndResults()
{
    CanvasResourcesProbe probe;
    KisPropertiesConfiguration settings(31);
    KisResourcesInterfaceSP resources(new KisResourcesInterface(32));
    KoCanvasResourcesInterfaceSP canvasResources(new KoCanvasResourcesInterface(33));

    probe.readOptionSetting(&settings, resources, canvasResources);
    probe.writeOptionSetting(&settings);
    const QList<KoResourceLoadResult> linked = probe.prepareLinkedResources(&settings, resources);
    const QList<KoResourceLoadResult> embedded = probe.prepareEmbeddedResources(&settings, resources);

    QCOMPARE(probe.readSettings, &settings);
    QCOMPARE(probe.readResources, resources.data());
    QCOMPARE(probe.readCanvasResources, canvasResources.data());
    QCOMPARE(probe.writtenSettings, &settings);
    QCOMPARE(probe.linkedSettings, &settings);
    QCOMPARE(probe.linkedResources, resources.data());
    QCOMPARE(linked.size(), 1);
    QCOMPARE(linked.constFirst().token, 51);
    QCOMPARE(probe.embeddedSettings, &settings);
    QCOMPARE(probe.embeddedResources, resources.data());
    QCOMPARE(embedded.size(), 1);
    QCOMPARE(embedded.constFirst().token, 52);
}

void KisPaintopPropertiesBaseContractTest::basePointersPreserveVirtualLifetimes()
{
    int basicDestructionCount = 0;
    std::unique_ptr<KisPaintopPropertiesBase> basic(new BasicProbe(&basicDestructionCount));
    basic.reset();
    QCOMPARE(basicDestructionCount, 1);

    int resourcesDestructionCount = 0;
    std::unique_ptr<KisPaintopPropertiesResourcesBase> resources(new ResourcesProbe(&resourcesDestructionCount));
    resources.reset();
    QCOMPARE(resourcesDestructionCount, 1);

    int canvasDestructionCount = 0;
    std::unique_ptr<KisPaintopPropertiesCanvasResourcesBase> canvas(new CanvasResourcesProbe(&canvasDestructionCount));
    canvas.reset();
    QCOMPARE(canvasDestructionCount, 1);
}

QTEST_GUILESS_MAIN(KisPaintopPropertiesBaseContractTest)

#include "KisPaintopPropertiesBaseContractTest.moc"
