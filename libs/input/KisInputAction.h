/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_INPUT_ACTION_H
#define KIS_INPUT_ACTION_H

#include <kritainput_export.h>

#include <KisInputActionGroup.h>

class QEvent;

/**
 * The command contract used by input sequence matching.
 *
 * The matcher borrows the action for as long as its shortcuts are registered.
 * The caller owns the action and must keep it alive until the matcher is
 * destroyed or cleared.
 */
class KRITAINPUT_EXPORT KisInputAction
{
public:
    virtual ~KisInputAction() = default;

    virtual void activate(int shortcut) = 0;
    virtual void deactivate(int shortcut) = 0;
    virtual void begin(int shortcut, QEvent *event) = 0;
    virtual void end(QEvent *event) = 0;
    virtual void inputEvent(QEvent *event) = 0;

    virtual bool supportsHiResInputEvents(int shortcut) const = 0;
    virtual KisInputActionGroup inputActionGroup(int shortcut) const = 0;
    virtual int priority() const = 0;
    virtual bool canIgnoreModifiers() const = 0;
    virtual bool isAvailable() const = 0;
};

#endif
