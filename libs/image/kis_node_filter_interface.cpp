/*
 *  SPDX-FileCopyrightText: 2008 Cyrille Berger <cberger@cberger.net>
 *  SPDX-FileCopyrightText: 2012 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_node_filter_interface.h"
#include "KisNodeFilterInterfaceFilterAccess_p.h"

#define SANITY_ACQUIRE_FILTER(filter)                                                                                  \
    do {                                                                                                               \
        if ((filter)) {                                                                                                \
            KisNodeFilterInterfaceFilterAccess::acquire((filter).data());                                              \
        }                                                                                                              \
    } while (0)

#define SANITY_RELEASE_FILTER(filter)                                                                                  \
    do {                                                                                                               \
        if (m_filterConfiguration && KisNodeFilterInterfaceFilterAccess::release(m_filterConfiguration.data())) {      \
            warnKrita;                                                                                                 \
            warnKrita << "WARNING: filter configuration has more than one user! LibrePaint will probably crash soon!"; \
            warnKrita << "WARNING:" << ppVar(this);                                                                    \
            warnKrita << "WARNING:" << ppVar(filter.data());                                                           \
            warnKrita;                                                                                                 \
        }                                                                                                              \
    } while (0)

KisNodeFilterInterface::KisNodeFilterInterface(KisFilterConfigurationSP filterConfig)
    : m_filterConfiguration(filterConfig)
{
    SANITY_ACQUIRE_FILTER(m_filterConfiguration);
    KIS_SAFE_ASSERT_RECOVER_NOOP(!filterConfig
                                 || KisNodeFilterInterfaceFilterAccess::hasLocalResourcesSnapshot(filterConfig.data()));
}

KisNodeFilterInterface::KisNodeFilterInterface(const KisNodeFilterInterface &rhs)
    : m_filterConfiguration(KisNodeFilterInterfaceFilterAccess::clone(rhs.m_filterConfiguration.data()))

{
    SANITY_ACQUIRE_FILTER(m_filterConfiguration);
}

KisNodeFilterInterface::~KisNodeFilterInterface()
{
    SANITY_RELEASE_FILTER(m_filterConfiguration);
}

KisFilterConfigurationSP KisNodeFilterInterface::filter() const
{
    return m_filterConfiguration;
}

void KisNodeFilterInterface::setFilter(KisFilterConfigurationSP filterConfig, bool /*checkCompareConfig*/)
{
    SANITY_RELEASE_FILTER(m_filterConfiguration);

    KIS_SAFE_ASSERT_RECOVER_RETURN(filterConfig);
    KIS_SAFE_ASSERT_RECOVER_NOOP(KisNodeFilterInterfaceFilterAccess::hasLocalResourcesSnapshot(filterConfig.data()));
    m_filterConfiguration = filterConfig;

    SANITY_ACQUIRE_FILTER(m_filterConfiguration);
}

void KisNodeFilterInterface::notifyColorSpaceChanged()
{
    /**
     * On a color space change we need to reset all the
     * caches stored inside filter configuration. The
     * current "standard" way for doing that is just to
     * clone the configuration (which clones everything
     * except of the caches). We should probably invent
     * a better approach for that cache resetting later.
     */

    if (m_filterConfiguration) {
        m_filterConfiguration = KisNodeFilterInterfaceFilterAccess::clone(m_filterConfiguration.data());
    }
}
