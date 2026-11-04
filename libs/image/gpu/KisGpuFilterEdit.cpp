/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuOperationDocument.h"
#include <QScopeGuard>
#include <limits>
#include <stdexcept>

namespace LibrePaint::Image::Gpu {

OperationId OperationDocument::beginFilterEdit()
{
    if (checkMutation() != Result::Success || m_nextOperation == std::numeric_limits<quint64>::max()) return {};
    const OperationId id{m_identity, ++m_nextOperation};
    if (beginExternalStroke(id) != Result::Success) return {};
    m_filterEdit = FilterEdit{id, {}, false};
    return id;
}

Result OperationDocument::updateFilterEdit(OperationId id, const std::function<bool()> &apply)
{
    if (m_closed) return Result::Closed;
    if (!m_filterEdit || m_filterEdit->id != id || m_filterEdit->updating) return Result::StaleOperation;
    if (!apply) return Result::InvalidInput;
    const auto previous = m_preview;
    const auto change = m_filterEdit->change;
    m_preview.clear();
    m_filterEdit->change = {};
    m_filterEdit->updating = true;
    bool accepted = false;
    const auto restore = qScopeGuard([&] {
        m_filterEdit->updating = false;
        if (!accepted) { m_preview = previous; m_filterEdit->change = change; }
    });
    try {
        accepted = apply();
        return accepted ? Result::Success : Result::InvalidInput;
    } catch (const std::exception &) {
        return Result::InvalidInput;
    }
}

Result OperationDocument::commitFilterEdit(OperationId id)
{
    if (m_closed) return Result::Closed;
    if (!m_filterEdit || m_filterEdit->id != id || m_filterEdit->updating) return Result::StaleOperation;
    if (!m_preview) return cancelFilterEdit(id);
    if (m_nextRevision == std::numeric_limits<quint64>::max()) return Result::CapacityExceeded;
    auto version = QSharedPointer<DocumentVersion>(new DocumentVersion(*m_preview));
    version->m_pendingOperation = {};
    version->m_modifiedRevision = version->m_revision = ++m_nextRevision;
    const auto change = m_filterEdit->change;
    m_filterEdit.reset();
    endExternalStroke(id);
    commitVersion(version, change);
    return Result::Success;
}

Result OperationDocument::cancelFilterEdit(OperationId id)
{
    if (m_closed) return Result::Closed;
    if (!m_filterEdit || m_filterEdit->id != id || m_filterEdit->updating) return Result::StaleOperation;
    m_filterEdit.reset();
    return endExternalStroke(id);
}

}
