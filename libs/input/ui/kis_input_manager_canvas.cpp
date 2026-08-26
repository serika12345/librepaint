/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_input_manager.h"

#include <QPointer>

#include <KoCanvasBase.h>
#include <KisToolCanvas.h>

#include "kis_input_manager_p.h"

void KisInputManager::addTrackedCanvas(KoCanvasBase *canvas)
{
    auto *toolCanvas = dynamic_cast<KisToolCanvas *>(canvas);
    KIS_SAFE_ASSERT_RECOVER_RETURN(toolCanvas);

    toolCanvas->setInputEventFilterConnection(
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
    toolCanvas->setInputCanvasWidgetChangedCallback(
        [guard = QPointer<KisInputManager>(this), canvas]() {
            if (!guard) {
                return;
            }
            guard->d->canvasSwitcher.removeCanvas(canvas);
            guard->d->canvasSwitcher.addCanvas(canvas);
        });
    d->canvasSwitcher.addCanvas(canvas);
}

void KisInputManager::removeTrackedCanvas(KoCanvasBase *canvas)
{
    d->canvasSwitcher.removeCanvas(canvas);
    auto *toolCanvas = dynamic_cast<KisToolCanvas *>(canvas);
    KIS_SAFE_ASSERT_RECOVER_RETURN(toolCanvas);
    toolCanvas->setInputCanvasWidgetChangedCallback({});
    toolCanvas->setInputEventFilterConnection({});
}
