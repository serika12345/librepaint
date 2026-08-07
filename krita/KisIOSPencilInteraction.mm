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

void installKisIOSPencilInteraction(void *nativeView, KisIOSPencilTapHandler handler)
{
    if (s_interaction) {
        return;
    }

    UIView *view = reinterpret_cast<UIView *>(nativeView);
    if (!view) {
        qWarning() << "Could not attach Apple Pencil interaction to the LibrePaint window";
        return;
    }

    s_handler = handler;

    // UIPencilInteraction holds its delegate weakly. Keep both objects alive
    // for the process lifetime, matching the lifetime of Krita's main window.
    s_delegate = [[KritaIOSPencilInteractionDelegate alloc] init];
    s_interaction = [[UIPencilInteraction alloc] init];
    s_interaction.delegate = s_delegate;
    [view addInteraction:s_interaction];

    qInfo() << "Installed Apple Pencil double-tap interaction";
}
