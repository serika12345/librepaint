/*
 * This file is part of Krita
 *
 * SPDX-FileCopyrightText: 2020 L. E. Segovia <amy@amyspark.me>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <KisRunnableStrokeJobUtils.h>
#include <boost/none_t.hpp>
#include <filter/kis_filter_configuration.h>
#include <kis_generator_layer.h>
#include <kis_processing_information.h>
#include <kis_processing_visitor.h>
#include <krita_utils.h>
#include <qlatin1stringview.h>

#include "kis_generator_stroke_strategy.h"
#include "KisQStringListFwd.h"
#include "KisRunnableBasedStrokeStrategy.h"
#include "kis_simple_stroke_strategy.h"
#include "kis_stroke_job_strategy.h"
#include "kis_types.h"
#include "kis_paint_device.h"
#include "kundo2magicstring.h"

KisGeneratorStrokeStrategy::KisGeneratorStrokeStrategy()
    : KisRunnableBasedStrokeStrategy(QLatin1String("KisGenerator"), kundo2_i18n("Fill Layer Render"))
{
    enableJob(KisSimpleStrokeStrategy::JOB_INIT, true, KisStrokeJobData::BARRIER, KisStrokeJobData::EXCLUSIVE);
    enableJob(KisSimpleStrokeStrategy::JOB_DOSTROKE);

    setRequestsOtherStrokesToEnd(false);
    setClearsRedoOnStart(false);
    setCanForgetAboutMe(false);
}

QVector<KisStrokeJobData *>KisGeneratorStrokeStrategy::createJobsData(const KisGeneratorLayerSP layer, QSharedPointer<boost::none_t> cookie, const KisGeneratorSP f, const KisPaintDeviceSP dev, const QRegion &region, const KisFilterConfigurationSP filterConfig)
{
    using namespace KritaUtils;

    QVector<KisStrokeJobData *> jobsData;

    QSharedPointer<KisProcessingVisitor::ProgressHelper> helper(new KisProcessingVisitor::ProgressHelper(layer));

    addJobBarrier(jobsData, nullptr);

    for (const auto& rc: region) {
        if (f->allowsSplittingIntoPatches()) {
            QVector<QRect> tiles = splitRectIntoPatches(rc, optimalPatchSize());

            for(const auto& tile: tiles) {
                KisProcessingInformation dstCfg(dev, tile.topLeft(), KisSelectionSP());
                addJobConcurrent(jobsData, [=]() {
                    f->generate(dstCfg, tile.size(), filterConfig, helper->updater());

                    // HACK ALERT!!!
                    // this avoids cyclic loop with KisRecalculateGeneratorLayerJob::run()
                    const_cast<KisGeneratorLayerSP &>(layer)->setDirtyWithoutUpdate({tile});

                    const_cast<QSharedPointer<boost::none_t> &>(cookie).clear();
                });
            }
        } else {
            KisProcessingInformation dstCfg(dev, rc.topLeft(), KisSelectionSP());

            addJobSequential(jobsData, [=]() {
                f->generate(dstCfg, rc.size(), filterConfig, helper->updater());

                // HACK ALERT!!!
                // this avoids cyclic loop with KisRecalculateGeneratorLayerJob::run()
                const_cast<KisGeneratorLayerSP &>(layer)->setDirtyWithoutUpdate({rc});

                const_cast<QSharedPointer<boost::none_t>&>(cookie).clear();
            });
        }
    }

    return jobsData;
}
KisGeneratorStrokeStrategy::~KisGeneratorStrokeStrategy()
{
}
