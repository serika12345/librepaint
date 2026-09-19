/*
 *  SPDX-FileCopyrightText: 2016 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_grid_config_test.h"

#include <simpletest.h>
#include "kis_grid_config.h"
#include "kis_guides_config.h"
#include <QDomDocument>
#include <QDomElement>


void KisGridConfigTest::testGridConfig()
{
    KisGridConfig config;
    config.setSpacing(QPoint(10,13));
    config.setOffset(QPoint(13,14));
    config.setOffsetAspectLocked(false);
    config.setSubdivision(4);
    config.setGridType(KisGridConfig::GRID_ISOMETRIC);

    config.setColorMain(QColor(12, 34, 56));
    config.setColorSubdivision(QColor(23, 45, 67));
    config.setColorVertical(QColor(34, 56, 78));

    config.setLineTypeMain(KisGridConfig::LINE_SOLID);
    QCOMPARE(config.penMain().style(), Qt::SolidLine);
    config.setLineTypeMain(KisGridConfig::LINE_DASHED);
    QCOMPARE(config.penMain().style(), Qt::CustomDashLine);
    QCOMPARE(config.penMain().dashPattern(), QVector<qreal>({5.0, 5.0}));
    config.setLineTypeSubdivision(KisGridConfig::LINE_NONE);
    QCOMPARE(config.penSubdivision().style(), Qt::NoPen);
    config.setLineTypeVertical(KisGridConfig::LINE_DOTTED);
    QCOMPARE(config.penVertical().style(), Qt::DotLine);

    QVERIFY(!config.isDefault());

    QDomDocument doc;
    QDomElement root = doc.createElement("TestXMLRoot");
    doc.appendChild(root);
    QDomElement el = config.saveDynamicDataToXml(doc, "test_tag");
    root.appendChild(el);

    QByteArray b = doc.toByteArray(4);
    //printf(b.data());

    KisGridConfig config2;
    QVERIFY(config2.isDefault());
    QVERIFY(config2.loadDynamicDataFromXml(el));

    QCOMPARE(config2, config);
    QVERIFY(!config2.isDefault());
    QCOMPARE(config2.gridType(), KisGridConfig::GRID_ISOMETRIC);
    QCOMPARE(config2.penMain().style(), Qt::CustomDashLine);
    QCOMPARE(config2.penMain().color(), QColor(12, 34, 56));
    QCOMPARE(config2.penSubdivision().style(), Qt::NoPen);
    QCOMPARE(config2.penSubdivision().color(), QColor(23, 45, 67));
    QCOMPARE(config2.penVertical().style(), Qt::DotLine);
    QCOMPARE(config2.penVertical().color(), QColor(34, 56, 78));
}

void KisGridConfigTest::testGuidesConfig()
{
    KisGuidesConfig config;
    config.setGuidesColor(QColor(12, 34, 56));
    config.setShowGuides(true);
    config.setLockGuides(true);
    config.setSnapToGuides(true);

    config.setGuidesLineType(KisGuidesConfig::LINE_SOLID);
    QCOMPARE(config.guidesPen().style(), Qt::SolidLine);
    config.setGuidesLineType(KisGuidesConfig::LINE_DASHED);
    QCOMPARE(config.guidesPen().style(), Qt::DashLine);
    config.setGuidesLineType(KisGuidesConfig::LINE_DOTTED);
    QCOMPARE(config.guidesPen().style(), Qt::DotLine);
    QCOMPARE(config.guidesPen().color(), QColor(12, 34, 56));

    config.addGuideLine(Qt::Horizontal, 100.0);
    config.addGuideLine(Qt::Horizontal, 200.0);

    config.addGuideLine(Qt::Vertical, 300.0);
    config.addGuideLine(Qt::Vertical, 400.0);

    QVERIFY(config.hasGuides());

    QDomDocument doc;
    QDomElement root = doc.createElement("TestXMLRoot");
    doc.appendChild(root);
    QDomElement el = config.saveToXml(doc, "test_tag");
    root.appendChild(el);

    QByteArray b = doc.toByteArray(4);
    //printf(b.data());

    KisGuidesConfig config2;
    QVERIFY(!config2.hasGuides());
    QVERIFY(config2.loadFromXml(el));

    QCOMPARE(config2.guidesLineType(), config.guidesLineType());
    QCOMPARE(config2.guidesColor(), config.guidesColor());
    QCOMPARE(config2, config);
    QVERIFY(config2.hasGuides());
    QCOMPARE(config2.guidesPen().style(), Qt::DotLine);
    QCOMPARE(config2.guidesPen().color(), QColor(12, 34, 56));
}

SIMPLE_TEST_MAIN(KisGridConfigTest)
