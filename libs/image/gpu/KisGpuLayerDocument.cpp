/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuLayerDocument.h"
#include "KisGpuEditReplay_p.h"
#include <limits>
#include <set>
#include <stdexcept>

KisGpuLayerDocument::KisGpuLayerDocument(std::shared_ptr<KisGpuTileStore> store, qsizetype retained, quint64 maximum)
    : m_store(std::move(store)), m_retainedEdits(qMax(qsizetype(0), retained)), m_maximumRecoveryBytes(maximum)
    , m_history{Frame{}}
{
    if (!m_store || historyBytes(m_history) > maximum) {
        throw std::invalid_argument("GPU layer document requires its store and a CPU history payload budget");
    }
}

KisGpuLayerDocument::~KisGpuLayerDocument()
{
    if (m_active) m_session->cancel(m_active->token);
}

KisGpuLayerDocument::State KisGpuLayerDocument::state() const
{
    if (m_restoring) return m_restoring->failed || !m_store->deviceAvailable() ? State::Failed : State::Restoring;
    if (!m_store->deviceAvailable() || (m_session && m_session->state() == KisGpuEditSession::State::Failed)) return State::Failed;
    if (!m_active) return State::Idle;
    return m_session->state() == KisGpuEditSession::State::Editing ? State::Editing : State::Committing;
}

KisGpuLayerDocument::Result KisGpuLayerDocument::readiness() const
{
    const auto current = state();
    return current == State::Idle ? Result::Accepted : current == State::Failed ? Result::GpuRejected : Result::Busy;
}

qsizetype KisGpuLayerDocument::indexOf(LayerId id) const
{
    const auto &frame = m_history[m_cursor];
    for (qsizetype i = 0; i < frame.size(); ++i) if (frame[i]->id == id) return i;
    return -1;
}

KisGpuTileStore::Version KisGpuLayerDocument::rasterVersion(const RasterRecovery &raster) const
{
    return raster ? m_rasterVersions.at(raster.get()) : KisGpuTileStore::Version{};
}

KisGpuLayerDocument::Layer KisGpuLayerDocument::layerValue(const LayerState &record) const
{
    return {record.id, record.name, {rasterVersion(record.pixels), rasterVersion(record.mask), record.opacity, record.operation}};
}

void KisGpuLayerDocument::pruneRasterVersions()
{
    std::set<const KisGpuEditSession::Recovery *> retained;
    for (const auto &frame : m_history) for (const auto &record : frame) {
        retained.insert(record->pixels.get());
        if (record->mask) retained.insert(record->mask.get());
    }
    for (auto i = m_rasterVersions.begin(); i != m_rasterVersions.end();) {
        if (!retained.count(i->first)) i = m_rasterVersions.erase(i);
        else ++i;
    }
}

quint64 KisGpuLayerDocument::recordMetadataBytes(const LayerState &record) const
{
    return sizeof(LayerState) + quint64(record.name.size()) * sizeof(QChar);
}

quint64 KisGpuLayerDocument::historyBytes(const QVector<Frame> &history) const
{
    std::set<const LayerState *> counted;
    std::set<const KisGpuEditSession::Recovery *> rasters;
    quint64 bytes = quint64(history.size()) * sizeof(Frame);
    for (const auto &frame : history) {
        bytes += quint64(frame.size()) * sizeof(Record);
        for (const auto &record : frame) if (counted.insert(record.get()).second) {
            bytes += recordMetadataBytes(*record);
            for (const auto &raster : {record->pixels, record->mask}) {
                if (raster && rasters.insert(raster.get()).second)
                    bytes += sizeof(KisGpuEditSession::Recovery) + KisGpuEditSession::ReplayState::recoveryBytes(*raster);
            }
        }
    }
    return bytes;
}

quint64 KisGpuLayerDocument::recoveryBytes() const
{
    if (m_restoring) return recoveryBytes(m_restoring->request);
    const auto working = m_session ? m_session->recovery() : std::nullopt;
    return historyBytes(m_history) + (working ? sizeof(KisGpuEditSession::Recovery) + KisGpuEditSession::ReplayState::recoveryBytes(*working) : 0);
}

KisGpuLayerDocument::Result KisGpuLayerDocument::publish(Frame frame)
{
    auto history = m_history;
    history.resize(m_cursor + 1);
    history.push_back(std::move(frame));
    if (history.size() - 1 > m_retainedEdits) history.remove(0, history.size() - 1 - m_retainedEdits);
    if (historyBytes(history) > m_maximumRecoveryBytes) return m_lastResult = Result::RecoveryBudgetExceeded;
    m_history = std::move(history);
    m_cursor = m_history.size() - 1;
    pruneRasterVersions();
    return m_lastResult = Result::Accepted;
}

