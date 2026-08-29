/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisWrappableHBoxLayout.h"

#include <QPointer>
#include <QTest>
#include <QWidget>

namespace
{
class FixedLayoutItem final : public QLayoutItem
{
public:
    FixedLayoutItem(const QSize &sizeHint, const QSize &minimumSize, bool *destroyed = nullptr)
        : m_sizeHint(sizeHint)
        , m_minimumSize(minimumSize)
        , m_destroyed(destroyed)
    {
    }

    ~FixedLayoutItem() override
    {
        if (m_destroyed) {
            *m_destroyed = true;
        }
    }

    Qt::Orientations expandingDirections() const override
    {
        return {};
    }

    QRect geometry() const override
    {
        return m_geometry;
    }

    bool isEmpty() const override
    {
        return false;
    }

    QSize maximumSize() const override
    {
        return QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    }

    QSize minimumSize() const override
    {
        return m_minimumSize;
    }

    void setGeometry(const QRect &geometry) override
    {
        m_geometry = geometry;
    }

    QSize sizeHint() const override
    {
        return m_sizeHint;
    }

private:
    const QSize m_sizeHint;
    const QSize m_minimumSize;
    bool *const m_destroyed;
    QRect m_geometry;
};
} // namespace

class KisWrappableHBoxLayoutContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultStateAndParentLifetime();
    void itemSequenceAndTransfer();
    void geometryWrapsItemsDeterministically();
    void sizeHintsPreserveCurrentMarginRules();
};

void KisWrappableHBoxLayoutContractTest::defaultStateAndParentLifetime()
{
    bool itemDestroyed = false;
    auto *parent = new QWidget;
    QPointer<KisWrappableHBoxLayout> layout = new KisWrappableHBoxLayout(parent);
    layout->addItem(new FixedLayoutItem(QSize(10, 10), QSize(5, 5), &itemDestroyed));

    QCOMPARE(layout->parentWidget(), parent);
    QCOMPARE(layout->contentsMargins(), QMargins());
    QCOMPARE(layout->count(), 1);
    QVERIFY(layout->hasHeightForWidth());

    delete parent;
    QVERIFY(layout.isNull());
    QVERIFY(itemDestroyed);
}

void KisWrappableHBoxLayoutContractTest::itemSequenceAndTransfer()
{
    KisWrappableHBoxLayout layout;
    bool secondDestroyed = false;
    auto *first = new FixedLayoutItem(QSize(10, 10), QSize(5, 5));
    auto *second = new FixedLayoutItem(QSize(20, 10), QSize(10, 5), &secondDestroyed);
    auto *third = new FixedLayoutItem(QSize(30, 10), QSize(15, 5));

    layout.addItem(first);
    layout.addItem(second);
    layout.addItem(third);

    QCOMPARE(layout.count(), 3);
    QCOMPARE(layout.itemAt(0), first);
    QCOMPARE(layout.itemAt(1), second);
    QCOMPARE(layout.itemAt(2), third);
    QVERIFY(!layout.itemAt(-1));
    QVERIFY(!layout.itemAt(3));
    QVERIFY(!layout.takeAt(-1));
    QVERIFY(!layout.takeAt(3));

    QLayoutItem *taken = layout.takeAt(1);
    QCOMPARE(taken, second);
    QCOMPARE(layout.count(), 2);
    QCOMPARE(layout.itemAt(0), first);
    QCOMPARE(layout.itemAt(1), third);
    QVERIFY(!secondDestroyed);
    delete taken;
    QVERIFY(secondDestroyed);
}

void KisWrappableHBoxLayoutContractTest::geometryWrapsItemsDeterministically()
{
    KisWrappableHBoxLayout layout;
    layout.setSpacing(5);
    auto *first = new FixedLayoutItem(QSize(30, 10), QSize(30, 10));
    auto *second = new FixedLayoutItem(QSize(20, 15), QSize(20, 15));
    auto *third = new FixedLayoutItem(QSize(25, 8), QSize(25, 8));
    layout.addItem(first);
    layout.addItem(second);
    layout.addItem(third);

    QCOMPARE(layout.heightForWidth(100), 15);
    layout.setGeometry(QRect(10, 20, 100, 60));
    QCOMPARE(first->geometry(), QRect(10, 20, 30, 10));
    QCOMPARE(second->geometry(), QRect(45, 20, 20, 15));
    QCOMPARE(third->geometry(), QRect(70, 20, 25, 8));

    QCOMPARE(layout.heightForWidth(55), 30);
    layout.setGeometry(QRect(10, 20, 55, 40));
    QCOMPARE(first->geometry(), QRect(10, 20, 30, 10));
    QCOMPARE(second->geometry(), QRect(10, 35, 20, 15));
    QCOMPARE(third->geometry(), QRect(35, 35, 25, 8));
}

void KisWrappableHBoxLayoutContractTest::sizeHintsPreserveCurrentMarginRules()
{
    KisWrappableHBoxLayout layout;
    layout.setContentsMargins(2, 3, 4, 5);
    layout.setSpacing(7);
    layout.addItem(new FixedLayoutItem(QSize(30, 10), QSize(30, 10)));
    layout.addItem(new FixedLayoutItem(QSize(20, 15), QSize(20, 15)));

    QCOMPARE(layout.minimumSize(), QSize(36, 23));
    QCOMPARE(layout.sizeHint(), QSize(36, 23));

    layout.setGeometry(QRect(0, 0, 80, 40));
    QCOMPARE(layout.heightForWidth(80), 23);
    QCOMPARE(layout.minimumSize(), QSize(30, 23));
    QCOMPARE(layout.sizeHint(), QSize(86, 31));
}

QTEST_MAIN(KisWrappableHBoxLayoutContractTest)
#include "KisWrappableHBoxLayoutContractTest.moc"
