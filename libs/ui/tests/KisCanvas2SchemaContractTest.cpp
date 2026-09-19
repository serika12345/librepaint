/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QtTest>

#include <functional>

#include "canvas/KisDecorationsManager.h"
#include "canvas/KoCanvasControllerWidget.h"
#include "canvas/kis_canvas2.h"
#include "canvas/kis_canvas_controller.h"
#include "canvas/kis_infinity_manager.h"
#include "canvas/kis_mirror_manager.h"
#include "nodes/kis_node_selection_adapter.h"


namespace
{
class CanvasControllerWidgetProbe final : public KoCanvasControllerWidget
{
public:
    using KoCanvasControllerWidget::KoCanvasControllerWidget;

    void ensureVisibleDoc(const QRectF &, bool) override;
    void zoomIn(const KoViewTransformStillPoint &) override;
    void zoomIn() override;
    void zoomOut(const KoViewTransformStillPoint &) override;
    void zoomOut() override;
    void setPreferredCenter(const QPointF &) override;
    QPointF preferredCenter() const override;
    void resetScrollBars() override;
    QPointF currentCursorPosition() const override;
    KoZoomState zoomState() const override;
    KisCanvasState canvasState() const override;

protected:
    void updateCanvasOffsetInternal(const QPointF &) override;
    void updateCanvasWidgetSizeInternal(const QSize &, qreal) override;
    void updateCanvasZoomInternal(KoZoomMode::Mode,
                                  qreal,
                                  qreal,
                                  qreal,
                                  const std::optional<KoViewTransformStillPoint> &) override;
    void zoomToInternal(const QRect &) override;
};
} // namespace

class KisCanvas2SchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void infinityManagerSchemaRemainStable();
};

void KisCanvas2SchemaContractTest::infinityManagerSchemaRemainStable()
{


    QCOMPARE(INFINITY_DECORATION_ID, QStringLiteral("infinity-decorations"));
}

// clang-format off
// clang-format on

// clang-format off
// clang-format on

QTEST_GUILESS_MAIN(KisCanvas2SchemaContractTest)

#include "KisCanvas2SchemaContractTest.moc"
