/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_WINTAB_PACKET_PEEKER_H
#define KIS_WINTAB_PACKET_PEEKER_H

#include "kritainputui_export.h"
#include "KisWintabMapping.h"
#include "KisWintabTrust.h"

#include <QAbstractNativeEventFilter>
#include <QPointF>
#include <QVector>

#include <array>
#include <memory>

class QTabletEvent;

/**
 * The Wintab calls the packet peeker needs. The Windows implementation loads
 * wintab32.dll with QLibrary. The tests insert a fake so the contract runs on
 * the platforms that have no Windows SDK.
 */
class KRITAINPUTUI_EXPORT KisWintabApi
{
public:
    struct Packet {
        qint32 x = 0;
        qint32 y = 0;
        quint32 time = 0;
        quint32 buttons = 0;
    };

    /**
     * The fields of LOGCONTEXTW that decide the packet range and the system
     * rectangle, and the packet mask the context was opened with.
     */
    struct Context {
        quint32 packetData = 0;
        qint32 inOrgX = 0;
        qint32 inOrgY = 0;
        qint32 inExtX = 0;
        qint32 inExtY = 0;
        qint32 sysOrgX = 0;
        qint32 sysOrgY = 0;
        qint32 sysExtX = 0;
        qint32 sysExtY = 0;
    };

    virtual ~KisWintabApi() = default;

    virtual bool getContext(quintptr context, Context *data) = 0;

    /**
     * WTPacketsPeek: copy at most maxPackets queued packets without removing
     * them, so Qt's own WTPacketsGet still receives the same packets.
     */
    virtual int packetsPeek(quintptr context, Packet *packets,
                            int maxPackets) = 0;
};

/**
 * Reads the Wintab packet queue before Qt does and keeps the corrected
 * position of the recent packets by pkTime.
 *
 * Q4 stage 4 (fork commit 051139ed72): official Qt scales the packet to the
 * virtual desktop. This filter pairs each WT_PACKET with a corrected logical
 * position; KisInputManager replaces only the position of the QTabletEvent
 * whose timestamp matches. The filter always returns false so Qt keeps
 * handling the message and calls WTPacketsGet as before.
 *
 * The file builds on every platform. Only the Windows messages and the
 * wintab32.dll calls are inside Q_OS_WIN, so the package runs without the
 * Windows SDK on Linux and macOS.
 */
class KRITAINPUTUI_EXPORT KisWintabPacketPeeker : public QAbstractNativeEventFilter
{
public:
    // Wintab 1.4 identifiers. They repeat wintab.h and pktdef.h so this file
    // builds without the Windows SDK.
    static constexpr quint32 PacketMessage = 0x7FF0; // WT_PACKET = WT_DEFBASE + 0
    static constexpr quint32 PacketData =
        0x0004 | // PK_TIME
        0x0020 | // PK_CURSOR
        0x0040 | // PK_BUTTONS
        0x0080 | // PK_X
        0x0100 | // PK_Y
        0x0200 | // PK_Z
        0x0400 | // PK_NORMAL_PRESSURE
        0x0800 | // PK_TANGENT_PRESSURE
        0x1000;  // PK_ORIENTATION

    static constexpr int EntryCount = 256;

    KisWintabPacketPeeker();
    explicit KisWintabPacketPeeker(KisWintabApi *api);
    ~KisWintabPacketPeeker() override;

    bool nativeEventFilter(const QByteArray &eventType, void *message,
                           qintptr *result) override;

    /**
     * The message handling without the Windows MSG, so the contract test can
     * drive it. Returns true when a WT_PACKET message was read.
     */
    bool handleNativeMessage(quint32 message, quintptr wParam, qintptr lParam);

    /**
     * The corrected logical position of the oldest unused packet with this
     * timestamp. Marks the packet as used, so a second event with the same
     * timestamp (the Wintab clock has a 1 ms resolution and the pen reports
     * faster) takes the next packet in arrival order.
     */
    bool lookup(quint64 timestamp, QPointF *logicalPosition);

    /**
     * Builds a copy of the event with only the local and global positions
     * corrected. Returns null when no packet matches the timestamp or the
     * proximity distrusted the driver's range, so the caller keeps the
     * original event.
     *
     * On the first TabletPress of a proximity the corrected position is
     * compared with @p cursorPosition, the logical QCursor::pos(), and the
     * decision is recorded as wintab_trust.
     */
    std::unique_ptr<QTabletEvent> correctedTabletEvent(
        const QTabletEvent &event, const QPointF &cursorPosition);

    /**
     * Starts a new proximity from QEvent::TabletEnterProximity. The trust
     * check returns to Undecided.
     */
    void beginProximity();

    /**
     * Ends the proximity from QEvent::TabletLeaveProximity.
     */
    void endProximity();

    /**
     * The screen list for the tests. An empty list uses QGuiApplication.
     */
    void setScreensForTesting(const QVector<KisWintabMapping::Screen> &screens);

    static KisWintabPacketPeeker *instance();

private:
    struct Entry {
        quint64 time = 0;
        qint32 packetX = 0;
        qint32 packetY = 0;
        quint32 buttons = 0;
        QPointF logicalPosition;
        bool valid = false;
        bool used = false;
    };

    QVector<KisWintabMapping::Screen> currentScreens() const;
    void store(const KisWintabApi::Packet &packet,
               const QPointF &logicalPosition);

    KisWintabApi *m_api = nullptr;
    std::unique_ptr<KisWintabApi> m_ownedApi;
    std::array<Entry, EntryCount> m_entries;
    int m_nextEntry = 0;
    std::array<KisWintabApi::Packet, EntryCount> m_packets;
    QVector<KisWintabMapping::Screen> m_testScreens;
    KisWintabTrust m_trust;
    bool m_warnedPacketData = false;

    static KisWintabPacketPeeker *s_instance;
};

#endif // KIS_WINTAB_PACKET_PEEKER_H
