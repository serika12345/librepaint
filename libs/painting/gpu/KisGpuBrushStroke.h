/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_BRUSH_STROKE_H
#define KIS_GPU_BRUSH_STROKE_H
#include "KisGpuEditSession.h"

/** Pressure-sized, isotropically spaced round/elliptical stroke input.
 * The borrowed edit session outlives this object; all calls use its submitting thread.
 * Input progress advances only after the session accepts the generated dab batch.
 */
class KisGpuBrushStroke
{
public:
    struct Sample {
        QPointF position;
        qreal pressure = 1;
    };
    struct Settings {
        QSizeF diameter = QSizeF(20, 20), fade = QSizeF(1, 1);
        qreal spacing = .25;
        bool pressureSize = true;
        quint32 rgba = 0xFF000000;
        KisGpuTileStore::CompositeOp operation = KisGpuTileStore::CompositeOp::Over;
        quint8 opacity = 255, coverage = 255;
        std::optional<KisGpuEditSession::Texture> texture;
        quint8 textureStrength = 255;
    };
    enum class Result { Accepted, InvalidInput, InputLimitExceeded, EditRejected };
    struct Update {
        Result result = Result::Accepted;
        KisGpuEditSession::Result editResult = KisGpuEditSession::Result::Accepted;
    };
    /** Throws std::invalid_argument for invalid brush settings or nonpositive generation limit. */
    KisGpuBrushStroke(KisGpuEditSession &session, KisGpuEditSession::Token token,
                      Settings settings, QRect clip, qsizetype maximumSteps = 4096);
    Update append(const QVector<Sample> &samples,
                  const std::optional<KisGpuEditSession::Selection> &selection = {});
    /** Reconstruct the whole stroke from its first sample and replace the working raster version. */
    Update replace(const QVector<Sample> &samples,
                   const std::optional<KisGpuEditSession::Selection> &selection = {});
private:
    struct Cursor {
        Sample previous;
        qreal accumulated = 0, spacing = 0;
        bool started = false;
    };
    Update paint(const QVector<Sample> &samples, bool replace,
                 const std::optional<KisGpuEditSession::Selection> &selection);
    KisGpuEditSession &m_session;
    KisGpuEditSession::Token m_token;
    Settings m_settings;
    QRect m_clip;
    qsizetype m_maximumSteps;
    Cursor m_cursor;
};
#endif
