/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisPopupSelfActivatingLineEdit.h"

#include <QApplication>
#include <QEvent>
#include <QPointer>
#include <QSignalSpy>
#include <QTest>
#include <QWidget>

class ActivationTrackingPopup : public QWidget
{
public:
    ActivationTrackingPopup()
        : QWidget(nullptr, Qt::Popup)
    {
    }

    void clearActivationEvents()
    {
        m_activationEvents = 0;
    }

    int activationEvents() const
    {
        return m_activationEvents;
    }

protected:
    bool event(QEvent *event) override
    {
        if (event->type() == QEvent::WindowActivate) {
            ++m_activationEvents;
        }
        return QWidget::event(event);
    }

private:
    int m_activationEvents = 0;
};

class KisPopupSelfActivatingLineEditContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parentOwnsAndDestroysLineEdit();
    void popupWindowActivatesWhenLineEditReceivesFocus();
};

void KisPopupSelfActivatingLineEditContractTest::parentOwnsAndDestroysLineEdit()
{
    auto *parent = new QWidget;
    auto *lineEdit = new KisPopupSelfActivatingLineEdit(parent);
    QPointer<KisPopupSelfActivatingLineEdit> guardedLineEdit(lineEdit);
    QSignalSpy destroyedSpy(lineEdit, &QObject::destroyed);

    QCOMPARE(lineEdit->parentWidget(), parent);

    delete parent;

    QVERIFY(guardedLineEdit.isNull());
    QCOMPARE(destroyedSpy.count(), 1);
}

void KisPopupSelfActivatingLineEditContractTest::popupWindowActivatesWhenLineEditReceivesFocus()
{
    QWidget activeWindow;
    activeWindow.resize(80, 40);
    activeWindow.show();
    QVERIFY(QTest::qWaitForWindowExposed(&activeWindow));
    activeWindow.activateWindow();
    QTRY_COMPARE(QApplication::activeWindow(), &activeWindow);

    ActivationTrackingPopup popup;
    popup.resize(160, 48);
    popup.setAttribute(Qt::WA_ShowWithoutActivating);
    KisPopupSelfActivatingLineEdit lineEdit(&popup);
    lineEdit.setGeometry(8, 8, 144, 32);

    popup.show();
    QVERIFY(QTest::qWaitForWindowExposed(&popup));
    activeWindow.activateWindow();
    QTRY_COMPARE(QApplication::activeWindow(), &activeWindow);
    QVERIFY(popup.isVisible());
    lineEdit.clearFocus();
    QCoreApplication::processEvents();
    popup.clearActivationEvents();

    lineEdit.setFocus(Qt::OtherFocusReason);

    QTRY_VERIFY(popup.activationEvents() > 0);
    QCOMPARE(QApplication::activeWindow(), &popup);
}

QTEST_MAIN(KisPopupSelfActivatingLineEditContractTest)

#include "KisPopupSelfActivatingLineEditContractTest.moc"
