/* This file is part of the KDE project
 *
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QPointer>
#include <QSignalSpy>
#include <QTest>
#include <QWidget>

#include "KoProgressBar.h"
#include "KoProgressProxy.h"

class KoProgressBarContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parentOwnsTheProgressBar();
    void rangeFormatAndActiveValuesUseTheWidgetState();
    void terminalValuesEmitDoneAndHideTheWidget();
    void progressProxyProvidesVirtualLifetime();
};

void KoProgressBarContractTest::parentOwnsTheProgressBar()
{
    QPointer<KoProgressBar> progressBar;
    {
        QWidget parent;
        progressBar = new KoProgressBar(&parent);

        QCOMPARE(progressBar->parentWidget(), &parent);
        QCOMPARE(progressBar->minimum(), 0);
        QCOMPARE(progressBar->maximum(), 100);
    }

    QVERIFY(progressBar.isNull());
}

void KoProgressBarContractTest::rangeFormatAndActiveValuesUseTheWidgetState()
{
    KoProgressBar progressBar;
    KoProgressProxy *proxy = &progressBar;
    QSignalSpy doneSpy(&progressBar, &KoProgressBar::done);

    proxy->setRange(10, 20);
    QCOMPARE(progressBar.minimum(), 10);
    QCOMPARE(proxy->maximum(), 20);

    proxy->setFormat(QStringLiteral("render %p% 描画"));
    QCOMPARE(progressBar.format(), QStringLiteral("render %p% 描画"));

    proxy->setValue(15);
    QCOMPARE(progressBar.value(), 15);
    QVERIFY(!progressBar.isHidden());
    QCOMPARE(doneSpy.count(), 0);

    progressBar.hide();
    proxy->setRange(0, 0);
    proxy->setValue(50);
    QVERIFY(!progressBar.isHidden());
    QCOMPARE(doneSpy.count(), 0);
}

void KoProgressBarContractTest::terminalValuesEmitDoneAndHideTheWidget()
{
    KoProgressBar progressBar;
    QSignalSpy doneSpy(&progressBar, &KoProgressBar::done);

    progressBar.setRange(10, 20);
    progressBar.setValue(20);
    QCOMPARE(doneSpy.count(), 1);
    QVERIFY(progressBar.isHidden());

    progressBar.setVisible(true);
    progressBar.setValue(9);
    QCOMPARE(doneSpy.count(), 2);
    QVERIFY(progressBar.isHidden());
}

void KoProgressBarContractTest::progressProxyProvidesVirtualLifetime()
{
    auto *progressBar = new KoProgressBar;
    QPointer<KoProgressBar> guardedProgressBar(progressBar);
    KoProgressProxy *proxy = progressBar;

    delete proxy;

    QVERIFY(guardedProgressBar.isNull());
}

QTEST_MAIN(KoProgressBarContractTest)

#include "KoProgressBarContractTest.moc"
