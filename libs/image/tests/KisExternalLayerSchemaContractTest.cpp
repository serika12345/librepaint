/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_external_layer_iface.h"

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{

#define ASSERT_SIGNATURE(method, ...)                                                                                  \
    static_assert(std::is_same_v<decltype(static_cast<__VA_ARGS__>(&KisExternalLayer::method)), __VA_ARGS__>)

class ExternalLayerConstructionProbe : public KisExternalLayer
{
public:
    using KisExternalLayer::KisExternalLayer;

    KisNodeSP clone() const override;
    bool allowAsChild(KisNodeSP) const override;
    KisPaintDeviceSP paintDevice() const override;
    KisPaintDeviceSP original() const override;
};

} // namespace

class KisExternalLayerSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeAndConstructionSchemaRemainStable();
    void presentationAndBoundsSignaturesRemainStable();
    void cacheSignatureRemainsStable();
    void geometryCommandSignaturesRemainStable();
    void colorManagementCommandSignaturesRemainStable();
};

void KisExternalLayerSchemaContractTest::typeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<KisExternalLayer>);
    static_assert(std::is_base_of_v<KisLayer, KisExternalLayer>);
    static_assert(std::is_constructible_v<ExternalLayerConstructionProbe, KisImageWSP, const QString &, quint8>);

    QVERIFY(true);
}

void KisExternalLayerSchemaContractTest::presentationAndBoundsSignaturesRemainStable()
{
    ASSERT_SIGNATURE(icon, QIcon (KisExternalLayer::*)() const);
    ASSERT_SIGNATURE(supportsPerspectiveTransform, bool (KisExternalLayer::*)() const);
    ASSERT_SIGNATURE(theoreticalBoundingRect, QRect (KisExternalLayer::*)() const);
}

void KisExternalLayerSchemaContractTest::cacheSignatureRemainsStable()
{
    ASSERT_SIGNATURE(resetCache, void (KisExternalLayer::*)(const KoColorSpace *));
    static_assert(std::is_same_v<decltype(std::declval<KisExternalLayer &>().resetCache()), void>);
}

void KisExternalLayerSchemaContractTest::geometryCommandSignaturesRemainStable()
{
    ASSERT_SIGNATURE(crop, KUndo2Command * (KisExternalLayer::*)(const QRect &));
    ASSERT_SIGNATURE(transform, KUndo2Command * (KisExternalLayer::*)(const QTransform &));
}

void KisExternalLayerSchemaContractTest::colorManagementCommandSignaturesRemainStable()
{
    using Intent = KoColorConversionTransformation::Intent;
    using Flags = KoColorConversionTransformation::ConversionFlags;

    ASSERT_SIGNATURE(setProfile, KUndo2Command * (KisExternalLayer::*)(const KoColorProfile *));
    ASSERT_SIGNATURE(convertTo, KUndo2Command * (KisExternalLayer::*)(const KoColorSpace *, Intent, Flags));
    static_assert(std::is_same_v<decltype(std::declval<KisExternalLayer &>().convertTo(nullptr)), KUndo2Command *>);
}

#undef ASSERT_SIGNATURE

QTEST_GUILESS_MAIN(KisExternalLayerSchemaContractTest)

#include "KisExternalLayerSchemaContractTest.moc"
