/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include <Canvas.h>
#include <QTest>

#include <type_traits>

#define ASSERT_CANVAS_SIGNATURE(method, signature)                                                                     \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&Canvas::method)), signature>)

class CanvasSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void canvasTypeLifetimeAndIdentitySchemaRemainsStable();
    void canvasZoomAndCenterSchemaRemainsStable();
    void canvasRotationAndMirrorSchemaRemainsStable();
    void canvasRenderingPolicySchemaRemainsStable();
    void canvasViewAssociationSchemaRemainsStable();
};

void CanvasSchemaContractTest::canvasTypeLifetimeAndIdentitySchemaRemainsStable()
{
    static_assert(std::is_class_v<Canvas>);
    static_assert(std::is_constructible_v<Canvas, KoCanvasBase *, QObject *>);
    static_assert(std::is_constructible_v<Canvas, KoCanvasBase *>);
    static_assert(std::has_virtual_destructor_v<Canvas>);
    ASSERT_CANVAS_SIGNATURE(operator==, bool (Canvas::*)(const Canvas &) const);
    ASSERT_CANVAS_SIGNATURE(operator!=, bool (Canvas::*)(const Canvas &) const);
}

void CanvasSchemaContractTest::canvasZoomAndCenterSchemaRemainsStable()
{
    ASSERT_CANVAS_SIGNATURE(zoomLevel, qreal (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(setZoomLevel, void (Canvas::*)(qreal));
    ASSERT_CANVAS_SIGNATURE(resetZoom, void (Canvas::*)());
    ASSERT_CANVAS_SIGNATURE(preferredCenter, QPointF (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(setPreferredCenter, void (Canvas::*)(const QPointF &));
    ASSERT_CANVAS_SIGNATURE(pan, void (Canvas::*)(const QPoint &));
}

void CanvasSchemaContractTest::canvasRotationAndMirrorSchemaRemainsStable()
{
    ASSERT_CANVAS_SIGNATURE(rotation, qreal (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(setRotation, void (Canvas::*)(qreal));
    ASSERT_CANVAS_SIGNATURE(resetRotation, void (Canvas::*)());
    ASSERT_CANVAS_SIGNATURE(mirror, bool (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(setMirror, void (Canvas::*)(bool));
}

void CanvasSchemaContractTest::canvasRenderingPolicySchemaRemainsStable()
{
    ASSERT_CANVAS_SIGNATURE(wrapAroundMode, bool (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(setWrapAroundMode, void (Canvas::*)(bool));
    ASSERT_CANVAS_SIGNATURE(levelOfDetailMode, bool (Canvas::*)() const);
    ASSERT_CANVAS_SIGNATURE(setLevelOfDetailMode, void (Canvas::*)(bool));
}

void CanvasSchemaContractTest::canvasViewAssociationSchemaRemainsStable()
{
    ASSERT_CANVAS_SIGNATURE(view, View * (Canvas::*)() const);
}

QTEST_GUILESS_MAIN(CanvasSchemaContractTest)
#include "CanvasSchemaContractTest.moc"
