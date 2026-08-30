/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <text/KoCssTextUtils.h>

class KoCssTextUtilsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void classifiesCssWordSeparatorsAndHangingPunctuation();
    void appliesWhitespaceCollapseAndHangingRules();
    void transformsAsciiCaseAndReportsStablePositions();
    void mapsFullWidthAndFullSizeKanaCharacters();
    void separatesGraphemesAndMarksJustificationOpportunities();
    void mapsUnicodeBidiControlsForEveryMode();
    void expandsRemovalToWholeUnicodeSequences();
    void selectsFontStyleValuesUsingCssPreferenceOrder();
};

void KoCssTextUtilsContractTest::classifiesCssWordSeparatorsAndHangingPunctuation()
{
    QVERIFY(KoCssTextUtils::IsCssWordSeparator(QStringLiteral(" ")));
    QVERIFY(KoCssTextUtils::IsCssWordSeparator(QString(QChar(0x00a0))));
    QVERIFY(KoCssTextUtils::IsCssWordSeparator(QString(QChar(0x1361))));
    QVERIFY(!KoCssTextUtils::IsCssWordSeparator(QStringLiteral("A")));
    QVERIFY(!KoCssTextUtils::IsCssWordSeparator(QStringLiteral("")));

    QVERIFY(KoCssTextUtils::characterCanHang(QChar('('), KoSvgText::HangFirst));
    QVERIFY(KoCssTextUtils::characterCanHang(QChar(')'), KoSvgText::HangLast));
    QVERIFY(KoCssTextUtils::characterCanHang(QChar(','), KoSvgText::HangEnd));
    QVERIFY(!KoCssTextUtils::characterCanHang(QChar(','), KoSvgText::HangFirst));
    QVERIFY(
        !KoCssTextUtils::characterCanHang(QChar('A'), KoSvgText::HangFirst | KoSvgText::HangLast | KoSvgText::HangEnd));
}

void KoCssTextUtilsContractTest::appliesWhitespaceCollapseAndHangingRules()
{
    QString text = QStringLiteral(" a\t b ");
    const QMap<int, KoSvgText::TextSpaceCollapse> methods{{0, KoSvgText::Collapse}};
    const QVector<bool> collapsed = KoCssTextUtils::collapseSpaces(&text, methods);

    QCOMPARE(text, QStringLiteral(" a  b "));
    QCOMPARE(collapsed, QVector<bool>({true, false, false, true, false, true}));

    QVERIFY(KoCssTextUtils::collapseLastSpace(QChar::LineFeed, KoSvgText::Preserve));
    QVERIFY(KoCssTextUtils::collapseLastSpace(QChar::Space, KoSvgText::Collapse));
    QVERIFY(!KoCssTextUtils::collapseLastSpace(QChar::Space, KoSvgText::Preserve));
    QVERIFY(!KoCssTextUtils::collapseLastSpace(QChar('x'), KoSvgText::Collapse));

    bool force = false;
    QVERIFY(KoCssTextUtils::hangLastSpace(QChar::Space, KoSvgText::Collapse, KoSvgText::NoWrap, force, false));
    QVERIFY(force);

    force = true;
    QVERIFY(KoCssTextUtils::hangLastSpace(QChar::Space, KoSvgText::Preserve, KoSvgText::Wrap, force, true));
    QVERIFY(!force);

    force = true;
    QVERIFY(!KoCssTextUtils::hangLastSpace(QChar::Space, KoSvgText::Preserve, KoSvgText::NoWrap, force, false));
    QVERIFY(force);
}

void KoCssTextUtilsContractTest::transformsAsciiCaseAndReportsStablePositions()
{
    const QString lower = QStringLiteral("alpha beta");
    const QString upper = QStringLiteral("ALPHA BETA");
    const QString capitalized = QStringLiteral("Alpha Beta");
    QVector<QPair<int, int>> positions;

    QCOMPARE(KoCssTextUtils::transformTextToUpperCase(lower, QStringLiteral("en"), positions), upper);
    QCOMPARE(positions.size(), lower.size());
    for (int i = 0; i < positions.size(); ++i) {
        QCOMPARE(positions.at(i), qMakePair(i, i));
    }

    positions.clear();
    QCOMPARE(KoCssTextUtils::transformTextToLowerCase(upper, QStringLiteral("en"), positions), lower);
    QCOMPARE(positions.size(), upper.size());
    for (int i = 0; i < positions.size(); ++i) {
        QCOMPARE(positions.at(i), qMakePair(i, i));
    }

    positions.clear();
    QCOMPARE(KoCssTextUtils::transformTextCapitalize(lower, QStringLiteral("en"), positions), capitalized);
    QCOMPARE(positions.size(), lower.size());
    for (int i = 0; i < positions.size(); ++i) {
        QCOMPARE(positions.at(i), qMakePair(i, i));
    }
}

