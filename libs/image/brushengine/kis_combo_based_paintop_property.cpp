/*
 *  SPDX-FileCopyrightText: 2016 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_combo_based_paintop_property.h"

#include "QIcon"
#include "kis_assert.h"
#include "kis_callback_based_paintop_property.h"
#include "kis_types.h"
#include "kis_uniform_paintop_property.h"
#include <qhashfunctions.h>
#include <qlist.h>
#include <qobject.h>


struct KisComboBasedPaintOpProperty::Private
{
    QList<QString> items;
    QList<QIcon> icons;
};

KisComboBasedPaintOpProperty::KisComboBasedPaintOpProperty(const KoID &id, KisPaintOpSettingsRestrictedSP settings, QObject *parent)
    : KisUniformPaintOpProperty(Combo, id, settings, parent)
    , m_d(new Private)
{
}

KisComboBasedPaintOpProperty::KisComboBasedPaintOpProperty(Type type, const KoID &id, KisPaintOpSettingsRestrictedSP settings, QObject *parent)
    : KisUniformPaintOpProperty(Combo, id, settings, parent)
    , m_d(new Private)
{
    KIS_ASSERT_RECOVER_RETURN(type == Combo);
}

KisComboBasedPaintOpProperty::KisComboBasedPaintOpProperty(Type type, SubType subType, const KoID &id, KisPaintOpSettingsRestrictedSP settings, QObject *parent)
    : KisUniformPaintOpProperty(Combo, subType, id, settings, parent)
    , m_d(new Private)
{
    KIS_ASSERT_RECOVER_RETURN(type == Combo);
}

KisComboBasedPaintOpProperty::~KisComboBasedPaintOpProperty()
{
}

QList<QString> KisComboBasedPaintOpProperty::items() const
{
    return m_d->items;
}

void KisComboBasedPaintOpProperty::setItems(const QList<QString> &list)
{
    m_d->items = list;
}

QList<QIcon> KisComboBasedPaintOpProperty::icons() const
{
    return m_d->icons;
}

void KisComboBasedPaintOpProperty::setIcons(const QList<QIcon> &list)
{
    m_d->icons = list;
}
