/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_projection_update_info.h"
#include "kis_update_info.h"

#include <QTest>

#include <type_traits>

namespace
{
class UpdateInfoConstructionProbe final : public KisUpdateInfo
{
public:
    UpdateInfoConstructionProbe() = default;

    QRect dirtyImageRect() const override;
    int levelOfDetail() const override;
};
} // namespace

class KisCanvasUpdateInfoSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void updateInfoTypeLifetimeAndVirtualQuerySchemaRemainStable();
    void markerUpdateInfoTypeBatchAndConstructionSchemaRemainStable();
    void markerUpdateInfoQuerySignaturesRemainStable();
    void projectionUpdateInfoTypeTransferAndQuerySchemaRemainStable();
    void projectionUpdateInfoTransferValueMembersRemainStable();
};

void KisCanvasUpdateInfoSchemaContractTest::updateInfoTypeLifetimeAndVirtualQuerySchemaRemainStable()
{
    static_assert(std::is_same_v<KisUpdateInfoSP, KisSharedPtr<KisUpdateInfo>>);
    static_assert(std::is_class_v<KisUpdateInfo>);
    static_assert(std::is_default_constructible_v<UpdateInfoConstructionProbe>);
    static_assert(std::has_virtual_destructor_v<KisUpdateInfo>);
    static_assert(std::is_same_v<decltype(&KisUpdateInfo::canBeCompressed), bool (KisUpdateInfo::*)() const>);
    static_assert(std::is_same_v<decltype(&KisUpdateInfo::dirtyImageRect), QRect (KisUpdateInfo::*)() const>);
    static_assert(std::is_same_v<decltype(&KisUpdateInfo::dirtyViewportRect), QRect (KisUpdateInfo::*)()>);
    static_assert(std::is_same_v<decltype(&KisUpdateInfo::levelOfDetail), int (KisUpdateInfo::*)() const>);
}

void KisCanvasUpdateInfoSchemaContractTest::markerUpdateInfoTypeBatchAndConstructionSchemaRemainStable()
{
    using Marker = KisMarkerUpdateInfo;
    using Type = Marker::Type;

    static_assert(std::is_class_v<Marker>);
    static_assert(std::is_enum_v<Type>);
    static_assert(Marker::StartBatch == 0);
    static_assert(Marker::EndBatch == 1);
    static_assert(Marker::BlockLodUpdates == 2);
    static_assert(Marker::UnblockLodUpdates == 3);
    static_assert(std::is_constructible_v<Marker, Type, const QRect &>);
}

void KisCanvasUpdateInfoSchemaContractTest::markerUpdateInfoQuerySignaturesRemainStable()
{
    using Marker = KisMarkerUpdateInfo;

    static_assert(std::is_same_v<decltype(&Marker::canBeCompressed), bool (Marker::*)() const>);
    static_assert(std::is_same_v<decltype(&Marker::dirtyImageRect), QRect (Marker::*)() const>);
    static_assert(std::is_same_v<decltype(&Marker::levelOfDetail), int (Marker::*)() const>);
    static_assert(std::is_same_v<decltype(&Marker::type), Marker::Type (Marker::*)() const>);
}

void KisCanvasUpdateInfoSchemaContractTest::projectionUpdateInfoTypeTransferAndQuerySchemaRemainStable()
{
    using Projection = KisProjectionUpdateInfo;
    using Transfer = Projection::TransferType;

    static_assert(std::is_same_v<KisProjectionUpdateInfoSP, KisSharedPtr<Projection>>);
    static_assert(std::is_class_v<Projection>);
    static_assert(std::is_enum_v<Transfer>);
    static_assert(Projection::Direct == 0);
    static_assert(Projection::Patch == 1);
    static_assert(std::is_default_constructible_v<Projection>);
    static_assert(std::is_constructible_v<Projection, const QRect &>);
    static_assert(std::is_same_v<decltype(&Projection::dirtyImageRect), QRect (Projection::*)() const>);
    static_assert(std::is_same_v<decltype(&Projection::dirtyViewportRect), QRect (Projection::*)()>);
    static_assert(std::is_same_v<decltype(&Projection::levelOfDetail), int (Projection::*)() const>);
}

void KisCanvasUpdateInfoSchemaContractTest::projectionUpdateInfoTransferValueMembersRemainStable()
{
    using Projection = KisProjectionUpdateInfo;

    static_assert(std::is_same_v<decltype(&Projection::borderWidth), qint32 Projection::*>);
    static_assert(std::is_same_v<decltype(&Projection::imageRect), QRect Projection::*>);
    static_assert(std::is_same_v<decltype(&Projection::renderHints), QPainter::RenderHints Projection::*>);
    static_assert(std::is_same_v<decltype(&Projection::scaleX), qreal Projection::*>);
    static_assert(std::is_same_v<decltype(&Projection::scaleY), qreal Projection::*>);
    static_assert(std::is_same_v<decltype(&Projection::transfer), Projection::TransferType Projection::*>);
    static_assert(std::is_same_v<decltype(&Projection::viewportRect), QRectF Projection::*>);
}

QTEST_APPLESS_MAIN(KisCanvasUpdateInfoSchemaContractTest)

#include "KisCanvasUpdateInfoSchemaContractTest.moc"