void KoCssTextUtilsContractTest::mapsFullWidthAndFullSizeKanaCharacters()
{
    QCOMPARE(KoCssTextUtils::transformTextFullWidth(QStringLiteral("A 1%")),
             QStringLiteral("\uff21\u3000\uff11\uff05"));
    QCOMPARE(KoCssTextUtils::transformTextFullSizeKana(QStringLiteral("\u3041\u30c3\uff6eA")),
             QStringLiteral("\u3042\u30c4\uff96A"));
}

void KoCssTextUtilsContractTest::separatesGraphemesAndMarksJustificationOpportunities()
{
    QCOMPARE(KoCssTextUtils::textToUnicodeGraphemeClusters(QStringLiteral("A\u030a B"), QStringLiteral("en")),
             QStringList({QStringLiteral("A\u030a"), QStringLiteral(" "), QStringLiteral("B")}));

    const QVector<QPair<bool, bool>> expected{
        qMakePair(false, false),
        qMakePair(true, true),
        qMakePair(false, false),
        qMakePair(true, true),
    };
    QCOMPARE(KoCssTextUtils::justificationOpportunities(QStringLiteral("A B\u4e2d"), QStringLiteral("en")), expected);
}

void KoCssTextUtilsContractTest::mapsUnicodeBidiControlsForEveryMode()
{
    struct BidiControls {
        KoSvgText::UnicodeBidi mode;
        QString leftToRightOpening;
        QString rightToLeftOpening;
        QString closing;
    };

    const QVector<BidiControls> controls{
        {KoSvgText::BidiNormal, QString(), QString(), QString()},
        {KoSvgText::BidiEmbed, QString(QChar(0x202a)), QString(QChar(0x202b)), QString(QChar(0x202c))},
        {KoSvgText::BidiOverride, QString(QChar(0x202d)), QString(QChar(0x202e)), QString(QChar(0x202c))},
        {KoSvgText::BidiIsolate, QString(QChar(0x2066)), QString(QChar(0x2067)), QString(QChar(0x2069))},
        {KoSvgText::BidiIsolateOverride,
         QString(QChar(0x2068)) + QString(QChar(0x202d)),
         QString(QChar(0x2068)) + QString(QChar(0x202e)),
         QString(QChar(0x202c)) + QString(QChar(0x2069))},
        {KoSvgText::BidiPlainText, QString(QChar(0x2068)), QString(QChar(0x2068)), QString(QChar(0x2069))},
    };

    for (const BidiControls &control : controls) {
        QCOMPARE(KoCssTextUtils::getBidiOpening(true, control.mode), control.leftToRightOpening);
        QCOMPARE(KoCssTextUtils::getBidiOpening(false, control.mode), control.rightToLeftOpening);
        QCOMPARE(KoCssTextUtils::getBidiClosing(control.mode), control.closing);
    }
}

void KoCssTextUtilsContractTest::expandsRemovalToWholeUnicodeSequences()
{
    QString ascii = QStringLiteral("ABCD");
    int start = 1;
    KoCssTextUtils::removeText(ascii, start, 2);
    QCOMPARE(ascii, QStringLiteral("AD"));
    QCOMPARE(start, 1);

    QString supplementary = QStringLiteral("A\U0001f600B");
    start = 2;
    KoCssTextUtils::removeText(supplementary, start, 1);
    QCOMPARE(supplementary, QStringLiteral("AB"));
    QCOMPARE(start, 1);

    QString variation = QStringLiteral("A\ufe0fB");
    start = 1;
    KoCssTextUtils::removeText(variation, start, 1);
    QCOMPARE(variation, QStringLiteral("B"));
    QCOMPARE(start, 0);
}

void KoCssTextUtilsContractTest::selectsFontStyleValuesUsingCssPreferenceOrder()
{
    QCOMPARE(KoCssTextUtils::cssSelectFontStyleValue({}, 82.0, 100.0, 100.0, false), 82.0);

    const QVector<qreal> values{120.0, 50.0, 100.0, 90.0};
    QCOMPARE(KoCssTextUtils::cssSelectFontStyleValue(values, 80.0, 100.0, 100.0, false), 50.0);
    QCOMPARE(KoCssTextUtils::cssSelectFontStyleValue(values, 110.0, 100.0, 100.0, false), 120.0);
    QCOMPARE(KoCssTextUtils::cssSelectFontStyleValue(values, 95.0, 90.0, 100.0, false), 100.0);

    const QVector<qreal> defaultFallback{80.0, 90.0, 110.0};
    QCOMPARE(KoCssTextUtils::cssSelectFontStyleValue(defaultFallback, 95.0, 90.0, 100.0, false), 90.0);
    QCOMPARE(KoCssTextUtils::cssSelectFontStyleValue(defaultFallback, 95.0, 90.0, 100.0, true), 95.0);
}

QTEST_GUILESS_MAIN(KoCssTextUtilsContractTest)

#include "KoCssTextUtilsContractTest.moc"
