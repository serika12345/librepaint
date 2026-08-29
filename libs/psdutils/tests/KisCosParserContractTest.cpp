/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "cos/kis_cos_parser.h"

#include <QTest>

class KisCosParserContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parsesTypedNestedObject();
    void decodesUtf16AndEscapedStrings();
};

void KisCosParserContractTest::parsesTypedNestedObject()
{
    QByteArray source =
        "<< /Enabled true /Count -3 /Ratio .5 /Name /Display /Hex <0a0B> "
        "/Items [ 1 false null ] /Child << /Label (nested) >> >>";

    KisCosParser parser;
    const QVariantHash result = parser.parseCosToJson(&source);

    QCOMPARE(result.value(QStringLiteral("/Enabled")), QVariant(true));
    QCOMPARE(result.value(QStringLiteral("/Count")), QVariant(-3));
    QCOMPARE(result.value(QStringLiteral("/Ratio")), QVariant(0.5));
    QCOMPARE(result.value(QStringLiteral("/Name")), QVariant(QStringLiteral("/Display")));
    QCOMPARE(result.value(QStringLiteral("/Hex")), QVariant(QStringLiteral("<0a0B>")));
    QCOMPARE(result.value(QStringLiteral("/Items")).toList(), QVariantList({1, false, QVariant()}));
    QCOMPARE(result.value(QStringLiteral("/Child")).toHash(),
             QVariantHash({{QStringLiteral("/Label"), QStringLiteral("nested")}}));
}

void KisCosParserContractTest::decodesUtf16AndEscapedStrings()
{
    QByteArray source("<< /Utf16 (");
    source.append(QByteArray::fromHex("feff00410042"));
    source.append(") /Escaped (line\\nparen\\(ok\\)) >>");

    KisCosParser parser;
    const QVariantHash result = parser.parseCosToJson(&source);

    QCOMPARE(result.value(QStringLiteral("/Utf16")), QVariant(QStringLiteral("AB")));
    QCOMPARE(result.value(QStringLiteral("/Escaped")), QVariant(QStringLiteral("line\nparen(ok)")));
}

QTEST_GUILESS_MAIN(KisCosParserContractTest)

#include "KisCosParserContractTest.moc"
