/*
 * SPDX-FileCopyrightText: 2026 Krita contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#import <UIKit/UIKit.h>

#include <QDebug>
#include <QPixmapCache>

#include "KisIOSMemoryWarningHandler.h"
#include "tiles3/kis_tile_data_store.h"

void installKisIOSMemoryWarningHandler()
{
    static id observer = nil;
    if (observer) {
        return;
    }

    observer = [[NSNotificationCenter defaultCenter]
        addObserverForName:UIApplicationDidReceiveMemoryWarningNotification
                    object:nil
                     queue:[NSOperationQueue mainQueue]
                usingBlock:^(NSNotification *) {
                    qWarning() << "iOS memory warning: purging LibrePaint tile and pixmap caches";
                    QPixmapCache::clear();
                    KisTileDataStore::instance()->purgeMemory();
                }];
}
