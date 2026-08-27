/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QBuffer>
#include <QPointer>
#include <QTest>

#include <KisRemoteFileFetcher.h>

namespace
{
const QUrl contractUrl(QStringLiteral("data:text/plain;base64,TGlicmVQYWludCBjb250cmFjdA=="));
const QByteArray contractContent("LibrePaint contract");
} // namespace

class KisRemoteFileFetcherTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parentOwnsFetcherLifetime();
    void dataUrlWritesAndClosesOutputDevice();
    void staticSuccessfulFetchReturnsEmptyData();
};

void KisRemoteFileFetcherTest::parentOwnsFetcherLifetime()
{
    auto *parent = new QObject;
    auto *fetcher = new KisRemoteFileFetcher(parent);
    QPointer<KisRemoteFileFetcher> guard(fetcher);

    QCOMPARE(fetcher->parent(), parent);
    delete parent;
    QVERIFY(guard.isNull());
}

void KisRemoteFileFetcherTest::dataUrlWritesAndClosesOutputDevice()
{
    QBuffer output;
    KisRemoteFileFetcher fetcher;

    QVERIFY(fetcher.fetchFile(contractUrl, &output));
    QVERIFY(!output.isOpen());
    QCOMPARE(output.data(), contractContent);
}

void KisRemoteFileFetcherTest::staticSuccessfulFetchReturnsEmptyData()
{
    QCOMPARE(KisRemoteFileFetcher::fetchFile(contractUrl), QByteArray());
}

QTEST_MAIN(KisRemoteFileFetcherTest)

#include "kis_remote_file_fetcher_test.moc"
