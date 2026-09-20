/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisClickableLabel.h"
#include "kis_icon_utils.h"

#include <QColor>
#include <QIcon>
#include <QLabel>
#include <QPixmap>
#include <QPointer>
#include <QPushButton>
#include <QSignalSpy>
#include <QStringList>
#include <QTest>
#include <QWidget>

namespace
{

QStringList requestedIconNames;

QSize displayedPixmapSize(const QLabel &label)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    return label.pixmap() ? label.pixmap()->size() : QSize();
#else
    return label.pixmap().size();
#endif
}

} // namespace

namespace KisIconUtils
{

QIcon loadIcon(const QString &name)
{
    requestedIconNames.append(name);

    QPixmap pixmap(2, 2);
    pixmap.fill(QColor(37, 89, 149));
    return QIcon(pixmap);
}

} // namespace KisIconUtils

class KisClickableLabelContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void ownsParentAndStartsDismissable();
    void dismissableStateControlsCloseButtonAndSignal();
    void everyMouseButtonUsesTheCurrentClickContract();
    void pixmapScalingUsesTheStoredUnscaledSource();
    void emptyPixmapKeepsTheCurrentSizingContract();
};

void KisClickableLabelContractTest::ownsParentAndStartsDismissable()
{
    requestedIconNames.clear();
    auto *parent = new QWidget;
    QPointer<KisClickableLabel> label = new KisClickableLabel(parent);
    QPointer<QPushButton> closeButton = label->findChild<QPushButton *>();

    QCOMPARE(label->parentWidget(), parent);
    QVERIFY(label->isDismissable());
    QVERIFY(closeButton);
    QCOMPARE(closeButton->parentWidget(), label.data());
    QCOMPARE(closeButton->geometry(), QRect(0, 0, 16, 16));
    QVERIFY(closeButton->isFlat());
    QVERIFY(!closeButton->isHidden());
    QVERIFY(!closeButton->icon().isNull());
    QCOMPARE(requestedIconNames, QStringList{QStringLiteral("dark_close-tab")});

    delete parent;
    QVERIFY(label.isNull());
    QVERIFY(closeButton.isNull());

    QPointer<KisClickableLabel> polymorphicLabel = new KisClickableLabel;
    QLabel *base = polymorphicLabel.data();
    delete base;
    QVERIFY(polymorphicLabel.isNull());
}

void KisClickableLabelContractTest::dismissableStateControlsCloseButtonAndSignal()
{
    KisClickableLabel label;
    QPushButton *closeButton = label.findChild<QPushButton *>();
    QSignalSpy dismissedSpy(&label, &KisClickableLabel::dismissed);

    QVERIFY(closeButton);
    label.setDismissable(false);
    QVERIFY(!label.isDismissable());
    QVERIFY(closeButton->isHidden());

    label.setDismissable();
    QVERIFY(label.isDismissable());
    QVERIFY(!closeButton->isHidden());

    closeButton->click();
    QCOMPARE(dismissedSpy.count(), 1);
}

void KisClickableLabelContractTest::everyMouseButtonUsesTheCurrentClickContract()
{
    QWidget host;
    host.resize(100, 60);
    KisClickableLabel label(&host);
    label.setGeometry(0, 0, 80, 40);
    QSignalSpy clickedSpy(&label, &KisClickableLabel::clicked);
    host.show();
    QVERIFY(QTest::qWaitForWindowExposed(&host));

    QTest::mouseClick(&label, Qt::LeftButton, Qt::NoModifier, QPoint(60, 20));
    QCOMPARE(clickedSpy.count(), 1);

    QTest::mouseClick(&label, Qt::RightButton, Qt::NoModifier, QPoint(60, 20));
    QCOMPARE(clickedSpy.count(), 2);
}

void KisClickableLabelContractTest::pixmapScalingUsesTheStoredUnscaledSource()
{
    QWidget host;
    host.resize(100, 60);
    KisClickableLabel label(&host);
    label.setGeometry(0, 0, 20, 10);

    QPixmap source(40, 20);
    source.fill(QColor(131, 73, 41));
    label.setUnscaledPixmap(source);

    QCOMPARE(label.maximumSize(), QSize(40, 20));
    QCOMPARE(displayedPixmapSize(label), QSize(20, 10));
    QVERIFY(label.hasHeightForWidth());
    QCOMPARE(label.heightForWidth(15), 8);

    QPixmap replacement(1, 1);
    replacement.fill(Qt::black);
    label.setPixmap(replacement);
    QCOMPARE(displayedPixmapSize(label), QSize(1, 1));
    label.updatePixmap();
    QCOMPARE(displayedPixmapSize(label), QSize(20, 10));

    host.show();
    QVERIFY(QTest::qWaitForWindowExposed(&host));
    label.resize(10, 5);
    QTRY_COMPARE(displayedPixmapSize(label), QSize(10, 5));
}

void KisClickableLabelContractTest::emptyPixmapKeepsTheCurrentSizingContract()
{
    KisClickableLabel label;
    label.resize(31, 17);

    QVERIFY(label.hasHeightForWidth());
    QCOMPARE(label.heightForWidth(100), 17);
    QCOMPARE(label.minimumSizeHint(), QSize());
    QCOMPARE(label.sizeHint(), QSize());

    label.updatePixmap();
    QVERIFY(displayedPixmapSize(label).isEmpty());
}

QTEST_MAIN(KisClickableLabelContractTest)

#include "KisClickableLabelContractTest.moc"
