/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <io/kis_document_io_presentation.h>

#include <KisImportExportErrorCode.h>

#include <QApplication>
#include <QSignalSpy>

class KisDocumentIoPresentationTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testSaveNotificationOrder();
    void testCancelledSaveHasNoPresentation();
    void testBatchFailureEmitsStatusWithoutDialog();
    void testBatchModeSuppressesLoadDialogs();
    void testLoadSuccessNotification();
    void testAutoSaveStatusMessages();
    void testAutoSaveFailureUsesStatusFallback();
};

void KisDocumentIoPresentationTest::testSaveNotificationOrder()
{
    KisDocumentIoPresentation presentation;
    QStringList notifications;

    connect(&presentation, &KisDocumentIoPresentation::savingCompleted, this, [&notifications] {
        notifications.append("completed");
    });
    connect(&presentation, &KisDocumentIoPresentation::savingFinished, this, [&notifications](const QString &) {
        notifications.append("savingFinished");
    });
    connect(&presentation, &KisDocumentIoPresentation::statusBarMessage, this, [&notifications](const QString &, int) {
        notifications.append("statusBarMessage");
    });

    presentation.notifySaveSucceeded(QStringLiteral("/tmp/example.kra"));

    QCOMPARE(notifications, QStringList({"completed", "savingFinished", "statusBarMessage"}));
}

void KisDocumentIoPresentationTest::testCancelledSaveHasNoPresentation()
{
    KisDocumentIoPresentation presentation;
    int statusMessageCount = 0;
    connect(&presentation,
            &KisDocumentIoPresentation::statusBarMessage,
            this,
            [&statusMessageCount](const QString &, int) {
                ++statusMessageCount;
            });

    presentation.presentSaveResult(QStringLiteral("/tmp/example.kra"),
                                   KisImportExportErrorCode(ImportExportCodes::Cancelled),
                                   QString(),
                                   QString(),
                                   false);

    QCOMPARE(statusMessageCount, 0);
}

void KisDocumentIoPresentationTest::testBatchFailureEmitsStatusWithoutDialog()
{
    KisDocumentIoPresentation presentation;
    QString statusMessage;
    connect(&presentation,
            &KisDocumentIoPresentation::statusBarMessage,
            this,
            [&statusMessage](const QString &message, int) {
                statusMessage = message;
            });

    presentation.presentSaveResult(QStringLiteral("/tmp/example.kra"),
                                   KisImportExportErrorCode(ImportExportCodes::ErrorWhileWriting),
                                   QStringLiteral("disk full"),
                                   QString(),
                                   true);

    QVERIFY(statusMessage.contains(QStringLiteral("example.kra")));
    QVERIFY(statusMessage.contains(QStringLiteral("disk full")));
}

void KisDocumentIoPresentationTest::testBatchModeSuppressesLoadDialogs()
{
    KisDocumentIoPresentation presentation;

    presentation.presentMissingFile(QStringLiteral("/tmp/missing.kra"), true);
    presentation.presentLoadFailure(QStringLiteral("example.kra"),
                                    KisImportExportErrorCode(ImportExportCodes::ErrorWhileReading),
                                    QStringLiteral("read failed"),
                                    QStringLiteral("partial data"),
                                    true);
    presentation.presentLoadWarning(QStringLiteral("example.kra"), QStringLiteral("partial data"), true);

    QVERIFY(!QApplication::activeModalWidget());
}

void KisDocumentIoPresentationTest::testLoadSuccessNotification()
{
    KisDocumentIoPresentation presentation;
    QSignalSpy loadingSpy(&presentation, &KisDocumentIoPresentation::loadingFinished);

    presentation.notifyLoadSucceeded();

    QCOMPARE(loadingSpy.count(), 1);
}

void KisDocumentIoPresentationTest::testAutoSaveStatusMessages()
{
    KisDocumentIoPresentation presentation;
    QStringList statusMessages;
    connect(&presentation,
            &KisDocumentIoPresentation::statusBarMessage,
            this,
            [&statusMessages](const QString &message, int) {
                statusMessages.append(message);
            });

    presentation.notifyAutoSaveStarted(QStringLiteral("/tmp/.example.kra-autosave.kra"));
    presentation.notifyAutoSavePostponed();
    presentation.notifyAutoSaveFinished(QStringLiteral(".example.kra-autosave.kra"));
    presentation.notifyAutoSaveFailed(QStringLiteral(".example.kra-autosave.kra"),
                                      KisImportExportErrorCode(ImportExportCodes::ErrorWhileWriting),
                                      QStringLiteral("disk full"));

    QCOMPARE(statusMessages.size(), 4);
    QVERIFY(statusMessages.at(0).contains(QStringLiteral("Autosaving")));
    QVERIFY(statusMessages.at(1).contains(QStringLiteral("postponed")));
    QVERIFY(statusMessages.at(2).contains(QStringLiteral("Finished autosaving")));
    QVERIFY(statusMessages.at(3).contains(QStringLiteral("disk full")));
}

void KisDocumentIoPresentationTest::testAutoSaveFailureUsesStatusFallback()
{
    KisDocumentIoPresentation presentation;
    QSignalSpy statusSpy(&presentation, &KisDocumentIoPresentation::statusBarMessage);

    presentation.notifyAutoSaveFailed(QStringLiteral("example.kra"),
                                      KisImportExportErrorCode(ImportExportCodes::ErrorWhileWriting),
                                      QString());

    QCOMPARE(statusSpy.count(), 1);
    const QList<QVariant> arguments = statusSpy.takeFirst();
    QVERIFY(arguments.at(0).toString().contains(QStringLiteral("example.kra")));
    QVERIFY(arguments.at(0).toString().contains(QStringLiteral("Error occurred")));
}

QTEST_MAIN(KisDocumentIoPresentationTest)

#include "kis_document_io_presentation_test.moc"
