/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2007 Boudewijn Rempt (boud@valdyas.org)
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */
#include "TestKoShapeFactory.h"

#include <simpletest.h>
#include <QBuffer>
#include <QPainterPath>
#include <QScopedPointer>

#include <KoBasicShapeFactory.h>
#include <KoShapeLoadingContext.h>
#include <KoPathShapeFactory.h>
#include <KoPathShape.h>
#include <KoShape.h>
#include <KoShapeFactoryBase.h>
#include <KoShapeRegistry.h>
#include <KoProperties.h>
#include <KoXmlNS.h>
#include <FlakeDebug.h>

namespace
{
class RecordingShapeFactory : public KoShapeFactoryBase
{
public:
    explicit RecordingShapeFactory(const QString &id)
        : KoShapeFactoryBase(id, id)
    {
    }

    bool supports(const QDomElement &, KoShapeLoadingContext &) const override
    {
        return false;
    }

    KoShape *createDefaultShape(KoDocumentResourceManager *) const override
    {
        QPainterPath path;
        path.addRect(QRectF(0.0, 0.0, 1.0, 1.0));
        auto *shape = KoPathShape::createShapeFromPainterPath(path);
        shape->setShapeId(id() + QStringLiteral("Created"));
        return shape;
    }

    KoShape *createShape(const KoProperties *properties,
                         KoDocumentResourceManager *resources) const override
    {
        rectangleProperties = QRectF(properties->doubleProperty("x", 0.0),
                                     properties->doubleProperty("y", 0.0),
                                     properties->doubleProperty("width", 0.0),
                                     properties->doubleProperty("height", 0.0));
        cornerRadiusX = properties->doubleProperty("rx", 0.0);
        cornerRadiusY = properties->doubleProperty("ry", 0.0);
        return createDefaultShape(resources);
    }

    mutable QRectF rectangleProperties;
    mutable qreal cornerRadiusX = 0.0;
    mutable qreal cornerRadiusY = 0.0;
};

class ShapeRegistryOverride
{
public:
    ShapeRegistryOverride(const QString &id, KoShapeFactoryBase *replacement)
        : m_registry(KoShapeRegistry::instance())
        , m_id(id)
        , m_original(m_registry->value(id))
    {
        m_registry->remove(id);
        if (replacement) {
            m_registry->add(replacement);
        }
    }

    ~ShapeRegistryOverride()
    {
        m_registry->remove(m_id);
        if (m_original) {
            m_registry->add(m_original);
        }
    }

private:
    KoShapeRegistry *m_registry;
    QString m_id;
    KoShapeFactoryBase *m_original;
};
}

void TestKoShapeFactory::testCreateFactory()
{
    KoShapeFactoryBase * factory = new KoPathShapeFactory(QStringList());
    QVERIFY(factory != 0);
    delete factory;
}

void TestKoShapeFactory::testSupportsQDomElement()
{
}

void TestKoShapeFactory::testPriority()
{
    KoShapeFactoryBase * factory = new KoPathShapeFactory(QStringList());
    QVERIFY(factory->loadingPriority() == 0);
    delete factory;
}

void TestKoShapeFactory::testCreateDefaultShape()
{
    KoShapeFactoryBase * factory = new KoPathShapeFactory(QStringList());
    KoShape *shape = factory->createDefaultShape();
    QVERIFY(shape != 0);
    delete shape;
    delete factory;
}

void TestKoShapeFactory::testCreateShape()
{
    KoShapeFactoryBase * factory = new KoPathShapeFactory(QStringList());
    KoShape *shape = factory->createShape(0);
    QVERIFY(shape != 0);
    delete shape;
    delete factory;
}

void TestKoShapeFactory::testBasicShapeFactoryUsesRegisteredFactories()
{
    RecordingShapeFactory rectangleFactory(QStringLiteral("RectangleShape"));
    RecordingShapeFactory ellipseFactory(QStringLiteral("EllipseShape"));
    ShapeRegistryOverride rectangleOverride(QStringLiteral("RectangleShape"),
                                            &rectangleFactory);
    ShapeRegistryOverride ellipseOverride(QStringLiteral("EllipseShape"),
                                          &ellipseFactory);

    const QRectF rectangleRect(3.0, 5.0, 40.0, 20.0);
    QScopedPointer<KoShape> rectangle(
        KoBasicShapeFactory::createRectangle(rectangleRect, 5.0, 6.0));
    QVERIFY(rectangle);
    QCOMPARE(rectangle->shapeId(), QStringLiteral("RectangleShapeCreated"));
    QCOMPARE(rectangleFactory.rectangleProperties, rectangleRect);
    QCOMPARE(rectangleFactory.cornerRadiusX, 25.0);
    QCOMPARE(rectangleFactory.cornerRadiusY, 60.0);

    const QRectF ellipseRect(7.0, 11.0, 31.0, 17.0);
    QScopedPointer<KoShape> ellipse(
        KoBasicShapeFactory::createEllipse(ellipseRect));
    QVERIFY(ellipse);
    QCOMPARE(ellipse->shapeId(), QStringLiteral("EllipseShapeCreated"));
    QCOMPARE(ellipse->size(), ellipseRect.size());
    QCOMPARE(ellipse->position(), ellipseRect.topLeft());
}

void TestKoShapeFactory::testBasicShapeFactoryCreatesFallbackPaths()
{
    ShapeRegistryOverride rectangleOverride(QStringLiteral("RectangleShape"),
                                            nullptr);
    ShapeRegistryOverride ellipseOverride(QStringLiteral("EllipseShape"),
                                          nullptr);

    const QRectF rectangleRect(3.0, 5.0, 40.0, 20.0);
    QScopedPointer<KoShape> rectangle(
        KoBasicShapeFactory::createRectangle(rectangleRect, 5.0, 6.0));
    QVERIFY(dynamic_cast<KoPathShape *>(rectangle.data()));
    QCOMPARE(rectangle->boundingRect(), rectangleRect);

    const QRectF ellipseRect(7.0, 11.0, 31.0, 17.0);
    QScopedPointer<KoShape> ellipse(
        KoBasicShapeFactory::createEllipse(ellipseRect));
    QVERIFY(dynamic_cast<KoPathShape *>(ellipse.data()));
    QCOMPARE(ellipse->boundingRect(), ellipseRect);
}

QTEST_GUILESS_MAIN(TestKoShapeFactory)
