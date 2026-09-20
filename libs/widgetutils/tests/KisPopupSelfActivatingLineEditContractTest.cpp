/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisPopupSelfActivatingLineEdit.h"

#include <QPointer>
#include <QSignalSpy>
#include <QTest>
#include <QWidget>

class KisPopupSelfActivatingLineEditContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void parentOwnsAndDestroysLineEdit();
};

void KisPopupSelfActivatingLineEditContractTest::parentOwnsAndDestroysLineEdit()
{
    auto *parent = new QWidget;
    auto *lineEdit = new KisPopupSelfActivatingLineEdit(parent);
    QPointer<KisPopupSelfActivatingLineEdit> guardedLineEdit(lineEdit);
    QSignalSpy destroyedSpy(lineEdit, &QObject::destroyed);

    QCOMPARE(lineEdit->parentWidget(), parent);

    delete parent;

    QVERIFY(guardedLineEdit.isNull());
    QCOMPARE(destroyedSpy.count(), 1);
}

QTEST_MAIN(KisPopupSelfActivatingLineEditContractTest)

#include "KisPopupSelfActivatingLineEditContractTest.moc"
