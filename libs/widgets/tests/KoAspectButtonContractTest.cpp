/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoAspectButton.h>

#include <QColor>
#include <QIcon>
#include <QImage>
#include <QPixmap>
#include <QPointer>
#include <QSignalSpy>
#include <QStringList>
#include <QTest>
#include <QToolButton>
#include <QWidget>

namespace
{

QStringList requestedIconNames;

QColor displayedIconColor(const KoAspectButton &button)
{
    return button.icon().pixmap(QSize(2, 2)).toImage().pixelColor(0, 0);
}

} // namespace

namespace KisIconUtils
{

QIcon loadIcon(const QString &name)
{
    requestedIconNames.append(name);

    QPixmap pixmap(2, 2);
    pixmap.fill(name == QStringLiteral("chain-icon") ? QColor(20, 120, 220) : QColor(220, 80, 20));
    return QIcon(pixmap);
}

} // namespace KisIconUtils

class KoAspectButtonContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void ownsParentStartsUnlockedAndHasVirtualLifetime();
    void setterGetterAndSignalTrackMeaningfulChanges();
    void explicitAndQtReleaseToggleOnlyWhenEnabled();
};

void KoAspectButtonContractTest::ownsParentStartsUnlockedAndHasVirtualLifetime()
{
    requestedIconNames.clear();
    auto *parent = new QWidget;
    QPointer<KoAspectButton> button = new KoAspectButton(parent);

    QCOMPARE(button->parentWidget(), parent);
    QVERIFY(!button->keepAspectRatio());
    QCOMPARE(button->size(), QSize(19, 34));
    QCOMPARE(button->minimumSize(), QSize(19, 34));
    QCOMPARE(button->maximumSize(), QSize(19, 34));
    QCOMPARE(button->iconSize(), QSize(9, 24));
    QVERIFY(button->autoRaise());
    QCOMPARE(requestedIconNames, QStringList{QStringLiteral("chain-broken-icon")});
    QCOMPARE(displayedIconColor(*button), QColor(220, 80, 20));

    delete parent;
    QVERIFY(button.isNull());

    QPointer<KoAspectButton> polymorphicButton = new KoAspectButton(nullptr);
    QToolButton *base = polymorphicButton.data();
    delete base;
    QVERIFY(polymorphicButton.isNull());
}

void KoAspectButtonContractTest::setterGetterAndSignalTrackMeaningfulChanges()
{
    KoAspectButton button(nullptr);
    requestedIconNames.clear();
    QSignalSpy keepSpy(&button, &KoAspectButton::keepAspectRatioChanged);

    button.setKeepAspectRatio(true);

    QVERIFY(button.keepAspectRatio());
    QCOMPARE(keepSpy.count(), 1);
    QCOMPARE(keepSpy.at(0).at(0).toBool(), true);
    QCOMPARE(requestedIconNames.constLast(), QStringLiteral("chain-icon"));
    QCOMPARE(displayedIconColor(button), QColor(20, 120, 220));

    button.setKeepAspectRatio(true);
    QCOMPARE(keepSpy.count(), 1);

    button.setKeepAspectRatio(false);
    QVERIFY(!button.keepAspectRatio());
    QCOMPARE(keepSpy.count(), 2);
    QCOMPARE(keepSpy.at(1).at(0).toBool(), false);
    QCOMPARE(requestedIconNames.constLast(), QStringLiteral("chain-broken-icon"));
    QCOMPARE(displayedIconColor(button), QColor(220, 80, 20));
}

void KoAspectButtonContractTest::explicitAndQtReleaseToggleOnlyWhenEnabled()
{
    KoAspectButton button(nullptr);
    QSignalSpy keepSpy(&button, &KoAspectButton::keepAspectRatioChanged);

    button.buttonReleased();
    QVERIFY(button.keepAspectRatio());
    QCOMPARE(keepSpy.count(), 1);
    QCOMPARE(keepSpy.at(0).at(0).toBool(), true);

    button.click();
    QVERIFY(!button.keepAspectRatio());
    QCOMPARE(keepSpy.count(), 2);
    QCOMPARE(keepSpy.at(1).at(0).toBool(), false);

    button.setEnabled(false);
    button.buttonReleased();
    button.click();
    QVERIFY(!button.keepAspectRatio());
    QCOMPARE(keepSpy.count(), 2);
}

QTEST_MAIN(KoAspectButtonContractTest)

#include "KoAspectButtonContractTest.moc"
