/*
 * SPDX-FileCopyrightText: 2026 Krita contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/*
 * Qt does not read the predicted touches of the operating system. This file
 * therefore adds a passive gesture recognizer and leaves the Qt fork
 * unchanged.
 *
 * Qt enters main() before UIKit connects the window scene, so the recognizer
 * attaches to the UIKit window instead of the view and reattaches when the
 * window becomes visible. KisIOSPencilInteraction.mm follows the same shape.
 *
 * The entry time and the coordinates use the same expressions as Qt, so the
 * predictor can match each entry to the real touches of the same event by
 * time.
 *
 * UIKit delivers touches to gesture recognizers before it delivers them to the
 * view, so the entry reaches the buffer before Qt processes the real touches.
 */

#import <UIKit/UIKit.h>
#import <UIKit/UIGestureRecognizerSubclass.h>

#include <QDebug>
#include <QPointF>
#include <QVector>

#include <KisLatencyProbe.h>
#include <KisPlatformPredictionBuffer.h>

#include "KisIOSPredictedTouches.h"

@interface KisPredictedTouchesRecognizer : UIGestureRecognizer
@end

static UIView *s_applicationView = nil;
static KisPredictedTouchesRecognizer *s_recognizer = nil;
static id s_windowDidBecomeVisibleObserver = nil;
static id s_windowDidBecomeKeyObserver = nil;
static id s_applicationDidBecomeActiveObserver = nil;

@implementation KisPredictedTouchesRecognizer

- (instancetype)init
{
    self = [super init];
    if (self) {
        self.allowedTouchTypes = @[ @(UITouchTypePencil) ];
        self.cancelsTouchesInView = NO;
        self.delaysTouchesBegan = NO;
        self.delaysTouchesEnded = NO;
    }
    return self;
}

- (void)pushEntryForTouch:(UITouch *)touch withEvent:(UIEvent *)event
{
    NSArray<UITouch *> *coalesced = [event coalescedTouchesForTouch:touch];
    if (coalesced.count == 0) {
        coalesced = @[ touch ];
    }
    UITouch *last = coalesced.lastObject;

    const quint64 keyMs = quint64(ulong(event.timestamp * 1000));

    // Qt takes locationInView: from the view that received the touch, so the
    // deltas use the same view.
    UIView *view = last.view ?: s_applicationView;

    QVector<KisPlatformPredictedPoint> points;
    const CGPoint lastLocation = [last locationInView:view];
    for (UITouch *predicted in [event predictedTouchesForTouch:touch]) {
        const CGPoint location = [predicted locationInView:view];
        KisPlatformPredictedPoint point;
        point.widgetDelta = QPointF(location.x - lastLocation.x,
                                    location.y - lastLocation.y);
        point.dtMs = (predicted.timestamp - last.timestamp) * 1000.0;
        points.append(point);
    }

    KisPlatformPredictionEntry entry;
    entry.keyMs = keyMs;
    entry.coalescedCount = int(coalesced.count);
    entry.points = points;

    KisLatencyProbe::mark("prediction_os_entry", 0, 0, points.size());
    KisPlatformPredictionBuffer::instance().push(entry);
}

- (void)pushEntriesForTouches:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
    for (UITouch *touch in touches) {
        if (touch.type != UITouchTypePencil) {
            continue;
        }
        [self pushEntryForTouch:touch withEvent:event];
    }
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
    NSUInteger pencilTouchCount = 0;
    for (UITouch *touch in touches) {
        if (touch.type == UITouchTypePencil) {
            ++pencilTouchCount;
        }
    }
    if (pencilTouchCount > 0) {
        KisLatencyProbe::mark("prediction_os_touch", 0, 0, qint64(pencilTouchCount));
    }

    [super touchesBegan:touches withEvent:event];
    [self pushEntriesForTouches:touches withEvent:event];
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
    [super touchesMoved:touches withEvent:event];
    [self pushEntriesForTouches:touches withEvent:event];
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
    [super touchesEnded:touches withEvent:event];
    self.state = UIGestureRecognizerStateFailed;
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event
{
    [super touchesCancelled:touches withEvent:event];
    self.state = UIGestureRecognizerStateFailed;
}

@end

static bool attachPredictedTouchesRecognizer()
{
    if (!s_applicationView) {
        return false;
    }

    UIView *window = s_applicationView.window;
    if (!window) {
        return false;
    }

    UIView *attachedView = s_recognizer.view;
    if (attachedView == window) {
        return true;
    }
    if (attachedView) {
        [attachedView removeGestureRecognizer:s_recognizer];
    }
    [window addGestureRecognizer:s_recognizer];

    KisPlatformPredictionBuffer::instance().setSupplierInstalled(true);
    KisLatencyProbe::mark("prediction_os_attach", 0, 0, 1);
    qInfo() << "Attached Apple Pencil predicted touch recognizer to the main window";
    return true;
}

void installKisIOSPredictedTouches(void *nativeView)
{
    UIView *view = reinterpret_cast<UIView *>(nativeView);
    if (view) {
        s_applicationView = view;
    }

    if (!s_recognizer) {
        // Keep the recognizer alive for the process lifetime.
        s_recognizer = [[KisPredictedTouchesRecognizer alloc] init];
    }

    if (!s_windowDidBecomeVisibleObserver) {
        NSNotificationCenter *notificationCenter = NSNotificationCenter.defaultCenter;
        s_windowDidBecomeVisibleObserver = [notificationCenter
            addObserverForName:UIWindowDidBecomeVisibleNotification
                        object:nil
                         queue:NSOperationQueue.mainQueue
                    usingBlock:^(NSNotification *notification) {
                        Q_UNUSED(notification);
                        attachPredictedTouchesRecognizer();
                    }];
        s_windowDidBecomeKeyObserver = [notificationCenter
            addObserverForName:UIWindowDidBecomeKeyNotification
                        object:nil
                         queue:NSOperationQueue.mainQueue
                    usingBlock:^(NSNotification *notification) {
                        Q_UNUSED(notification);
                        attachPredictedTouchesRecognizer();
                    }];
        s_applicationDidBecomeActiveObserver = [notificationCenter
            addObserverForName:UIApplicationDidBecomeActiveNotification
                        object:nil
                         queue:NSOperationQueue.mainQueue
                    usingBlock:^(NSNotification *) {
                        attachPredictedTouchesRecognizer();
                    }];
    }

    if (!attachPredictedTouchesRecognizer()) {
        // Qt enters main() before UIKit connects its UIWindowScene. Retry once
        // the event loop starts; the observers above also cover later window
        // activation and foreground transitions without changing the target.
        dispatch_async(dispatch_get_main_queue(), ^{
            if (!attachPredictedTouchesRecognizer()) {
                qWarning() << "Could not attach Apple Pencil predicted touch recognizer to the LibrePaint main window";
            }
        });
    }
}
