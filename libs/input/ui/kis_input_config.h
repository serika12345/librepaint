/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_INPUT_CONFIG_H
#define KIS_INPUT_CONFIG_H

#include <QString>

#include <KConfigGroup>

class KisInputConfig
{
public:
    explicit KisInputConfig(bool readOnly);
    ~KisInputConfig();

    bool disableTouchOnCanvas() const;
    bool useRightMiddleTabletButtonWorkaround() const;
    int tabletEventsDelay() const;
    bool trackTabletEventLatency() const;
    bool ignoreHighFunctionKeys() const;
    bool testingAcceptCompressedTabletEvents() const;
    bool testingCompressBrushEvents() const;
    bool shouldEatDriverShortcuts() const;
    bool useTimestampsForBrushSpeed() const;
    int speedValueSmoothing() const;

    QString currentInputProfile() const;
    void setCurrentInputProfile(const QString &name);

private:
    KConfigGroup m_config;
    bool m_readOnly;
};

#endif
