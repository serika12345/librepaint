/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuLayerDocument.h"
#include "KisGpuEditReplay_p.h"
#include <set>
#include <stdexcept>

KisGpuLayerDocument::KisGpuLayerDocument(std::shared_ptr<KisGpuTileStore> store, qsizetype retained,
                                       Recovery request, quint64 maximum)
    : KisGpuLayerDocument(std::move(store), retained, maximum)
{
    if (!validRecovery(request)) throw std::invalid_argument("Invalid GPU layer document recovery history or CPU payload budget");
    m_restoring.emplace();
    m_restoring->request = std::move(request);
    std::set<const RecoveryLayer *> counted;
    for (const auto &frame : m_restoring->request.history) {
        for (const auto &record : frame) {
            if (counted.insert(record.get()).second) m_restoring->records.push_back(record);
        }
    }
}

quint64 KisGpuLayerDocument::recoveryBytes(const Recovery &request) const
{
    quint64 bytes = quint64(request.history.size()) * sizeof(Frame);
    std::set<const RecoveryLayer *> counted;
    std::set<const KisGpuEditSession::Recovery *> rasters;
    for (const auto &frame : request.history) {
        bytes += quint64(frame.size()) * sizeof(Record);
        for (const auto &record : frame) {
            if (counted.insert(record.get()).second) {
                bytes += sizeof(LayerState) + quint64(record->name.size()) * sizeof(QChar);
                for (const auto &raster : {record->pixels, record->mask}) {
                    if (raster && rasters.insert(raster.get()).second)
                        bytes += KisGpuEditSession::ReplayState::recoveryBytes(*raster);
                }
            }
        }
    }
    if (request.working) bytes += KisGpuEditSession::ReplayState::recoveryBytes(request.working->data);
    return bytes;
}

std::optional<quint64> KisGpuLayerDocument::workingMaximum(const Recovery &request) const
{
    const auto &work = *request.working;
    const auto &base = *work.base;
    const auto working = KisGpuEditSession::ReplayState::recoveryBytes(work.data);
    const auto existing = recoveryBytes(request) - working;
    const auto extra = sizeof(LayerState) + quint64(base.name.size()) * sizeof(QChar)
        + sizeof(Frame) + quint64(request.history[request.cursor].size()) * sizeof(Record);
    return existing > m_maximumRecoveryBytes || extra > m_maximumRecoveryBytes - existing
        ? std::nullopt : std::optional<quint64>(m_maximumRecoveryBytes - existing - extra);
}

bool KisGpuLayerDocument::validRecovery(const Recovery &request) const
{
    using Session = KisGpuEditSession;
    if (request.history.isEmpty() || request.history.size() - 1 > m_retainedEdits
        || request.cursor < 0 || request.cursor >= request.history.size() || !request.nextId) return false;
    const auto completed = [&](const Session::Recovery &data) {
        return Session::ReplayState::validRecovery(data, 0, m_maximumRecoveryBytes)
            && data.cursor == data.edits.size() && !data.working;
    };
    LayerId highest = 0;
    std::set<const RecoveryLayer *> checked;
    for (const auto &frame : request.history) {
        std::set<LayerId> ids;
        for (const auto &record : frame) {
            if (!record || !record->id || !ids.insert(record->id).second) return false;
            highest = qMax(highest, record->id);
            if (checked.insert(record.get()).second) {
                if ((record->operation != KisGpuTileStore::CompositeOp::Over && record->operation != KisGpuTileStore::CompositeOp::Erase)
                    || !record->pixels || !completed(*record->pixels) || (record->mask && !completed(*record->mask))) return false;
            }
        }
    }
    if (request.nextId <= highest) return false;
    if (request.working) {
        const auto &work = *request.working;
        const auto &frame = request.history[request.cursor];
        if (!work.base || !frame.contains(work.base) || (work.target != Target::Pixels && work.target != Target::Mask)
            || !Session::ReplayState::validRecovery(work.data, 0, m_maximumRecoveryBytes)
            || work.data.cursor != work.data.edits.size()
            || (work.completed ? !completed(work.data) : !work.data.working.has_value())
            || !workingMaximum(request)
            || Session::ReplayState::recoveryBytes(work.data) > *workingMaximum(request)) return false;
    }
    return recoveryBytes(request) <= m_maximumRecoveryBytes;
}

