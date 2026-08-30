/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoSliderCombo.h>

#include <QComboBox>
#include <QLineEdit>
#include <QLocale>
#include <QPointer>
#include <QSignalSpy>
#include <QSlider>
#include <QTest>
#include <QWidget>

namespace
{
class DefaultLocaleScope
{
public:
    DefaultLocaleScope()
        : m_previousLocale()
    {
        QLocale::setDefault(QLocale::c());
    }

    ~DefaultLocaleScope()
    {
        QLocale::setDefault(m_previousLocale);
    }

private:
    const QLocale m_previousLocale;
};
} // namespace

class KoSliderComboContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void ownsParentAndExposesStableDefaults();
    void settersClampValuesAndEmitFinalNotifications();
    void sliderReportsIntermediateAndFinalValues();
    void ignoresConfiguredDecimalPrecision();
    void acceptsOutOfRangeLineEditValue();
};

void KoSliderComboContractTest::ownsParentAndExposesStableDefaults()
{
    DefaultLocaleScope localeScope;
    auto *parent = new QWidget;
    QPointer<KoSliderCombo> combo = new KoSliderCombo(parent);

    QCOMPARE(combo->parentWidget(), parent);
    QCOMPARE(combo->minimum(), 0.0);
    QCOMPARE(combo->maximum(), 100.0);
    QCOMPARE(combo->decimals(), 2.0);
    QCOMPARE(combo->value(), 0.0);
    QCOMPARE(combo->sizeHint(), combo->minimumSizeHint());
    QVERIFY(combo->minimumSizeHint().width() > 0);
    QVERIFY(combo->minimumSizeHint().height() > 0);

    delete parent;
    QVERIFY(combo.isNull());

    QPointer<KoSliderCombo> polymorphicCombo = new KoSliderCombo(nullptr);
    QComboBox *base = polymorphicCombo.data();
    delete base;
    QVERIFY(polymorphicCombo.isNull());
}

void KoSliderComboContractTest::settersClampValuesAndEmitFinalNotifications()
{
    DefaultLocaleScope localeScope;
    KoSliderCombo combo(nullptr);
    QSignalSpy valueSpy(&combo, &KoSliderCombo::valueChanged);

    combo.setMinimum(-2.0);
    combo.setMaximum(3.0);
    combo.setDecimals(3);

    QCOMPARE(combo.minimum(), -2.0);
    QCOMPARE(combo.maximum(), 3.0);
    QCOMPARE(combo.decimals(), 3.0);

    combo.setValue(1.25);
    QCOMPARE(combo.value(), 1.25);
    QCOMPARE(valueSpy.count(), 1);
    QCOMPARE(valueSpy.at(0).at(0).toDouble(), 1.25);
    QCOMPARE(valueSpy.at(0).at(1).toBool(), true);

    combo.setValue(-20.0);
    QCOMPARE(combo.value(), -2.0);
    QCOMPARE(valueSpy.count(), 2);
    QCOMPARE(valueSpy.at(1).at(0).toDouble(), -2.0);
    QCOMPARE(valueSpy.at(1).at(1).toBool(), true);

    combo.setValue(20.0);
    QCOMPARE(combo.value(), 3.0);
    QCOMPARE(valueSpy.count(), 3);
    QCOMPARE(valueSpy.at(2).at(0).toDouble(), 3.0);
    QCOMPARE(valueSpy.at(2).at(1).toBool(), true);

    combo.setValue(3.0);
    QCOMPARE(valueSpy.count(), 4);
    QCOMPARE(valueSpy.at(3).at(0).toDouble(), 3.0);
    QCOMPARE(valueSpy.at(3).at(1).toBool(), true);
}

void KoSliderComboContractTest::sliderReportsIntermediateAndFinalValues()
{
    DefaultLocaleScope localeScope;
    KoSliderCombo combo(nullptr);
    combo.setMinimum(10.0);
    combo.setMaximum(20.0);
    combo.setDecimals(2);
    combo.setValue(10.0);
    QSignalSpy valueSpy(&combo, &KoSliderCombo::valueChanged);
    QSlider *slider = combo.findChild<QSlider *>();
    QVERIFY(slider);

    slider->setValue(64);

    QCOMPARE(combo.value(), 12.5);
    QCOMPARE(valueSpy.count(), 1);
    QCOMPARE(valueSpy.at(0).at(0).toDouble(), 12.5);
    QCOMPARE(valueSpy.at(0).at(1).toBool(), false);

    QVERIFY(QMetaObject::invokeMethod(slider, "sliderReleased", Qt::DirectConnection));
    QCOMPARE(valueSpy.count(), 2);
    QCOMPARE(valueSpy.at(1).at(0).toDouble(), 12.5);
    QCOMPARE(valueSpy.at(1).at(1).toBool(), true);
}

void KoSliderComboContractTest::ignoresConfiguredDecimalPrecision()
{
    DefaultLocaleScope localeScope;
    KoSliderCombo combo(nullptr);
    combo.setMinimum(0.0);
    combo.setMaximum(10.0);
    combo.setDecimals(1);
    QSignalSpy valueSpy(&combo, &KoSliderCombo::valueChanged);

    combo.setValue(1.26);

    QCOMPARE(combo.currentText(), QStringLiteral("1.260000"));
    QCOMPARE(combo.value(), 1.26);
    QCOMPARE(valueSpy.count(), 1);
    QCOMPARE(valueSpy.at(0).at(0).toDouble(), 1.26);
    QCOMPARE(valueSpy.at(0).at(1).toBool(), true);

    combo.setDecimals(0);
    combo.setValue(1.26);
    QCOMPARE(combo.currentText(), QStringLiteral("1.260000"));
    QCOMPARE(combo.value(), 1.26);
}

void KoSliderComboContractTest::acceptsOutOfRangeLineEditValue()
{
    DefaultLocaleScope localeScope;
    KoSliderCombo combo(nullptr);
    combo.setMinimum(-5.0);
    combo.setMaximum(5.0);
    combo.setDecimals(1);
    QSignalSpy valueSpy(&combo, &KoSliderCombo::valueChanged);

    combo.lineEdit()->setText(QStringLiteral("25.5"));
    QVERIFY(QMetaObject::invokeMethod(combo.lineEdit(), "editingFinished", Qt::DirectConnection));

    QCOMPARE(combo.value(), 25.5);
    QVERIFY(combo.value() > combo.maximum());
    QCOMPARE(valueSpy.count(), 1);
    QCOMPARE(valueSpy.at(0).at(0).toDouble(), 25.5);
    QCOMPARE(valueSpy.at(0).at(1).toBool(), true);
}

QTEST_MAIN(KoSliderComboContractTest)

#include "KoSliderComboContractTest.moc"
