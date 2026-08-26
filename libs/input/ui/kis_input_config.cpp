/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_input_config.h"

#include <KSharedConfig>
#include <KoPointerEvent.h>

KisInputConfig::KisInputConfig(bool readOnly)
    : m_config(KSharedConfig::openConfig()->group(QString()))
    , m_readOnly(readOnly)
{
}

KisInputConfig::~KisInputConfig()
{
    if (!m_readOnly) {
        m_config.sync();
    }
}

bool KisInputConfig::disableTouchOnCanvas() const
{
    enum TouchPainting {
        TouchPaintingAuto,
        TouchPaintingEnabled,
        TouchPaintingDisabled,
    };

    switch (m_config.readEntry("touchPainting", int(TouchPaintingAuto))) {
    case TouchPaintingEnabled:
        return false;
    case TouchPaintingDisabled:
        return true;
    default:
        return KoPointerEvent::tabletInputReceived();
    }
}

bool KisInputConfig::useRightMiddleTabletButtonWorkaround() const
{
    return m_config.readEntry("useRightMiddleTabletButtonWorkaround", false);
}

int KisInputConfig::tabletEventsDelay() const
{
    return m_config.readEntry("tabletEventsDelay", 10);
}

bool KisInputConfig::trackTabletEventLatency() const
{
    return m_config.readEntry("trackTabletEventLatency", false);
}

bool KisInputConfig::ignoreHighFunctionKeys() const
{
    return m_config.readEntry("ignoreHighFunctionKeys", true);
}

bool KisInputConfig::testingAcceptCompressedTabletEvents() const
{
    return m_config.readEntry("testingAcceptCompressedTabletEvents", false);
}

bool KisInputConfig::testingCompressBrushEvents() const
{
    return m_config.readEntry("testingCompressBrushEvents", false);
}

bool KisInputConfig::shouldEatDriverShortcuts() const
{
    return m_config.readEntry("shouldEatDriverShortcuts", false);
}

bool KisInputConfig::useTimestampsForBrushSpeed() const
{
    return m_config.readEntry("useTimestampsForBrushSpeed", false);
}

int KisInputConfig::speedValueSmoothing() const
{
    return m_config.readEntry("speedValueSmoothing", 3);
}

QString KisInputConfig::currentInputProfile() const
{
    return m_config.readEntry("currentInputProfile", QString());
}

void KisInputConfig::setCurrentInputProfile(const QString &name)
{
    m_config.writeEntry("currentInputProfile", name);
}
