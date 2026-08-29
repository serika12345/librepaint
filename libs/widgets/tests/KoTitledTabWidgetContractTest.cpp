/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoTitledTabWidget.h>

#include <QLabel>
#include <QTest>
#include <QWidget>

class KoTitledTabWidgetContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void ownsCornerTitleAndTracksCurrentTab();
};

void KoTitledTabWidgetContractTest::ownsCornerTitleAndTracksCurrentTab()
{
    QWidget parent;
    KoTitledTabWidget tabs(&parent);

    QCOMPARE(tabs.parentWidget(), &parent);

    QLabel *title = qobject_cast<QLabel *>(tabs.cornerWidget(Qt::TopRightCorner));
    QVERIFY(title);
    QCOMPARE(title->parentWidget(), &tabs);

    auto *firstPage = new QWidget;
    firstPage->setWindowTitle(QStringLiteral("First page title"));
    tabs.addTab(firstPage, QStringLiteral("First"));

    QCOMPARE(title->text(), firstPage->windowTitle());
    QVERIFY(!title->isHidden());

    auto *untitledPage = new QWidget;
    tabs.addTab(untitledPage, QStringLiteral("Untitled"));
    tabs.setCurrentIndex(1);

    QCOMPARE(title->text(), QString());
    QVERIFY(title->isHidden());

    auto *lastPage = new QWidget;
    lastPage->setWindowTitle(QStringLiteral("Last page title"));
    tabs.addTab(lastPage, QStringLiteral("Last"));
    tabs.setCurrentIndex(2);

    QCOMPARE(title->text(), lastPage->windowTitle());
    QVERIFY(!title->isHidden());
}

QTEST_MAIN(KoTitledTabWidgetContractTest)

#include "KoTitledTabWidgetContractTest.moc"
