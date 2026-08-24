/*
 *  This file is part of the KDE project
 *  SPDX-FileCopyrightText: 2012 Arjen Hiemstra <ahiemstra@heimr.nl>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#ifndef KISTOUCHSHORTCUT_H
#define KISTOUCHSHORTCUT_H

#include "kis_abstract_shortcut.h"

#include <functional>

#include <KisTouchGestureType.h>

class QTouchEvent;
/**
 * @brief The KisTouchShortcut class only handles touch gestures
 * it _does not_ handle tool invocation i.e painting (which is being
 * handled in KisShortcutMatcher).
 */
class KRITAINPUT_EXPORT KisTouchShortcut : public KisAbstractShortcut
{
    public:
        KisTouchShortcut(KisInputAction* action,
                         int index,
                         KisTouchGestureType type);
        ~KisTouchShortcut() override;

        int priority() const override;
        bool isHoldType() const;

        void setMinimumTouchPoints( int min );
        void setMaximumTouchPoints( int max );
        void setDisabledWhenTouchPaintingActive(bool value);
        void setTouchPaintingActiveCallback(std::function<bool()> callback);

        bool matchTapType(QTouchEvent *event);
        bool matchDragType(QTouchEvent *event);
        bool matchHoldType(QTouchEvent *event);
        bool matchTouchPoint(QTouchEvent *event);

    private:
        class Private;
        Private * const d;
};

#endif // KISTOUCHSHORTCUT_H
