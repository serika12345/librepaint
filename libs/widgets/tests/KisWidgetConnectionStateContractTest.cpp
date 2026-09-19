/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisWidgetConnectionUtils.h"

#include <QCheckBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QTest>
#include <QWidget>

class WidgetModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool checked READ checked WRITE setChecked NOTIFY checkedChanged)
    Q_PROPERTY(int amount READ amount WRITE setAmount NOTIFY amountChanged)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(IntSpinBoxState amountState READ amountState NOTIFY amountStateChanged)
    Q_PROPERTY(bool controlEnabled READ controlEnabled WRITE setControlEnabled NOTIFY controlEnabledChanged)
    Q_PROPERTY(bool controlVisible READ controlVisible WRITE setControlVisible NOTIFY controlVisibleChanged)

public:
    bool checked() const { return m_checked; }
    int amount() const { return m_amount; }
    QString text() const { return m_text; }
    IntSpinBoxState amountState() const { return m_amountState; }
    bool controlEnabled() const { return m_controlEnabled; }
    bool controlVisible() const { return m_controlVisible; }

    void setChecked(bool value)
    {
        if (m_checked == value) return;
        m_checked = value;
        Q_EMIT checkedChanged(value);
    }

    void setAmount(int value)
    {
        if (m_amount == value) return;
        m_amount = value;
        Q_EMIT amountChanged(value);
    }

    void setText(const QString &value)
    {
        if (m_text == value) return;
        m_text = value;
        Q_EMIT textChanged(value);
    }

    void setAmountState(const IntSpinBoxState &state)
    {
        m_amountState = state;
        Q_EMIT amountStateChanged(state);
    }

    void setControlEnabled(bool value)
    {
        if (m_controlEnabled == value) return;
        m_controlEnabled = value;
        Q_EMIT controlEnabledChanged(value);
    }

    void setControlVisible(bool value)
    {
        if (m_controlVisible == value) return;
        m_controlVisible = value;
        Q_EMIT controlVisibleChanged(value);
    }

Q_SIGNALS:
    void checkedChanged(bool value);
    void amountChanged(int value);
    void textChanged(const QString &value);
    void amountStateChanged(IntSpinBoxState state);
    void controlEnabledChanged(bool value);
    void controlVisibleChanged(bool value);

private:
    bool m_checked = true;
    int m_amount = 7;
    QString m_text = QStringLiteral("initial");
    IntSpinBoxState m_amountState {7, -4, 19, false};
    bool m_controlEnabled = false;
    bool m_controlVisible = false;
};

class KisWidgetConnectionStateContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void valuesSynchronizeBetweenModelAndControls();
    void stateConnectionAppliesControlStateAndWritesValue();
    void enabledAndVisiblePropertiesFollowModel();
};

void KisWidgetConnectionStateContractTest::valuesSynchronizeBetweenModelAndControls()
{
    WidgetModel model;
    QCheckBox checkBox;
    QSpinBox spinBox;
    QLineEdit lineEdit;
    spinBox.setRange(-100, 100);

    KisWidgetConnectionUtils::connectControl(&checkBox, &model, "checked");
    KisWidgetConnectionUtils::connectControl(&spinBox, &model, "amount");
    KisWidgetConnectionUtils::connectControl(&lineEdit, &model, "text");

    QVERIFY(checkBox.isChecked());
    QCOMPARE(spinBox.value(), 7);
    QCOMPARE(lineEdit.text(), QStringLiteral("initial"));

    checkBox.setChecked(false);
    spinBox.setValue(-12);
    lineEdit.setText(QStringLiteral("利用者の入力"));
    QCOMPARE(model.checked(), false);
    QCOMPARE(model.amount(), -12);
    QCOMPARE(model.text(), QStringLiteral("利用者の入力"));

    model.setChecked(true);
    model.setAmount(42);
    model.setText(QStringLiteral("model update"));
    QVERIFY(checkBox.isChecked());
    QCOMPARE(spinBox.value(), 42);
    QCOMPARE(lineEdit.text(), QStringLiteral("model update"));
}

void KisWidgetConnectionStateContractTest::stateConnectionAppliesControlStateAndWritesValue()
{
    WidgetModel model;
    QSpinBox spinBox;

    KisWidgetConnectionUtils::connectControlState(&spinBox, &model, "amountState", "amount");

    QCOMPARE(spinBox.minimum(), -4);
    QCOMPARE(spinBox.maximum(), 19);
    QCOMPARE(spinBox.value(), 7);
    QVERIFY(!spinBox.isEnabled());

    model.setAmountState({12, 10, 15, true});
    QCOMPARE(spinBox.minimum(), 10);
    QCOMPARE(spinBox.maximum(), 15);
    QCOMPARE(spinBox.value(), 12);
    QVERIFY(spinBox.isEnabled());

    spinBox.setValue(14);
    QCOMPARE(model.amount(), 14);
}

void KisWidgetConnectionStateContractTest::enabledAndVisiblePropertiesFollowModel()
{
    WidgetModel model;
    QWidget widget;

    KisWidgetConnectionUtils::connectWidgetEnabledToProperty(&widget, &model, "controlEnabled");
    KisWidgetConnectionUtils::connectWidgetVisibleToProperty(&widget, &model, "controlVisible");

    QVERIFY(!widget.isEnabled());
    QVERIFY(widget.isHidden());

    model.setControlEnabled(true);
    model.setControlVisible(true);
    QVERIFY(widget.isEnabled());
    QVERIFY(!widget.isHidden());

    model.setControlEnabled(false);
    model.setControlVisible(false);
    QVERIFY(!widget.isEnabled());
    QVERIFY(widget.isHidden());
}

QTEST_MAIN(KisWidgetConnectionStateContractTest)

#include "KisWidgetConnectionStateContractTest.moc"
