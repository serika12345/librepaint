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
    for (qsizetype i = 0; i < frame.size(); ++i) if (frame[i]->layer.id == id) return i;
    return -1;
}

quint64 KisGpuLayerDocument::recordBytes(const LayerState &record) const
{
    return sizeof(LayerState) + quint64(record.layer.name.size()) * sizeof(QChar)
        + KisGpuEditSession::ReplayState::recoveryBytes(record.pixels)
        + (record.mask ? KisGpuEditSession::ReplayState::recoveryBytes(*record.mask) : 0);
}

quint64 KisGpuLayerDocument::historyBytes(const QVector<Frame> &history) const
{
    std::set<const LayerState *> counted;
    quint64 bytes = quint64(history.size()) * sizeof(Frame);
    for (const auto &frame : history) {
        bytes += quint64(frame.size()) * sizeof(Record);
        for (const auto &record : frame) if (counted.insert(record.get()).second) bytes += recordBytes(*record);
    }
    return bytes;
}

quint64 KisGpuLayerDocument::recoveryBytes() const
{
    const auto working = m_session ? m_session->recovery() : std::nullopt;
    return historyBytes(m_history) + (working ? KisGpuEditSession::ReplayState::recoveryBytes(*working) : 0);
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
    record->layer = {m_nextId, std::move(name), {m_store->emptyVersion()}};
    frame.insert(index, std::move(record));
    const auto result = publish(std::move(frame));
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
    if (frame[index]->layer.name == name) return Result::Accepted;
    auto record = std::make_shared<LayerState>(*frame[index]);
    record->layer.name = std::move(name);
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
    if (frame[index]->layer.paint.opacity == opacity && frame[index]->layer.paint.operation == operation) return Result::Accepted;
    auto record = std::make_shared<LayerState>(*frame[index]);
    record->layer.paint.opacity = opacity;
    record->layer.paint.operation = operation;
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
    record->layer.paint.mask = {};
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
    const auto recovery = target == Target::Pixels ? record.pixels : record.mask.value_or(KisGpuEditSession::Recovery{});
    const auto version = target == Target::Pixels ? record.layer.paint.pixels
        : (record.mask ? record.layer.paint.mask : m_store->emptyVersion());
    const auto existing = historyBytes(m_history);
    const auto extra = recordBytes(record) - KisGpuEditSession::ReplayState::recoveryBytes(recovery)
        + sizeof(Frame) + quint64(m_history[m_cursor].size()) * sizeof(Record);
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
    if (!m_active) return;
    m_session->poll();
    if (m_session->state() != KisGpuEditSession::State::Idle) return;
    const auto index = indexOf(m_active->id);
    auto frame = m_history[m_cursor];
    const auto &previous = *frame[index];
    const auto before = m_active->target == Target::Pixels ? previous.layer.paint.pixels : previous.layer.paint.mask;
    if (!(m_session->head() == before) && (m_active->target == Target::Pixels || m_session->head().tileCount() || previous.mask)) {
        auto record = std::make_shared<LayerState>(previous);
        if (m_active->target == Target::Pixels) {
            record->layer.paint.pixels = m_session->head();
            record->pixels = *m_session->recovery();
        } else {
            record->layer.paint.mask = m_session->head();
            record->mask = *m_session->recovery();
        }
        frame[index] = std::move(record);
        if (publish(std::move(frame)) != Result::Accepted) return;
    }
    m_active.reset();
    m_session.reset();
}

QVector<KisGpuLayerDocument::Layer> KisGpuLayerDocument::layers() const
{
    QVector<Layer> result;
    for (const auto &record : m_history[m_cursor]) result.push_back(record->layer);
    return result;
}

QVector<KisGpuTileStore::Layer> KisGpuLayerDocument::projectionLayers(bool includePreview) const
{
    QVector<KisGpuTileStore::Layer> result;
    for (const auto &record : m_history[m_cursor]) {
        auto layer = record->layer.paint;
        if (includePreview && m_active && record->layer.id == m_active->id) {
            if (m_active->target == Target::Pixels) layer.pixels = m_session->preview();
            else if (record->mask || m_session->preview().tileCount()) layer.mask = m_session->preview();
        }
        result.push_back(layer);
    }
    return result;
}

bool KisGpuLayerDocument::undo()
{
    if (readiness() != Result::Accepted || !m_cursor) return false;
    --m_cursor;
    return true;
}

bool KisGpuLayerDocument::redo()
{
    if (readiness() != Result::Accepted || m_cursor + 1 >= m_history.size()) return false;
    ++m_cursor;
    return true;
}

void KisGpuLayerDocument::clearHistory()
{
    m_history = {m_history[m_cursor]};
    m_cursor = 0;
}
