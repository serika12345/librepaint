/*
 * SPDX-FileCopyrightText: 2026 Krita contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#import <UIKit/UIKit.h>

#include <QCoreApplication>
#include <QDebug>
#include <QMetaObject>

#include "KisIOSPencilInteraction.h"

static KisIOSPencilTapHandler s_handler = nullptr;

static void queueTapAction(KisIOSPencilTapAction action)
{
    QMetaObject::invokeMethod(QCoreApplication::instance(), [action]() {
        if (s_handler) {
            s_handler(action);
        }
    }, Qt::QueuedConnection);
}

static void handlePencilTap()
{
    switch (UIPencilInteraction.preferredTapAction) {
    case UIPencilPreferredActionSwitchEraser:
        queueTapAction(KisIOSPencilTapAction::SwitchEraser);
        break;
    case UIPencilPreferredActionSwitchPrevious:
        queueTapAction(KisIOSPencilTapAction::SwitchPrevious);
        break;
    case UIPencilPreferredActionIgnore:
        break;
    default:
        qInfo() << "Apple Pencil double tap preference is not supported yet:"
                << UIPencilInteraction.preferredTapAction;
        break;
    }
}

@interface KritaIOSPencilInteractionDelegate : NSObject <UIPencilInteractionDelegate>
@end

@implementation KritaIOSPencilInteractionDelegate

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#pragma clang diagnostic ignored "-Wdeprecated-implementations"
- (void)pencilInteractionDidTap:(UIPencilInteraction *)interaction
{
    Q_UNUSED(interaction);
    handlePencilTap();
}
#pragma clang diagnostic pop

- (void)pencilInteraction:(UIPencilInteraction *)interaction
             didReceiveTap:(UIPencilInteractionTap *)tap API_AVAILABLE(ios(17.5))
{
    Q_UNUSED(interaction);
    Q_UNUSED(tap);
    handlePencilTap();
}

@end

static KritaIOSPencilInteractionDelegate *s_delegate = nil;
static UIPencilInteraction *s_interaction = nil;
static id s_windowDidBecomeVisibleObserver = nil;
static id s_windowDidBecomeKeyObserver = nil;
static id s_applicationDidBecomeActiveObserver = nil;

static UIWindow *activeApplicationWindow()
{
    UIApplication *application = UIApplication.sharedApplication;

    for (UIScene *scene in application.connectedScenes) {
        if (![scene isKindOfClass:UIWindowScene.class]) {
            continue;
        }

        UIWindowScene *windowScene = static_cast<UIWindowScene *>(scene);
        if (windowScene.activationState != UISceneActivationStateForegroundActive
            && windowScene.activationState != UISceneActivationStateForegroundInactive) {
            continue;
        }

        for (UIWindow *window in windowScene.windows) {
            if (window.isKeyWindow) {
                return window;
            }
        }

        for (UIWindow *window in windowScene.windows) {
            if (!window.isHidden && window.alpha > 0.0) {
                return window;
            }
        }
    }

    return nil;
}

static bool attachPencilInteraction(UIView *candidateView)
{
    UIWindow *window = candidateView.window;
    if (!window && [candidateView isKindOfClass:UIWindow.class]) {
        window = static_cast<UIWindow *>(candidateView);
    }
    if (!window) {
        window = activeApplicationWindow();
    }
    if (!window) {
        return false;
    }

    UIView *attachedView = s_interaction.view;
    if (attachedView == window) {
        return true;
    }
    if (attachedView) {
        [attachedView removeInteraction:s_interaction];
    }
    [window addInteraction:s_interaction];

    qInfo() << "Attached Apple Pencil double-tap interaction to the active window";
    return true;
}

void installKisIOSPencilInteraction(void *nativeView, KisIOSPencilTapHandler handler)
{
    UIView *view = reinterpret_cast<UIView *>(nativeView);
    s_handler = handler;

    if (!s_interaction) {
        // UIPencilInteraction holds its delegate weakly. Keep both objects
        // alive for the process lifetime.
        s_delegate = [[KritaIOSPencilInteractionDelegate alloc] init];
        s_interaction = [[UIPencilInteraction alloc] init];
        s_interaction.delegate = s_delegate;
    }

    if (!s_windowDidBecomeVisibleObserver) {
        NSNotificationCenter *notificationCenter = NSNotificationCenter.defaultCenter;
        s_windowDidBecomeVisibleObserver = [notificationCenter
            addObserverForName:UIWindowDidBecomeVisibleNotification
                        object:nil
                         queue:NSOperationQueue.mainQueue
                    usingBlock:^(NSNotification *notification) {
                        attachPencilInteraction(static_cast<UIView *>(notification.object));
                    }];
        s_windowDidBecomeKeyObserver = [notificationCenter
            addObserverForName:UIWindowDidBecomeKeyNotification
                        object:nil
                         queue:NSOperationQueue.mainQueue
                    usingBlock:^(NSNotification *notification) {
                        attachPencilInteraction(static_cast<UIView *>(notification.object));
                    }];
        s_applicationDidBecomeActiveObserver = [notificationCenter
            addObserverForName:UIApplicationDidBecomeActiveNotification
                        object:nil
                         queue:NSOperationQueue.mainQueue
                    usingBlock:^(NSNotification *) {
                        attachPencilInteraction(nil);
                    }];
    }

    if (!attachPencilInteraction(view)) {
        // Qt enters main() before UIKit connects its UIWindowScene. Retry once
        // the event loop starts; the observers above also cover later window
        // replacement and foreground transitions.
        dispatch_async(dispatch_get_main_queue(), ^{
            if (!attachPencilInteraction(nil)) {
                qWarning() << "Could not attach Apple Pencil interaction to an active LibrePaint window";
            }
        });
    }
}
