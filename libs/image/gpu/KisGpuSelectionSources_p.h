/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_SELECTION_SOURCES_P_H
#define KIS_GPU_SELECTION_SOURCES_P_H
#include "KisGpuSelectionMask.h"
namespace LibrePaint::Image::Gpu {
/** Inputs owned only while generating Alpha8 selection arithmetic. */
struct SelectionCombinationSource
{
    enum class Pixels : quint8 { Selection, InvertTransparency };
    QSharedPointer<const SelectionMask> first, second;
    Pixels pixels = Pixels::Selection;
    SelectionCombination action = SelectionCombination::Replace;
    QRect extent;
    bool inverted = false;
    QList<QPoint> tiles;
    quint8 defaultCoverage() const;
};

/** Frozen input for moving a generated selection without reading its pixels. */
struct SelectionTranslationSource
{
    QSharedPointer<const SelectionMask> source;
    QPoint offset;
    QRect extent;
    QRect clip;
    QList<QPoint> tiles;
};

/** Frozen raw layer alpha produced by ordinary transparency-mask painting. */
struct SelectionPaintSource
{
    QSharedPointer<const DocumentVersion> source;
    quint64 layer = 0;
    QList<QPoint> tiles;
    QList<QRect> allocatedBlocks;
};

/** Transparency-mask gray and independent selection opacity through a frozen
 * native affine filter. The generated mask retains only its resulting pixels. */
struct SelectionMaskTransformSource
{
    QSharedPointer<const SelectionMask> source, selection;
    QSharedPointer<const AffinePlan> plan;
    QRect copyRect, output;
    bool inverted = false;
    QList<QPoint> tiles;
    /** Native Alpha8 destination block grid, reduced modulo 64. */
    QPoint storageOrigin;
};


inline quint8 SelectionCombinationSource::defaultCoverage() const
{
    const int a = first ? first->defaultCoverage() : 0;
    if (pixels == Pixels::InvertTransparency) return quint8(255 - a);
    return 0;
}
}
#endif
