/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoColorBackground.h"

#include <QBrush>
#include <QColor>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QTest>

namespace
{

class OtherBackground final : public KoShapeBackground
{
public:
    void paint(QPainter &, const QPainterPath &) const override
    {
    }

    bool compareTo(const KoShapeBackground *) const override
    {
        return false;
    }
};

class LifetimeColorBackground final : public KoColorBackground
{
public:
    explicit LifetimeColorBackground(int *destructionCount)
        : m_destructionCount(destructionCount)
    {
    }

    ~LifetimeColorBackground() override
    {
        ++*m_destructionCount;
    }

private:
    int *m_destructionCount;
};

} // namespace

class KoColorBackgroundContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructorsExposeConsistentColorStyleAndBrush();
    void copyAndAssignmentDetachWhenColorChanges();
    void comparisonMatchesColorButCurrentlyIgnoresStyle();
    void paintFillsOnlyTheRequestedPath();
    void baseOwnershipDestroysDerivedColorBackground();
};

void KoColorBackgroundContractTest::constructorsExposeConsistentColorStyleAndBrush()
{
    const KoColorBackground defaultBackground;
    QCOMPARE(defaultBackground.color(), QColor(Qt::black));
    QCOMPARE(defaultBackground.style(), Qt::SolidPattern);
    QCOMPARE(defaultBackground.brush(), QBrush(Qt::black, Qt::SolidPattern));

    const QColor translucentColor(12, 34, 56, 78);
    const KoColorBackground patternedBackground(translucentColor, Qt::Dense4Pattern);
    QCOMPARE(patternedBackground.color(), translucentColor);
    QCOMPARE(patternedBackground.style(), Qt::Dense4Pattern);
    QCOMPARE(patternedBackground.brush(), QBrush(translucentColor, Qt::Dense4Pattern));

    const KoColorBackground noBrushBackground(Qt::red, Qt::NoBrush);
    const KoColorBackground gradientStyleBackground(Qt::blue, Qt::LinearGradientPattern);
    QCOMPARE(noBrushBackground.style(), Qt::SolidPattern);
    QCOMPARE(gradientStyleBackground.style(), Qt::SolidPattern);
}

void KoColorBackgroundContractTest::copyAndAssignmentDetachWhenColorChanges()
{
    const QColor originalColor(190, 20, 30);
    KoColorBackground original(originalColor, Qt::Dense3Pattern);
    KoColorBackground copied(original);
    KoColorBackground assigned;
    assigned = original;

    copied.setColor(QColor(30, 170, 40));
    assigned.setColor(QColor(40, 50, 180));

    QCOMPARE(original.color(), originalColor);
    QCOMPARE(copied.color(), QColor(30, 170, 40));
    QCOMPARE(assigned.color(), QColor(40, 50, 180));
    QCOMPARE(original.style(), Qt::Dense3Pattern);
    QCOMPARE(copied.style(), Qt::Dense3Pattern);
    QCOMPARE(assigned.style(), Qt::Dense3Pattern);
}

void KoColorBackgroundContractTest::comparisonMatchesColorButCurrentlyIgnoresStyle()
{
    const KoColorBackground solidRed(Qt::red, Qt::SolidPattern);
    const KoColorBackground denseRed(Qt::red, Qt::Dense1Pattern);
    const KoColorBackground solidBlue(Qt::blue, Qt::SolidPattern);
    const OtherBackground otherType;

    QVERIFY(solidRed.compareTo(&solidRed));
    QVERIFY(solidRed.compareTo(&denseRed));
    QVERIFY(!solidRed.compareTo(&solidBlue));
    QVERIFY(!solidRed.compareTo(&otherType));
}

void KoColorBackgroundContractTest::paintFillsOnlyTheRequestedPath()
{
    const QColor fillColor(17, 93, 201);
    const KoColorBackground background(fillColor);
    const KoShapeBackground *base = &background;
    QImage image(10, 10, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(Qt::NoPen);
    QPainterPath fillPath;
    fillPath.addRect(QRectF(2.0, 2.0, 4.0, 4.0));

    base->paint(painter, fillPath);
    painter.end();

    QCOMPARE(image.pixelColor(3, 3), fillColor);
    QCOMPARE(image.pixelColor(0, 0), QColor(Qt::transparent));
    QCOMPARE(image.pixelColor(8, 8), QColor(Qt::transparent));
}

void KoColorBackgroundContractTest::baseOwnershipDestroysDerivedColorBackground()
{
    int destructionCount = 0;
    KoShapeBackground *background = new LifetimeColorBackground(&destructionCount);

    delete background;
    QCOMPARE(destructionCount, 1);
}

QTEST_GUILESS_MAIN(KoColorBackgroundContractTest)

#include "KoColorBackgroundContractTest.moc"
