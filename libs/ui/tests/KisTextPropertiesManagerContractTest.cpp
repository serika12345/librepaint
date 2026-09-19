/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoCanvasResourceProvider.h>
#include <KoSvgTextPropertyData.h>
#include <KoSvgTextPropertiesInterface.h>
#include <canvas/kis_canvas_resource_provider.h>
#include <document/KisTextPropertiesManager.h>

#include <QSignalSpy>
#include <QTest>

namespace
{
KoSvgTextProperties propertiesWithWeight(int weight)
{
    KoSvgTextProperties properties;
    properties.setProperty(KoSvgTextProperties::FontWeightId, weight);
    return properties;
}

class TextPropertiesInterfaceProbe final : public KoSvgTextPropertiesInterface
{
public:
    QList<KoSvgTextProperties> getSelectedProperties() override
    {
        return selectedProperties;
    }

    QList<KoSvgTextProperties> getCharacterProperties() override
    {
        return {};
    }

    KoSvgTextProperties getInheritedProperties() override
    {
        return {};
    }

    void setPropertiesOnSelected(KoSvgTextProperties properties,
                                 QSet<KoSvgTextProperties::PropertyId> removeProperties) override
    {
        ++setPropertiesCallCount;
        lastSetProperties = properties;
        lastRemovedProperties = removeProperties;
    }

    void setCharacterPropertiesOnSelected(KoSvgTextProperties,
                                          QSet<KoSvgTextProperties::PropertyId>) override
    {
    }

    bool spanSelection() override
    {
        return false;
    }

    bool characterPropertiesEnabled() override
    {
        return false;
    }

    QList<KoSvgTextProperties> selectedProperties;
    KoSvgTextProperties lastSetProperties;
    QSet<KoSvgTextProperties::PropertyId> lastRemovedProperties;
    int setPropertiesCallCount {0};
};
} // namespace

class KisTextPropertiesManagerContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void mixedSelectionAndDockerEditsReachTheTextTool();
};

void KisTextPropertiesManagerContractTest::mixedSelectionAndDockerEditsReachTheTextTool()
{
    // Consumer: The text-properties docker and SVG text tool.
    // Operation: Select text with mixed weights, choose a weight in the docker, then clear it.
    // Observable result: The canvas exposes the mixed state and sends the selected value or removal to the text tool.
    // Failure impact: The docker hides mixed formatting, or font changes and resets do not reach selected text.
    KoCanvasResourceProvider resourceManager;
    KisCanvasResourceProvider canvasProvider(nullptr);
    canvasProvider.setResourceManager(&resourceManager);

    TextPropertiesInterfaceProbe textTool;
    textTool.selectedProperties = {
        propertiesWithWeight(400),
        propertiesWithWeight(600),
    };

    KisTextPropertiesManager manager;
    manager.setCanvasResourceProvider(&canvasProvider);

    QSignalSpy canvasStateChanged(&canvasProvider, &KisCanvasResourceProvider::sigTextPropertiesChanged);
    manager.setTextPropertiesInterface(&textTool);

    QCOMPARE(canvasStateChanged.count(), 1);
    QCOMPARE(textTool.setPropertiesCallCount, 0);

    KoSvgTextPropertyData dockerState = canvasProvider.textPropertyData();
    QVERIFY(dockerState.enabled);
    QVERIFY(dockerState.tristate.contains(KoSvgTextProperties::FontWeightId));
    QVERIFY(!dockerState.commonProperties.hasProperty(KoSvgTextProperties::FontWeightId));

    dockerState.tristate.remove(KoSvgTextProperties::FontWeightId);
    dockerState.commonProperties.setProperty(KoSvgTextProperties::FontWeightId, 700);
    canvasProvider.setTextPropertyData(dockerState);

    QCOMPARE(canvasStateChanged.count(), 2);
    QCOMPARE(textTool.setPropertiesCallCount, 1);
    QCOMPARE(textTool.lastSetProperties.property(KoSvgTextProperties::FontWeightId).toInt(), 700);
    QVERIFY(textTool.lastRemovedProperties.isEmpty());

    dockerState.commonProperties.removeProperty(KoSvgTextProperties::FontWeightId);
    canvasProvider.setTextPropertyData(dockerState);

    QCOMPARE(canvasStateChanged.count(), 3);
    QCOMPARE(textTool.setPropertiesCallCount, 2);
    QVERIFY(!textTool.lastSetProperties.hasProperty(KoSvgTextProperties::FontWeightId));
    QVERIFY(textTool.lastRemovedProperties.contains(KoSvgTextProperties::FontWeightId));
}

QTEST_MAIN(KisTextPropertiesManagerContractTest)

#include "KisTextPropertiesManagerContractTest.moc"
