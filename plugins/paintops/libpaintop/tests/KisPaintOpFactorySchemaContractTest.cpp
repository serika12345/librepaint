/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <brushengine/kis_paintop_factory.h>
#include <kis_brush_based_paintop_settings.h>

#include <QTest>


namespace
{
class FactoryProbe final : public KisPaintOpFactory
{
public:
    using KisPaintOpFactory::KisPaintOpFactory;

    KisPaintOp *createOp(KisPaintOpSettingsSP, KisPainter *, KisNodeSP, KisImageSP) override;
    QString id() const override;
    QString name() const override;
    QString category() const override;
    bool lodSizeThresholdSupported() const override;
    QList<KoResourceLoadResult> prepareLinkedResources(KisPaintOpSettingsSP, KisResourcesInterfaceSP) override;
    QList<KoResourceLoadResult> prepareEmbeddedResources(KisPaintOpSettingsSP, KisResourcesInterfaceSP) override;
    KisPaintOpSettingsSP createSettings(KisResourcesInterfaceSP) override;
    KisPaintOpConfigWidget *
    createConfigWidget(QWidget *, KisResourcesInterfaceSP, KoCanvasResourcesInterfaceSP) override;
};

} // namespace

class KisPaintOpFactorySchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void paintOpFactoryTypeVisibilityAndLifetimeSchemaRemainStable();
};

void KisPaintOpFactorySchemaContractTest::paintOpFactoryTypeVisibilityAndLifetimeSchemaRemainStable()
{


    QCOMPARE(static_cast<int>(KisPaintOpFactory::AUTO), 0);
    QCOMPARE(static_cast<int>(KisPaintOpFactory::ALWAYS), 1);
    QCOMPARE(static_cast<int>(KisPaintOpFactory::NEVER), 2);
}

QTEST_APPLESS_MAIN(KisPaintOpFactorySchemaContractTest)

#include "KisPaintOpFactorySchemaContractTest.moc"
