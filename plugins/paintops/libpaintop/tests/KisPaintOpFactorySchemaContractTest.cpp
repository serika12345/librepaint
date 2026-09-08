/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <brushengine/kis_paintop_factory.h>

#include <QTest>

#include <type_traits>

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

#define ASSERT_PAINTOP_FACTORY_SIGNATURE(method, signature)                                                            \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisPaintOpFactory::method)), signature>)
} // namespace

class KisPaintOpFactorySchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void paintOpFactoryTypeVisibilityAndLifetimeSchemaRemainStable();
    void paintOpFactoryIdentityAndClassificationSignaturesRemainStable();
    void paintOpFactoryCreationSignaturesRemainStable();
    void paintOpFactoryResourceSignaturesRemainStable();
    void paintOpFactoryPrioritySignaturesRemainStable();
};

void KisPaintOpFactorySchemaContractTest::paintOpFactoryTypeVisibilityAndLifetimeSchemaRemainStable()
{
    using Visibility = KisPaintOpFactory::PaintopVisibility;

    static_assert(std::is_class_v<KisPaintOpFactory>);
    static_assert(std::is_enum_v<Visibility>);
    static_assert(std::is_constructible_v<FactoryProbe, const QStringList &>);
    static_assert(std::is_destructible_v<KisPaintOpFactory>);
    static_assert(std::has_virtual_destructor_v<KisPaintOpFactory>);

    QCOMPARE(static_cast<int>(KisPaintOpFactory::AUTO), 0);
    QCOMPARE(static_cast<int>(KisPaintOpFactory::ALWAYS), 1);
    QCOMPARE(static_cast<int>(KisPaintOpFactory::NEVER), 2);
}

void KisPaintOpFactorySchemaContractTest::paintOpFactoryIdentityAndClassificationSignaturesRemainStable()
{
    using CategoryStable = QString (*)();

    ASSERT_PAINTOP_FACTORY_SIGNATURE(category, QString (KisPaintOpFactory::*)() const);
    static_assert(
        std::is_same_v<decltype(static_cast<CategoryStable>(&KisPaintOpFactory::categoryStable)), CategoryStable>);
    ASSERT_PAINTOP_FACTORY_SIGNATURE(icon, QIcon (KisPaintOpFactory::*)());
    ASSERT_PAINTOP_FACTORY_SIGNATURE(id, QString (KisPaintOpFactory::*)() const);
    ASSERT_PAINTOP_FACTORY_SIGNATURE(lodSizeThresholdSupported, bool (KisPaintOpFactory::*)() const);
    ASSERT_PAINTOP_FACTORY_SIGNATURE(name, QString (KisPaintOpFactory::*)() const);
}

void KisPaintOpFactorySchemaContractTest::paintOpFactoryCreationSignaturesRemainStable()
{
    ASSERT_PAINTOP_FACTORY_SIGNATURE(
        createConfigWidget,
        KisPaintOpConfigWidget
            * (KisPaintOpFactory::*)(QWidget *, KisResourcesInterfaceSP, KoCanvasResourcesInterfaceSP));
    ASSERT_PAINTOP_FACTORY_SIGNATURE(createInterstrokeDataFactory,
                                     KisInterstrokeDataFactory
                                         * (KisPaintOpFactory::*)(KisPaintOpSettingsSP, KisResourcesInterfaceSP) const);
    ASSERT_PAINTOP_FACTORY_SIGNATURE(
        createOp,
        KisPaintOp * (KisPaintOpFactory::*)(KisPaintOpSettingsSP, KisPainter *, KisNodeSP, KisImageSP));
    ASSERT_PAINTOP_FACTORY_SIGNATURE(createSettings,
                                     KisPaintOpSettingsSP (KisPaintOpFactory::*)(KisResourcesInterfaceSP));
    ASSERT_PAINTOP_FACTORY_SIGNATURE(preinitializePaintOpIfNeeded, void (KisPaintOpFactory::*)(KisPaintOpSettingsSP));
}

void KisPaintOpFactorySchemaContractTest::paintOpFactoryResourceSignaturesRemainStable()
{
    ASSERT_PAINTOP_FACTORY_SIGNATURE(
        prepareEmbeddedResources,
        QList<KoResourceLoadResult> (KisPaintOpFactory::*)(KisPaintOpSettingsSP, KisResourcesInterfaceSP));
    ASSERT_PAINTOP_FACTORY_SIGNATURE(
        prepareLinkedResources,
        QList<KoResourceLoadResult> (KisPaintOpFactory::*)(KisPaintOpSettingsSP, KisResourcesInterfaceSP));
    ASSERT_PAINTOP_FACTORY_SIGNATURE(whiteListedCompositeOps, QStringList (KisPaintOpFactory::*)() const);
}

void KisPaintOpFactorySchemaContractTest::paintOpFactoryPrioritySignaturesRemainStable()
{
    ASSERT_PAINTOP_FACTORY_SIGNATURE(priority, int (KisPaintOpFactory::*)() const);
    ASSERT_PAINTOP_FACTORY_SIGNATURE(setPriority, void (KisPaintOpFactory::*)(int));
}

#undef ASSERT_PAINTOP_FACTORY_SIGNATURE

QTEST_APPLESS_MAIN(KisPaintOpFactorySchemaContractTest)

#include "KisPaintOpFactorySchemaContractTest.moc"
