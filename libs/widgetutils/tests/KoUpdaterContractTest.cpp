/* This file is part of the KDE project
 *
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QCoreApplication>
#include <QPointer>
#include <QSignalSpy>
#include <QTest>

#include "KoProgressProxy.h"
#include "KoUpdater.h"
#include "KoUpdaterPrivate_p.h"

namespace
{

void deliverDeferredDeletes()
{
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

class DestructorProbe : public KoUpdater
{
public:
    DestructorProbe(KoUpdaterPrivate *updaterPrivate, bool *destroyed)
        : KoUpdater(updaterPrivate)
        , m_destroyed(destroyed)
    {
    }

    ~DestructorProbe() override
    {
        *m_destroyed = true;
    }

private:
    bool *m_destroyed;
};

class ProgressProxyProbe : public KoProgressProxy
{
public:
    ProgressProxyProbe(int maximum, bool *destroyed)
        : m_maximum(maximum)
        , m_destroyed(destroyed)
    {
    }

    ~ProgressProxyProbe() override
    {
        *m_destroyed = true;
    }

    int maximum() const override
    {
        return m_maximum;
    }

    void setValue(int value) override
    {
        lastValue = value;
    }

    void setRange(int minimum, int maximum) override
    {
        lastMinimum = minimum;
        lastMaximum = maximum;
    }

    void setFormat(const QString &format) override
    {
        lastFormat = format;
    }

    int lastValue = -1;
    int lastMinimum = -1;
    int lastMaximum = -1;
    QString lastFormat;

private:
    int m_maximum;
    bool *m_destroyed;
};

} // namespace

class KoUpdaterContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultsExposeProgressState();
    void progressProxyFormatsNestedNames();
    void progressAndRangeMapToSignals();
    void namesAndCancellationAreForwarded();
    void virtualLifetimeUsesThePublicBase();
    void dummyHolderOwnsANoopUpdater();
};

void KoUpdaterContractTest::progressProxyFormatsNestedNames()
{
    bool destroyed = false;
    KoProgressProxy *proxy = new ProgressProxyProbe(100, &destroyed);
    auto *probe = static_cast<ProgressProxyProbe *>(proxy);

    QCOMPARE(proxy->maximum(), 100);
    proxy->setValue(42);
    QCOMPARE(probe->lastValue, 42);
    proxy->setRange(5, 95);
    QCOMPARE(probe->lastMinimum, 5);
    QCOMPARE(probe->lastMaximum, 95);

    proxy->setAutoNestedName(QString());
    QCOMPARE(probe->lastFormat, QStringLiteral("%p%"));
    proxy->setAutoNestedName(QStringLiteral("render"));
    QCOMPARE(probe->lastFormat, QStringLiteral("render: %p%"));

    delete proxy;
    QCOMPARE(destroyed, true);

    bool zeroMaximumDestroyed = false;
    ProgressProxyProbe zeroMaximumProxy(0, &zeroMaximumDestroyed);
    zeroMaximumProxy.setAutoNestedName(QStringLiteral("waiting"));
    QCOMPARE(zeroMaximumProxy.lastFormat, QStringLiteral("waiting"));
}

void KoUpdaterContractTest::defaultsExposeProgressState()
{
    KoUpdaterPrivate updaterPrivate(3, QStringLiteral("task"));
    KoUpdater *updater = updaterPrivate.connectedUpdater();

    QVERIFY(updater);
    QCOMPARE(updater->d.data(), &updaterPrivate);
    QCOMPARE(updater->progress(), 0);
    QCOMPARE(updater->interrupted(), false);
    QCOMPARE(updater->min, 0);
    QCOMPARE(updater->max, 100);
    QCOMPARE(updater->range, 100);
    QCOMPARE(updater->maximum(), 100);
}

void KoUpdaterContractTest::progressAndRangeMapToSignals()
{
    KoUpdaterPrivate updaterPrivate(1, QString());
    KoUpdater *updater = updaterPrivate.connectedUpdater();
    QSignalSpy progressSpy(updater, &KoUpdater::sigProgress);
    QSignalSpy rangeSpy(updater, &KoUpdater::sigHasValidRangeChanged);

    updater->setProgress(25);
    updater->setProgress(25);
    QCOMPARE(updater->progress(), 25);
    QCOMPARE(progressSpy.count(), 1);

    updater->setProgress(0);
    updater->setProgress(0);
    QCOMPARE(progressSpy.count(), 3);

    updater->setRange(10, 30);
    QCOMPARE(updater->min, 10);
    QCOMPARE(updater->max, 30);
    QCOMPARE(updater->range, 20);
    QCOMPARE(updater->maximum(), 30);
    QCOMPARE(rangeSpy.takeFirst().at(0).toBool(), true);

    updater->setValue(20);
    QCOMPARE(updater->progress(), 50);
    updater->setValue(-10);
    QCOMPARE(updater->progress(), 0);
    updater->setValue(90);
    QCOMPARE(updater->progress(), 100);

    updater->setRange(7, 7);
    QCOMPARE(rangeSpy.takeFirst().at(0).toBool(), false);
    updater->setValue(500);
    QCOMPARE(updater->progress(), 7);
}

void KoUpdaterContractTest::namesAndCancellationAreForwarded()
{
    KoUpdaterPrivate updaterPrivate(1, QString());
    KoUpdater *updater = updaterPrivate.connectedUpdater();
    QSignalSpy cancelSpy(updater, &KoUpdater::sigCancel);
    QSignalSpy forwardedCancelSpy(&updaterPrivate, &KoUpdaterPrivate::sigCancelled);
    QSignalSpy nameSpy(updater, &KoUpdater::sigNestedNameChanged);

    updater->setFormat(QStringLiteral("render %p%"));
    updater->setAutoNestedName(QStringLiteral("layer"));
    QCOMPARE(nameSpy.count(), 2);
    QCOMPARE(nameSpy.at(0).at(0).toString(), QStringLiteral("render %p%"));
    QCOMPARE(nameSpy.at(1).at(0).toString(), QStringLiteral("layer"));

    updater->cancel();
    QCOMPARE(cancelSpy.count(), 1);
    QCOMPARE(forwardedCancelSpy.count(), 1);

    updaterPrivate.setInterrupted();
    QCOMPARE(updater->interrupted(), true);
}

void KoUpdaterContractTest::virtualLifetimeUsesThePublicBase()
{
    KoUpdaterPrivate updaterPrivate(1, QString());
    bool destroyed = false;
    KoProgressProxy *updater = new DestructorProbe(&updaterPrivate, &destroyed);

    delete updater;

    QCOMPARE(destroyed, true);
}

void KoUpdaterContractTest::dummyHolderOwnsANoopUpdater()
{
    QPointer<KoUpdater> updater;
    {
        KoDummyUpdaterHolder holder;
        updater = holder.updater();

        QVERIFY(updater);
        QCOMPARE(holder.updater(), updater.data());
        QCOMPARE(updater->progress(), 0);
        QCOMPARE(updater->maximum(), 100);
    }

    deliverDeferredDeletes();
    deliverDeferredDeletes();
    QVERIFY(updater.isNull());
}

QTEST_GUILESS_MAIN(KoUpdaterContractTest)

#include "KoUpdaterContractTest.moc"
