/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "events/KisLongPressEventFilter.h"

#include <QContextMenuEvent>
#include <QCoreApplication>
#include <QMouseEvent>
#include <QTest>
#include <QWidget>
#include <QStyleHints>

namespace
{
class ContextMenuProbe final : public QWidget
{
public:
    using QWidget::QWidget;

    int contextMenuCount() const
    {
        return m_contextMenuCount;
    }

protected:
    void contextMenuEvent(QContextMenuEvent *event) override
    {
        ++m_contextMenuCount;
        event->accept();
    }

private:
    int m_contextMenuCount {0};
};
} // namespace

class KisLongPressEventFilterContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void enabledPropertyTurnsLongPressIntoContextMenu();
    void disabledPropertyLeavesLongPressWithTheWidget();
};

void KisLongPressEventFilterContractTest::enabledPropertyTurnsLongPressIntoContextMenu()
{
    ContextMenuProbe target;
    target.setContextMenuPolicy(Qt::DefaultContextMenu);
    target.resize(80, 80);
    target.show();
    QVERIFY(target.isVisible());
    QTest::qWait(20);

    target.setProperty(KisLongPressEventFilter::ENABLED_PROPERTY, true);
    KisLongPressEventFilter filter;
    const QPoint localPosition(10, 10);
    const QPoint globalPosition = target.mapToGlobal(localPosition);
    QMouseEvent press(QEvent::MouseButtonPress,
                      QPointF(localPosition),
                      QPointF(globalPosition),
                      Qt::LeftButton,
                      Qt::LeftButton,
                      Qt::NoModifier);

    QVERIFY(filter.eventFilter(&target, &press));
    QTRY_COMPARE_WITH_TIMEOUT(
        target.contextMenuCount(),
        1,
        qApp->styleHints()->mousePressAndHoldInterval() + 200);
}

void KisLongPressEventFilterContractTest::disabledPropertyLeavesLongPressWithTheWidget()
{
    ContextMenuProbe target;
    target.setContextMenuPolicy(Qt::DefaultContextMenu);
    target.resize(80, 80);
    target.show();
    QVERIFY(target.isVisible());
    QTest::qWait(20);

    target.setProperty(KisLongPressEventFilter::ENABLED_PROPERTY, false);
    KisLongPressEventFilter filter;
    const QPoint localPosition(10, 10);
    const QPoint globalPosition = target.mapToGlobal(localPosition);
    QMouseEvent press(QEvent::MouseButtonPress,
                      QPointF(localPosition),
                      QPointF(globalPosition),
                      Qt::LeftButton,
                      Qt::LeftButton,
                      Qt::NoModifier);

    QVERIFY(!filter.eventFilter(&target, &press));
    QTest::qWait(qApp->styleHints()->mousePressAndHoldInterval() + 200);
    QCoreApplication::processEvents();
    QCOMPARE(target.contextMenuCount(), 0);
}

QTEST_MAIN(KisLongPressEventFilterContractTest)

#include "KisLongPressEventFilterContractTest.moc"
