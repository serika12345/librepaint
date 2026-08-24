/*
 *  This file is part of the KDE project
 *  SPDX-FileCopyrightText: 2012 Arjen Hiemstra <ahiemstra@heimr.nl>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *
 */

#include <kis_touch_shortcut.h>

#include <KisInputAction.h>

#include <QTouchEvent>

#include <utility>

class KisTouchShortcut::Private
{
public:
    Private(KisTouchGestureType type)
        : minTouchPoints(0)
        , maxTouchPoints(0)
        , type(type)
        , disabledWhenTouchPaintingActive(false)
    { }

    int minTouchPoints;
    int maxTouchPoints;
    KisTouchGestureType type;
    bool disabledWhenTouchPaintingActive;
    std::function<bool()> touchPaintingActive = []() { return false; };
};

KisTouchShortcut::KisTouchShortcut(KisInputAction* action,
                                   int index,
                                   KisTouchGestureType type)
    : KisAbstractShortcut(action, index)
    , d(new Private(type))
{

}

KisTouchShortcut::~KisTouchShortcut()
{
    delete d;
}

int KisTouchShortcut::priority() const
{
    return action()->priority();
}

bool KisTouchShortcut::isHoldType() const
{
    return d->type == KisTouchGestureType::Hold;
}

void KisTouchShortcut::setMinimumTouchPoints(int min)
{
    d->minTouchPoints = min;
}

void KisTouchShortcut::setMaximumTouchPoints(int max)
{
    d->maxTouchPoints = max;
}

void KisTouchShortcut::setDisabledWhenTouchPaintingActive(bool value)
{
    d->disabledWhenTouchPaintingActive = value;
}

void KisTouchShortcut::setTouchPaintingActiveCallback(
    std::function<bool()> callback)
{
    d->touchPaintingActive = callback
        ? std::move(callback)
        : []() { return false; };
}

bool KisTouchShortcut::matchTapType(QTouchEvent *event)
{
    return matchTouchPoint(event)
        && d->type == KisTouchGestureType::Tap;
}

bool KisTouchShortcut::matchDragType(QTouchEvent *event)
{
    return matchTouchPoint(event)
        && d->type == KisTouchGestureType::Drag;
}

bool KisTouchShortcut::matchHoldType(QTouchEvent *event)
{
    return isHoldType() && matchTouchPoint(event);
}

bool KisTouchShortcut::matchTouchPoint(QTouchEvent *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const int touchPointCount = event->points().count();
#else
    const int touchPointCount = event->touchPoints().count();
#endif

    return (!d->disabledWhenTouchPaintingActive || !d->touchPaintingActive())
        && touchPointCount >= d->minTouchPoints
        && touchPointCount <= d->maxTouchPoints;
}
