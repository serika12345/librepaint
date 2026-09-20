/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisStretchedIconButton.h"

#include <QAction>
#include <QIconEngine>
#include <QLabel>
#include <QPainter>
#include <QPointer>
#include <QResizeEvent>
#include <QSignalSpy>
#include <QTest>
#include <QWidget>

#include <memory>

namespace
{

struct IconRequests {
    QVector<QSize> sizes;
};

class RecordingIconEngine final : public QIconEngine
{
public:
    explicit RecordingIconEngine(std::shared_ptr<IconRequests> requests)
        : m_requests(std::move(requests))
    {
    }

    QIconEngine *clone() const override
    {
        return new RecordingIconEngine(m_requests);
    }

    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) override
    {
        painter->drawPixmap(rect, pixmap(rect.size(), mode, state));
    }

    QPixmap pixmap(const QSize &size, QIcon::Mode, QIcon::State) override
    {
        m_requests->sizes.append(size);
        QPixmap pixmap(size.expandedTo(QSize(1, 1)));
        pixmap.fill(Qt::red);
        return pixmap;
    }

private:
    std::shared_ptr<IconRequests> m_requests;
};

class ResizeDispatchButton final : public KisStretchedIconButton
{
public:
    explicit ResizeDispatchButton(QWidget *parent = nullptr)
        : KisStretchedIconButton(parent)
    {
    }

    void dispatchResize(const QSize &size, const QSize &oldSize)
    {
        QResizeEvent event(size, oldSize);
        resizeEvent(&event);
    }
};

QIcon solidIcon(const QColor &color)
{
    QPixmap pixmap(16, 12);
    pixmap.fill(color);
    return QIcon(pixmap);
}

} // namespace

class KisStretchedIconButtonContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parentOwnsInitialDisabledButton();
    void stretchedIconIsStoredAndRequestedAgainOnResize();
    void associatedActionSynchronizesPresentationAndTrigger();
    void reassociationDisconnectsOldActionAndNullClearsState();
    void destroyedActionLeavesStalePresentation();
};

void KisStretchedIconButtonContractTest::parentOwnsInitialDisabledButton()
{
    auto *parent = new QWidget;
    QPointer<KisStretchedIconButton> button = new KisStretchedIconButton(parent);

    QCOMPARE(button->parentWidget(), parent);
    QCOMPARE(button->associatedAction(), nullptr);
    QVERIFY(button->stretchedIcon().isNull());
    QCOMPARE(button->text(), QString());
    QVERIFY(!button->isEnabled());
    QVERIFY(button->isHidden());

    QLabel *label = button->findChild<QLabel *>();
    QVERIFY(label);
    QCOMPARE(label->parentWidget(), button.data());
    QCOMPARE(label->alignment(), Qt::AlignHCenter | Qt::AlignVCenter);

    delete parent;
    QVERIFY(button.isNull());
}

void KisStretchedIconButtonContractTest::stretchedIconIsStoredAndRequestedAgainOnResize()
{
    ResizeDispatchButton button;
    QLabel *label = button.findChild<QLabel *>();
    QVERIFY(label);

    auto requests = std::make_shared<IconRequests>();
    const QIcon icon(new RecordingIconEngine(requests));

    button.resize(60, 40);
    label->resize(32, 20);
    button.setStretchedIcon(icon);

    QCOMPARE(button.stretchedIcon().cacheKey(), icon.cacheKey());
    QVERIFY(!label->pixmap().isNull());
    QCOMPARE(requests->sizes.size(), 1);
    const QSize firstRequest = requests->sizes.constLast();

    button.resize(140, 100);
    label->resize(112, 72);
    button.dispatchResize(button.size(), QSize(60, 40));

    QCOMPARE(requests->sizes.size(), 2);
    QVERIFY(requests->sizes.constLast() != firstRequest);
    QVERIFY(!label->pixmap().isNull());
}

