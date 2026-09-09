/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_marker_painter.h"
#include "kis_outline_generator.h"

#include <QTest>

#include <type_traits>

namespace
{

#define ASSERT_SIGNATURE(type, method, ...)                                                                            \
    static_assert(std::is_same_v<decltype(static_cast<__VA_ARGS__>(&type::method)), __VA_ARGS__>)

} // namespace

class KisImageOutlineSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void markerTypeLifetimeAndRangeSchemaRemainStable();
    void markerPaintingSignaturesRemainStable();
    void outlineTypeLifetimeAndConstructionSchemaRemainStable();
    void outlineRawBufferSignatureRemainsStable();
    void outlineDeviceAndModeSignaturesRemainStable();
};

void KisImageOutlineSchemaContractTest::markerTypeLifetimeAndRangeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisMarkerPainter>);
    static_assert(std::is_constructible_v<KisMarkerPainter, KisPaintDeviceSP, const KoColor &>);
    static_assert(std::is_destructible_v<KisMarkerPainter>);
    static_assert(std::is_same_v<decltype(KisMarkerPainter::ValidNumberRangeValue), const qint32>);
    static_assert(KisMarkerPainter::ValidNumberRangeValue == 2140000000);
}

void KisImageOutlineSchemaContractTest::markerPaintingSignaturesRemainStable()
{
    ASSERT_SIGNATURE(KisMarkerPainter, fillFullCircle, void (KisMarkerPainter::*)(const QPointF &, qreal));
    ASSERT_SIGNATURE(
        KisMarkerPainter,
        fillHalfBrushDiff,
        void (KisMarkerPainter::*)(const QPointF &, const QPointF &, const QPointF &, const QPointF &, qreal));
    ASSERT_SIGNATURE(KisMarkerPainter,
                     fillCirclesDiff,
                     void (KisMarkerPainter::*)(const QPointF &, qreal, const QPointF &, qreal));
}

void KisImageOutlineSchemaContractTest::outlineTypeLifetimeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<KisOutlineGenerator>);
    static_assert(std::is_constructible_v<KisOutlineGenerator, const KoColorSpace *, quint8>);
    static_assert(std::is_destructible_v<KisOutlineGenerator>);
}

void KisImageOutlineSchemaContractTest::outlineRawBufferSignatureRemainsStable()
{
    ASSERT_SIGNATURE(KisOutlineGenerator,
                     outline,
                     QVector<QPolygon> (KisOutlineGenerator::*)(quint8 *, qint32, qint32, qint32, qint32));
}

void KisImageOutlineSchemaContractTest::outlineDeviceAndModeSignaturesRemainStable()
{
    ASSERT_SIGNATURE(
        KisOutlineGenerator,
        outline,
        QVector<QPolygon> (KisOutlineGenerator::*)(const KisPaintDevice *, qint32, qint32, qint32, qint32));
    ASSERT_SIGNATURE(KisOutlineGenerator, setSimpleOutline, void (KisOutlineGenerator::*)(bool));
}

#undef ASSERT_SIGNATURE

QTEST_GUILESS_MAIN(KisImageOutlineSchemaContractTest)

#include "KisImageOutlineSchemaContractTest.moc"