KisGpuLayerDocument::LayerResult KisGpuLayerDocument::addLayer(QString name, qsizetype index)
{
    const auto ready = readiness();
    if (ready != Result::Accepted) return {ready, 0};
    auto frame = m_history[m_cursor];
    if (index == -1) index = frame.size();
    if (index < 0 || index > frame.size() || m_nextId == std::numeric_limits<LayerId>::max()) return {};
    auto record = std::make_shared<LayerState>();
    record->id = m_nextId;
    record->name = std::move(name);
    const auto pixels = record->pixels;
    frame.insert(index, std::move(record));
    const auto result = publish(std::move(frame));
    if (result == Result::Accepted) m_rasterVersions[pixels.get()] = m_store->emptyVersion();
    return {result, result == Result::Accepted ? m_nextId++ : 0};
}

KisGpuLayerDocument::Result KisGpuLayerDocument::removeLayer(LayerId id)
{
    const auto ready = readiness();
    if (ready != Result::Accepted) return ready;
    const auto index = indexOf(id);
    if (index < 0) return Result::InvalidLayer;
    auto frame = m_history[m_cursor];
    frame.removeAt(index);
    return publish(std::move(frame));
}

KisGpuLayerDocument::Result KisGpuLayerDocument::moveLayer(LayerId id, qsizetype destination)
{
    const auto ready = readiness();
    if (ready != Result::Accepted) return ready;
    const auto index = indexOf(id);
    if (index < 0) return Result::InvalidLayer;
    auto frame = m_history[m_cursor];
    if (destination < 0 || destination >= frame.size()) return Result::InvalidInput;
    if (index == destination) return Result::Accepted;
    const auto record = frame.takeAt(index);
    frame.insert(destination, record);
    return publish(std::move(frame));
}

KisGpuLayerDocument::Result KisGpuLayerDocument::renameLayer(LayerId id, QString name)
{
    const auto ready = readiness();
    if (ready != Result::Accepted) return ready;
    const auto index = indexOf(id);
    if (index < 0) return Result::InvalidLayer;
    auto frame = m_history[m_cursor];
    if (frame[index]->name == name) return Result::Accepted;
    auto record = std::make_shared<LayerState>(*frame[index]);
    record->name = std::move(name);
    frame[index] = std::move(record);
    return publish(std::move(frame));
}

KisGpuLayerDocument::Result KisGpuLayerDocument::setLayerProperties(LayerId id, quint8 opacity, KisGpuTileStore::CompositeOp operation)
{
    const auto ready = readiness();
    if (ready != Result::Accepted) return ready;
    const auto index = indexOf(id);
    if (index < 0) return Result::InvalidLayer;
    if (operation != KisGpuTileStore::CompositeOp::Over && operation != KisGpuTileStore::CompositeOp::Erase) return Result::InvalidInput;
    auto frame = m_history[m_cursor];
    if (frame[index]->opacity == opacity && frame[index]->operation == operation) return Result::Accepted;
    auto record = std::make_shared<LayerState>(*frame[index]);
    record->opacity = opacity;
    record->operation = operation;
    frame[index] = std::move(record);
    return publish(std::move(frame));
}

KisGpuLayerDocument::Result KisGpuLayerDocument::clearMask(LayerId id)
{
    const auto ready = readiness();
    if (ready != Result::Accepted) return ready;
    const auto index = indexOf(id);
    if (index < 0) return Result::InvalidLayer;
    auto frame = m_history[m_cursor];
    if (!frame[index]->mask) return Result::Accepted;
    auto record = std::make_shared<LayerState>(*frame[index]);
    record->mask.reset();
    frame[index] = std::move(record);
    return publish(std::move(frame));
}