void KisStretchedIconButtonContractTest::associatedActionSynchronizesPresentationAndTrigger()
{
    QWidget parent;
    KisStretchedIconButton button(&parent);
    QAction action(&parent);
    const QIcon icon = solidIcon(Qt::green);
    action.setIcon(icon);
    action.setText(QStringLiteral("Action text is not shown"));
    action.setToolTip(QStringLiteral("Tool tip"));
    action.setStatusTip(QStringLiteral("Status tip"));
    action.setWhatsThis(QStringLiteral("What is this"));
    action.setEnabled(true);
    action.setVisible(true);

    QSignalSpy triggeredSpy(&action, &QAction::triggered);
    QVERIFY(triggeredSpy.isValid());

    button.setAssociatedAction(&action);

    QCOMPARE(button.associatedAction(), &action);
    QCOMPARE(button.stretchedIcon().cacheKey(), icon.cacheKey());
    QCOMPARE(button.text(), QString());
    QCOMPARE(button.toolTip(), action.toolTip());
    QCOMPARE(button.statusTip(), action.statusTip());
    QCOMPARE(button.whatsThis(), action.whatsThis());
    QVERIFY(button.isEnabled());
    QVERIFY(!button.isHidden());

    button.click();
    QCOMPARE(triggeredSpy.count(), 1);

    const QIcon replacementIcon = solidIcon(Qt::blue);
    action.setIcon(replacementIcon);
    action.setToolTip(QStringLiteral("Updated tool tip"));
    action.setStatusTip(QStringLiteral("Updated status tip"));
    action.setWhatsThis(QStringLiteral("Updated what is this"));
    action.setEnabled(false);
    action.setVisible(false);

    QCOMPARE(button.stretchedIcon().cacheKey(), replacementIcon.cacheKey());
    QCOMPARE(button.toolTip(), action.toolTip());
    QCOMPARE(button.statusTip(), action.statusTip());
    QCOMPARE(button.whatsThis(), action.whatsThis());
    QVERIFY(!button.isEnabled());
    QVERIFY(button.isHidden());
}

void KisStretchedIconButtonContractTest::reassociationDisconnectsOldActionAndNullClearsState()
{
    QWidget parent;
    KisStretchedIconButton button(&parent);
    QAction first(&parent);
    QAction second(&parent);
    first.setToolTip(QStringLiteral("First"));
    second.setToolTip(QStringLiteral("Second"));

    QSignalSpy firstTriggeredSpy(&first, &QAction::triggered);
    QSignalSpy secondTriggeredSpy(&second, &QAction::triggered);
    QVERIFY(firstTriggeredSpy.isValid());
    QVERIFY(secondTriggeredSpy.isValid());

    button.setAssociatedAction(&first);
    button.setAssociatedAction(&second);
    QCOMPARE(button.associatedAction(), &second);
    QCOMPARE(button.toolTip(), QStringLiteral("Second"));

    first.setToolTip(QStringLiteral("Changed after disconnect"));
    QCOMPARE(button.toolTip(), QStringLiteral("Second"));

    button.click();
    QCOMPARE(firstTriggeredSpy.count(), 0);
    QCOMPARE(secondTriggeredSpy.count(), 1);

    button.setAssociatedAction(nullptr);
    QCOMPARE(button.associatedAction(), nullptr);
    QVERIFY(button.stretchedIcon().isNull());
    QCOMPARE(button.toolTip(), QString());
    QCOMPARE(button.statusTip(), QString());
    QCOMPARE(button.whatsThis(), QString());
    QVERIFY(!button.isEnabled());
    QVERIFY(button.isHidden());

    second.setToolTip(QStringLiteral("Changed after clear"));
    QCOMPARE(button.toolTip(), QString());
}

void KisStretchedIconButtonContractTest::destroyedActionLeavesStalePresentation()
{
    QWidget parent;
    KisStretchedIconButton button(&parent);
    auto action = std::make_unique<QAction>();
    const QIcon icon = solidIcon(Qt::yellow);
    action->setIcon(icon);
    action->setToolTip(QStringLiteral("Stale tool tip"));
    action->setStatusTip(QStringLiteral("Stale status tip"));
    action->setWhatsThis(QStringLiteral("Stale what is this"));
    button.setAssociatedAction(action.get());

    action.reset();

    QCOMPARE(button.associatedAction(), nullptr);
    QCOMPARE(button.stretchedIcon().cacheKey(), icon.cacheKey());
    QCOMPARE(button.toolTip(), QStringLiteral("Stale tool tip"));
    QCOMPARE(button.statusTip(), QStringLiteral("Stale status tip"));
    QCOMPARE(button.whatsThis(), QStringLiteral("Stale what is this"));
    QVERIFY(button.isEnabled());
    QVERIFY(!button.isHidden());
}

QTEST_MAIN(KisStretchedIconButtonContractTest)

#include "KisStretchedIconButtonContractTest.moc"
