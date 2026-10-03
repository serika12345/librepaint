/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <KoColor.h>
#include <KoColorSpaceRegistry.h>
#include <KoID.h>

#include <KisGlobalResourcesInterface.h>
#include <filter/kis_filter.h>
#include <kis_group_layer.h>
#include <kis_image.h>
#include <kis_paint_device.h>
#include <kis_paint_layer.h>
#include <kis_undo_stores.h>
#include <strokes/kis_filter_stroke_strategy.h>

#include "kis_resources_snapshot.h"

namespace
{
class SolidRedFilter final : public KisFilter
{
public:
    SolidRedFilter()
        : KisFilter(KoID("boundary-red", "Boundary red"),
                    KoID("test", "Test"),
                    QStringLiteral("Boundary red"))
    {
    }

    void processImpl(KisPaintDeviceSP device,
                     const QRect &rect,
                     const KisFilterConfigurationSP,
                     KoUpdater *) const override
    {
        device->fill(rect, KoColor(Qt::red, device->colorSpace()));
    }
};
}

class FilterStrokeLibraryBoundaryContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void filterJobFromAnotherLibraryChangesPixels();
};

void FilterStrokeLibraryBoundaryContractTest::filterJobFromAnotherLibraryChangesPixels()
{
    // The test library constructs the job; kritapainting identifies and runs it.
    auto *undoStore = new KisSurrogateUndoStore();
    KisImageSP image = new KisImage(undoStore, 32, 32,
                                   KoColorSpaceRegistry::instance()->rgb8(),
                                   QStringLiteral("filter library boundary"));
    KisPaintLayerSP layer = new KisPaintLayer(image, QStringLiteral("paint"), OPACITY_OPAQUE_U8);
    image->setWorkingThreadsLimit(1);
    image->addNode(layer, image->rootLayer());
    layer->paintDevice()->fill(QRect(0, 0, 32, 32), KoColor(Qt::white, image->colorSpace()));
    image->initialRefreshGraph();
    image->waitForDone();

    KisFilterSP filter = new SolidRedFilter();
    KisResourcesSnapshotSP resources = new KisResourcesSnapshot(image, layer);
    KisFilterConfigurationSP configuration =
        filter->defaultConfiguration(KisGlobalResourcesInterface::instance());
    const KisStrokeId stroke = image->startStroke(
        new KisFilterStrokeStrategy(filter, configuration, resources));
    image->addJob(stroke, new KisFilterStrokeStrategy::FilterJobData());
    image->addJob(stroke, new KisFilterStrokeStrategy::IdleBarrierData());
    image->endStroke(stroke);
    image->waitForDone();

    const QImage result = layer->paintDevice()->convertToQImage(0, 0, 0, 32, 32);
    QCOMPARE(result.pixelColor(16, 16), QColor(Qt::red));
}

QTEST_MAIN(FilterStrokeLibraryBoundaryContractTest)

#include "FilterStrokeLibraryBoundaryContractTest.moc"
