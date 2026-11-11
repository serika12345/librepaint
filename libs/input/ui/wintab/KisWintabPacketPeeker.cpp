/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisWintabPacketPeeker.h"

#include <QDebug>
#include <QGuiApplication>
#include <QScreen>
#include <QTabletEvent>
#include <qpa/qplatformscreen.h>

#include <KisLatencyProbe.h>

#ifdef Q_OS_WIN
#include <windows.h>
#include <QtCore/QLibrary>
#endif

KisWintabPacketPeeker *KisWintabPacketPeeker::s_instance = nullptr;

#ifdef Q_OS_WIN

namespace
{

constexpr int LogContextNameLength = 40; // LCNAMELEN

// Wintab's context handle. windows.h has no HCTX.
using WintabContextHandle = HANDLE;

/**
 * LOGCONTEXTW from wintab.h 1.4. The layout is repeated here because the
 * project does not carry the Windows SDK's wintab header. Only the fields the
 * mapping reads are used; the others keep the offsets right.
 */
struct WintabLogContext {
    WCHAR lcName[LogContextNameLength];
    quint32 lcOptions;
    quint32 lcStatus;
    quint32 lcLocks;
    quint32 lcMsgBase;
    quint32 lcDevice;
    quint32 lcPktRate;
    quint32 lcPktData;
    quint32 lcPktMode;
    quint32 lcMoveMask;
    quint32 lcBtnDnMask;
    quint32 lcBtnUpMask;
    qint32 lcInOrgX;
    qint32 lcInOrgY;
    qint32 lcInOrgZ;
    qint32 lcInExtX;
    qint32 lcInExtY;
    qint32 lcInExtZ;
    qint32 lcOutOrgX;
    qint32 lcOutOrgY;
    qint32 lcOutOrgZ;
    qint32 lcOutExtX;
    qint32 lcOutExtY;
    qint32 lcOutExtZ;
    quint32 lcSensX;
    quint32 lcSensY;
    quint32 lcSensZ;
    qint32 lcSysMode;
    qint32 lcSysOrgX;
    qint32 lcSysOrgY;
    qint32 lcSysExtX;
    qint32 lcSysExtY;
    quint32 lcSysSensX;
    quint32 lcSysSensY;
};

/**
 * PACKET from pktdef.h for the requested mask
 * PK_TIME | PK_CURSOR | PK_BUTTONS | PK_X | PK_Y | PK_Z |
 * PK_NORMAL_PRESSURE | PK_TANGENT_PRESSURE | PK_ORIENTATION.
 */
struct WintabPacket {
    quint32 pkTime;
    quint32 pkCursor;
    quint32 pkButtons;
    qint32 pkX;
    qint32 pkY;
    qint32 pkZ;
    quint32 pkNormalPressure;
    quint32 pkTangentPressure;
    struct {
        quint32 azimuth;
        quint32 altitude;
        quint32 twist;
    } pkOrientation;
};

static_assert(sizeof(WintabLogContext) == 212,
              "LOGCONTEXTW layout must match wintab.h");
static_assert(sizeof(WintabPacket) == 44,
              "PACKET layout must match pktdef.h");

class WintabApiWindows : public KisWintabApi
{
public:
    WintabApiWindows()
    {
        m_library = new QLibrary(QStringLiteral("wintab32"));
        if (m_library->load()) {
            m_getContext = reinterpret_cast<GetContextFunction>(
                m_library->resolve("WTGetW"));
            m_packetsPeek = reinterpret_cast<PacketsPeekFunction>(
                m_library->resolve("WTPacketsPeek"));
        }
    }

    ~WintabApiWindows() override
    {
        delete m_library;
    }

    bool getContext(quintptr context, Context *data) override
    {
        if (!m_getContext) {
            return false;
        }

        WintabLogContext logContext;
        memset(&logContext, 0, sizeof(logContext));

        if (!m_getContext(reinterpret_cast<WintabContextHandle>(context), &logContext)) {
            return false;
        }

        data->packetData = logContext.lcPktData;
        data->inOrgX = logContext.lcInOrgX;
        data->inOrgY = logContext.lcInOrgY;
        data->inExtX = logContext.lcInExtX;
        data->inExtY = logContext.lcInExtY;
        data->sysOrgX = logContext.lcSysOrgX;
        data->sysOrgY = logContext.lcSysOrgY;
        data->sysExtX = logContext.lcSysExtX;
        data->sysExtY = logContext.lcSysExtY;
        return true;
    }

