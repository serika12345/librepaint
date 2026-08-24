/*
 *  SPDX-FileCopyrightText: 2009 Sven Langkamp <sven.langkamp@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KO_BASIC_SHAPE_FACTORY_H
#define KO_BASIC_SHAPE_FACTORY_H

#include <kritaflake_export.h>

#include <QRectF>

class KoShape;

/**
 * Creates basic shapes through registered shape factories and supplies path
 * shapes when the corresponding plugin is unavailable.
 */
class KRITAFLAKE_EXPORT KoBasicShapeFactory
{
public:
    static KoShape *createRectangle(const QRectF &rect,
                                    qreal roundCornersX,
                                    qreal roundCornersY);

    static KoShape *createEllipse(const QRectF &rect);
};

#endif
