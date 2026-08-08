/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "KisAndroidSplash.h"

#include "KisApplication.h"
#include "kis_splash_screen.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QFile>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QJniEnvironment>
#include <QJniObject>
using AndroidJniEnvironment = QJniEnvironment;
using AndroidJniObject = QJniObject;
#else
#include <QAndroidJniEnvironment>
#include <QAndroidJniObject>
using AndroidJniEnvironment = QAndroidJniEnvironment;
using AndroidJniObject = QAndroidJniObject;
#endif

KisAndroidSplash *KisAndroidSplash::instance()
{
    KisApplication *app = qobject_cast<KisApplication *>(KisApplication::instance());
    return app ? app->androidSplash() : nullptr;
}

void KisAndroidSplash::show()
{
    AndroidJniEnvironment env;
    jbyteArray splashBytes = nullptr;

    QFile file(KisSplashScreen::imageResourcePath());
    if (file.open(QIODevice::ReadOnly)) {
        const QByteArray data = file.readAll();
        if (!data.isEmpty()) {
            const jsize length = static_cast<jsize>(data.size());
            splashBytes = env->NewByteArray(length);
            env->SetByteArrayRegion(splashBytes,
                                    0,
                                    length,
                                    reinterpret_cast<const jbyte *>(data.constData()));
        }
    } else {
        qWarning("Failed to read Android splash image from '%s': %s",
                 qUtf8Printable(file.fileName()),
                 qUtf8Printable(file.errorString()));
    }

    const AndroidJniObject versionObject = AndroidJniObject::fromString(QCoreApplication::applicationVersion());
    AndroidJniObject::callStaticMethod<void>("org/krita/android/MainActivity",
                                             "showSplashDialog",
                                             "([BLjava/lang/String;)V",
                                             splashBytes,
                                             versionObject.object<jstring>());

    if (splashBytes) {
        env->DeleteLocalRef(splashBytes);
    }
}

void KisAndroidSplash::setLoaded(bool loaded)
{
    AndroidJniObject::callStaticMethod<void>("org/krita/android/MainActivity",
                                             "setLoaded",
                                             "(Z)V",
                                             jboolean(loaded));
}

void KisAndroidSplash::setLoadingText(const QString &text)
{
    const AndroidJniObject textObject = AndroidJniObject::fromString(text);
    AndroidJniObject::callStaticMethod<void>("org/krita/android/MainActivity",
                                             "setLoadingText",
                                             "(Ljava/lang/String;)V",
                                             textObject.object<jstring>());
}

KisAndroidSplash::KisAndroidSplash(QObject *parent)
    : QObject(parent)
{
}

extern "C" JNIEXPORT void JNICALL Java_org_krita_android_JNIWrappers_onSplashDialogDismissed(JNIEnv * /*env*/,
                                                                                              jobject /*obj*/)
{
    KisAndroidSplash *splash = KisAndroidSplash::instance();
    if (splash) {
        Q_EMIT splash->sigSplashDialogDismissed();
    }
}