    int packetsPeek(quintptr context, Packet *packets, int maxPackets) override
    {
        if (!m_packetsPeek || maxPackets <= 0) {
            return 0;
        }

        const int capped = qMin(maxPackets, int(m_rawPackets.size()));
        const int count = m_packetsPeek(reinterpret_cast<WintabContextHandle>(context),
                                        capped, m_rawPackets.data());
        const int copied = qBound(0, count, capped);

        for (int i = 0; i < copied; ++i) {
            packets[i].x = m_rawPackets[i].pkX;
            packets[i].y = m_rawPackets[i].pkY;
            packets[i].time = m_rawPackets[i].pkTime;
            packets[i].buttons = m_rawPackets[i].pkButtons;
        }

        return copied;
    }

private:
    using GetContextFunction = BOOL (WINAPI *)(WintabContextHandle, WintabLogContext *);
    using PacketsPeekFunction = int (WINAPI *)(WintabContextHandle, int, WintabPacket *);

    QLibrary *m_library = nullptr;
    GetContextFunction m_getContext = nullptr;
    PacketsPeekFunction m_packetsPeek = nullptr;
    std::array<WintabPacket, KisWintabPacketPeeker::EntryCount> m_rawPackets;
};

} // namespace

#endif // Q_OS_WIN

KisWintabPacketPeeker::KisWintabPacketPeeker()
#ifdef Q_OS_WIN
    : m_ownedApi(std::make_unique<WintabApiWindows>())
#endif
{
#ifdef Q_OS_WIN
    m_api = m_ownedApi.get();
#endif
    s_instance = this;
}

KisWintabPacketPeeker::KisWintabPacketPeeker(KisWintabApi *api)
    : m_api(api)
{
    s_instance = this;
}

