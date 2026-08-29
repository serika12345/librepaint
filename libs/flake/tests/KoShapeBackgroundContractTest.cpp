/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoShapeBackground.h"

#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QTest>

namespace
{
class BackgroundProbe : public KoShapeBackground
{
public:
    explicit BackgroundProbe(int *destructionCount = nullptr)
        : m_destructionCount(destructionCount)
    {
    }

    ~BackgroundProbe() override
    {
        if (m_destructionCount) {
            ++*m_destructionCount;
        }
    }

    void paint(QPainter &painter, const QPainterPath &fillPath) const override
    {
        ++paintCallCount;
        observedPainter = &painter;
        observedPath = fillPath;
        painter.fillPath(fillPath, Qt::red);
    }

    bool compareTo(const KoShapeBackground *other) const override
    {
        ++compareCallCount;
        observedComparison = other;
        return other == matchingBackground;
    }

    mutable int paintCallCount = 0;
    mutable int compareCallCount = 0;
    mutable const QPainter *observedPainter = nullptr;
    mutable QPainterPath observedPath;
    mutable const KoShapeBackground *observedComparison = nullptr;
    const KoShapeBackground *matchingBackground = nullptr;

private:
    int *m_destructionCount;
};

class StatefulBackgroundProbe : public BackgroundProbe
{
public:
    bool hasTransparency() const override
    {
        return transparent;
    }

    explicit operator bool() const override
    {
        return enabled;
    }

    bool transparent = false;
    bool enabled = true;
};
} // namespace

class KoShapeBackgroundContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void baseDefaultsAndVirtualDestructionAreStable();
    void paintDispatchesWithPainterAndFillPath();
    void comparisonDispatchesWithOtherIdentity();
    void derivedTransparencyAndBooleanStateAreDispatched();
};

void KoShapeBackgroundContractTest::baseDefaultsAndVirtualDestructionAreStable()
{
    int destructionCount = 0;
    KoShapeBackground *background = new BackgroundProbe(&destructionCount);

    QVERIFY(!background->hasTransparency());
    QVERIFY(static_cast<bool>(*background));

    delete background;
    QCOMPARE(destructionCount, 1);
}

void KoShapeBackgroundContractTest::paintDispatchesWithPainterAndFillPath()
{
    QImage image(8, 8, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    QPainterPath fillPath;
    fillPath.addRect(QRectF(1.0, 1.0, 4.0, 4.0));

    BackgroundProbe background;
    const KoShapeBackground *base = &background;
    base->paint(painter, fillPath);
    painter.end();

    QCOMPARE(background.paintCallCount, 1);
    QCOMPARE(background.observedPainter, &painter);
    QVERIFY(background.observedPath == fillPath);
    QCOMPARE(image.pixelColor(2, 2), QColor(Qt::red));
    QCOMPARE(image.pixelColor(7, 7), QColor(Qt::transparent));
}

void KoShapeBackgroundContractTest::comparisonDispatchesWithOtherIdentity()
{
    BackgroundProbe background;
    BackgroundProbe matchingBackground;
    BackgroundProbe otherBackground;
    background.matchingBackground = &matchingBackground;

    const KoShapeBackground *base = &background;
    QVERIFY(base->compareTo(&matchingBackground));
    QCOMPARE(background.observedComparison, &matchingBackground);

    QVERIFY(!base->compareTo(&otherBackground));
    QCOMPARE(background.observedComparison, &otherBackground);
    QCOMPARE(background.compareCallCount, 2);
}

void KoShapeBackgroundContractTest::derivedTransparencyAndBooleanStateAreDispatched()
{
    StatefulBackgroundProbe background;
    const KoShapeBackground *base = &background;

    background.transparent = true;
    background.enabled = false;
    QVERIFY(base->hasTransparency());
    QVERIFY(!static_cast<bool>(*base));

    background.transparent = false;
    background.enabled = true;
    QVERIFY(!base->hasTransparency());
    QVERIFY(static_cast<bool>(*base));
}

QTEST_GUILESS_MAIN(KoShapeBackgroundContractTest)

#include "KoShapeBackgroundContractTest.moc"
