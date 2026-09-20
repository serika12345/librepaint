/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <surfacecolormanagement/KisOutputColorInfoInterface.h>
#include <surfacecolormanagement/KisSurfaceColorManagerInterface.h>

#include <QPromise>
#include <QTest>

#include <memory>
#include <optional>
#include <utility>

namespace
{
using namespace KisSurfaceColorimetry;

SurfaceDescription
makeDescription(NamedPrimaries primaries, NamedTransferFunction transferFunction, uint32_t maximumLuminance)
{
    SurfaceDescription description;
    description.colorSpace.primaries = primaries;
    description.colorSpace.transferFunction = transferFunction;
    description.colorSpace.luminance = Luminance(5, maximumLuminance, 203);
    return description;
}

class OutputColorInfoProbe final : public KisOutputColorInfoInterface
{
public:
    OutputColorInfoProbe(bool ready,
                         const QScreen *describedScreen,
                         SurfaceDescription description,
                         bool *destroyed,
                         QObject *parent)
        : KisOutputColorInfoInterface(parent)
        , m_ready(ready)
        , m_describedScreen(describedScreen)
        , m_description(std::move(description))
        , m_destroyed(destroyed)
    {
    }

    ~OutputColorInfoProbe() override
    {
        *m_destroyed = true;
    }

    bool isReady() const override
    {
        ++readyCallCount;
        return m_ready;
    }

    std::optional<SurfaceDescription> outputDescription(const QScreen *screen) const override
    {
        ++descriptionCallCount;
        lastRequestedScreen = screen;
        return screen == m_describedScreen ? std::optional<SurfaceDescription>(m_description) : std::nullopt;
    }

    void publishDescriptionThenReady(QScreen *screen, const SurfaceDescription &description, bool ready)
    {
        Q_EMIT sigOutputDescriptionChanged(screen, description);
        Q_EMIT sigReadyChanged(ready);
    }

    mutable int readyCallCount{0};
    mutable int descriptionCallCount{0};
    mutable const QScreen *lastRequestedScreen{nullptr};

private:
    bool m_ready;
    const QScreen *m_describedScreen;
    SurfaceDescription m_description;
    bool *m_destroyed;
};

class SurfaceColorManagerProbe final : public KisSurfaceColorManagerInterface
{
public:
    SurfaceColorManagerProbe(QWindow *window,
                             bool ready,
                             SurfaceDescription supportedDescription,
                             RenderIntent supportedIntent,
                             SurfaceDescription currentDescription,
                             RenderIntent currentIntent,
                             SurfaceDescription preferredDescription,
                             bool *destroyed,
                             QObject *parent)
        : KisSurfaceColorManagerInterface(window, parent)
        , m_ready(ready)
        , m_supportedDescription(std::move(supportedDescription))
        , m_supportedIntent(supportedIntent)
        , m_currentDescription(std::move(currentDescription))
        , m_currentIntent(currentIntent)
        , m_preferredDescription(std::move(preferredDescription))
        , m_destroyed(destroyed)
    {
    }

    ~SurfaceColorManagerProbe() override
    {
        *m_destroyed = true;
    }

    bool isReady() const override
    {
        ++readyCallCount;
        return m_ready;
    }

    bool supportsSurfaceDescription(const SurfaceDescription &description) override
    {
        ++supportsDescriptionCallCount;
        lastSupportedDescription = description;
        return description == m_supportedDescription;
    }

    bool supportsRenderIntent(const RenderIntent &intent) override
    {
        ++supportsIntentCallCount;
        lastSupportedIntent = intent;
        return intent == m_supportedIntent;
    }

    QFuture<bool> setSurfaceDescription(const SurfaceDescription &description, RenderIntent intent) override
    {
        ++setDescriptionCallCount;
        lastSetDescription = description;
        lastSetIntent = intent;
        m_setPromise = std::make_shared<QPromise<bool>>();
        m_setPromise->start();
        return m_setPromise->future();
    }

    void unsetSurfaceDescription() override
    {
        ++unsetDescriptionCallCount;
    }

    std::optional<SurfaceDescription> surfaceDescription() const override
    {
        ++surfaceDescriptionCallCount;
        return m_currentDescription;
    }

    std::optional<RenderIntent> renderingIntent() const override
    {
        ++renderingIntentCallCount;
        return m_currentIntent;
    }

    std::optional<SurfaceDescription> preferredSurfaceDescription() const override
    {
        ++preferredDescriptionCallCount;
        return m_preferredDescription;
    }

    QWindow *storedWindow() const
    {
        return m_window;
    }

    void completeSetDescription(bool result)
    {
        m_setPromise->addResult(result);
        m_setPromise->finish();
    }