KisGpuLayerDocument::Recovery KisGpuLayerDocument::recovery() const
{
    if (m_restoring) return m_restoring->request;
    Recovery result;
    result.cursor = m_cursor;
    result.nextId = m_nextId;
    std::map<const LayerState *, std::shared_ptr<const RecoveryLayer>> records;
    for (const auto &frame : m_history) {
        RecoveryFrame output;
        for (const auto &record : frame) {
            auto &cpu = records[record.get()];
            if (!cpu) cpu = std::make_shared<const RecoveryLayer>(RecoveryLayer{
                record->layer.id, record->layer.name, record->layer.paint.opacity, record->layer.paint.operation,
                record->pixels, record->mask});
            output.push_back(cpu);
        }
        result.history.push_back(std::move(output));
    }
    if (m_active) {
        const auto &record = m_history[m_cursor][indexOf(m_active->id)];
        auto data = *m_session->recovery();
        const auto before = m_active->target == Target::Pixels ? record->layer.paint.pixels : record->layer.paint.mask;
        const bool completed = !(m_session->head() == before)
            && (m_active->target == Target::Pixels || m_session->head().tileCount() || record->mask);
        if (completed) { data.working.reset(); data.commitRequested = false; }
        if (completed || data.working) result.working = RecoveryWorking{
            records.at(record.get()), m_active->target, std::move(data), completed};
    }
    return result;
}

KisGpuLayerDocument::Editing KisGpuLayerDocument::currentEditing() const
{
    return m_active ? Editing{Result::Accepted, m_session, m_active->token} : Editing{};
}

KisGpuTileStore::Error KisGpuLayerDocument::recoveryError() const
{
    if (!m_restoring) return KisGpuTileStore::Error::None;
    return !m_store->deviceAvailable() ? KisGpuTileStore::Error::DeviceLost : m_restoring->error;
}

void KisGpuLayerDocument::pollRecovery()
{
    using Session = KisGpuEditSession;
    auto &restore = *m_restoring;
    if (restore.failed) return;
    const auto fail = [&](KisGpuTileStore::Error error = KisGpuTileStore::Error::DeviceLost) {
        restore.failed = true;
        restore.error = error;
        restore.session.reset();
        restore.rasters.clear();
        restore.restored.clear();
    };
    if (!m_store->deviceAvailable()) { fail(); return; }
    const auto replay = [&](const Session::Recovery &data) {
        try {
            return std::shared_ptr<Session>(new Session(*m_store, 0, data, m_maximumRecoveryBytes),
                                           [store = m_store](Session *value) { delete value; });
        } catch (const std::invalid_argument &) {
            if (!m_store->deviceAvailable()) { fail(); return std::shared_ptr<Session>{}; }
            throw;
        }
    };
    if (restore.index < restore.records.size()) {
        const auto &source = restore.records[restore.index];
        const auto &raster = restore.maskPhase ? source->mask : source->pixels;
        if (!restore.rasters.count(raster.get())) {
            if (!restore.session) restore.session = replay(*raster);
            if (!restore.session) return;
            restore.session->poll();
            if (restore.session->state() == Session::State::Failed) { fail(restore.session->lastGpuError()); return; }
            if (restore.session->state() != Session::State::Idle) return;
            restore.rasters[raster.get()] = restore.session->head();
            restore.session.reset();
        }
        if (!restore.maskPhase && source->mask) {
            restore.maskPhase = true;
            return;
        }
        const auto mask = source->mask ? restore.rasters.at(source->mask.get()) : KisGpuTileStore::Version{};
        restore.restored[source.get()] = std::make_shared<const LayerState>(LayerState{
            {source->id, source->name, {restore.rasters.at(source->pixels.get()), mask, source->opacity, source->operation}},
            source->pixels, source->mask});
        restore.maskPhase = false;
        ++restore.index;
        return;
    }
    if (restore.request.working) {
        const auto &work = *restore.request.working;
        if (!restore.session) {
            const auto &base = *restore.restored.at(work.base.get());
            const auto version = work.target == Target::Pixels ? base.layer.paint.pixels
                : (base.mask ? base.layer.paint.mask : m_store->emptyVersion());
            try {
                restore.session = std::shared_ptr<Session>(work.completed
                    ? new Session(*m_store, 0, work.data, *workingMaximum(restore.request))
                    : new Session(*m_store, version, work.data, *workingMaximum(restore.request)),
                    [store = m_store](Session *value) { delete value; });
            } catch (const std::invalid_argument &) {
                if (!m_store->deviceAvailable()) { fail(); return; }
                throw;
            }
        }
        restore.session->poll();
        const auto state = restore.session->state();
        if (state == Session::State::Failed) { fail(restore.session->lastGpuError()); return; }
        if (state == Session::State::Restoring) return;
        if (work.completed) {
            const auto token = restore.session->begin();
            if (!token || restore.session->commit(token) != Session::Result::Accepted) {
                fail();
                return;
            }
        }
        m_session = restore.session;
        m_active = Active{work.base->id, work.target, m_session->currentToken()};
    }
    QVector<Frame> history;
    for (const auto &frame : restore.request.history) {
        Frame output;
        for (const auto &record : frame) output.push_back(restore.restored.at(record.get()));
        history.push_back(std::move(output));
    }
    m_history = std::move(history);
    m_cursor = restore.request.cursor;
    m_nextId = restore.request.nextId;
    m_restoring.reset();
}
