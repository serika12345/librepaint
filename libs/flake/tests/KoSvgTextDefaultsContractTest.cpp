/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "text/KoSvgTextProperties.h"

#include <QSemaphore>
#include <QTest>
#include <QThread>
#include <array>
#include <memory>
#include <vector>

class KoSvgTextDefaultsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void concurrentFirstLayoutUsesCompleteFontDefaults();
};

void KoSvgTextDefaultsContractTest::concurrentFirstLayoutUsesCompleteFontDefaults()
{
    // Consumer: text layout resolving inherited CSS font settings on workers.
    // First use must publish the complete settings to every layout operation.
    constexpr int workers = 32;
    QSemaphore ready;
    QSemaphore begin;
    std::array<KoCSSFontInfo, workers> fonts;
    std::vector<std::unique_ptr<QThread>> threads;
    for (int i = 0; i < workers; ++i) {
        threads.emplace_back(QThread::create([&, i] {
            ready.release();
            begin.acquire();
            fonts[i] = KoSvgTextProperties().cssFontInfo();
        }));
        threads.back()->start();
    }
    ready.acquire(workers);
    begin.release(workers);
    for (const auto &thread : threads) thread->wait();

    for (const auto &font : fonts) {
        QCOMPARE(font.weight, 400);
        QCOMPARE(font.width, 100);
        QCOMPARE(font.size, 12.0);
        QCOMPARE(font.families, QStringList{QStringLiteral("sans-serif")});
        QVERIFY(font.automaticOpticalSizing);
    }
}

QTEST_APPLESS_MAIN(KoSvgTextDefaultsContractTest)

#include "KoSvgTextDefaultsContractTest.moc"
