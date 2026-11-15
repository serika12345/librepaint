/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_WINTAB_MAPPING_H
#define KIS_WINTAB_MAPPING_H

#include "kritainputui_export.h"

#include <QPointF>
#include <QRect>
#include <QVector>

/**
 * The tablet range mapping that the Qt fork commit 051139ed72 added to
 * QWindowsTabletSupport. Official Qt 6.11.3 always scales the packet to the
 * whole virtual desktop. When the driver maps the tablet to one screen or to
 * a part of the screen, the position is off by more than 20 px and Qt falls
 * back to the integer mouse position.
 *
 * nativePosition() repeats QWindowsTabletDeviceData::scaleCoordinates() of
 * official Qt and replaces only the target rectangle, including the sign
 * handling for the x and y extents. logicalPosition() converts the native
 * pixel position with the screen that contains it.
 *
 * This class is pure computation so it builds and is tested on every
 * platform. The Wintab call and the native event filter live in
 * KisWintabPacketPeeker.
 */
class KRITAINPUTUI_EXPORT KisWintabMapping
{
public:
    enum class Mode {
        Wintab,        // lcSysOrg/lcSysExt from the current Wintab context
        VirtualScreen, // the union of the screens; correction is a no-op
        Custom         // the rectangle saved by the settings page
    };

    /**
     * The part of LOGCONTEXTW the mapping needs. minX/minY of Qt are always
     * 0 because the context requests the packet range from the origin.
     */
    struct Range {
        int inOrgX = 0;
        int inOrgY = 0;
        int inExtX = 0;
        int inExtY = 0;
        int sysOrgX = 0;
        int sysOrgY = 0;
        int sysExtX = 0;
        int sysExtY = 0;
    };

    struct Screen {
        QRect nativeRect;
        QRect logicalRect;
        qreal devicePixelRatio = 1.0;
    };

    /**
     * The settings page and the startup path set the mode here. The value is
     * read by KisWintabPacketPeeker for every WT_PACKET message.
     */
    static void setConfiguredMode(Mode mode, const QRect &customRect);
    static Mode configuredMode();
    static QRect configuredCustomRect();

    /**
     * The rectangle the packet is scaled to. Wintab uses the system
     * rectangle of the driver; an empty system rectangle falls back to the
     * virtual desktop so that a driver without one keeps Qt's result.
     */
    static QRect targetRect(Mode mode, const Range &range,
                            const QRect &virtualDesktopRect,
                            const QRect &customRect);

    /**
     * The native pixel position of a packet, with Qt's formula. A packet
     * outside the input range is clamped to the edge of the target.
     */
    static QPointF nativePosition(const Range &range, const QRect &targetRect,
                                  int packetX, int packetY);

    /**
     * The logical position on the screen that contains the native position.
     * A position outside every screen uses the nearest screen.
     */
    static QPointF logicalPosition(const QPointF &nativePosition,
                                   const QVector<Screen> &screens);
};

#endif // KIS_WINTAB_MAPPING_H