    void publishReadyThenPreferred(bool ready, const SurfaceDescription &description)
    {
        Q_EMIT sigReadyChanged(ready);
        Q_EMIT sigPreferredSurfaceDescriptionChanged(description);
    }

    mutable int readyCallCount{0};
    int supportsDescriptionCallCount{0};
    int supportsIntentCallCount{0};
    int setDescriptionCallCount{0};
    int unsetDescriptionCallCount{0};
    mutable int surfaceDescriptionCallCount{0};
    mutable int renderingIntentCallCount{0};
    mutable int preferredDescriptionCallCount{0};
    std::optional<SurfaceDescription> lastSupportedDescription;
    std::optional<RenderIntent> lastSupportedIntent;
    std::optional<SurfaceDescription> lastSetDescription;
    std::optional<RenderIntent> lastSetIntent;

private:
    bool m_ready;
    SurfaceDescription m_supportedDescription;
    RenderIntent m_supportedIntent;
    SurfaceDescription m_currentDescription;
    RenderIntent m_currentIntent;
    SurfaceDescription m_preferredDescription;
    bool *m_destroyed;
    std::shared_ptr<QPromise<bool>> m_setPromise;
};
} // namespace

class SurfaceColorInterfacesContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void outputInterfacePreservesOwnershipDispatchAndSignalSequence();
    void surfaceManagerPreservesWindowDispatchFutureAndSignalSequence();
};

void SurfaceColorInterfacesContractTest::outputInterfacePreservesOwnershipDispatchAndSignalSequence()
{
    QObject screenToken;
    QObject unmatchedScreenToken;
    QScreen *screen = reinterpret_cast<QScreen *>(&screenToken);
    const QScreen *unmatchedScreen = reinterpret_cast<const QScreen *>(&unmatchedScreenToken);
    const SurfaceDescription description =
        makeDescription(NamedPrimaries::primaries_bt2020, NamedTransferFunction::transfer_function_st2084_pq, 1000);
    bool destroyed = false;

    {
        QObject owner;
        auto *probe = new OutputColorInfoProbe(true, screen, description, &destroyed, &owner);
        KisOutputColorInfoInterface *interface = probe;

        QCOMPARE(interface->parent(), &owner);
        QCOMPARE(owner.children(), QObjectList{interface});
        QVERIFY(interface->isReady());
        QCOMPARE(probe->readyCallCount, 1);

        const std::optional<SurfaceDescription> observedDescription = interface->outputDescription(screen);
        QVERIFY(observedDescription && *observedDescription == description);
        QCOMPARE(probe->lastRequestedScreen, screen);
        QVERIFY(!interface->outputDescription(unmatchedScreen));
        QCOMPARE(probe->lastRequestedScreen, unmatchedScreen);
        QCOMPARE(probe->descriptionCallCount, 2);

        QStringList signalSequence;
        QScreen *signaledScreen = nullptr;
        SurfaceDescription signaledDescription;
        bool signaledReady = false;
        connect(interface,
                &KisOutputColorInfoInterface::sigOutputDescriptionChanged,
                [&](QScreen *value, const SurfaceDescription &valueDescription) {
                    signalSequence.append(QStringLiteral("description"));
                    signaledScreen = value;
                    signaledDescription = valueDescription;
                });
        connect(interface, &KisOutputColorInfoInterface::sigReadyChanged, [&](bool value) {
            signalSequence.append(QStringLiteral("ready"));
            signaledReady = value;
        });

        probe->publishDescriptionThenReady(screen, description, true);
        QCOMPARE(signalSequence, QStringList({QStringLiteral("description"), QStringLiteral("ready")}));
        QCOMPARE(signaledScreen, screen);
        QVERIFY(signaledDescription == description);
        QVERIFY(signaledReady);
        QVERIFY(!destroyed);
    }

    QVERIFY(destroyed);
}

