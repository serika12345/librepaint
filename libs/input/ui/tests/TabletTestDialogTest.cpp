/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "wintab/drawpile_tablettester/tablettester.h"

#include <QAccessible>
#include <QApplication>
#include <QStandardPaths>
#include <QTabletEvent>
#include <QTest>

#include <KLocalizedString>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QPointingDevice>
#endif

// Keep production assertion failures fatal in this standalone widget test.
void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected assertion %s at %s:%d", assertion, file, line);
}

void kis_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("Unexpected assertion %s at %s:%d", assertion, file, line);
}

namespace
{

QAccessibleInterface *findAccessible(QAccessibleInterface *root, QAccessible::Role role, const QString &name = {})
{
    if (!root) {
        return nullptr;
    }
    if (root->role() == role && (name.isEmpty() || root->text(QAccessible::Name) == name)) {
        return root;
    }
    for (int i = 0; i < root->childCount(); ++i) {
        if (auto *result = findAccessible(root->child(i), role, name)) {
            return result;
        }
    }
    return nullptr;
}

void sendProximity(bool eraser, QEvent::Type eventType)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const auto pointerType = eraser ? QPointingDevice::PointerType::Eraser : QPointingDevice::PointerType::Pen;
    const QPointingDevice device(QStringLiteral("Test tablet"),
                                 1,
                                 QInputDevice::DeviceType::Stylus,
                                 pointerType,
                                 QInputDevice::Capability::Position,
                                 1,
                                 1);
    QTabletEvent event(eventType,
                       &device,
                       QPointF(),
                       QPointF(),
                       0.0,
                       0.0,
                       0.0,
                       0.0,
                       0.0,
                       0.0,
                       Qt::NoModifier,
                       Qt::NoButton,
                       Qt::NoButton);
#else
    QTabletEvent event(eventType,
                       QPointF(),
                       QPointF(),
                       QTabletEvent::Stylus,
                       eraser ? QTabletEvent::Eraser : QTabletEvent::Pen,
                       0.0,
                       0,
                       0,
                       0.0,
                       0.0,
                       0,
                       Qt::NoModifier,
                       1);
#endif
    QApplication::sendEvent(qApp, &event);
}

} // namespace

class TabletTestDialogTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void reportsToolProximity_data();
    void reportsToolProximity();
    void clearRemovesReportsAndAllowsFurtherInput();
    void closeEndsTheDialog();
};

void TabletTestDialogTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    KLocalizedString::setApplicationDomain("krita");
    KLocalizedString::setLanguages({QStringLiteral("en_US")});
}

void TabletTestDialogTest::reportsToolProximity_data()
{
    QTest::addColumn<bool>("eraser");
    QTest::addColumn<int>("eventType");
    QTest::addColumn<QString>("message");

    QTest::newRow("pen-enters") << false << int(QEvent::TabletEnterProximity) << QStringLiteral("Pen tip brought near");
    QTest::newRow("pen-leaves") << false << int(QEvent::TabletLeaveProximity) << QStringLiteral("Pen tip taken away");
    QTest::newRow("eraser-enters") << true << int(QEvent::TabletEnterProximity)
                                   << QStringLiteral("Eraser brought near");
    QTest::newRow("eraser-leaves") << true << int(QEvent::TabletLeaveProximity) << QStringLiteral("Eraser taken away");
}

void TabletTestDialogTest::reportsToolProximity()
{
    QFETCH(bool, eraser);
    QFETCH(int, eventType);
    QFETCH(QString, message);

    TabletTestDialog dialog;
    auto *report = findAccessible(QAccessible::queryAccessibleInterface(&dialog), QAccessible::EditableText);
    QVERIFY(report);
    QVERIFY(!report->text(QAccessible::Value).contains(message));

    sendProximity(eraser, QEvent::Type(eventType));

    QVERIFY2(report->text(QAccessible::Value).contains(message), qPrintable(report->text(QAccessible::Value)));
}

void TabletTestDialogTest::clearRemovesReportsAndAllowsFurtherInput()
{
    TabletTestDialog dialog;
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    auto *root = QAccessible::queryAccessibleInterface(&dialog);
    auto *report = findAccessible(root, QAccessible::EditableText);
    auto *clear = findAccessible(root, QAccessible::Button, QStringLiteral("Clear"));
    QVERIFY(report);
    QVERIFY(clear);
    QVERIFY(clear->actionInterface());
    sendProximity(false, QEvent::TabletEnterProximity);
    QVERIFY(report->text(QAccessible::Value).contains(QStringLiteral("Pen tip brought near")));

    clear->actionInterface()->doAction(QAccessibleActionInterface::pressAction());

    QTRY_VERIFY(report->text(QAccessible::Value).isEmpty());
    sendProximity(true, QEvent::TabletLeaveProximity);
    QVERIFY(report->text(QAccessible::Value).contains(QStringLiteral("Eraser taken away")));
    QVERIFY(!report->text(QAccessible::Value).contains(QStringLiteral("Pen tip brought near")));
}

void TabletTestDialogTest::closeEndsTheDialog()
{
    TabletTestDialog dialog;
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    QVERIFY(dialog.isVisible());
    auto *close =
        findAccessible(QAccessible::queryAccessibleInterface(&dialog), QAccessible::Button, QStringLiteral("Close"));
    QVERIFY(close);
    QVERIFY(close->actionInterface());

    close->actionInterface()->doAction(QAccessibleActionInterface::pressAction());

    QTRY_VERIFY(!dialog.isVisible());
}

QTEST_MAIN(TabletTestDialogTest)

#include "TabletTestDialogTest.moc"