KisWintabPacketPeeker::~KisWintabPacketPeeker()
{
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

KisWintabPacketPeeker *KisWintabPacketPeeker::instance()
{
    return s_instance;
}

void KisWintabPacketPeeker::setScreensForTesting(
    const QVector<KisWintabMapping::Screen> &screens)
{
    m_testScreens = screens;
}

QVector<KisWintabMapping::Screen> KisWintabPacketPeeker::currentScreens() const
{
    if (!m_testScreens.isEmpty()) {
        return m_testScreens;
    }

    QVector<KisWintabMapping::Screen> screens;

    if (!QGuiApplication::instance()) {
        return screens;
    }

    const QList<QScreen *> guiScreens = QGuiApplication::screens();
    screens.reserve(guiScreens.size());

    for (QScreen *screen : guiScreens) {
        KisWintabMapping::Screen entry;
        entry.logicalRect = screen->geometry();
        entry.nativeRect = screen->handle()
            ? screen->handle()->geometry() : entry.logicalRect;
        entry.devicePixelRatio = screen->devicePixelRatio();
        screens.append(entry);
    }

    return screens;
}

void KisWintabPacketPeeker::store(const KisWintabApi::Packet &packet,
                                  const QPointF &logicalPosition)
{
    // WTPacketsPeek does not remove the packet, so the next WT_PACKET message
    // sees the same queue again. pkSerialNumber is not part of PACKETDATA;
    // the time, position and button bits identify the packet. A used entry
    // stays in the table, so a repeated peek does not add a second copy.
    for (const Entry &entry : m_entries) {
        if (entry.valid &&
            entry.time == packet.time &&
            entry.packetX == packet.x &&
            entry.packetY == packet.y &&
            entry.buttons == packet.buttons) {
            return;
        }
    }

    Entry &entry = m_entries[m_nextEntry];
    entry.time = packet.time;
    entry.packetX = packet.x;
    entry.packetY = packet.y;
    entry.buttons = packet.buttons;
    entry.logicalPosition = logicalPosition;
    entry.valid = true;
    entry.used = false;
    m_nextEntry = (m_nextEntry + 1) % EntryCount;
}

bool KisWintabPacketPeeker::lookup(quint64 timestamp,
                                   QPointF *logicalPosition)
{
    // m_nextEntry is the oldest slot, so scanning from there returns the
    // packets of one timestamp in the order they arrived.
    for (int i = 0; i < EntryCount; ++i) {
        Entry &entry = m_entries[(m_nextEntry + i) % EntryCount];

        if (entry.valid && !entry.used && entry.time == timestamp) {
            entry.used = true;
            if (logicalPosition) {
                *logicalPosition = entry.logicalPosition;
            }
            return true;
        }
    }
    return false;
}

void KisWintabPacketPeeker::beginProximity()
{
    m_trust.beginProximity();
}

void KisWintabPacketPeeker::endProximity()
{
    m_trust.endProximity();
}

bool KisWintabPacketPeeker::handleNativeMessage(quint32 message,
                                                quintptr wParam,
                                                qintptr lParam)
{
    Q_UNUSED(wParam);

    if (message != PacketMessage || !m_api) {
        return false;
    }

    const quintptr contextHandle = quintptr(lParam);

    KisWintabApi::Context context;
    if (!m_api->getContext(contextHandle, &context)) {
        return false;
    }

    if (context.packetData != PacketData) {
        if (!m_warnedPacketData) {
            m_warnedPacketData = true;
            qWarning("KisWintabPacketPeeker: the Wintab packet mask is 0x%x, "
                     "not 0x%x; the tablet range correction stays off",
                     context.packetData, PacketData);
        }
        return false;
    }

    const int count = m_api->packetsPeek(contextHandle, m_packets.data(),
                                         int(m_packets.size()));
    if (count <= 0) {
        return false;
    }

    const QVector<KisWintabMapping::Screen> screens = currentScreens();
    if (screens.isEmpty()) {
        return false;
    }

    QRect virtualDesktop;
    for (const KisWintabMapping::Screen &screen : screens) {
        virtualDesktop |= screen.nativeRect;
    }

    KisWintabMapping::Range range;
    range.inOrgX = context.inOrgX;
    range.inOrgY = context.inOrgY;
    range.inExtX = context.inExtX;
    range.inExtY = context.inExtY;
    range.sysOrgX = context.sysOrgX;
    range.sysOrgY = context.sysOrgY;
    range.sysExtX = context.sysExtX;
    range.sysExtY = context.sysExtY;

    const QRect target = KisWintabMapping::targetRect(
        KisWintabMapping::configuredMode(), range, virtualDesktop,
        KisWintabMapping::configuredCustomRect());

    const bool correction = target.isValid() && target != virtualDesktop;

    KisLatencyProbe::mark("wintab_peek", correction ? 1 : 0, count);

    if (!correction) {
        return true;
    }

    for (int i = 0; i < count; ++i) {
        const KisWintabApi::Packet &packet = m_packets[i];
        const QPointF nativePosition = KisWintabMapping::nativePosition(
            range, target, packet.x, packet.y);
        store(packet,
              KisWintabMapping::logicalPosition(nativePosition, screens));
    }

    return true;
}

std::unique_ptr<QTabletEvent> KisWintabPacketPeeker::correctedTabletEvent(
    const QTabletEvent &event, const QPointF &cursorPosition)
{
    // Qt can miss the proximity enter event; the first tablet event then
    // starts the proximity.
    m_trust.noteTabletEvent();

    if (!m_trust.correctionAllowed()) {
        return nullptr;
    }

    QPointF correctedPosition;
    const bool found = lookup(event.timestamp(), &correctedPosition);

    KisLatencyProbe::mark("wintab_fix", found ? 1 : 0, found ? 0 : 1);

    if (!found) {
        return nullptr;
    }

    // The first press of a proximity decides whether the driver's range is
    // right. A distrusted proximity keeps the Qt position from this press
    // until the end of the proximity.
    if (event.type() == QEvent::TabletPress &&
        m_trust.state() == KisWintabTrust::State::Undecided) {

        const KisWintabTrust::State state =
            m_trust.decide(correctedPosition, cursorPosition);

        KisLatencyProbe::mark(
            "wintab_trust", state == KisWintabTrust::State::Trusted ? 1 : 0,
            qRound(m_trust.differencePixels() * 10.0));

        if (state != KisWintabTrust::State::Trusted) {
            return nullptr;
        }
    }

    const QPointF delta = correctedPosition - event.globalPosition();
    if (delta.isNull()) {
        return nullptr;
    }

    auto corrected = std::make_unique<QTabletEvent>(
        event.type(), event.pointingDevice(),
        event.position() + delta, correctedPosition,
        event.pressure(), event.xTilt(), event.yTilt(),
        event.tangentialPressure(), event.rotation(), event.z(),
        event.modifiers(), event.button(), event.buttons());
    corrected->setTimestamp(event.timestamp());
    corrected->setAccepted(event.isAccepted());
    return corrected;
}

bool KisWintabPacketPeeker::nativeEventFilter(const QByteArray &eventType,
                                              void *message,
                                              qintptr *result)
{
    Q_UNUSED(result);

#ifdef Q_OS_WIN
    if (eventType != "windows_generic_MSG") {
        return false;
    }

    const MSG *msg = static_cast<const MSG *>(message);
    handleNativeMessage(msg->message, quintptr(msg->wParam),
                        qintptr(msg->lParam));
#else
    Q_UNUSED(eventType);
    Q_UNUSED(message);
#endif

    // The filter only reads the queue: Qt's own WTPacketsGet must still take
    // the same packets out.
    return false;
}
