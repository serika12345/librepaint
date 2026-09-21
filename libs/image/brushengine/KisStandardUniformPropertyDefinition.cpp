/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisStandardUniformPropertyDefinition.h"
#include <optional>
#include <qlatin1stringview.h>

std::optional<KisStandardUniformPropertyDefinition> standardUniformPropertyDefinition(const QString &id)
{
    if (id == QLatin1String("size")) {
        return KisStandardUniformPropertyDefinition{KisStandardUniformPropertyDefinition::Kind::Size,
                                                    0.0,
                                                    0.0,
                                                    1.0,
                                                    3.0,
                                                    2,
                                                    true,
                                                    true};
    }

    if (id == QLatin1String("opacity")) {
        return KisStandardUniformPropertyDefinition{KisStandardUniformPropertyDefinition::Kind::Opacity,
                                                    0.0,
                                                    1.0,
                                                    0.01,
                                                    1.0,
                                                    2,
                                                    false,
                                                    false};
    }

    if (id == QLatin1String("flow")) {
        return KisStandardUniformPropertyDefinition{KisStandardUniformPropertyDefinition::Kind::Flow,
                                                    0.0,
                                                    1.0,
                                                    0.01,
                                                    1.0,
                                                    2,
                                                    false,
                                                    false};
    }

    return std::nullopt;
}
