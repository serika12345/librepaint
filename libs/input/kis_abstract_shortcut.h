/*
 *  SPDX-FileCopyrightText: 2012 Arjen Hiemstra <ahiemstra@heimr.nl>
 *  SPDX-FileCopyrightText: 2012 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef __KIS_ABSTRACT_SHORTCUT_H
#define __KIS_ABSTRACT_SHORTCUT_H

#include <Qt>
#include <QSet>
#include <kritainput_export.h>
#include "KisInputActionGroup.h"

class KisInputAction;


class KRITAINPUT_EXPORT KisAbstractShortcut
{
public:
    KisAbstractShortcut(KisInputAction *action, int index);
    virtual ~KisAbstractShortcut();

    /**
     * The priority of the shortcut. The shortcut with the
     * greatest value will be chosen for execution
     */
    virtual int priority() const = 0;

    /**
     * The action associated with this shortcut.
     */
    KisInputAction* action() const;

    /**
     * Set the action associated with this shortcut.
     */
    void setAction(KisInputAction *action);

    /**
     * The index of the shortcut.
     *
     * \see KisInputAction::begin()
     */
    int shortcutIndex() const;

    /**
     * Returns true if the shortcut is enabled at the moment
     */
    bool isAvailable(KisInputActionGroupsMask mask) const;

protected:
    bool compareKeys(const QSet<Qt::Key> &keys1,
                     const QSet<Qt::Key> &keys2);

private:
    class Private;
    Private * const m_d;
};

#endif /* __KIS_ABSTRACT_SHORTCUT_H */
