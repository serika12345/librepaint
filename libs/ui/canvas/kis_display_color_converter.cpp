/*
 * SPDX-FileCopyrightText: 2014 Dmitry Kazakov <dimula73@gmail.com>
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "kis_display_color_converter.h"

#include <cmath>

#include <QGlobalStatic>
#include <QApplication>
#include <QPalette>
#include <QPointer>

#include <KoCanvasResourceProvider.h>
#include <KoColor.h>
#include <KoColorDisplayRendererInterface.h>
#include <KoColorSpaceRegistry.h>

#include <color/kis_display_color_transform.h>
#include <kis_fixed_paint_device.h>
#include <kis_paint_device.h>

#include "canvas/KisDisplayConfig.h"
#include "workspace/KisMainWindow.h"
#include "application/KisPart.h"
#include "canvas/kis_canvas_resource_provider.h"
#include "application/kis_config.h"
#include "kis_config_notifier.h"
#include "kis_node.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QGuiApplication>
#endif

Q_GLOBAL_STATIC(KisDisplayColorConverter, s_instance)

struct KisDisplayColorConverter::Private
{
    Private(KisDisplayColorConverter *converter,
            KoCanvasResourceProvider *canvasResourceManager)
        : q(converter)
        , resourceManager(canvasResourceManager)
        , displayRenderer(
              new DisplayRenderer(converter, canvasResourceManager))
    {
    }

    KisDisplayColorConverter *const q;
    KoCanvasResourceProvider *resourceManager {nullptr};
    const KoColorSpace *nodeColorSpace {nullptr};
    KisMultiSurfaceDisplayConfig multiSurfaceDisplayConfig;
    QSharedPointer<KisDisplayFilter> displayFilter;
    KoColor intermediateForegroundColor;
    KisNodeSP connectedNode;
    KisHandlePalette handlePalette;
    QPalette systemPalette;
    KisDisplayColorTransform transform;

    void notifyDisplayConfigurationChanged()
    {
        Q_EMIT q->displayConfigurationChanged();
    }

    void slotCanvasResourceChanged(int key, const QVariant &value);
    void slotUpdateCurrentNodeColorSpace();
    void selectPaintingColorSpace();
    void updateIntermediateForegroundColor(const KoColor &color);
    void setCurrentNode(KisNodeSP node);

    class DisplayRenderer : public KoColorDisplayRendererInterface
    {
    public:
        DisplayRenderer(KisDisplayColorConverter *displayColorConverter,
                        KoCanvasResourceProvider *resourceManager)
            : m_displayColorConverter(displayColorConverter)
            , m_resourceManager(resourceManager)
        {
            displayColorConverter->connect(
                displayColorConverter,
                SIGNAL(displayConfigurationChanged()),
                this,
                SIGNAL(displayConfigurationChanged()),
                Qt::UniqueConnection);
        }

        QImage toQImage(const KoColorSpace *sourceColorSpace,
                        const quint8 *data,
                        QSize size,
                        bool proofPaintColors = false) const override
        {
            return m_displayColorConverter->toQImage(
                sourceColorSpace, data, size, proofPaintColors);
        }

        QColor toQColor(const KoColor &color,
                        bool proofToPaintColors = false) const override
        {
            return m_displayColorConverter->toQColor(
                color, proofToPaintColors);
        }

        KoColor approximateFromRenderedQColor(
            const QColor &color) const override
        {
            return m_displayColorConverter->approximateFromRenderedQColor(
                color);
        }

        KoColor fromHsv(int h, int s, int v, int a) const override
        {
            return m_displayColorConverter->fromHsv(h, s, v, a);
        }

        void getHsv(const KoColor &color,
                    int *h,
                    int *s,
                    int *v,
                    int *a) const override
        {
            m_displayColorConverter->getHsv(color, h, s, v, a);
        }

        qreal minVisibleFloatValue(
            const KoChannelInfo *channelInfo) const override
        {
            return channelInfo->getUIMin();
        }

        qreal maxVisibleFloatValue(
            const KoChannelInfo *channelInfo) const override
        {
            qreal maximum = channelInfo->getUIMax();
            if (m_resourceManager) {
                const qreal exposure = m_resourceManager
                    ->resource(KoCanvasResource::HdrExposure)
                    .value<qreal>();
                maximum *= std::pow(2.0, -exposure);
            }
            return maximum;
        }

        const KoColorSpace *getPaintingColorSpace() const override
        {
            return m_displayColorConverter->paintingColorSpace();
        }

        QColor convertColorToDisplayColorSpace(
            KoColor color) const override
        {
            return m_displayColorConverter->convertColorToDisplayColorSpace(
                color);
        }

        QImage convertImageToDisplayColorSpace(
            const QImage source) const override
        {
            KisPaintDeviceSP sourceDevice = new KisPaintDevice(
                KoColorSpaceRegistry::instance()->rgb8());
            sourceDevice->convertFromQImage(
                source,
                KoColorSpaceRegistry::instance()->p709SRGBProfile(),
                0,
                0);
            QImage destination =
                m_displayColorConverter->convertImageToDisplayColorSpace(
                    sourceDevice);
            destination.setDevicePixelRatio(source.devicePixelRatio());
            return destination;
        }

        KisHandlePalette handlePaletteForDisplayColorSpace() const override
        {
            return m_displayColorConverter
                ->handlePaletteForDisplayColorSpace();
        }

        QPalette systemPaletteForDisplayColorSpace() const override
        {
            return m_displayColorConverter
                ->systemPaletteForDisplayColorSpace();
        }

    private:
        KisDisplayColorConverter *m_displayColorConverter;
        QPointer<KoCanvasResourceProvider> m_resourceManager;
    };

    QScopedPointer<KoColorDisplayRendererInterface> displayRenderer;
};

KisDisplayColorConverter::KisDisplayColorConverter(
    KoCanvasResourceProvider *resourceManager,
    QObject *parent)
    : QObject(parent)
    , m_d(new Private(this, resourceManager))
{
    connect(m_d->resourceManager,
            SIGNAL(canvasResourceChanged(int,QVariant)),
            SLOT(slotCanvasResourceChanged(int,QVariant)));
    connect(KisConfigNotifier::instance(),
            SIGNAL(configChanged()),
            SLOT(selectPaintingColorSpace()));

    m_d->setCurrentNode(nullptr);
    setDisplayFilter({});
    updatePalettes();
    connect(this,
            SIGNAL(displayConfigurationChanged()),
            this,
            SLOT(updatePalettes()));
    connect(KisPart::instance()->currentMainwindow(),
            SIGNAL(themeChanged()),
            this,
            SLOT(updatePalettes()));
}

KisDisplayColorConverter::KisDisplayColorConverter()
    : m_d(new Private(this, nullptr))
{
    setDisplayFilter({});
    m_d->setCurrentNode(nullptr);
}

KisDisplayColorConverter::~KisDisplayColorConverter() = default;

void KisDisplayColorConverter::setImageColorSpace(const KoColorSpace *colorSpace)
{
    m_d->transform.setInputColorSpace(colorSpace);
    m_d->notifyDisplayConfigurationChanged();
}

KisDisplayColorConverter *KisDisplayColorConverter::dumbConverterInstance()
{
    return s_instance;
}

KoColorDisplayRendererInterface *
KisDisplayColorConverter::displayRendererInterface() const
{
    return m_d->displayRenderer.data();
}

void KisDisplayColorConverter::Private::updateIntermediateForegroundColor(
    const KoColor &color)
{
    KIS_ASSERT_RECOVER_RETURN(displayFilter);
    intermediateForegroundColor = transform.captureVisualRepresentation(color);
}

void KisDisplayColorConverter::Private::slotCanvasResourceChanged(
    int key,
    const QVariant &value)
{
    if (key == KoCanvasResource::CurrentKritaNode) {
        setCurrentNode(value.value<KisNodeWSP>());
    } else if (transform.usesDisplayFilter() &&
               key == KoCanvasResource::ForegroundColor) {
        updateIntermediateForegroundColor(value.value<KoColor>());
    }
}

void KisDisplayColorConverter::Private::slotUpdateCurrentNodeColorSpace()
{
    setCurrentNode(connectedNode);
}

void KisDisplayColorConverter::updatePalettes()
{
    KisHandlePalette palette;
    KoColor color;

    color.fromQColor(palette.gradientFillColor);
    palette.gradientFillColor = convertColorToDisplayColorSpace(color);
    color.fromQColor(palette.highlightColor);
    palette.highlightColor = convertColorToDisplayColorSpace(color);
    color.fromQColor(palette.highlightOutlineColor);
    palette.highlightOutlineColor = convertColorToDisplayColorSpace(color);
    color.fromQColor(palette.primaryColor);
    palette.primaryColor = convertColorToDisplayColorSpace(color);
    color.fromQColor(palette.secondaryColor);
    palette.secondaryColor = convertColorToDisplayColorSpace(color);
    color.fromQColor(palette.selectionColor);
    palette.selectionColor = convertColorToDisplayColorSpace(color);
    color.fromQColor(palette.white);
    palette.white = convertColorToDisplayColorSpace(color);
    color.fromQColor(palette.black);
    palette.black = convertColorToDisplayColorSpace(color);

    QPalette systemPalette = qApp->palette();
    for (int role = 0; role < QPalette::NColorRoles; ++role) {
        for (int group = 0; group < QPalette::NColorGroups; ++group) {
            color.fromQColor(
                systemPalette
                    .brush(QPalette::ColorGroup(group),
                           QPalette::ColorRole(role))
                    .color());
            systemPalette.setBrush(
                QPalette::ColorGroup(group),
                QPalette::ColorRole(role),
                convertColorToDisplayColorSpace(color));
        }
    }

    m_d->handlePalette = palette;
    m_d->systemPalette = systemPalette;
}

namespace {
KisPaintDeviceSP findValidDevice(KisNodeSP node)
{
    return node->paintDevice() ? node->paintDevice() : node->original();
}
}

void KisDisplayColorConverter::Private::setCurrentNode(KisNodeSP node)
{
    if (connectedNode) {
        KisPaintDeviceSP device = findValidDevice(connectedNode);
        if (device) {
            q->disconnect(device, nullptr);
        }
    }

    nodeColorSpace = nullptr;
    if (node) {
        KisPaintDeviceSP device = findValidDevice(node);
        nodeColorSpace = device ? device->compositionSourceColorSpace()
                                : node->colorSpace();
        KIS_SAFE_ASSERT_RECOVER_NOOP(nodeColorSpace);

        if (device) {
            q->connect(device,
                       SIGNAL(profileChanged(const KoColorProfile*)),
                       SLOT(slotUpdateCurrentNodeColorSpace()),
                       Qt::UniqueConnection);
            q->connect(device,
                       SIGNAL(colorSpaceChanged(const KoColorSpace*)),
                       SLOT(slotUpdateCurrentNodeColorSpace()),
                       Qt::UniqueConnection);
        }
    }

    if (!nodeColorSpace) {
        nodeColorSpace = KoColorSpaceRegistry::instance()->rgb8();
    }
    connectedNode = node;
    selectPaintingColorSpace();
}

void KisDisplayColorConverter::Private::selectPaintingColorSpace()
{
    KisConfig config(true);
    const KoColorSpace *paintingColorSpace =
        config.customColorSelectorColorSpace();
    if (!paintingColorSpace || displayFilter) {
        paintingColorSpace = nodeColorSpace;
    }
    transform.setPaintingColorSpace(paintingColorSpace);
    notifyDisplayConfigurationChanged();
}

const KoColorSpace *KisDisplayColorConverter::paintingColorSpace() const
{
    KIS_SAFE_ASSERT_RECOVER(m_d->transform.paintingColorSpace()) {
        return KoColorSpaceRegistry::instance()->rgb8();
    }
    return m_d->transform.paintingColorSpace();
}

const KoColorSpace *KisDisplayColorConverter::nodeColorSpace() const
{
    return m_d->nodeColorSpace;
}

void KisDisplayColorConverter::setMultiSurfaceDisplayConfig(
    const KisMultiSurfaceDisplayConfig &config)
{
    if (m_d->multiSurfaceDisplayConfig == config) {
        return;
    }

    m_d->multiSurfaceDisplayConfig = config;
    m_d->transform.setDisplayConfiguration(config.uiProfile,
                                           config.canvasProfile,
                                           config.intent,
                                           config.conversionFlags);
    m_d->notifyDisplayConfigurationChanged();
}

void KisDisplayColorConverter::setDisplayFilter(
    QSharedPointer<KisDisplayFilter> displayFilter)
{
    if (m_d->displayFilter && displayFilter &&
        displayFilter->lockCurrentColorVisualRepresentation()) {
        m_d->transform.setDisplayFilter(displayFilter);
        if (m_d->resourceManager) {
            m_d->resourceManager->setForegroundColor(
                m_d->transform.restorePaintingColor(
                    m_d->intermediateForegroundColor));
        }
    } else {
        m_d->transform.setDisplayFilter(displayFilter);
    }

    m_d->displayFilter = std::move(displayFilter);
    if (m_d->displayFilter && m_d->resourceManager) {
        m_d->updateIntermediateForegroundColor(
            m_d->resourceManager->foregroundColor());
    }
    m_d->selectPaintingColorSpace();
}

KisDisplayConfig KisDisplayColorConverter::displayConfig() const
{
    return m_d->multiSurfaceDisplayConfig.uiDisplayConfig();
}

QSharedPointer<KisDisplayFilter>
KisDisplayColorConverter::displayFilter() const
{
    return m_d->displayFilter;
}

KisMultiSurfaceDisplayConfig
KisDisplayColorConverter::multiSurfaceDisplayConfig() const
{
    return m_d->multiSurfaceDisplayConfig;
}

KisDisplayColorConverter::ConversionOptions
KisDisplayColorConverter::conversionOptions() const
{
    return m_d->transform.conversionOptions();
}

QColor KisDisplayColorConverter::toQColor(const KoColor &color,
                                          bool proofToPaintColors) const
{
    return m_d->transform.toQColor(color, proofToPaintColors);
}

KoColor KisDisplayColorConverter::approximateFromRenderedQColor(
    const QColor &color) const
{
    return m_d->transform.approximateFromRenderedQColor(color);
}

bool KisDisplayColorConverter::canSkipDisplayConversion(
    const KoColorSpace *colorSpace) const
{
    return m_d->transform.canSkipDisplayConversion(colorSpace);
}

KoColor KisDisplayColorConverter::applyDisplayFiltering(
    const KoColor &color,
    const KoID &bitDepthId) const
{
    return m_d->transform.applyDisplayFiltering(color, bitDepthId);
}

void KisDisplayColorConverter::applyDisplayFilteringF32(
    KisFixedPaintDeviceSP device,
    const KoColorSpace *destinationColorSpace) const
{
    m_d->transform.applyDisplayFilteringF32(device, destinationColorSpace);
}

QColor KisDisplayColorConverter::convertColorToDisplayColorSpace(
    const KoColor color,
    bool applyOcio) const
{
    return m_d->transform.convertColorToDisplayColorSpace(color, applyOcio);
}

QImage KisDisplayColorConverter::convertImageToDisplayColorSpace(
    KisPaintDeviceSP sourceDevice,
    QRect source,
    bool applyOcio) const
{
    return m_d->transform.convertImageToDisplayColorSpace(
        sourceDevice, source, applyOcio);
}

KisHandlePalette
KisDisplayColorConverter::handlePaletteForDisplayColorSpace() const
{
    return m_d->handlePalette;
}

QPalette KisDisplayColorConverter::systemPaletteForDisplayColorSpace() const
{
    return m_d->systemPalette;
}

QImage KisDisplayColorConverter::toQImage(KisPaintDeviceSP sourceDevice,
                                          bool proofPaintColors) const
{
    return m_d->transform.toQImage(sourceDevice, proofPaintColors);
}

QImage KisDisplayColorConverter::toQImage(
    const KoColorSpace *sourceColorSpace,
    const quint8 *data,
    QSize size,
    bool proofPaintColors) const
{
    return m_d->transform.toQImage(
        sourceColorSpace, data, size, proofPaintColors);
}

KoColor KisDisplayColorConverter::fromHsv(int h,
                                          int s,
                                          int v,
                                          int a) const
{
    return m_d->transform.fromHsv(h, s, v, a);
}

void KisDisplayColorConverter::getHsv(const KoColor &color,
                                      int *h,
                                      int *s,
                                      int *v,
                                      int *a) const
{
    m_d->transform.getHsv(color, h, s, v, a);
}

KoColor KisDisplayColorConverter::fromHsvF(qreal h,
                                           qreal s,
                                           qreal v,
                                           qreal a)
{
    return m_d->transform.fromHsvF(h, s, v, a);
}

void KisDisplayColorConverter::getHsvF(const KoColor &color,
                                       qreal *h,
                                       qreal *s,
                                       qreal *v,
                                       qreal *a)
{
    m_d->transform.getHsvF(color, h, s, v, a);
}

KoColor KisDisplayColorConverter::fromHslF(qreal h,
                                           qreal s,
                                           qreal l,
                                           qreal a)
{
    return m_d->transform.fromHslF(h, s, l, a);
}

void KisDisplayColorConverter::getHslF(const KoColor &color,
                                       qreal *h,
                                       qreal *s,
                                       qreal *l,
                                       qreal *a)
{
    m_d->transform.getHslF(color, h, s, l, a);
}

KoColor KisDisplayColorConverter::fromHsiF(qreal h,
                                           qreal s,
                                           qreal i)
{
    return m_d->transform.fromHsiF(h, s, i);
}

void KisDisplayColorConverter::getHsiF(const KoColor &color,
                                       qreal *h,
                                       qreal *s,
                                       qreal *i)
{
    m_d->transform.getHsiF(color, h, s, i);
}

KoColor KisDisplayColorConverter::fromHsyF(qreal h,
                                           qreal s,
                                           qreal y,
                                           qreal redWeight,
                                           qreal greenWeight,
                                           qreal blueWeight,
                                           qreal gamma)
{
    return m_d->transform.fromHsyF(
        h, s, y, redWeight, greenWeight, blueWeight, gamma);
}

void KisDisplayColorConverter::getHsyF(const KoColor &color,
                                       qreal *h,
                                       qreal *s,
                                       qreal *y,
                                       qreal redWeight,
                                       qreal greenWeight,
                                       qreal blueWeight,
                                       qreal gamma)
{
    m_d->transform.getHsyF(
        color, h, s, y, redWeight, greenWeight, blueWeight, gamma);
}

#include "moc_kis_display_color_converter.cpp"
