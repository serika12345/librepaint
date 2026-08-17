/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>
#include <QWidget>

#include <KisSynchronousImportUserFeedback.h>

class TestImportExportUiBoundary : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void interactiveDecisionIsReported();
    void batchModeSuppressesInteraction();
};

void TestImportExportUiBoundary::interactiveDecisionIsReported()
{
    QWidget parent;
    KisSynchronousImportUserFeedback feedback(&parent, false);

    QCOMPARE(feedback.askUser([](QWidget *dialogParent) {
                 return dialogParent != nullptr;
             }),
             KisImportUserFeedbackInterface::Success);
    QCOMPARE(feedback.askUser([](QWidget *) {
                 return false;
             }),
             KisImportUserFeedbackInterface::UserCancelled);
}

void TestImportExportUiBoundary::batchModeSuppressesInteraction()
{
    QWidget parent;
    KisSynchronousImportUserFeedback feedback(&parent, true);
    bool callbackInvoked = false;

    QCOMPARE(feedback.askUser([&callbackInvoked](QWidget *) {
                 callbackInvoked = true;
                 return true;
             }),
             KisImportUserFeedbackInterface::SuppressedByBatchMode);
    QVERIFY(!callbackInvoked);
}

QTEST_MAIN(TestImportExportUiBoundary)

#include "TestImportExportUiBoundary.moc"
