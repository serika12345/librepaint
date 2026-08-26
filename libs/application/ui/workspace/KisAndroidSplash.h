/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef KISANDROIDSPLASH_H
#define KISANDROIDSPLASH_H

#include <QObject>
#include <QString>

#include "kritaui_export.h"

class KRITAUI_EXPORT KisAndroidSplash : public QObject
{
    Q_OBJECT
    friend class KisApplication;

public:
    static KisAndroidSplash *instance();

    static void show();
    static void setLoaded(bool loaded);
    static void setLoadingText(const QString &text);

Q_SIGNALS:
    void sigSplashDialogDismissed();

private:
    explicit KisAndroidSplash(QObject *parent = nullptr);
};

#endif // KISANDROIDSPLASH_H
