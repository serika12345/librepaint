/*
 *  SPDX-FileCopyrightText: 2017 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisHandleStyle.h"
#include "kis_painting_tweaks.h"
#include <qcolor.h>
#include <qnamespace.h>
#include <qscopedpointer.h>

namespace {
void initDashedStyle(const QColor &baseColor, const QColor &handleFill, KisHandleStyle *style, const QColor &white = QColor(Qt::white)) {
    QPen ants;
    QPen outline;
    KisPaintingTweaks::initAntsPen(&ants, &outline, 4, 4, baseColor, white);

    style->lineIterations << KisHandleStyle::IterationStyle(outline, Qt::NoBrush);
    style->lineIterations << KisHandleStyle::IterationStyle(ants, Qt::NoBrush);

    QPen handlePen(baseColor);
    handlePen.setWidth(2);
    handlePen.setCosmetic(true);
    handlePen.setJoinStyle(Qt::RoundJoin);

    style->handleIterations << KisHandleStyle::IterationStyle(handlePen, handleFill);
}

}


KisHandleStyle &KisHandleStyle::inheritStyle()
{
    static QScopedPointer<KisHandleStyle> style;

    if (!style) {
        style.reset(new KisHandleStyle());
        style->lineIterations << KisHandleStyle::IterationStyle();
        style->handleIterations << KisHandleStyle::IterationStyle();
    }

    return *style;
}

KisHandleStyle &KisHandleStyle::primarySelection(KisHandlePalette palette)
{
    static QScopedPointer<KisHandleStyle> style;

    if (!style) {
        style.reset(new KisHandleStyle());
        initDashedStyle(palette.primaryColor, palette.white, style.data(), palette.white);
    }

    return *style;
}

KisHandleStyle &KisHandleStyle::secondarySelection(KisHandlePalette palette)
{
    static QScopedPointer<KisHandleStyle> style;

    if (!style) {
        style.reset(new KisHandleStyle());
        initDashedStyle(palette.secondaryColor, palette.white, style.data(), palette.white);
    }

    return *style;
}

KisHandleStyle &KisHandleStyle::gradientHandles(KisHandlePalette palette)
{
    static QScopedPointer<KisHandleStyle> style;

    if (!style) {
        style.reset(new KisHandleStyle());
        initDashedStyle(palette.primaryColor, palette.gradientFillColor, style.data(), palette.white);
    }

    return *style;
}

KisHandleStyle &KisHandleStyle::gradientArrows(KisHandlePalette palette)
{
    return primarySelection(palette);
}


KisHandleStyle &KisHandleStyle::highlightedPrimaryHandles(KisHandlePalette palette)
{
    static QScopedPointer<KisHandleStyle> style;

    if (!style) {
        style.reset(new KisHandleStyle());
        initDashedStyle(palette.highlightOutlineColor, palette.highlightColor, style.data(), palette.white);
    }

    return *style;
}

KisHandleStyle &KisHandleStyle::highlightedPrimaryHandlesWithSolidOutline(KisHandlePalette palette)
{
    static QScopedPointer<KisHandleStyle> style;

    if (!style) {
        style.reset(new KisHandleStyle());
        QPen h = QPen(palette.highlightOutlineColor, 2);
        h.setCosmetic(true);
        style->handleIterations << KisHandleStyle::IterationStyle(h, palette.highlightColor);
        QPen l = QPen(palette.highlightOutlineColor, 1);
        l.setCosmetic(true);
        l.setJoinStyle(Qt::RoundJoin);
        style->lineIterations << KisHandleStyle::IterationStyle(l, Qt::NoBrush);
    }

    return *style;
}

KisHandleStyle &KisHandleStyle::partiallyHighlightedPrimaryHandles(KisHandlePalette palette)
{
    static QScopedPointer<KisHandleStyle> style;

    if (!style) {
        style.reset(new KisHandleStyle());
        initDashedStyle(palette.highlightOutlineColor, palette.selectionColor, style.data(), palette.white);
    }

    return *style;
}

KisHandleStyle &KisHandleStyle::selectedPrimaryHandles(KisHandlePalette palette)
{
    static QScopedPointer<KisHandleStyle> style;

    if (!style) {
        style.reset(new KisHandleStyle());
        initDashedStyle(palette.primaryColor, palette.selectionColor, style.data(), palette.white);
    }

    return *style;
}

