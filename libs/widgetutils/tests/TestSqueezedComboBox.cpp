/*
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "TestSqueezedComboBox.h"

#include <KisSqueezedComboBox.h>

#include <QCoreApplication>
#include <QPixmap>
#include <QPointer>
#include <QTest>

void TestSqueezedComboBox::constructorPreservesParentNameAndOwnership()
{
    QPointer<QWidget> parent = new QWidget;
    QPointer<KisSqueezedComboBox> combo = new KisSqueezedComboBox(parent, "presets");

    QCOMPARE(combo->parentWidget(), parent.data());
    QCOMPARE(combo->objectName(), QStringLiteral("presets"));
    QCOMPARE(combo->minimumWidth(), 100);
    QVERIFY(combo->sizeHint().width() > 0);
    QVERIFY(combo->sizeHint().height() > 0);

    delete parent.data();
    QVERIFY(parent.isNull());
    QVERIFY(combo.isNull());
}

void TestSqueezedComboBox::textItemsPreserveOriginalValuesSelectionAndReset()
{
    KisSqueezedComboBox combo;
    combo.resize(1000, 40);

    combo.addSqueezedItem(QStringLiteral("Alpha brush"), 11);
    combo.addSqueezedItem(QStringLiteral("Beta brush"), QStringLiteral("beta-data"));

    QCOMPARE(combo.originalTexts(), QStringList({QStringLiteral("Alpha brush"), QStringLiteral("Beta brush")}));
    QVERIFY(combo.contains(QStringLiteral("Alpha brush")));
    QVERIFY(!combo.contains(QString()));
    QVERIFY(!combo.contains(QStringLiteral("Missing brush")));
    QCOMPARE(combo.findOriginalText(QStringLiteral("Alpha brush")), 0);
    QCOMPARE(combo.findOriginalText(QStringLiteral("Beta brush")), 1);
    QCOMPARE(combo.findOriginalText(QStringLiteral("Missing brush")), -1);
    QCOMPARE(combo.itemData(0), QVariant(11));
    QCOMPARE(combo.itemData(1), QVariant(QStringLiteral("beta-data")));
    QCOMPARE(combo.itemData(0, Qt::ToolTipRole), QVariant(QStringLiteral("Alpha brush")));
    QCOMPARE(combo.currentUnsqueezedText(), QStringLiteral("Alpha brush"));

    combo.setCurrent(QStringLiteral("Beta brush"));
    QCOMPARE(combo.currentIndex(), 1);
    QCOMPARE(combo.currentUnsqueezedText(), QStringLiteral("Beta brush"));
    combo.setCurrent(QStringLiteral("Missing brush"));
    QCOMPARE(combo.currentIndex(), 1);

    const QStringList replacements({QStringLiteral("Ink"), QStringLiteral("Paint"), QStringLiteral("Pencil")});
    combo.resetOriginalTexts(replacements);
    QCOMPARE(combo.originalTexts(), replacements);
    QCOMPARE(combo.count(), replacements.size());
    QVERIFY(!combo.contains(QStringLiteral("Beta brush")));
    combo.resetOriginalTexts(replacements);
    QCOMPARE(combo.originalTexts(), replacements);
}

void TestSqueezedComboBox::iconAndTextInsertionPreserveMetadataAtTheEnd()
{
    QPixmap pixels(4, 4);
    pixels.fill(Qt::red);
    const QIcon icon(pixels);

    KisSqueezedComboBox combo;
    combo.resize(1000, 40);
    combo.addSqueezedItem(icon, QStringLiteral("Icon alpha"), 21);
    combo.insertSqueezedItem(QStringLiteral("Text beta"), combo.count(), 22);
    combo.insertSqueezedItem(icon, QStringLiteral("Icon gamma"), combo.count(), 23);

    QCOMPARE(combo.originalTexts(),
             QStringList({QStringLiteral("Icon alpha"), QStringLiteral("Text beta"), QStringLiteral("Icon gamma")}));
    QCOMPARE(combo.itemData(0), QVariant(21));
    QCOMPARE(combo.itemData(1), QVariant(22));
    QCOMPARE(combo.itemData(2), QVariant(23));
    QVERIFY(!combo.itemIcon(0).isNull());
    QVERIFY(combo.itemIcon(1).isNull());
    QVERIFY(!combo.itemIcon(2).isNull());
    QCOMPARE(combo.itemData(2, Qt::ToolTipRole), QVariant(QStringLiteral("Icon gamma")));
}

void TestSqueezedComboBox::squeezingPreservesTheRightmostVisibleSuffix()
{
    const QString original = QStringLiteral("A very long brush preset name with a meaningful suffix.kpp");

    QWidget wideWidget;
    wideWidget.resize(1000, 40);
    QCOMPARE(KisSqueezedComboBox::squeezeText(original, &wideWidget), original);

    QWidget narrowWidget;
    narrowWidget.resize(100, 40);
    const QString squeezed = KisSqueezedComboBox::squeezeText(original, &narrowWidget);
    QVERIFY(squeezed.startsWith(QStringLiteral("...")));
    QVERIFY(squeezed.size() < original.size());
    QVERIFY(original.endsWith(squeezed.mid(3)));

    KisSqueezedComboBox combo;
    combo.show();
    QVERIFY(QTest::qWaitForWindowExposed(&combo));
    combo.resize(1000, 40);
    QCoreApplication::processEvents();
    combo.addSqueezedItem(original);
    QCOMPARE(combo.itemText(0), original);

    combo.resize(100, 40);
    QTRY_VERIFY_WITH_TIMEOUT(combo.itemText(0).startsWith(QStringLiteral("...")), 1000);
    QVERIFY(original.endsWith(combo.itemText(0).mid(3)));
    QCOMPARE(combo.itemData(0, Qt::ToolTipRole), QVariant(original));

    combo.resize(1000, 40);
    QTRY_COMPARE_WITH_TIMEOUT(combo.itemText(0), original, 1000);
}

void TestSqueezedComboBox::middleInsertionAndRemovalExposeCurrentIndexMismatch()
{
    KisSqueezedComboBox combo;
    combo.resize(1000, 40);
    combo.addSqueezedItem(QStringLiteral("Alpha"));
    combo.addSqueezedItem(QStringLiteral("Beta"));

    combo.insertSqueezedItem(QStringLiteral("Inserted"), 0);
    QCOMPARE(combo.count(), 3);
    QCOMPARE(combo.itemData(0, Qt::ToolTipRole), QVariant(QStringLiteral("Inserted")));
    QCOMPARE(combo.itemData(1, Qt::ToolTipRole), QVariant(QStringLiteral("Alpha")));
    QCOMPARE(combo.itemData(2, Qt::ToolTipRole), QVariant(QStringLiteral("Beta")));

    QCOMPARE(combo.originalTexts(), QStringList({QStringLiteral("Inserted"), QStringLiteral("Beta")}));
    QVERIFY(!combo.contains(QStringLiteral("Alpha")));
    QCOMPARE(combo.findOriginalText(QStringLiteral("Beta")), 1);
    combo.setCurrent(QStringLiteral("Beta"));
    QCOMPARE(combo.currentIndex(), 1);
    QCOMPARE(combo.currentText(), QStringLiteral("Alpha"));
    QCOMPARE(combo.currentUnsqueezedText(), QStringLiteral("Beta"));

    combo.removeSqueezedItem(0);
    QCOMPARE(combo.count(), 2);
    QCOMPARE(combo.originalTexts(), QStringList({QStringLiteral("Beta")}));
    QCOMPARE(combo.findOriginalText(QStringLiteral("Beta")), -1);
}

QTEST_MAIN(TestSqueezedComboBox)
