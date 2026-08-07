/*
 * SPDX-FileCopyrightText: 2026 Krita contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#import <UIKit/UIKit.h>

#include <QDebug>

#include "KisIOSLifecycleHandler.h"

namespace
{
KisIOSLifecycleHandler s_lifecycleHandler = nullptr;
id s_willResignActiveObserver = nil;
id s_didEnterBackgroundObserver = nil;
id s_willEnterForegroundObserver = nil;
id s_didBecomeActiveObserver = nil;
UIBackgroundTaskIdentifier s_backgroundTask = UIBackgroundTaskInvalid;

void notifyLifecycleEvent(KisIOSLifecycleEvent event)
{
    if (s_lifecycleHandler) {
        s_lifecycleHandler(event);
    }
}

void beginBackgroundTask()
{
    if (s_backgroundTask != UIBackgroundTaskInvalid) {
        qWarning() << "iPadOS background task was already active";
        return;
    }

    UIApplication *application = UIApplication.sharedApplication;
    s_backgroundTask =
        [application beginBackgroundTaskWithName:@"LibrePaint autosave recovery"
                               expirationHandler:^{
        qWarning() << "iPadOS background time expired while saving recovery data";
        notifyLifecycleEvent(KisIOSLifecycleEvent::BackgroundTaskExpired);
        finishKisIOSBackgroundTask();
    }];

    if (s_backgroundTask == UIBackgroundTaskInvalid) {
        qWarning() << "iPadOS did not grant background execution time for recovery autosave";
    }
}
}

void finishKisIOSBackgroundTask()
{
    void (^finishBlock)(void) = ^{
        if (s_backgroundTask == UIBackgroundTaskInvalid) {
            return;
        }

        const UIBackgroundTaskIdentifier task = s_backgroundTask;
        s_backgroundTask = UIBackgroundTaskInvalid;
        [UIApplication.sharedApplication endBackgroundTask:task];
        qInfo() << "iPadOS recovery autosave background task finished";
    };

    if (NSThread.isMainThread) {
        finishBlock();
    } else {
        dispatch_async(dispatch_get_main_queue(), finishBlock);
    }
}

void installKisIOSLifecycleHandler(KisIOSLifecycleHandler lifecycleHandler)
{
    if (s_willResignActiveObserver) {
        return;
    }

    s_lifecycleHandler = lifecycleHandler;
    NSNotificationCenter *notificationCenter = NSNotificationCenter.defaultCenter;

    s_willResignActiveObserver = [notificationCenter
        addObserverForName:UIApplicationWillResignActiveNotification
                    object:nil
                     queue:NSOperationQueue.mainQueue
                usingBlock:^(NSNotification *) {
                    qInfo() << "iPadOS is making LibrePaint inactive";
                    notifyLifecycleEvent(KisIOSLifecycleEvent::WillResignActive);
                }];

    s_didEnterBackgroundObserver = [notificationCenter
        addObserverForName:UIApplicationDidEnterBackgroundNotification
                    object:nil
                     queue:NSOperationQueue.mainQueue
                usingBlock:^(NSNotification *) {
                    qInfo() << "iPadOS moved LibrePaint to the background";
                    beginBackgroundTask();
                    notifyLifecycleEvent(KisIOSLifecycleEvent::DidEnterBackground);
                }];

    s_willEnterForegroundObserver = [notificationCenter
        addObserverForName:UIApplicationWillEnterForegroundNotification
                    object:nil
                     queue:NSOperationQueue.mainQueue
                usingBlock:^(NSNotification *) {
                    qInfo() << "iPadOS is returning LibrePaint to the foreground";
                    notifyLifecycleEvent(KisIOSLifecycleEvent::WillEnterForeground);
                }];

    s_didBecomeActiveObserver = [notificationCenter
        addObserverForName:UIApplicationDidBecomeActiveNotification
                    object:nil
                     queue:NSOperationQueue.mainQueue
                usingBlock:^(NSNotification *) {
                    qInfo() << "iPadOS returned LibrePaint to the foreground";
                    notifyLifecycleEvent(KisIOSLifecycleEvent::DidBecomeActive);
                }];
}
