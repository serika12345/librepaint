/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <KisImportExportErrorCode.h>

#include <memory>

class KisImportExportErrorCodeTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void resultClassification();
    void simpleErrorMessages();
    void customFailureMessage();
    void fileErrorsPreserveContext();
    void diagnosticFormatting();
};

void KisImportExportErrorCodeTest::resultClassification()
{
    KisImportExportErrorCode unspecified;
    KisImportExportErrorCode success(ImportExportCodes::OK);
    const KisImportExportErrorCode cancellation(ImportExportCodes::Cancelled);
    const KisImportExportErrorCode failure(ImportExportCodes::ErrorWhileWriting);
    const KisImportExportErrorCode internalError(ImportExportCodes::InternalError);

    QVERIFY(!unspecified.isOk());
    QVERIFY(!unspecified.isCancelled());
    QVERIFY(!unspecified.isInternalError());
    QVERIFY(success.isOk());
    QVERIFY(!success.isCancelled());
    QVERIFY(!success.isInternalError());
    QVERIFY(!failure.isOk());
    QVERIFY(!failure.isCancelled());
    QVERIFY(!failure.isInternalError());
    QVERIFY(cancellation.isCancelled());
    QVERIFY(internalError.isInternalError());
    QVERIFY(success == KisImportExportErrorCode(ImportExportCodes::OK));
    QVERIFY(!(success == cancellation));
}

void KisImportExportErrorCodeTest::simpleErrorMessages()
{
    QCOMPARE(KisImportExportErrorCode(ImportExportCodes::InternalError).errorMessage(),
             QStringLiteral("Unexpected error."));
    QCOMPARE(KisImportExportErrorCode(ImportExportCodes::FileNotExist).errorMessage(),
             QStringLiteral("The file doesn't exist."));
    QCOMPARE(KisImportExportErrorCode(ImportExportCodes::NoAccessToRead).errorMessage(),
             QStringLiteral("Permission denied: LibrePaint is not allowed to read the file."));
    QCOMPARE(KisImportExportErrorCode(ImportExportCodes::ErrorWhileReading).errorMessage(),
             QStringLiteral("Error occurred while reading from the file."));
    QCOMPARE(KisImportExportErrorCode(ImportExportCodes::FileFormatIncorrect).errorMessage(),
             QStringLiteral("The file format cannot be parsed."));
    QCOMPARE(KisImportExportErrorCode(ImportExportCodes::FormatFeaturesUnsupported).errorMessage(),
             QStringLiteral("The file format contains unsupported features."));
    QCOMPARE(KisImportExportErrorCode(ImportExportCodes::FormatColorSpaceUnsupported).errorMessage(),
             QStringLiteral("The file format contains unsupported color space."));
    QCOMPARE(KisImportExportErrorCode(ImportExportCodes::CannotCreateFile).errorMessage(),
             QStringLiteral("The file cannot be created."));
    QCOMPARE(KisImportExportErrorCode(ImportExportCodes::NoAccessToWrite).errorMessage(),
             QStringLiteral("Permission denied: LibrePaint is not allowed to write to the file."));
    QCOMPARE(KisImportExportErrorCode(ImportExportCodes::ErrorWhileWriting).errorMessage(),
             QStringLiteral("Error occurred while writing to the file."));
    QCOMPARE(KisImportExportErrorCode(ImportExportCodes::InsufficientMemory).errorMessage(),
             QStringLiteral("There is not enough disk space left to save the file."));
    QCOMPARE(KisImportExportErrorCode(ImportExportCodes::FileFormatNotSupported).errorMessage(),
             QStringLiteral("LibrePaint does not support this file format."));
    QCOMPARE(KisImportExportErrorCode(ImportExportCodes::Cancelled).errorMessage(),
             QStringLiteral("The action was cancelled by the user."));
    QCOMPARE(KisImportExportErrorCode(ImportExportCodes::Failure).errorMessage(), QStringLiteral("Unknown error."));
    QCOMPARE(KisImportExportErrorCode(ImportExportCodes::Busy).errorMessage(), QStringLiteral("Image is busy."));
    QCOMPARE(KisImportExportErrorCode(ImportExportCodes::OK).errorMessage(),
             QStringLiteral("The action has been completed successfully."));
}

void KisImportExportErrorCodeTest::customFailureMessage()
{
    const KisImportExportErrorCode failure(QStringLiteral("The decoder rejected chunk 4."));

    QCOMPARE(failure.errorMessage(), QStringLiteral("The decoder rejected chunk 4."));
    QVERIFY(!failure.isOk());
}

void KisImportExportErrorCodeTest::fileErrorsPreserveContext()
{
    KisImportExportErrorCannotRead readError(QFileDevice::PermissionsError);
    KisImportExportErrorCannotRead matchingReadError(QFileDevice::PermissionsError);
    KisImportExportErrorCannotRead otherReadError(QFileDevice::OpenError);
    QCOMPARE(readError.errorMessage(),
             QStringLiteral("Cannot open file for reading. Reason: Permission denied. LibrePaint is not allowed to "
                            "read or write to the file."));
    QVERIFY(readError == matchingReadError);
    QVERIFY(!(readError == otherReadError));

    KisImportExportErrorCannotWrite writeError(QFileDevice::ResourceError);
    KisImportExportErrorCannotWrite matchingWriteError(QFileDevice::ResourceError);
    KisImportExportErrorCannotWrite otherWriteError(QFileDevice::WriteError);
    QCOMPARE(writeError.errorMessage(),
             QStringLiteral("Cannot open file for writing. Reason: Out of resources (e.g. out of memory)."));
    QVERIFY(writeError == matchingWriteError);
    QVERIFY(!(writeError == otherWriteError));

    KisImportExportErrorCode wrappedRead(readError);
    KisImportExportErrorCode wrappedWrite(writeError);
    QCOMPARE(wrappedRead.errorMessage(), readError.errorMessage());
    QCOMPARE(wrappedWrite.errorMessage(), writeError.errorMessage());
    QVERIFY(wrappedRead == KisImportExportErrorCode(matchingReadError));
    QVERIFY(wrappedWrite == KisImportExportErrorCode(matchingWriteError));
    QVERIFY(!(wrappedRead == wrappedWrite));

    std::unique_ptr<KisImportExportComplexError> polymorphicError =
        std::make_unique<KisImportExportErrorCannotRead>(QFileDevice::ReadError);
    QCOMPARE(polymorphicError->errorMessage(),
             QStringLiteral("Cannot open file for reading. Reason: An error occurred when reading from the file."));
}

void KisImportExportErrorCodeTest::diagnosticFormatting()
{
    QString diagnostic;
    {
        QDebug debug(&diagnostic);
        debug << KisImportExportErrorCode(KisImportExportErrorCannotWrite(QFileDevice::PermissionsError));
    }

    QVERIFY(diagnostic.simplified().startsWith(QStringLiteral("Cannot write: 0")));
    QVERIFY(diagnostic.contains(QStringLiteral("Cannot write:")));
    QVERIFY(diagnostic.contains(QStringLiteral("Cannot open file for writing.")));
    QVERIFY(diagnostic.contains(QStringLiteral("Permission denied.")));
}

QTEST_GUILESS_MAIN(KisImportExportErrorCodeTest)

#include "kis_import_export_error_code_test.moc"