KisGpuLayerDocument::Editing KisGpuLayerDocument::begin(LayerId id, Target target)
{
    const auto ready = readiness();
    if (ready != Result::Accepted) return {ready, {}, {}};
    const auto index = indexOf(id);
    if (index < 0) return {Result::InvalidLayer, {}, {}};
    if (target != Target::Pixels && target != Target::Mask) return {Result::InvalidInput, {}, {}};
    const auto &record = *m_history[m_cursor][index];
    const auto recovery = target == Target::Pixels ? *record.pixels : (record.mask ? *record.mask : KisGpuEditSession::Recovery{});
    const auto version = target == Target::Pixels ? rasterVersion(record.pixels)
        : (record.mask ? rasterVersion(record.mask) : m_store->emptyVersion());
    const auto existing = historyBytes(m_history);
    const auto extra = recordMetadataBytes(record) + sizeof(KisGpuEditSession::Recovery) + sizeof(Frame) + quint64(m_history[m_cursor].size()) * sizeof(Record);
    if (extra > m_maximumRecoveryBytes - existing
        || KisGpuEditSession::ReplayState::recoveryBytes(recovery) > m_maximumRecoveryBytes - existing - extra) {
        return {Result::RecoveryBudgetExceeded, {}, {}};
    }
    // The lease owns the concrete store until the last input consumer releases the session.
    std::shared_ptr<KisGpuEditSession> session;
    try {
        session = std::shared_ptr<KisGpuEditSession>(
            new KisGpuEditSession(*m_store, version, recovery, m_maximumRecoveryBytes - existing - extra),
            [store = m_store](KisGpuEditSession *value) { delete value; });
    } catch (const std::invalid_argument &) {
        if (!m_store->deviceAvailable()) return {Result::GpuRejected, {}, {}};
        throw;
    }
    const auto token = session->begin();
    if (!token) return {Result::GpuRejected, {}, {}};
    m_session = std::move(session);
    m_active = Active{id, target, token};
    m_lastResult = Result::Accepted;
    return {Result::Accepted, m_session, token};
}

KisGpuLayerDocument::Result KisGpuLayerDocument::commit(const KisGpuEditSession::Token &token)
{
    if (!m_active || token != m_active->token) return Result::Stale;
    return m_session->commit(token) == KisGpuEditSession::Result::Accepted ? Result::Accepted : Result::Busy;
}

KisGpuLayerDocument::Result KisGpuLayerDocument::cancel(const KisGpuEditSession::Token &token)
{
    if (!m_active || token != m_active->token) return Result::Stale;
    m_session->cancel(token);
    m_active.reset();
    m_session.reset();
    return m_lastResult = Result::Accepted;
}

void KisGpuLayerDocument::poll()
{
    m_store->poll();
    if (m_restoring) { pollRecovery(); return; }
    if (!m_active) return;
    m_session->poll();
    if (m_session->state() != KisGpuEditSession::State::Idle) return;
    const auto index = indexOf(m_active->id);
    auto frame = m_history[m_cursor];
    const auto &previous = *frame[index];
    const auto before = rasterVersion(m_active->target == Target::Pixels ? previous.pixels : previous.mask);
    if (!(m_session->head() == before) && (m_active->target == Target::Pixels || m_session->head().tileCount() || previous.mask)) {
        auto record = std::make_shared<LayerState>(previous);
        if (m_active->target == Target::Pixels) {
            record->pixels = std::make_shared<const KisGpuEditSession::Recovery>(*m_session->recovery());
        } else {
            record->mask = std::make_shared<const KisGpuEditSession::Recovery>(*m_session->recovery());
        }
        const auto raster = m_active->target == Target::Pixels ? record->pixels : record->mask;
        frame[index] = std::move(record);
        if (publish(std::move(frame)) != Result::Accepted) return;
        m_rasterVersions[raster.get()] = m_session->head();
    }
    m_active.reset();
    m_session.reset();
}

QVector<KisGpuLayerDocument::Layer> KisGpuLayerDocument::layers() const
{
    QVector<Layer> result;
    for (const auto &record : m_history[m_cursor]) result.push_back(layerValue(*record));
    return result;
}

QVector<KisGpuTileStore::Layer> KisGpuLayerDocument::projectionLayers(bool includePreview) const
{
    QVector<KisGpuTileStore::Layer> result;
    for (const auto &record : m_history[m_cursor]) {
        auto layer = layerValue(*record).paint;
        if (includePreview && m_active && record->id == m_active->id) {
            if (m_active->target == Target::Pixels) layer.pixels = m_session->preview();
            else if (record->mask || m_session->preview().tileCount()) layer.mask = m_session->preview();
        }
        result.push_back(layer);
    }
    return result;
}

bool KisGpuLayerDocument::clearHistory()
{
    if (m_restoring) return false;
    m_history = {m_history[m_cursor]};
    m_cursor = 0;
    pruneRasterVersions();
    return true;
}
