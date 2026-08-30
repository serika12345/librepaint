/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_double_parse_spin_box.h"

#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QLocale>
#include <QPointer>
#include <QSignalSpy>
#include <QTest>
#include <QWidget>

namespace
{

constexpr double comparisonTolerance = 0.0001;

class DoubleParseSpinBoxProbe : public KisDoubleParseSpinBox
{
public:
    explicit DoubleParseSpinBoxProbe(QWidget *parent = nullptr)
        : KisDoubleParseSpinBox(parent)
    {
        setKeyboardTracking(false);
        setLocale(QLocale::c());
        setDecimals(4);
        setRange(-1000.0, 1000.0);
    }

    void interpretExpression(const QString &expression)
    {
        lineEdit()->setText(expression);
        interpretText();
    }
};

void compareWithinTolerance(double actual, double expected)
{
    QVERIFY2(
        qAbs(actual - expected) <= comparisonTolerance,
        qPrintable(
            QStringLiteral("actual %1 differs from expected %2").arg(actual, 0, 'g', 16).arg(expected, 0, 'g', 16)));
}

} // namespace

class KisDoubleParseSpinBoxContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void ownsParentStartsValidAndHasVirtualLifetime();
    void evaluatesFiniteExpressionsAndPreservesCleanText();
    void invalidExpressionPreservesValueAndReportsInput();
    void explicitOverwriteRecoversFromParsingError();
    void stepByUsesTheConfiguredStepAndNormalizesText();
};

void KisDoubleParseSpinBoxContractTest::ownsParentStartsValidAndHasVirtualLifetime()
{
    auto *parent = new QWidget;
    QPointer<KisDoubleParseSpinBox> spinBox = new KisDoubleParseSpinBox(parent);

    QCOMPARE(spinBox->parentWidget(), parent);
    QVERIFY(spinBox->isLastValid());
    compareWithinTolerance(spinBox->value(), 0.0);

    delete parent;
    QVERIFY(spinBox.isNull());

    QPointer<KisDoubleParseSpinBox> polymorphicSpinBox = new KisDoubleParseSpinBox;
    QDoubleSpinBox *base = polymorphicSpinBox.data();
    delete base;
    QVERIFY(polymorphicSpinBox.isNull());
}

void KisDoubleParseSpinBoxContractTest::evaluatesFiniteExpressionsAndPreservesCleanText()
{
    DoubleParseSpinBoxProbe spinBox;

    spinBox.interpretExpression(QStringLiteral("1.5*4"));

    compareWithinTolerance(spinBox.value(), 6.0);
    QCOMPARE(spinBox.veryCleanText(), QStringLiteral("1.5*4"));
    QVERIFY(spinBox.isLastValid());
}

void KisDoubleParseSpinBoxContractTest::invalidExpressionPreservesValueAndReportsInput()
{
    DoubleParseSpinBoxProbe spinBox;
    spinBox.setValue(3.125, true);
    QSignalSpy errorSpy(&spinBox, &KisDoubleParseSpinBox::errorWhileParsing);

    spinBox.interpretExpression(QStringLiteral("3.125+"));

    compareWithinTolerance(spinBox.value(), 3.125);
    QVERIFY(!spinBox.isLastValid());
    QCOMPARE(spinBox.veryCleanText(), QStringLiteral("3.125+"));
    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(errorSpy.at(0).at(0).toString(), QStringLiteral("3.125+"));
}

void KisDoubleParseSpinBoxContractTest::explicitOverwriteRecoversFromParsingError()
{
    DoubleParseSpinBoxProbe spinBox;
    spinBox.setValue(3.125, true);
    spinBox.interpretExpression(QStringLiteral("3.125+"));
    QSignalSpy recoverySpy(&spinBox, &KisDoubleParseSpinBox::noMoreParsingError);

    spinBox.setValue(3.125, true);

    QVERIFY(spinBox.isLastValid());
    compareWithinTolerance(spinBox.value(), 3.125);
    QCOMPARE(spinBox.veryCleanText(), QStringLiteral("3.1250"));
    QCOMPARE(recoverySpy.count(), 1);

    spinBox.setValue(3.125, true);
    QCOMPARE(recoverySpy.count(), 1);
}

void KisDoubleParseSpinBoxContractTest::stepByUsesTheConfiguredStepAndNormalizesText()
{
    DoubleParseSpinBoxProbe spinBox;
    spinBox.setSingleStep(0.25);
    spinBox.interpretExpression(QStringLiteral("2.5*2"));

    spinBox.stepBy(3);

    compareWithinTolerance(spinBox.value(), 5.75);
    QCOMPARE(spinBox.veryCleanText(), QStringLiteral("5.7500"));
    QVERIFY(spinBox.isLastValid());

    spinBox.stepBy(0);
    compareWithinTolerance(spinBox.value(), 5.75);
    QCOMPARE(spinBox.veryCleanText(), QStringLiteral("5.7500"));
}

QTEST_MAIN(KisDoubleParseSpinBoxContractTest)

#include "KisDoubleParseSpinBoxContractTest.moc"
