/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_int_parse_spin_box.h"

#include <QLineEdit>
#include <QLocale>
#include <QPointer>
#include <QSignalSpy>
#include <QSpinBox>
#include <QTest>
#include <QWidget>

namespace
{

class IntParseSpinBoxProbe : public KisIntParseSpinBox
{
public:
    explicit IntParseSpinBoxProbe(QWidget *parent = nullptr)
        : KisIntParseSpinBox(parent)
    {
        setKeyboardTracking(false);
        setLocale(QLocale::c());
        setRange(-1000, 1000);
    }

    void interpretExpression(const QString &expression)
    {
        lineEdit()->setText(expression);
        interpretText();
    }
};

} // namespace

class KisIntParseSpinBoxContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void ownsParentStartsValidAndHasVirtualLifetime();
    void evaluatesIntegerExpressionsAndPreservesCleanText();
    void invalidExpressionPreservesValueAndReportsInput();
    void explicitOverwriteRecoversFromParsingError();
    void stepByUsesTheConfiguredStepAndNormalizesText();
};

void KisIntParseSpinBoxContractTest::ownsParentStartsValidAndHasVirtualLifetime()
{
    auto *parent = new QWidget;
    QPointer<KisIntParseSpinBox> spinBox = new KisIntParseSpinBox(parent);

    QCOMPARE(spinBox->parentWidget(), parent);
    QVERIFY(spinBox->isLastValid());
    QCOMPARE(spinBox->value(), 0);

    delete parent;
    QVERIFY(spinBox.isNull());

    QPointer<KisIntParseSpinBox> polymorphicSpinBox = new KisIntParseSpinBox;
    QSpinBox *base = polymorphicSpinBox.data();
    delete base;
    QVERIFY(polymorphicSpinBox.isNull());
}

void KisIntParseSpinBoxContractTest::evaluatesIntegerExpressionsAndPreservesCleanText()
{
    IntParseSpinBoxProbe spinBox;

    spinBox.interpretExpression(QStringLiteral("7*6"));

    QCOMPARE(spinBox.value(), 42);
    QCOMPARE(spinBox.veryCleanText(), QStringLiteral("7*6"));
    QVERIFY(spinBox.isLastValid());
}

void KisIntParseSpinBoxContractTest::invalidExpressionPreservesValueAndReportsInput()
{
    IntParseSpinBoxProbe spinBox;
    spinBox.setValue(31, true);
    QSignalSpy errorSpy(&spinBox, &KisIntParseSpinBox::errorWhileParsing);

    spinBox.interpretExpression(QStringLiteral("31+"));

    QCOMPARE(spinBox.value(), 31);
    QVERIFY(!spinBox.isLastValid());
    QCOMPARE(spinBox.veryCleanText(), QStringLiteral("31+"));
    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(errorSpy.at(0).at(0).toString(), QStringLiteral("31+"));
}

void KisIntParseSpinBoxContractTest::explicitOverwriteRecoversFromParsingError()
{
    IntParseSpinBoxProbe spinBox;
    spinBox.setValue(31, true);
    spinBox.interpretExpression(QStringLiteral("31+"));
    QSignalSpy recoverySpy(&spinBox, &KisIntParseSpinBox::noMoreParsingError);

    spinBox.setValue(31, true);

    QVERIFY(spinBox.isLastValid());
    QCOMPARE(spinBox.value(), 31);
    QCOMPARE(spinBox.veryCleanText(), QStringLiteral("31"));
    QCOMPARE(recoverySpy.count(), 1);

    spinBox.setValue(31, true);
    QCOMPARE(recoverySpy.count(), 1);
}

void KisIntParseSpinBoxContractTest::stepByUsesTheConfiguredStepAndNormalizesText()
{
    IntParseSpinBoxProbe spinBox;
    spinBox.setSingleStep(3);
    spinBox.interpretExpression(QStringLiteral("2*5"));

    spinBox.stepBy(2);

    QCOMPARE(spinBox.value(), 16);
    QCOMPARE(spinBox.veryCleanText(), QStringLiteral("16"));
    QVERIFY(spinBox.isLastValid());

    spinBox.stepBy(0);
    QCOMPARE(spinBox.value(), 16);
    QCOMPARE(spinBox.veryCleanText(), QStringLiteral("16"));
}

QTEST_MAIN(KisIntParseSpinBoxContractTest)

#include "KisIntParseSpinBoxContractTest.moc"
