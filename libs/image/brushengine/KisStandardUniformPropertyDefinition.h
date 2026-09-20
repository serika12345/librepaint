/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_STANDARD_UNIFORM_PROPERTY_DEFINITION_H
#define KIS_STANDARD_UNIFORM_PROPERTY_DEFINITION_H

#include <optional>

#include <QString>
#include <QtGlobal>

struct KisStandardUniformPropertyDefinition {
    enum class Kind {
        Size,
        Opacity,
        Flow,
    };

    Kind kind;
    qreal minimum;
    qreal maximum;
    qreal singleStep;
    qreal exponentRatio;
    int decimals;
    bool usesBrushSizeMaximum;
    bool usesPixelSuffix;
};

std::optional<KisStandardUniformPropertyDefinition> standardUniformPropertyDefinition(const QString &id);

#endif // KIS_STANDARD_UNIFORM_PROPERTY_DEFINITION_H
