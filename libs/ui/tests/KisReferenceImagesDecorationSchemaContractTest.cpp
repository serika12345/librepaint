/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisImageThumbnailStrokeStrategy.h"
#include "KisReferenceImagesDecoration.h"
#include "kis_guides_decoration.h"

#include <QTest>


namespace
{
class ThumbnailStrategyProbe final : public KisImageThumbnailStrokeStrategyBase
{
public:
    using KisImageThumbnailStrokeStrategyBase::KisImageThumbnailStrokeStrategyBase;

private:
    void reportThumbnailGenerationCompleted(KisPaintDeviceSP, const QRect &) override;
};
} // namespace

class KisReferenceImagesDecorationSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void guidesDecorationSchemaRemainStable();
};

void KisReferenceImagesDecorationSchemaContractTest::guidesDecorationSchemaRemainStable()
{


    QCOMPARE(GUIDES_DECORATION_ID, QStringLiteral("guides-decoration"));
}

QTEST_GUILESS_MAIN(KisReferenceImagesDecorationSchemaContractTest)

#include "KisReferenceImagesDecorationSchemaContractTest.moc"
