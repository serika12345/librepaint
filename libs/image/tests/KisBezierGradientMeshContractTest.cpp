/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisBezierGradientMesh.h"

#include <QTest>

void kis_assert_exception(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected assertion '%s' at %s:%d", assertion, file, line);
}

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected assertion '%s' at %s:%d", assertion, file, line);
}

class KisBezierGradientMeshContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void patchesExposeCornerColors();
    void subdivisionInterpolatesColorsAndPreservesEndpoints();
    void copiesOwnGeometryAndColors();
};

void KisBezierGradientMeshContractTest::patchesExposeCornerColors()
{
    KisBezierGradientMesh mesh;
    mesh.node(0, 0).color = QColor(Qt::red);
    mesh.node(1, 0).color = QColor(Qt::green);
    mesh.node(0, 1).color = QColor(Qt::blue);
    mesh.node(1, 1).color = QColor(Qt::yellow);

    const auto patch = mesh.makePatch(0, 0);
    QCOMPARE(patch.colors[0], QColor(Qt::red));
    QCOMPARE(patch.colors[1], QColor(Qt::green));
    QCOMPARE(patch.colors[2], QColor(Qt::blue));
    QCOMPARE(patch.colors[3], QColor(Qt::yellow));
}

void KisBezierGradientMeshContractTest::subdivisionInterpolatesColorsAndPreservesEndpoints()
{
    KisBezierGradientMesh mesh;
    for (int row = 0; row < 2; ++row) {
        mesh.node(0, row).color = QColor::fromRgbF(0.2, 0.4, 0.6, 0.8);
        mesh.node(1, row).color = QColor::fromRgbF(0.8, 0.2, 0.4, 0.6);
    }
    const auto before = mesh;
    const int column = mesh.subdivideColumn(0.5);
    QCOMPARE(mesh.size(), QSize(3, 2));
    for (int row = 0; row < 2; ++row) {
        const QColor middle = mesh.node(column, row).color;
        const qreal tolerance = 2.0 / 65535.0;
        QVERIFY(qAbs(middle.redF() - 0.5) <= tolerance);
        QVERIFY(qAbs(middle.greenF() - 0.3) <= tolerance);
        QVERIFY(qAbs(middle.blueF() - 0.5) <= tolerance);
        QVERIFY(qAbs(middle.alphaF() - 0.7) <= tolerance);
        QCOMPARE(mesh.node(0, row).color, before.node(0, row).color);
        QCOMPARE(mesh.node(2, row).color, before.node(1, row).color);
        QCOMPARE(mesh.node(0, row).node, before.node(0, row).node);
        QCOMPARE(mesh.node(2, row).node, before.node(1, row).node);
    }
}

void KisBezierGradientMeshContractTest::copiesOwnGeometryAndColors()
{
    KisBezierGradientMesh original;
    original.node(0, 0).color = QColor(Qt::red);
    auto changed = original;
    QVERIFY(changed == original);
    changed.node(0, 0).color = QColor(Qt::blue);
    QVERIFY(changed != original);
    QCOMPARE(original.node(0, 0).color, QColor(Qt::red));

    changed = original;
    const QPointF position = original.node(0, 0).node;
    changed.node(0, 0).node += QPointF(1.0, 2.0);
    QVERIFY(changed != original);
    QCOMPARE(original.node(0, 0).node, position);
}

QTEST_APPLESS_MAIN(KisBezierGradientMeshContractTest)

#include "KisBezierGradientMeshContractTest.moc"
