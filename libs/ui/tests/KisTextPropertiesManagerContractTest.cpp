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
#include <QVariant>

namespace
{
KoSvgTextProperties propertiesWithWeight(int weight)
{
    KoSvgTextProperties properties;
    properties.setProperty(KoSvgTextProperties::FontWeightId, weight);
    return properties;
}

KoSvgTextProperties propertiesWithValue(KoSvgTextProperties::PropertyId id, const QVariant &value)
{
    KoSvgTextProperties properties;
    properties.setProperty(id, value);
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
        return characterProperties;
    }

    KoSvgTextProperties getInheritedProperties() override
    {
        return inheritedProperties;
    }

    void setPropertiesOnSelected(KoSvgTextProperties properties,
                                 QSet<KoSvgTextProperties::PropertyId> removeProperties) override
    {
        ++setPropertiesCallCount;
        lastSetProperties = properties;
        lastRemovedProperties = removeProperties;
    }

    void setCharacterPropertiesOnSelected(KoSvgTextProperties properties,
                                          QSet<KoSvgTextProperties::PropertyId> removeProperties) override
    {
        ++setCharacterPropertiesCallCount;
        lastSetCharacterProperties = properties;
        lastRemovedCharacterProperties = removeProperties;
    }

    bool spanSelection() override
    {
        return selectsSpan;
    }

    bool characterPropertiesEnabled() override
    {
        return characterSelectionEnabled;
    }

    void notifyTextSelectionChanged()
    {
        Q_EMIT textSelectionChanged();
    }

    void notifyCharacterSelectionChanged()
    {
        Q_EMIT textCharacterSelectionChanged();
    }

    QList<KoSvgTextProperties> selectedProperties;
    QList<KoSvgTextProperties> characterProperties;
    KoSvgTextProperties inheritedProperties;
    KoSvgTextProperties lastSetProperties;
    KoSvgTextProperties lastSetCharacterProperties;
    QSet<KoSvgTextProperties::PropertyId> lastRemovedProperties;
    QSet<KoSvgTextProperties::PropertyId> lastRemovedCharacterProperties;
    int setPropertiesCallCount {0};
    int setCharacterPropertiesCallCount {0};
    bool selectsSpan {false};
    bool characterSelectionEnabled {false};
};
} // namespace

class KisTextPropertiesManagerContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void mixedSelectionAndDockerEditsReachTheTextTool();
    void characterSelectionAndDockerEditsReachTheTextTool();
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

    textTool.selectedProperties = {propertiesWithWeight(500)};
    textTool.notifyTextSelectionChanged();

    QCOMPARE(canvasStateChanged.count(), 2);
    QCOMPARE(textTool.setPropertiesCallCount, 0);
    dockerState = canvasProvider.textPropertyData();
    QVERIFY(!dockerState.tristate.contains(KoSvgTextProperties::FontWeightId));
    QCOMPARE(dockerState.commonProperties.property(KoSvgTextProperties::FontWeightId).toInt(), 500);

    dockerState.tristate.remove(KoSvgTextProperties::FontWeightId);
    dockerState.commonProperties.setProperty(KoSvgTextProperties::FontWeightId, 700);
    canvasProvider.setTextPropertyData(dockerState);

    QCOMPARE(canvasStateChanged.count(), 3);
    QCOMPARE(textTool.setPropertiesCallCount, 1);
    QCOMPARE(textTool.lastSetProperties.property(KoSvgTextProperties::FontWeightId).toInt(), 700);
    QVERIFY(textTool.lastRemovedProperties.isEmpty());

    dockerState.commonProperties.removeProperty(KoSvgTextProperties::FontWeightId);
    canvasProvider.setTextPropertyData(dockerState);

    QCOMPARE(canvasStateChanged.count(), 4);
    QCOMPARE(textTool.setPropertiesCallCount, 2);
    QVERIFY(!textTool.lastSetProperties.hasProperty(KoSvgTextProperties::FontWeightId));
    QVERIFY(textTool.lastRemovedProperties.contains(KoSvgTextProperties::FontWeightId));
}

void KisTextPropertiesManagerContractTest::characterSelectionAndDockerEditsReachTheTextTool()
{
    // Consumer: The text-properties docker and SVG text tool during character-range editing.
    // Operation: Select characters, update the selection, choose a weight in the docker, then clear it.
    // Observable result: The canvas preserves inherited and span state, then sends character formatting changes to the text tool.
    // Failure impact: Character formatting controls show the wrong state or fail to apply and clear formatting in selected text.
    KoCanvasResourceProvider resourceManager;
    KisCanvasResourceProvider canvasProvider(nullptr);
    canvasProvider.setResourceManager(&resourceManager);

    TextPropertiesInterfaceProbe textTool;
    textTool.characterSelectionEnabled = true;
    textTool.selectsSpan = true;
    textTool.characterProperties = {propertiesWithWeight(400)};
    textTool.inheritedProperties = propertiesWithValue(KoSvgTextProperties::DirectionId, QStringLiteral("rtl"));

    KisTextPropertiesManager manager;
    manager.setCanvasResourceProvider(&canvasProvider);

    QSignalSpy canvasStateChanged(&canvasProvider, &KisCanvasResourceProvider::sigCharacterPropertiesChanged);
    manager.setTextPropertiesInterface(&textTool);

    QCOMPARE(canvasStateChanged.count(), 1);
    QCOMPARE(textTool.setCharacterPropertiesCallCount, 0);

    KoSvgTextPropertyData dockerState = canvasProvider.characterTextPropertyData();
    QVERIFY(dockerState.enabled);
    QVERIFY(dockerState.spanSelection);
    QCOMPARE(dockerState.commonProperties.property(KoSvgTextProperties::FontWeightId).toInt(), 400);
    QCOMPARE(dockerState.inheritedProperties.property(KoSvgTextProperties::DirectionId).toString(), QStringLiteral("rtl"));

    textTool.characterProperties = {propertiesWithWeight(500)};
    textTool.notifyCharacterSelectionChanged();

    QCOMPARE(canvasStateChanged.count(), 2);
    QCOMPARE(textTool.setCharacterPropertiesCallCount, 0);
    dockerState = canvasProvider.characterTextPropertyData();
    QCOMPARE(dockerState.commonProperties.property(KoSvgTextProperties::FontWeightId).toInt(), 500);

    dockerState.commonProperties.setProperty(KoSvgTextProperties::FontWeightId, 700);
    canvasProvider.setCharacterPropertyData(dockerState);

    QCOMPARE(canvasStateChanged.count(), 3);
    QCOMPARE(textTool.setCharacterPropertiesCallCount, 1);
    QCOMPARE(textTool.lastSetCharacterProperties.property(KoSvgTextProperties::FontWeightId).toInt(), 700);
    QVERIFY(textTool.lastRemovedCharacterProperties.isEmpty());

    dockerState.commonProperties.removeProperty(KoSvgTextProperties::FontWeightId);
    canvasProvider.setCharacterPropertyData(dockerState);

    QCOMPARE(canvasStateChanged.count(), 4);
    QCOMPARE(textTool.setCharacterPropertiesCallCount, 2);
    QVERIFY(!textTool.lastSetCharacterProperties.hasProperty(KoSvgTextProperties::FontWeightId));
    QVERIFY(textTool.lastRemovedCharacterProperties.contains(KoSvgTextProperties::FontWeightId));
}

QTEST_MAIN(KisTextPropertiesManagerContractTest)

#include "KisTextPropertiesManagerContractTest.moc"
