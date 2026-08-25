/*
 *  SPDX-FileCopyrightText: 2016 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_callback_based_paintop_property.h"

#include "kis_combo_based_paintop_property.h"
#include "kis_paintop_settings.h"
#include "kis_slider_based_paintop_property.h"
#include "kis_uniform_paintop_property.h"

template<class ParentClass>
KisCallbackBasedPaintopProperty<ParentClass>::KisCallbackBasedPaintopProperty(typename ParentClass::Type type,
                                                                              typename ParentClass::SubType subType,
                                                                              const KoID &id,
                                                                              KisPaintOpSettingsRestrictedSP settings,
                                                                              QObject *parent)
    : ParentClass(type, subType, id, settings, parent)
{
}

template<class ParentClass>
KisCallbackBasedPaintopProperty<ParentClass>::KisCallbackBasedPaintopProperty(typename ParentClass::Type type,
                                                                              const KoID &id,
                                                                              KisPaintOpSettingsRestrictedSP settings,
                                                                              QObject *parent)
    : ParentClass(type, id, settings, parent)
{
}

template<class ParentClass>
KisCallbackBasedPaintopProperty<ParentClass>::KisCallbackBasedPaintopProperty(const KoID &id, KisPaintOpSettingsRestrictedSP settings, QObject *parent)
    : ParentClass(id, settings, parent)
{
}

template <class ParentClass>
void KisCallbackBasedPaintopProperty<ParentClass>::setReadCallback(Callback func)
{
    m_readFunc = func;
}

template <class ParentClass>
void KisCallbackBasedPaintopProperty<ParentClass>::setWriteCallback(Callback func)
{
    m_writeFunc = func;
}

template <class ParentClass>
void KisCallbackBasedPaintopProperty<ParentClass>::setIsVisibleCallback(VisibleCallback func)
{
    m_visibleFunc = func;
}

template <class ParentClass>
void KisCallbackBasedPaintopProperty<ParentClass>::readValueImpl()
{
    if (m_readFunc) m_readFunc(this);
}

template <class ParentClass>
void KisCallbackBasedPaintopProperty<ParentClass>::writeValueImpl()
{
    if (m_writeFunc) m_writeFunc(this);
}

template <class ParentClass>
bool KisCallbackBasedPaintopProperty<ParentClass>::isVisible() const
{
    return m_visibleFunc ? m_visibleFunc(this) : true;
}

template class KRITAIMAGE_EXPORT_INSTANCE
    KisCallbackBasedPaintopProperty<KisUniformPaintOpProperty>;
template class KRITAIMAGE_EXPORT_INSTANCE
    KisCallbackBasedPaintopProperty<KisComboBasedPaintOpProperty>;
template class KRITAIMAGE_EXPORT_INSTANCE
    KisCallbackBasedPaintopProperty<KisSliderBasedPaintOpProperty<int>>;
template class KRITAIMAGE_EXPORT_INSTANCE
    KisCallbackBasedPaintopProperty<KisSliderBasedPaintOpProperty<qreal>>;
