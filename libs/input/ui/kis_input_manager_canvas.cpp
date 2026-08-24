/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_input_manager.h"

#include <QPointer>

#include <kis_canvas2.h>

#include "kis_input_manager_p.h"

void KisInputManager::addTrackedCanvas(KisCanvas2 *canvas)
{
    canvas->setInputEventFilterConnection(
        [guard = QPointer<KisInputManager>(this)](QObject *filter, bool attach, int priority) {
            if (!guard) {
                return;
            }
            if (attach) {
                guard->attachPriorityEventFilter(filter, priority);
            } else {
                guard->detachPriorityEventFilter(filter);
            }
        });
    canvas->setInputCanvasWidgetChangedCallback(
        [guard = QPointer<KisInputManager>(this), canvas]() {
            if (!guard) {
                return;
            }
            guard->d->canvasSwitcher.removeCanvas(canvas);
            guard->d->canvasSwitcher.addCanvas(canvas);
        });
    d->canvasSwitcher.addCanvas(canvas);
}

void KisInputManager::removeTrackedCanvas(KisCanvas2 *canvas)
{
    d->canvasSwitcher.removeCanvas(canvas);
    canvas->setInputCanvasWidgetChangedCallback({});
    canvas->setInputEventFilterConnection({});
}
