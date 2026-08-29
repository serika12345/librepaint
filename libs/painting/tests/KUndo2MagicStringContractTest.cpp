/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kundo2magicstring.h"

#include <QTest>

class KUndo2MagicStringContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultAndRawStringsExposeStableRepresentations();
    void nonTranslatableFormattingSupportsEveryArgumentCount();
    void translatedFormattingSupportsEveryArgumentCount();
    void contextualFormattingSupportsEveryArgumentCount();
    void pluralFormattingSupportsEveryArgumentCount();
    void contextualPluralFormattingSupportsEveryArgumentCount();
};

void KUndo2MagicStringContractTest::defaultAndRawStringsExposeStableRepresentations()
{
    const KUndo2MagicString empty;
    QVERIFY(empty.isEmpty());
    QVERIFY(empty.toString().isEmpty());
    QVERIFY(empty.toSecondaryString().isEmpty());

    const KUndo2MagicString primaryOnly = kundo2_noi18n(QStringLiteral("Paint Layer"));
    QVERIFY(!primaryOnly.isEmpty());
    QCOMPARE(primaryOnly.toString(), QStringLiteral("Paint Layer"));
    QCOMPARE(primaryOnly.toSecondaryString(), QStringLiteral("Paint Layer"));
    QVERIFY(primaryOnly == kundo2_noi18n(QStringLiteral("Paint Layer")));

    const KUndo2MagicString split = kundo2_noi18n(QStringLiteral("Paint Layer\nLayer"));
    QCOMPARE(split.toString(), QStringLiteral("Paint Layer"));
    QCOMPARE(split.toSecondaryString(), QStringLiteral("Layer"));

    QString debugText;
    {
        QDebug debug(&debugText);
        debug.noquote() << split;
    }
    QCOMPARE(debugText.trimmed(), QStringLiteral("Paint Layer(Layer)"));
}

void KUndo2MagicStringContractTest::nonTranslatableFormattingSupportsEveryArgumentCount()
{
    QCOMPARE(kundo2_noi18n("%1", QStringLiteral("one")).toString(), QStringLiteral("one"));
    QCOMPARE(kundo2_noi18n("%1 %2", QStringLiteral("one"), 2).toString(), QStringLiteral("one 2"));
    QCOMPARE(kundo2_noi18n("%1 %2 %3", QStringLiteral("one"), 2, 3.5).toString(), QStringLiteral("one 2 3.5"));
    QCOMPARE(kundo2_noi18n("%1 %2 %3 %4", QStringLiteral("one"), 2, 3.5, QStringLiteral("four")).toString(),
             QStringLiteral("one 2 3.5 four"));
}

void KUndo2MagicStringContractTest::translatedFormattingSupportsEveryArgumentCount()
{
    QCOMPARE(kundo2_i18n("Undo action").toString(), QStringLiteral("Undo action"));
    QCOMPARE(kundo2_i18n("Undo %1", QStringLiteral("one")).toString(), QStringLiteral("Undo one"));
    QCOMPARE(kundo2_i18n("Undo %1 %2", QStringLiteral("one"), 2).toString(), QStringLiteral("Undo one 2"));
    QCOMPARE(kundo2_i18n("Undo %1 %2 %3", QStringLiteral("one"), 2, 3).toString(), QStringLiteral("Undo one 2 3"));
    QCOMPARE(kundo2_i18n("Undo %1 %2 %3 %4", QStringLiteral("one"), 2, 3, QStringLiteral("four")).toString(),
             QStringLiteral("Undo one 2 3 four"));
}

void KUndo2MagicStringContractTest::contextualFormattingSupportsEveryArgumentCount()
{
    QCOMPARE(prependContext("layer-history"), QStringLiteral("(qtundo-format) layer-history"));
    QCOMPARE(kundo2_i18nc("layer-history", "Rename").toString(), QStringLiteral("Rename"));
    QCOMPARE(kundo2_i18nc("layer-history", "Rename %1", QStringLiteral("one")).toString(),
             QStringLiteral("Rename one"));
    QCOMPARE(kundo2_i18nc("layer-history", "Rename %1 %2", QStringLiteral("one"), 2).toString(),
             QStringLiteral("Rename one 2"));
    QCOMPARE(kundo2_i18nc("layer-history", "Rename %1 %2 %3", QStringLiteral("one"), 2, 3).toString(),
             QStringLiteral("Rename one 2 3"));
    QCOMPARE(kundo2_i18nc("layer-history", "Rename %1 %2 %3 %4", QStringLiteral("one"), 2, 3, QStringLiteral("four"))
                 .toString(),
             QStringLiteral("Rename one 2 3 four"));
}

void KUndo2MagicStringContractTest::pluralFormattingSupportsEveryArgumentCount()
{
    QCOMPARE(kundo2_i18np("%1 file", "%1 files", 1).toString(), QStringLiteral("1 file"));
    QCOMPARE(kundo2_i18np("%1 %2 file", "%1 %2 files", 2, QStringLiteral("saved")).toString(),
             QStringLiteral("2 saved files"));
    QCOMPARE(
        kundo2_i18np("%1 %2 %3 file", "%1 %2 %3 files", 3, QStringLiteral("red"), QStringLiteral("saved")).toString(),
        QStringLiteral("3 red saved files"));
    QCOMPARE(kundo2_i18np("%1 %2 %3 %4 file",
                          "%1 %2 %3 %4 files",
                          4,
                          QStringLiteral("red"),
                          QStringLiteral("layer"),
                          QStringLiteral("saved"))
                 .toString(),
             QStringLiteral("4 red layer saved files"));
}

void KUndo2MagicStringContractTest::contextualPluralFormattingSupportsEveryArgumentCount()
{
    QCOMPARE(kundo2_i18ncp("layer-history", "%1 file", "%1 files", 1).toString(), QStringLiteral("1 file"));
    QCOMPARE(kundo2_i18ncp("layer-history", "%1 %2 file", "%1 %2 files", 2, QStringLiteral("saved")).toString(),
             QStringLiteral("2 saved files"));
    QCOMPARE(kundo2_i18ncp("layer-history",
                           "%1 %2 %3 file",
                           "%1 %2 %3 files",
                           3,
                           QStringLiteral("red"),
                           QStringLiteral("saved"))
                 .toString(),
             QStringLiteral("3 red saved files"));
    QCOMPARE(kundo2_i18ncp("layer-history",
                           "%1 %2 %3 %4 file",
                           "%1 %2 %3 %4 files",
                           4,
                           QStringLiteral("red"),
                           QStringLiteral("layer"),
                           QStringLiteral("saved"))
                 .toString(),
             QStringLiteral("4 red layer saved files"));
}

QTEST_GUILESS_MAIN(KUndo2MagicStringContractTest)

#include "KUndo2MagicStringContractTest.moc"
