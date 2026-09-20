/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "cos/kis_cos_parser.h"
#include "cos/kis_cos_writer.h"

#include <QTest>

class KisCosWriterContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void writesTypedCosObjectThatRoundTrips();
    void writesRootlessCompactTxt2Object();
};

void KisCosWriterContractTest::writesTypedCosObjectThatRoundTrips()
{
    const QVariantHash source{
        {QStringLiteral("/Bool"), true},
        {QStringLiteral("/Double"), 0.5},
        {QStringLiteral("/Doubles"), QVariantList({0.25, 1.5})},
        {QStringLiteral("/Empty"), QVariantList()},
        {QStringLiteral("/Int"), 12},
        {QStringLiteral("/Ints"), QVariantList({1, 2, 3})},
        {QStringLiteral("/Nested"), QVariantHash({{QStringLiteral("/Value"), 7}})},
        {QStringLiteral("/StreamTag"), QStringLiteral("/Name")},
        {QStringLiteral("/Text"), QStringLiteral("A(B)\\C")},
    };

    QByteArray encoded = KisCosWriter::writeCosFromVariantHash(source);

    QVERIFY(encoded.startsWith("\n\n<<\n"));
    QVERIFY(encoded.endsWith(">>\n"));

    KisCosParser parser;
    QCOMPARE(parser.parseCosToJson(&encoded), source);
}

void KisCosWriterContractTest::writesRootlessCompactTxt2Object()
{
    const QVariantHash source{{QStringLiteral("/Count"), 7}};

    QCOMPARE(KisCosWriter::writeTxt2FromVariantHash(source), QByteArray(" /Count 7"));
}

QTEST_GUILESS_MAIN(KisCosWriterContractTest)

#include "KisCosWriterContractTest.moc"
