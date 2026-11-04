/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuOperationDocument.h"
#include <QScopeGuard>
#include <limits>
#include <stdexcept>

namespace LibrePaint::Image::Gpu {

OperationId OperationDocument::beginPixelEdit(HistoryAction action)
{
    if (checkMutation() != Result::Success || m_nextOperation == std::numeric_limits<quint64>::max()) return {};
    const OperationId id{m_identity, ++m_nextOperation};
    if (beginExternalStroke(id) != Result::Success) return {};
    m_pixelEdit = PixelEdit{id, {}, false, action};
    return id;
}

Result OperationDocument::updatePixelEdit(OperationId id, const std::function<bool()> &apply)
{
    if (m_closed) return Result::Closed;
    if (!m_pixelEdit || m_pixelEdit->id != id || m_pixelEdit->updating) return Result::StaleOperation;
    if (!apply) return Result::InvalidInput;
    const auto previous = m_preview;
    const auto change = m_pixelEdit->change;
    m_preview.clear();
    m_pixelEdit->change = {};
    m_pixelEdit->updating = true;
    bool accepted = false;
    const auto restore = qScopeGuard([&] {
        m_pixelEdit->updating = false;
        if (!accepted) { m_preview = previous; m_pixelEdit->change = change; }
    });
    try {
        accepted = apply();
        return accepted ? Result::Success : Result::InvalidInput;
    } catch (const std::exception &) {
        return Result::InvalidInput;
    }
}

Result OperationDocument::commitPixelEdit(OperationId id)
{
    if (m_closed) return Result::Closed;
    if (!m_pixelEdit || m_pixelEdit->id != id || m_pixelEdit->updating) return Result::StaleOperation;
    if (!m_preview) return cancelPixelEdit(id);
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*m_preview));
    version->m_pendingOperation = {};
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    auto change = m_pixelEdit->change;
    change.pixelEditAction = m_pixelEdit->action;
    m_pixelEdit.reset();
    endExternalStroke(id);
    commitVersion(version, change);
    return Result::Success;
}

Result OperationDocument::cancelPixelEdit(OperationId id)
{
    if (m_closed) return Result::Closed;
    if (!m_pixelEdit || m_pixelEdit->id != id || m_pixelEdit->updating) return Result::StaleOperation;
    m_pixelEdit.reset();
    return endExternalStroke(id);
}

}
