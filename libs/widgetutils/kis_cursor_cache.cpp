
/*
 *  SPDX-FileCopyrightText: 2016 Michael Abrahams <miabraha@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_cursor_cache.h"

#include <QScreen>
#include <QWindow>
#include <QBitmap>
#include <qcursor.h>
#include <qglobalstatic.h>
#include <qlogging.h>
#include <QDebug>
#include <QPainter>
#include <QApplication>
#include <qnamespace.h>
#include <qpoint.h>

#include "KoResourcePaths.h"

Q_GLOBAL_STATIC(KisCursorCache, s_instance)

namespace {

    QCursor loadImpl(const QString &cursorName, int hotspotX, int hotspotY, int width, int height) {
#ifdef Q_OS_ANDROID
        // On Android, large cursors render glitchy, so we avoid scaling the cursors till we find a solution to that.
        QPixmap cursorImage = QPixmap(":/" + cursorName);
#else
        QPixmap cursorImage = QIcon(":/" + cursorName).pixmap(width > -1? width: 32, height > -1? height: 32);
#endif

        if (cursorImage.isNull()) {
            qWarning() << "Could not load cursor from qrc, trying filesystem" << cursorName;
            cursorImage = QPixmap(KoResourcePaths::findAsset("kis_pics", cursorName));
            if (cursorImage.isNull()) {
                qWarning() << "Could not load cursor from filesystem" << cursorName;
                return Qt::ArrowCursor;
            }
        }

        int hX = hotspotX;
        int hY = hotspotY;
#ifdef Q_OS_LINUX
        if (qEnvironmentVariable("QT_QPA_PLATFORM") == "xcb") {
            hX = (hotspotX >= 0? hotspotX * cursorImage.devicePixelRatio(): (cursorImage.width() /2));
            hY = (hotspotY >= 0? hotspotY * cursorImage.devicePixelRatio(): (cursorImage.height()/2));
        }
#endif

        return QCursor(cursorImage, hX, hY);
    }

}

KisCursorCache::KisCursorCache() {}

KisCursorCache* KisCursorCache::instance()
{
    return s_instance;
}

QCursor KisCursorCache::load(const QString & cursorName, int width, int height, int hotspotX, int hotspotY)
{
    if (cursorHash.contains(cursorName)) {
        return cursorHash[ cursorName ].second;
    }

    // Otherwise, construct the cursor
    QCursor newCursor = loadImpl(cursorName, hotspotX, hotspotY, width, height);
    cursorHash.insert(cursorName, QPair<QPoint, QCursor>(QPoint(hotspotX, hotspotY), newCursor));
    return newCursor;
}
