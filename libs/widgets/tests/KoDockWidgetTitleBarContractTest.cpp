/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoDockWidgetTitleBar.h>

#include <QAbstractButton>
#include <QAction>
#include <QDockWidget>
#include <QIcon>
#include <QLabel>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QTest>

namespace
{

QStringList requestedIconNames;

QList<QAbstractButton *> directButtons(const QWidget &widget)
{
    return widget.findChildren<QAbstractButton *>(QString(), Qt::FindDirectChildrenOnly);
}

QAbstractButton *lockButton(const QWidget &widget)
{
    const auto buttons = directButtons(widget);
    for (QAbstractButton *button : buttons) {
        if (button->isCheckable()) {
            return button;
        }
    }

    return nullptr;
}

QList<QAbstractButton *> nonLockButtons(const QWidget &widget)
{
    QList<QAbstractButton *> result;
    const auto buttons = directButtons(widget);
    for (QAbstractButton *button : buttons) {
        if (!button->isCheckable()) {
            result.append(button);
        }
    }

    return result;
}

} // namespace

namespace KisIconUtils
{

QIcon loadIcon(const QString &name)
{
    requestedIconNames.append(name);
    return {};
}

} // namespace KisIconUtils

class KoDockWidgetTitleBarContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructsControlsAndFollowsWidgetLifetime();
    void lockRoundTripPreservesDockFeaturesAndActions();
    void updateIconsRequestsAssetsForTheCurrentLockState();
    void repeatedLockDropsThePreviouslySavedFeatures();
};

void KoDockWidgetTitleBarContractTest::constructsControlsAndFollowsWidgetLifetime()
{
    auto *dock = new QDockWidget;
    dock->setWindowTitle(QStringLiteral("Layers Ω"));
    dock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable
                      | QDockWidget::DockWidgetMovable);

    QPointer<KoDockWidgetTitleBar> titleBar = new KoDockWidgetTitleBar(dock);

    QCOMPARE(titleBar->parentWidget(), dock);
    QCOMPARE(directButtons(*titleBar).size(), 3);
    QVERIFY(lockButton(*titleBar));
    QCOMPARE(nonLockButtons(*titleBar).size(), 2);
    QLabel *titleLabel = titleBar->findChild<QLabel *>(QString(), Qt::FindDirectChildrenOnly);
    QVERIFY(titleLabel);
    QCOMPARE(titleLabel->text(), dock->windowTitle());

    delete dock;
    QVERIFY(titleBar.isNull());

    QDockWidget survivingDock;
    QPointer<KoDockWidgetTitleBar> polymorphicTitleBar = new KoDockWidgetTitleBar(&survivingDock);
    QWidget *base = polymorphicTitleBar.data();
    delete base;
    QVERIFY(polymorphicTitleBar.isNull());
}

void KoDockWidgetTitleBarContractTest::lockRoundTripPreservesDockFeaturesAndActions()
{
    QDockWidget dock;
    const QDockWidget::DockWidgetFeatures originalFeatures =
        QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable;
    dock.setFeatures(originalFeatures);
    KoDockWidgetTitleBar titleBar(&dock);
    QAbstractButton *lock = lockButton(titleBar);
    const auto otherButtons = nonLockButtons(titleBar);
    QVERIFY(lock);
    QCOMPARE(otherButtons.size(), 2);

    titleBar.setLocked(true);

    QCOMPARE(dock.features(), QDockWidget::NoDockWidgetFeatures);
    QVERIFY(dock.property("Locked").toBool());
    QVERIFY(lock->isChecked());
    QVERIFY(!dock.toggleViewAction()->isEnabled());
    for (QAbstractButton *button : otherButtons) {
        QVERIFY(!button->isEnabled());
        QVERIFY(button->isHidden());
    }

    titleBar.setLocked(false);

    QCOMPARE(dock.features(), originalFeatures);
    QVERIFY(!dock.property("Locked").toBool());
    QVERIFY(!lock->isChecked());
    QVERIFY(dock.toggleViewAction()->isEnabled());
    for (QAbstractButton *button : otherButtons) {
        QVERIFY(button->isEnabled());
        QVERIFY(!button->isHidden());
    }
}

void KoDockWidgetTitleBarContractTest::updateIconsRequestsAssetsForTheCurrentLockState()
{
    QDockWidget dock;
    KoDockWidgetTitleBar titleBar(&dock);

    requestedIconNames.clear();
    titleBar.updateIcons();
    QCOMPARE(
        requestedIconNames,
        QStringList({QStringLiteral("docker_lock_a"), QStringLiteral("docker_float"), QStringLiteral("docker_close")}));

    titleBar.setLocked(true);
    requestedIconNames.clear();
    titleBar.updateIcons();
    QCOMPARE(
        requestedIconNames,
        QStringList({QStringLiteral("docker_lock_b"), QStringLiteral("docker_float"), QStringLiteral("docker_close")}));
}

void KoDockWidgetTitleBarContractTest::repeatedLockDropsThePreviouslySavedFeatures()
{
    QDockWidget dock;
    dock.setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable
                     | QDockWidget::DockWidgetMovable);
    KoDockWidgetTitleBar titleBar(&dock);

    titleBar.setLocked(true);
    titleBar.setLocked(true);
    titleBar.setLocked(false);

    QCOMPARE(dock.features(), QDockWidget::NoDockWidgetFeatures);
}

QTEST_MAIN(KoDockWidgetTitleBarContractTest)

#include "KoDockWidgetTitleBarContractTest.moc"
