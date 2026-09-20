/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_queues_progress_updater.h"

#include <KoProgressProxy.h>

#include <QCoreApplication>
#include <QEvent>
#include <QSignalSpy>
#include <QTest>

void KoProgressProxy::setAutoNestedName(const QString &)
{
}

namespace
{
class ProgressProxyProbe : public KoProgressProxy
{
public:
    int maximum() const override
    {
        return maximumValue;
    }

    void setValue(int value) override
    {
        calls.append(QStringLiteral("value"));
        currentValue = value;
    }

    void setRange(int minimum, int maximum) override
    {
        calls.append(QStringLiteral("range"));
        minimumValue = minimum;
        maximumValue = maximum;
    }

    void setFormat(const QString &format) override
    {
        calls.append(QStringLiteral("format"));
        currentFormat = format;
    }

    QStringList calls;
    QString currentFormat;
    int minimumValue = -1;
    int maximumValue = -1;
    int currentValue = -1;
};
} // namespace

class KisQueuesProgressUpdaterContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructionKeepsBorrowedProxyAndQObjectParent();
    void firstQueuedWorkStartsTickingOnlyOnce();
    void zeroAndHideStopAndAllowRestart();
    void queuedStopResetsBorrowedProgressProxy();
};

void KisQueuesProgressUpdaterContractTest::constructionKeepsBorrowedProxyAndQObjectParent()
{
    QObject parent;
    ProgressProxyProbe proxy;
    auto *updater = new KisQueuesProgressUpdater(&proxy, &parent);

    QCOMPARE(updater->parent(), &parent);
    delete updater;
    QCOMPARE(proxy.calls, QStringList());
}

void KisQueuesProgressUpdaterContractTest::firstQueuedWorkStartsTickingOnlyOnce()
{
    ProgressProxyProbe proxy;
    KisQueuesProgressUpdater updater(&proxy);
    QSignalSpy startSpy(&updater, &KisQueuesProgressUpdater::sigStartTicking);
    QSignalSpy stopSpy(&updater, &KisQueuesProgressUpdater::sigStopTicking);

    updater.updateProgress(8, QStringLiteral("projection"));
    updater.updateProgress(7, QStringLiteral("projection"));
    updater.updateProgress(12, QStringLiteral("replacement"));

    QCOMPARE(startSpy.count(), 1);
    QCOMPARE(stopSpy.count(), 0);
}

void KisQueuesProgressUpdaterContractTest::zeroAndHideStopAndAllowRestart()
{
    ProgressProxyProbe proxy;
    KisQueuesProgressUpdater updater(&proxy);
    QSignalSpy startSpy(&updater, &KisQueuesProgressUpdater::sigStartTicking);
    QSignalSpy stopSpy(&updater, &KisQueuesProgressUpdater::sigStopTicking);

    updater.updateProgress(4, QStringLiteral("first"));
    updater.updateProgress(0, QStringLiteral("ignored"));
    updater.updateProgress(3, QStringLiteral("second"));
    updater.hide();
    updater.hide();

    QCOMPARE(startSpy.count(), 2);
    QCOMPARE(stopSpy.count(), 2);
}

void KisQueuesProgressUpdaterContractTest::queuedStopResetsBorrowedProgressProxy()
{
    ProgressProxyProbe proxy;
    KisQueuesProgressUpdater updater(&proxy);

    updater.updateProgress(6, QStringLiteral("tiles"));
    updater.hide();
    QCoreApplication::sendPostedEvents(&updater, QEvent::MetaCall);

    QCOMPARE(proxy.calls, QStringList({QStringLiteral("range"), QStringLiteral("value"), QStringLiteral("format")}));
    QCOMPARE(proxy.minimumValue, 0);
    QCOMPARE(proxy.maximumValue, 100);
    QCOMPARE(proxy.currentValue, 100);
    QCOMPARE(proxy.currentFormat, QStringLiteral("%p%"));
}

QTEST_GUILESS_MAIN(KisQueuesProgressUpdaterContractTest)

#include "KisQueuesProgressUpdaterContractTest.moc"