void SurfaceColorInterfacesContractTest::surfaceManagerPreservesWindowDispatchFutureAndSignalSequence()
{
    QObject windowToken;
    QWindow *window = reinterpret_cast<QWindow *>(&windowToken);
    const SurfaceDescription supportedDescription =
        makeDescription(NamedPrimaries::primaries_display_p3, NamedTransferFunction::transfer_function_ext_linear, 203);
    const SurfaceDescription unsupportedDescription =
        makeDescription(NamedPrimaries::primaries_adobe_rgb, NamedTransferFunction::transfer_function_gamma22, 160);
    const SurfaceDescription currentDescription =
        makeDescription(NamedPrimaries::primaries_srgb, NamedTransferFunction::transfer_function_srgb, 80);
    const SurfaceDescription preferredDescription =
        makeDescription(NamedPrimaries::primaries_bt2020, NamedTransferFunction::transfer_function_st2084_pq, 1200);
    const RenderIntent supportedIntent = RenderIntent::render_intent_relative_bpc;
    const RenderIntent unsupportedIntent = RenderIntent::render_intent_saturation;
    const RenderIntent currentIntent = RenderIntent::render_intent_perceptual;
    bool destroyed = false;

    {
        QObject owner;
        auto *probe = new SurfaceColorManagerProbe(window,
                                                   true,
                                                   supportedDescription,
                                                   supportedIntent,
                                                   currentDescription,
                                                   currentIntent,
                                                   preferredDescription,
                                                   &destroyed,
                                                   &owner);
        KisSurfaceColorManagerInterface *interface = probe;

        QCOMPARE(interface->parent(), &owner);
        QCOMPARE(owner.children(), QObjectList{interface});
        QCOMPARE(probe->storedWindow(), window);
        QVERIFY(interface->isReady());
        QCOMPARE(probe->readyCallCount, 1);

        QVERIFY(interface->supportsSurfaceDescription(supportedDescription));
        QVERIFY(probe->lastSupportedDescription && *probe->lastSupportedDescription == supportedDescription);
        QVERIFY(!interface->supportsSurfaceDescription(unsupportedDescription));
        QVERIFY(probe->lastSupportedDescription && *probe->lastSupportedDescription == unsupportedDescription);
        QCOMPARE(probe->supportsDescriptionCallCount, 2);

        QVERIFY(interface->supportsRenderIntent(supportedIntent));
        QVERIFY(probe->lastSupportedIntent && *probe->lastSupportedIntent == supportedIntent);
        QVERIFY(!interface->supportsRenderIntent(unsupportedIntent));
        QVERIFY(probe->lastSupportedIntent && *probe->lastSupportedIntent == unsupportedIntent);
        QCOMPARE(probe->supportsIntentCallCount, 2);

        const std::optional<SurfaceDescription> observedCurrentDescription = interface->surfaceDescription();
        QVERIFY(observedCurrentDescription && *observedCurrentDescription == currentDescription);
        const std::optional<RenderIntent> observedCurrentIntent = interface->renderingIntent();
        QVERIFY(observedCurrentIntent && *observedCurrentIntent == currentIntent);
        const std::optional<SurfaceDescription> observedPreferredDescription = interface->preferredSurfaceDescription();
        QVERIFY(observedPreferredDescription && *observedPreferredDescription == preferredDescription);
        QCOMPARE(probe->surfaceDescriptionCallCount, 1);
        QCOMPARE(probe->renderingIntentCallCount, 1);
        QCOMPARE(probe->preferredDescriptionCallCount, 1);

        QFuture<bool> setResult = interface->setSurfaceDescription(supportedDescription, supportedIntent);
        QCOMPARE(probe->setDescriptionCallCount, 1);
        QVERIFY(probe->lastSetDescription && *probe->lastSetDescription == supportedDescription);
        QVERIFY(probe->lastSetIntent && *probe->lastSetIntent == supportedIntent);
        QVERIFY(setResult.isStarted());
        QVERIFY(!setResult.isFinished());
        probe->completeSetDescription(true);
        setResult.waitForFinished();
        QVERIFY(setResult.isFinished());
        QCOMPARE(setResult.resultCount(), 1);
        QVERIFY(setResult.result());

        interface->unsetSurfaceDescription();
        QCOMPARE(probe->unsetDescriptionCallCount, 1);

        QStringList signalSequence;
        bool signaledReady = false;
        SurfaceDescription signaledPreferredDescription;
        connect(interface, &KisSurfaceColorManagerInterface::sigReadyChanged, [&](bool value) {
            signalSequence.append(QStringLiteral("ready"));
            signaledReady = value;
        });
        connect(interface,
                &KisSurfaceColorManagerInterface::sigPreferredSurfaceDescriptionChanged,
                [&](const SurfaceDescription &value) {
                    signalSequence.append(QStringLiteral("preferred"));
                    signaledPreferredDescription = value;
                });

        probe->publishReadyThenPreferred(true, preferredDescription);
        QCOMPARE(signalSequence, QStringList({QStringLiteral("ready"), QStringLiteral("preferred")}));
        QVERIFY(signaledReady);
        QVERIFY(signaledPreferredDescription == preferredDescription);
        QVERIFY(!destroyed);
    }

    QVERIFY(destroyed);
}

QTEST_GUILESS_MAIN(SurfaceColorInterfacesContractTest)

#include "SurfaceColorInterfacesContractTest.moc"
