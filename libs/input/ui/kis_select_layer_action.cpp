/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2012 Arjen Hiemstra <ahiemstra@heimr.nl>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_select_layer_action.h"

#include <QApplication>
#include <QTouchEvent>

#include <klocalizedstring.h>

#include <kis_canvas2.h>
#include <kis_cursor.h>
#include <actions/KisLayerSelectionAction.h>

#include "kis_input_manager.h"

#include <kis_assert.h>

class KisSelectLayerAction::Private
{
public:
    int shortcut {makeShortcut(LayerSelectionMode_TopLayer, SelectionOverrideMode_Replace)};

    static int makeShortcut(LayerSelectionMode layerSelectionMode, SelectionOverrideMode selectionOverrideMode)
    {
        // Store the layer selection mode on the second byte and the selection override mode on the first one
        return (layerSelectionMode << 8) | selectionOverrideMode;
    }

    static int layerSelectionMode(int shortcut)
    {
        // Get the layer selection mode from the second byte
        return (shortcut >> 8) & 0xFF;
    }

    static int selectionOverrideMode(int shortcut)
    {
        // Get the selection override mode from the first byte
        return shortcut & 0xFF;
    }

};

KisSelectLayerAction::KisSelectLayerAction()
    : KisAbstractInputAction("Select Layer")
    , d(new Private)
{
    setName(i18n("Select Layer"));
    setDescription(i18n("Select layers under the cursor position"));

    QHash<QString, int> shortcuts;
    shortcuts.insert(i18n("Select Top Layer (Replace Selection)"),
                     d->makeShortcut(LayerSelectionMode_TopLayer, SelectionOverrideMode_Replace));
    shortcuts.insert(i18n("Select All Layers (Replace Selection)"),
                     d->makeShortcut(LayerSelectionMode_AllLayers, SelectionOverrideMode_Replace));
    shortcuts.insert(i18n("Select from Menu (Replace Selection)"),
                     d->makeShortcut(LayerSelectionMode_Ask, SelectionOverrideMode_Replace));
    shortcuts.insert(i18n("Select Top Layer (Add to Selection)"),
                     d->makeShortcut(LayerSelectionMode_TopLayer, SelectionOverrideMode_Add));
    shortcuts.insert(i18n("Select All Layers (Add to Selection)"),
                     d->makeShortcut(LayerSelectionMode_AllLayers, SelectionOverrideMode_Add));
    shortcuts.insert(i18n("Select from Menu (Add to Selection)"),
                     d->makeShortcut(LayerSelectionMode_Ask, SelectionOverrideMode_Add));
    setShortcutIndexes(shortcuts);
}

KisSelectLayerAction::~KisSelectLayerAction()
{
    delete d;
}

int KisSelectLayerAction::priority() const
{
    return 5;
}

void KisSelectLayerAction::activate(int shortcut)
{
    Q_UNUSED(shortcut);
    QApplication::setOverrideCursor(KisCursor::pickLayerCursor());
}

void KisSelectLayerAction::deactivate(int shortcut)
{
    Q_UNUSED(shortcut);
    QApplication::restoreOverrideCursor();
}

void KisSelectLayerAction::begin(int shortcut, QEvent *event)
{
    KisAbstractInputAction::begin(shortcut, event);

    d->shortcut = shortcut;
    inputEvent(event);
}

void KisSelectLayerAction::inputEvent(QEvent *event)
{
    // Event not recognized
    if (!event || (event->type() != QEvent::MouseMove && event->type() != QEvent::TabletMove &&
                   event->type() != QTouchEvent::TouchUpdate && event->type() != QEvent::MouseButtonPress &&
                   event->type() != QEvent::TabletPress && event->type() != QTouchEvent::TouchBegin)) {
        return;
    }

    const int layerSelectionMode = d->layerSelectionMode(d->shortcut);
    const int selectionOverrideMode = d->selectionOverrideMode(d->shortcut);

    // Shortcut not recognized
    KIS_SAFE_ASSERT_RECOVER_RETURN(
        (layerSelectionMode == LayerSelectionMode_TopLayer ||
         layerSelectionMode == LayerSelectionMode_AllLayers ||
         layerSelectionMode == LayerSelectionMode_Ask) &&
        (selectionOverrideMode == SelectionOverrideMode_Replace ||
         selectionOverrideMode == SelectionOverrideMode_Add)
    );

    KisCanvas2 *canvas = inputManager()->canvas();
    KisLayerSelectionAction::select(
        canvas,
        canvas->coordinatesConverter()->widgetToImage(eventPosF(event)).toPoint(),
        eventPos(event),
        layerSelectionMode,
        selectionOverrideMode);
}
