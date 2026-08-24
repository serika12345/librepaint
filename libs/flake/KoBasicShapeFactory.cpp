/*
 *  SPDX-FileCopyrightText: 2009 Sven Langkamp <sven.langkamp@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoBasicShapeFactory.h"

#include <QPainterPath>

#include <KoPathShape.h>
#include <KoProperties.h>
#include <KoShapeFactoryBase.h>
#include <KoShapeRegistry.h>

KoShape *KoBasicShapeFactory::createRectangle(const QRectF &rect,
                                              qreal roundCornersX,
                                              qreal roundCornersY)
{
    KoShape *shape;

    KoShapeFactoryBase *rectangleFactory =
        KoShapeRegistry::instance()->value("RectangleShape");
    if (rectangleFactory) {
        KoProperties properties;
        properties.setProperty("x", rect.x());
        properties.setProperty("y", rect.y());
        properties.setProperty("width", rect.width());
        properties.setProperty("height", rect.height());
        properties.setProperty("rx", 2 * 100.0 * roundCornersX / rect.width());
        properties.setProperty("ry", 2 * 100.0 * roundCornersY / rect.height());

        shape = rectangleFactory->createShape(&properties);
    } else {
        QPainterPath path;
        if (roundCornersX > 0 || roundCornersY > 0) {
            path.addRoundedRect(rect, roundCornersX, roundCornersY);
        } else {
            path.addRect(rect);
        }
        KoPathShape *pathShape = KoPathShape::createShapeFromPainterPath(path);
        pathShape->normalize();
        shape = pathShape;
    }
    return shape;
}

KoShape *KoBasicShapeFactory::createEllipse(const QRectF &rect)
{
    KoShape *shape;
    KoShapeFactoryBase *ellipseFactory =
        KoShapeRegistry::instance()->value("EllipseShape");
    if (ellipseFactory) {
        shape = ellipseFactory->createDefaultShape();
        shape->setSize(rect.size());
        shape->setPosition(rect.topLeft());
    } else {
        auto *path = new KoPathShape;
        path->setShapeId(KoPathShapeId);

        const QPointF rightMiddle(rect.left() + rect.width(),
                                  rect.top() + rect.height() / 2);
        path->moveTo(rightMiddle);
        path->arcTo(rect.width() / 2, rect.height() / 2, 0, 360.0);
        path->close();
        path->normalize();
        shape = path;
    }
    return shape;
}
