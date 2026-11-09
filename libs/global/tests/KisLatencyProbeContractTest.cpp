/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "KisLatencyProbe.h"

#include <QFile>
#include <QTemporaryDir>
#include <QTest>

// scripts/perf/pen-latency/analyze.py reads the probe file. It needs the
// clock frequency line, the column header, and rows in recording order with
// non-decreasing times, so that markers can be matched against PresentMon.
class KisLatencyProbeContractTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    QString m_path;

private Q_SLOTS:
    void initTestCase()
    {
        QVERIFY(m_dir.isValid());
        m_path = m_dir.filePath(QStringLiteral("probe.csv"));
        // The probe reads the variable once, at its first use.
        qputenv("LIBREPAINT_LATENCY_LOG", QFile::encodeName(m_path));
    }

    void writesMarkersInRecordingOrder()
    {
        QVERIFY(KisLatencyProbe::enabled());
        KisLatencyProbe::mark("outline_request", 12.5, 34.25, 7);
        KisLatencyProbe::mark("paintgl_begin");

        QStringList lines;
        // The writer thread flushes every 500 ms.
        QTRY_VERIFY_WITH_TIMEOUT(
            [&] {
                QFile file(m_path);
                if (!file.open(QIODevice::ReadOnly)) return false;
                lines = QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
                return lines.size() >= 4;
            }(),
            5000);

        QVERIFY(lines[0].startsWith(QLatin1String("# frequency=")));
        QVERIFY(lines[0].mid(12).toLongLong() > 0);
        QCOMPARE(lines[1], QStringLiteral("time,event,x,y,extra,thread"));

        const QStringList first = lines[2].split(QLatin1Char(','));
        const QStringList second = lines[3].split(QLatin1Char(','));
        QCOMPARE(first.size(), 6);
        QCOMPARE(first[1], QStringLiteral("outline_request"));
        QCOMPARE(first[2].toDouble(), 12.5);
        QCOMPARE(first[3].toDouble(), 34.25);
        QCOMPARE(first[4], QStringLiteral("7"));
        QCOMPARE(second[1], QStringLiteral("paintgl_begin"));
        QVERIFY(second[0].toLongLong() >= first[0].toLongLong());
    }
};

QTEST_GUILESS_MAIN(KisLatencyProbeContractTest)

#include "KisLatencyProbeContractTest.moc"
