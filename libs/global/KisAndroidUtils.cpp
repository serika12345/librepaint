/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "KisAndroidUtils.h"
#include "KisAndroidLogHandler.h"
#include <kis_debug.h>

#include <QFile>
#include <QTemporaryFile>

#include <QJniEnvironment>
#include <QJniObject>

#include <unistd.h>

namespace KisAndroidUtils
{

void performInitialSetup()
{
    KisAndroidLogHandler::handler_init();

    QJniObject activity = QJniObject::callStaticObjectMethod("org/qtproject/qt/android/QtNative",
                                                             "activity",
                                                             "()Landroid/app/Activity;");
    if (activity.isValid()) {
        activity.callMethod<void>("copyAssets", "()V");
    } else {
        qWarning("performInitialSetup: activity not valid");
    }
}

bool looksLikeXiaomiDevice()
{
    // The device isn't going to change, so let's cache the slow JNI call.
    static bool checked;
    static bool result;
    if (!checked) {
        checked = true;
        result =
            QJniObject::callStaticMethod<jboolean>("org/krita/android/MainActivity", "looksLikeXiaomiDevice", "()Z");
    }
    return result;
}

bool isLowMemoryKillReportSupported()
{
    // The support is device-bound and will never change, so cache the JNI call.
    static bool checked;
    static bool result;
    if (!checked) {
        checked = true;
        result = QJniObject::callStaticMethod<jboolean>("org/krita/android/MainActivity",
                                                        "isLowMemoryKillReportSupported",
                                                        "()Z");
    }
    return result;
}

void clearJniException(const QString &location)
{
    QJniEnvironment env;
    if (env->ExceptionCheck()) {
        warnKrita << "JNI exception occurred" << location;
        env->ExceptionDescribe();
        env->ExceptionClear();
    }
}

bool isInFullScreen()
{
    QJniObject activity = QJniObject::callStaticObjectMethod("org/qtproject/qt/android/QtNative",
                                                             "activity",
                                                             "()Landroid/app/Activity;");
    KisAndroidUtils::clearJniException(QStringLiteral("getting activity in isInFullScreen"));
    if (activity.isValid()) {
        bool fullScreen = activity.callMethod<jboolean>("isInFullScreen", "()Z");
        KisAndroidUtils::clearJniException(QStringLiteral("calling isInFullScreen"));
        return fullScreen;
    } else {
        qWarning("isInFullScreen: activity not valid");
        return false;
    }
}

void setFullScreen(bool fullScreen)
{
    QJniObject activity = QJniObject::callStaticObjectMethod("org/qtproject/qt/android/QtNative",
                                                             "activity",
                                                             "()Landroid/app/Activity;");
    KisAndroidUtils::clearJniException(QStringLiteral("getting activity in setFullScreen"));
    if (activity.isValid()) {
        activity.callMethod<void>("setFullScreenOnUiThread", "(Z)V", jboolean(fullScreen));
        KisAndroidUtils::clearJniException(QStringLiteral("calling setFullScreenOnUiThread"));
    } else {
        qWarning("setFullScreen: activity not valid");
    }
}

namespace
{
bool clearPendingJniException(const QString &location)
{
    QJniEnvironment env;
    if (!env->ExceptionCheck()) {
        return false;
    }

    warnKrita << "JNI exception occurred" << location;
    env->ExceptionDescribe();
    env->ExceptionClear();
    return true;
}

QString androidDescriptorMode(QIODevice::OpenMode openMode)
{
    if (openMode.testFlag(QIODevice::Append)) {
        return QStringLiteral("wa");
    }
    if (openMode.testFlag(QIODevice::WriteOnly) && openMode.testFlag(QIODevice::Truncate)) {
        return QStringLiteral("rwt");
    }
    if (openMode.testFlag(QIODevice::ReadOnly) && openMode.testFlag(QIODevice::WriteOnly)) {
        return QStringLiteral("rw");
    }
    if (openMode.testFlag(QIODevice::WriteOnly)) {
        return QStringLiteral("w");
    }
    return QStringLiteral("r");
}

bool openFile(QFile &file,
              const QString &path,
              QIODevice::OpenMode openMode,
              QString *outErrorMessage)
{
    file.setFileName(path);
    if (!path.startsWith(QLatin1String("content://"))) {
        if (file.open(openMode)) {
            return true;
        }
        if (outErrorMessage) {
            *outErrorMessage = QStringLiteral("failed to open file '%1': %2").arg(path).arg(file.errorString());
        }
        return false;
    }

    const QJniObject activity = QJniObject::callStaticObjectMethod(
        "org/qtproject/qt/android/QtNative",
        "activity",
        "()Landroid/app/Activity;");
    if (clearPendingJniException(QStringLiteral("getting activity to open an Android content URI"))
        || !activity.isValid()) {
        if (outErrorMessage) {
            *outErrorMessage = QStringLiteral("failed to get the Android activity for '%1'").arg(path);
        }
        return false;
    }

    const QJniObject resolver = activity.callObjectMethod(
        "getContentResolver",
        "()Landroid/content/ContentResolver;");
    if (clearPendingJniException(QStringLiteral("getting ContentResolver to open an Android content URI"))
        || !resolver.isValid()) {
        if (outErrorMessage) {
            *outErrorMessage = QStringLiteral("failed to get Android ContentResolver for '%1'").arg(path);
        }
        return false;
    }

    const QJniObject javaPath = QJniObject::fromString(path);
    const QJniObject uri = QJniObject::callStaticObjectMethod(
        "android/net/Uri",
        "parse",
        "(Ljava/lang/String;)Landroid/net/Uri;",
        javaPath.object<jstring>());
    if (clearPendingJniException(QStringLiteral("parsing an Android content URI")) || !uri.isValid()) {
        if (outErrorMessage) {
            *outErrorMessage = QStringLiteral("failed to parse Android content URI '%1'").arg(path);
        }
        return false;
    }

    const QJniObject javaMode = QJniObject::fromString(androidDescriptorMode(openMode));
    const QJniObject descriptor = resolver.callObjectMethod(
        "openFileDescriptor",
        "(Landroid/net/Uri;Ljava/lang/String;)Landroid/os/ParcelFileDescriptor;",
        uri.object<jobject>(),
        javaMode.object<jstring>());
    if (clearPendingJniException(QStringLiteral("opening an Android content URI")) || !descriptor.isValid()) {
        if (outErrorMessage) {
            *outErrorMessage = QStringLiteral("failed to open Android content URI '%1'").arg(path);
        }
        return false;
    }

    const int fileDescriptor = descriptor.callMethod<jint>("detachFd", "()I");
    if (clearPendingJniException(QStringLiteral("detaching an Android content URI file descriptor"))
        || fileDescriptor < 0) {
        if (outErrorMessage) {
            *outErrorMessage = QStringLiteral("failed to acquire a file descriptor for Android content URI '%1'")
                                   .arg(path);
        }
        return false;
    }

    if (!file.open(fileDescriptor, openMode, QFileDevice::AutoCloseHandle)) {
        ::close(fileDescriptor);
        if (outErrorMessage) {
            *outErrorMessage = QStringLiteral("failed to attach the file descriptor for Android content URI '%1': %2")
                                   .arg(path)
                                   .arg(file.errorString());
        }
        return false;
    }

    return true;
}

bool copyFileContents(const QString &inputPath,
                      const QString &outputPath,
                      QFile &inputFile,
                      QFile &outputFile,
                      QString *outErrorMessage)
{
    QByteArray buffer;
    buffer.resize(BUFSIZ);
    while (true) {
        qint64 read = inputFile.read(buffer.data(), BUFSIZ);
        if (read < 0) {
            if (outErrorMessage) {
                *outErrorMessage = QStringLiteral("failed to read from input file '%1': %2")
                                       .arg(inputPath)
                                       .arg(inputFile.errorString());
            }
            return false;
        } else if (read > 0) {
            qint64 written = outputFile.write(buffer, read);
            if (written < 0) {
                if (outErrorMessage) {
                    *outErrorMessage = QStringLiteral("failed to write %1 byte(s) to output file '%2': %3")
                                           .arg(read)
                                           .arg(outputPath)
                                           .arg(outputFile.errorString());
                }
                return false;
            } else if (written != read) {
                if (outErrorMessage) {
                    *outErrorMessage =
                        QStringLiteral("tried to write %1 byte(s) to output file '%2', but only wrote %3")
                            .arg(read)
                            .arg(outputPath)
                            .arg(written);
                }
                return false;
            }
        } else {
            if (outputFile.flush()) {
                if (outErrorMessage) {
                    outErrorMessage->clear();
                }
                return true;
            } else {
                if (outErrorMessage) {
                    *outErrorMessage = QStringLiteral("failed to flush output file '%1': %2")
                                           .arg(outputPath)
                                           .arg(outputFile.errorString());
                }
                return false;
            }
        }
    }
}
} // namespace

bool copyFile(const QString &inputPath, const QString &outputPath, QString *outErrorMessage)
{
    QFile inputFile;
    if (!openFile(inputFile, inputPath, QIODevice::ReadOnly, outErrorMessage)) {
        return false;
    }

    QFile outputFile;
    if (!openFile(outputFile, outputPath, QIODevice::WriteOnly | QIODevice::Truncate, outErrorMessage)) {
        return false;
    }

    return copyFileContents(inputPath, outputPath, inputFile, outputFile, outErrorMessage);
}

bool copyFileToTemporary(const QString &inputPath, QTemporaryFile &outputFile, QString *outErrorMessage)
{
    QFile inputFile;
    if (!openFile(inputFile, inputPath, QIODevice::ReadOnly, outErrorMessage)) {
        return false;
    }

    if (!outputFile.open()) {
        if (outErrorMessage) {
            *outErrorMessage = QStringLiteral("failed to open temporary output file: %1").arg(outputFile.errorString());
        }
        return false;
    }

    if (outErrorMessage) {
        outErrorMessage->clear();
    }
    return copyFileContents(inputPath, outputFile.fileName(), inputFile, outputFile, outErrorMessage);
}

} // namespace KisAndroidUtils
