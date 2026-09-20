/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>
#include <QWidget>

#include <KisImportUserFeedbackInterface.h>
#include <KisSynchronousImportUserFeedback.h>

#include <memory>

class KisImportUserFeedbackTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void successfulQuestionUsesParent();
    void rejectedQuestionReportsCancellation();
    void batchAndExpiredParentSuppressQuestions();
};

void KisImportUserFeedbackTest::successfulQuestionUsesParent()
{
    QCOMPARE(int(KisImportUserFeedbackInterface::Success), 0);
    QWidget parent;
    std::unique_ptr<KisImportUserFeedbackInterface> feedback =
        std::make_unique<KisSynchronousImportUserFeedback>(&parent, false);
    QWidget *receivedParent = nullptr;
    int callbackCount = 0;
    KisImportUserFeedbackInterface::AskCallback callback = [&](QWidget *callbackParent) {
        receivedParent = callbackParent;
        ++callbackCount;
        return true;
    };

    QCOMPARE(feedback->askUser(callback), KisImportUserFeedbackInterface::Success);
    QCOMPARE(callbackCount, 1);
    QCOMPARE(receivedParent, &parent);
}

void KisImportUserFeedbackTest::rejectedQuestionReportsCancellation()
{
    QCOMPARE(int(KisImportUserFeedbackInterface::UserCancelled), 1);
    QWidget parent;
    KisSynchronousImportUserFeedback feedback(&parent, false);
    QWidget *receivedParent = nullptr;
    int callbackCount = 0;

    const KisImportUserFeedbackInterface::Result result = feedback.askUser([&](QWidget *callbackParent) {
        receivedParent = callbackParent;
        ++callbackCount;
        return false;
    });

    QCOMPARE(result, KisImportUserFeedbackInterface::UserCancelled);
    QCOMPARE(callbackCount, 1);
    QCOMPARE(receivedParent, &parent);
}

void KisImportUserFeedbackTest::batchAndExpiredParentSuppressQuestions()
{
    QCOMPARE(int(KisImportUserFeedbackInterface::SuppressedByBatchMode), 2);
    int callbackCount = 0;
    auto callback = [&](QWidget *) {
        ++callbackCount;
        return true;
    };

    QWidget parent;
    KisSynchronousImportUserFeedback batchFeedback(&parent, true);
    QCOMPARE(batchFeedback.askUser(callback), KisImportUserFeedbackInterface::SuppressedByBatchMode);

    KisSynchronousImportUserFeedback missingParentFeedback(nullptr, false);
    QCOMPARE(missingParentFeedback.askUser(callback), KisImportUserFeedbackInterface::SuppressedByBatchMode);

    auto *temporaryParent = new QWidget;
    KisSynchronousImportUserFeedback expiredParentFeedback(temporaryParent, false);
    delete temporaryParent;
    QCOMPARE(expiredParentFeedback.askUser(callback), KisImportUserFeedbackInterface::SuppressedByBatchMode);
    QCOMPARE(callbackCount, 0);
}

QTEST_MAIN(KisImportUserFeedbackTest)

#include "kis_import_user_feedback_test.moc"
