/*
 * SPDX-FileCopyrightText: 2026 Krita contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/*
 * Qt does not read the predicted touches of the operating system. This file
 * therefore attaches a passive gesture recognizer to the Qt view and leaves
 * the Qt fork unchanged.
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
    UIView *view = self.view;

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

static KisPredictedTouchesRecognizer *s_recognizer = nil;

void installKisIOSPredictedTouches(void *nativeView)
{
    if (s_recognizer) {
        return;
    }

    UIView *view = reinterpret_cast<UIView *>(nativeView);
    if (!view) {
        qWarning() << "Could not attach Apple Pencil predicted touches: no native view";
        return;
    }

    // Keep the recognizer alive for the process lifetime.
    s_recognizer = [[KisPredictedTouchesRecognizer alloc] init];
    [view addGestureRecognizer:s_recognizer];
    KisPlatformPredictionBuffer::instance().setSupplierInstalled(true);
    qInfo() << "Attached Apple Pencil predicted touch recognizer";
}
