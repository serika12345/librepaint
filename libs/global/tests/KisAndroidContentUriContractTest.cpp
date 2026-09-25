/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisAndroidUtils.h"

#include <QDir>
#include <QFile>
#include <QJniEnvironment>
#include <QJniObject>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QTest>

class KisAndroidContentUriContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void copiesDocumentDataThroughContentResolver();
};

void KisAndroidContentUriContractTest::copiesDocumentDataThroughContentResolver()
{
    // Consumer: Android document save and verification through the system file picker.
    // Operation: copy bytes to and from a content URI backed by the package FileProvider.
    // Result: the round-tripped document bytes are identical to the source bytes.
    // Failure: Save As cannot write or verify a system-selected Android document.
    QStandardPaths::setTestModeEnabled(true);

    const QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QVERIFY(QDir().mkpath(dataPath));
    const QString targetPath = dataPath + QStringLiteral("/android-content-uri-contract.bin");

    QFile target(targetPath);
    QVERIFY(target.open(QIODevice::WriteOnly | QIODevice::Truncate));
    target.close();

    const QJniObject activity = QJniObject::callStaticObjectMethod(
        "org/qtproject/qt/android/QtNative",
        "activity",
        "()Landroid/app/Activity;");
    QVERIFY(activity.isValid());

    const QJniObject javaTargetPath = QJniObject::fromString(targetPath);
    const QJniObject javaTarget("java/io/File",
                                "(Ljava/lang/String;)V",
                                javaTargetPath.object<jstring>());
    const QJniObject authority = QJniObject::fromString(QStringLiteral("org.librepaint.tests.qtprovider"));
    const QJniObject uri = QJniObject::callStaticObjectMethod(
        "androidx/core/content/FileProvider",
        "getUriForFile",
        "(Landroid/content/Context;Ljava/lang/String;Ljava/io/File;)Landroid/net/Uri;",
        activity.object<jobject>(),
        authority.object<jstring>(),
        javaTarget.object<jobject>());

    QJniEnvironment env;
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        QFAIL("FileProvider rejected the Android content URI contract target");
    }
    QVERIFY(uri.isValid());

    const QByteArray expected = QByteArrayLiteral("LibrePaint Android content URI contract\n")
        + QByteArray(1, '\0') + QByteArrayLiteral("payload");
    QTemporaryFile source;
    QVERIFY(source.open());
    QCOMPARE(source.write(expected), expected.size());
    QVERIFY(source.flush());

    QString errorMessage;
    QVERIFY2(KisAndroidUtils::copyFile(source.fileName(), uri.toString(), &errorMessage),
             qPrintable(errorMessage));

    QTemporaryFile roundTrip;
    QVERIFY2(KisAndroidUtils::copyFileToTemporary(uri.toString(), roundTrip, &errorMessage),
             qPrintable(errorMessage));
    QVERIFY(roundTrip.seek(0));
    QCOMPARE(roundTrip.readAll(), expected);

    QVERIFY(QFile::remove(targetPath));
}

QTEST_MAIN(KisAndroidContentUriContractTest)

#include "KisAndroidContentUriContractTest.moc"
