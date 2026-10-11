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
    /** CPU-only accepted input progress; copy together with the matching edit and brush settings. */
    class Progress {
    public:
        Progress() = default;
    private:
        friend class KisGpuBrushStroke;
        Sample previous;
        qreal accumulated = 0, spacing = 0;
        bool started = false;
        std::optional<QPoint> previousDab;
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
        /** Uncolored center-sample dulling; each dab picks up the preceding dab center. */
        std::optional<quint8> smudgeRate;
    };
    enum class Result { Accepted, InvalidInput, InputLimitExceeded, EditRejected };
    struct Update {
        Result result = Result::Accepted;
        KisGpuEditSession::Result editResult = KisGpuEditSession::Result::Accepted;
        bool sampleComplete = true;
    };
    static bool validSample(const Sample &sample);
    static bool validSettings(const Settings &settings);
    /** Throws std::invalid_argument for invalid brush settings or nonpositive generation limit. */
    KisGpuBrushStroke(KisGpuEditSession &session, KisGpuEditSession::Token token,
                      Settings settings, QRect clip, qsizetype maximumSteps = 4096);
    /** Continue a restored edit with the same settings and resources prepared on its session. */
    KisGpuBrushStroke(KisGpuEditSession &session, KisGpuEditSession::Token token,
                      Settings settings, QRect clip, Progress progress, qsizetype maximumSteps = 4096);
    Progress progress() const { return m_cursor; }
    Update append(const QVector<Sample> &samples,
                  const std::optional<KisGpuEditSession::Selection> &selection = {});
    /** Accept a bounded prefix; repeat the same sample until sampleComplete is true.
     * Refusal keeps the prior progress, including the unfinished segment.
     */
    Update appendPartial(const Sample &sample,
                         const std::optional<KisGpuEditSession::Selection> &selection = {});
    /** Reconstruct the whole stroke from its first sample and replace the working raster version. */
    Update replace(const QVector<Sample> &samples,
                   const std::optional<KisGpuEditSession::Selection> &selection = {});
private:
    Update paint(const QVector<Sample> &samples, bool replace,
                 const std::optional<KisGpuEditSession::Selection> &selection, bool partial = false);
    KisGpuEditSession &m_session;
    KisGpuEditSession::Token m_token;
    Settings m_settings;
    QRect m_clip;
    qsizetype m_maximumSteps;
    Progress m_cursor;
};
#endif
