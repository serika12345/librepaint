/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuBrushStroke.h"
#include "KisDabSpacing.h"
#include <cmath>
#include <limits>
#include <stdexcept>

namespace {
bool finiteFloat(qreal value) { return std::isfinite(value) && qAbs(value) <= std::numeric_limits<float>::max(); }
bool positiveFloat(qreal value) { return finiteFloat(value) && value > 0; }
bool validSample(const KisGpuBrushStroke::Sample &sample)
{
    return finiteFloat(sample.position.x()) && finiteFloat(sample.position.y())
        && std::isfinite(sample.pressure) && sample.pressure >= 0 && sample.pressure <= 1;
}
}

KisGpuBrushStroke::KisGpuBrushStroke(KisGpuEditSession &session, KisGpuEditSession::Token token,
                                    Settings settings, QRect clip, qsizetype maximumSteps)
    : m_session(session), m_token(std::move(token)), m_settings(settings), m_clip(clip), m_maximumSteps(maximumSteps)
{
    if (!positiveFloat(settings.diameter.width()) || !positiveFloat(settings.diameter.height())
        || !positiveFloat(settings.fade.width()) || !positiveFloat(settings.fade.height())
        || settings.fade.width() > 1 || settings.fade.height() > 1 || !finiteFloat(settings.spacing) || settings.spacing < 0
        || !std::isfinite(qMax(settings.diameter.width(), settings.diameter.height()) * settings.spacing)
        || (settings.operation != KisGpuTileStore::CompositeOp::Over && settings.operation != KisGpuTileStore::CompositeOp::Erase)
        || maximumSteps <= 0) throw std::invalid_argument("Invalid GPU brush size, fade, spacing, operation or generation limit");
}

KisGpuBrushStroke::Update KisGpuBrushStroke::append(const QVector<Sample> &samples,
    const std::optional<KisGpuEditSession::Selection> &selection)
{
    return paint(samples, false, selection);
}

KisGpuBrushStroke::Update KisGpuBrushStroke::replace(const QVector<Sample> &samples,
    const std::optional<KisGpuEditSession::Selection> &selection)
{
    return paint(samples, true, selection);
}

KisGpuBrushStroke::Update KisGpuBrushStroke::paint(const QVector<Sample> &samples, bool replace,
    const std::optional<KisGpuEditSession::Selection> &selection)
{
    if (samples.size() > m_maximumSteps) return {Result::InputLimitExceeded};
    for (const auto &sample : samples) if (!validSample(sample)) return {Result::InvalidInput};
    Cursor cursor = replace ? Cursor{} : m_cursor;
    QVector<KisGpuTileStore::DabCommand> commands;
    qsizetype steps = 0;
    const auto place = [&](const Sample &sample) {
        if (steps == m_maximumSteps) return false;
        ++steps;
        const auto size = m_settings.diameter * (m_settings.pressureSize ? sample.pressure : 1.0);
        const bool tooSmall = size.width() < .01 || size.height() < .01;
        cursor.spacing = tooSmall ? 0 : qMax(size.width(), size.height()) * m_settings.spacing;
        if (tooSmall) return true;
        KisGpuTileStore::DabCommand command;
        command.center = sample.position;
        command.diameter = size;
        command.fade = m_settings.fade;
        command.rgba = m_settings.rgba;
        command.operation = m_settings.operation;
        command.opacity = m_settings.opacity;
        command.coverage = m_settings.coverage;
        commands.push_back(command);
        return true;
    };
    for (const auto &sample : samples) {
        if (!cursor.started) {
            if (!place(sample)) return {Result::InputLimitExceeded};
            cursor.started = true;
        } else {
            auto point = cursor.previous;
            while (true) {
                const auto accumulated = cursor.accumulated;
                const qreal t = KisDabSpacing::nextIsotropic(point.position, sample.position, cursor.spacing, cursor.accumulated);
                if (t < 0) break;
                if (!std::isfinite(t) || t > 1 || (t == 0 && accumulated == 0)) return {Result::InvalidInput};
                Sample next{(1 - t) * point.position + t * sample.position,
                            (1 - t) * point.pressure + t * sample.pressure};
                if (!validSample(next) || (t > 0 && next.position == point.position)) return {Result::InvalidInput};
                if (!place(next)) return {Result::InputLimitExceeded};
                point = next;
            }
        }
        cursor.previous = sample;
    }
    const auto result = m_settings.texture
        ? (replace ? m_session.replace(m_token, commands, m_clip, *m_settings.texture, selection)
                   : m_session.append(m_token, commands, m_clip, *m_settings.texture, selection))
        : selection
        ? (replace ? m_session.replace(m_token, commands, m_clip, *selection) : m_session.append(m_token, commands, m_clip, *selection))
        : (replace ? m_session.replace(m_token, commands, m_clip) : m_session.append(m_token, commands, m_clip));
    if (result != KisGpuEditSession::Result::Accepted) return {Result::EditRejected, result};
    m_cursor = cursor;
    return {};
}
