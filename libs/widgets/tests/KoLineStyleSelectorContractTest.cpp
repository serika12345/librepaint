/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoLineStyleSelector.h>

#include <QComboBox>
#include <QPen>
#include <QPointer>
#include <QTest>

class KoLineStyleSelectorContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void followsParentOwnershipAndHasVirtualLifetime();
    void selectsStandardStyles();
    void registersCustomStylesOnlyOnce();
    void temporarilySelectsUnknownCustomStyles();
};

void KoLineStyleSelectorContractTest::followsParentOwnershipAndHasVirtualLifetime()
{
    auto *parent = new QWidget;
    QPointer<KoLineStyleSelector> selector = new KoLineStyleSelector(parent);

    QCOMPARE(selector->parentWidget(), parent);
    delete parent;
    QVERIFY(selector.isNull());

    QPointer<KoLineStyleSelector> polymorphicSelector = new KoLineStyleSelector;
    QComboBox *base = polymorphicSelector.data();
    delete base;
    QVERIFY(polymorphicSelector.isNull());
}

void KoLineStyleSelectorContractTest::selectsStandardStyles()
{
    KoLineStyleSelector selector;

    QCOMPARE(selector.count(), static_cast<int>(Qt::CustomDashLine));
    for (int value = Qt::NoPen; value < Qt::CustomDashLine; ++value) {
        const auto style = static_cast<Qt::PenStyle>(value);
        selector.setLineStyle(style);

        QCOMPARE(selector.lineStyle(), style);
        QCOMPARE(selector.lineDashes(), QPen(style).dashPattern());
    }
}

void KoLineStyleSelectorContractTest::registersCustomStylesOnlyOnce()
{
    KoLineStyleSelector selector;
    const QVector<qreal> customStyle{1.0, 2.0, 3.0, 4.0};
    const int initialCount = selector.count();

    QVERIFY(selector.addCustomStyle(customStyle));
    QCOMPARE(selector.count(), initialCount + 1);
    QVERIFY(!selector.addCustomStyle(customStyle));
    QCOMPARE(selector.count(), initialCount + 1);

    const QVector<qreal> standardStyle = QPen(Qt::DashLine).dashPattern();
    QVERIFY(!selector.addCustomStyle(standardStyle));

    selector.setLineStyle(Qt::CustomDashLine, customStyle);
    QCOMPARE(selector.lineStyle(), Qt::CustomDashLine);
    QCOMPARE(selector.lineDashes(), customStyle);
    QCOMPARE(selector.currentIndex(), initialCount);
}

void KoLineStyleSelectorContractTest::temporarilySelectsUnknownCustomStyles()
{
    KoLineStyleSelector selector;
    const QVector<qreal> temporaryStyle{5.0, 1.0, 2.0, 1.0};
    const int initialCount = selector.count();

    selector.setLineStyle(Qt::CustomDashLine, temporaryStyle);
    QCOMPARE(selector.count(), initialCount + 1);
    QCOMPARE(selector.currentIndex(), initialCount);
    QCOMPARE(selector.lineStyle(), Qt::CustomDashLine);
    QCOMPARE(selector.lineDashes(), temporaryStyle);

    selector.setLineStyle(Qt::DotLine);
    QCOMPARE(selector.count(), initialCount);
    QCOMPARE(selector.lineStyle(), Qt::DotLine);

    selector.setLineStyle(static_cast<Qt::PenStyle>(Qt::CustomDashLine + 1));
    QCOMPARE(selector.lineStyle(), Qt::DotLine);
}

QTEST_MAIN(KoLineStyleSelectorContractTest)

#include "KoLineStyleSelectorContractTest.moc"
