/*
 *  SPDX-FileCopyrightText: 2024 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KoSvgTextPropertyData.h"
#include <QDebug>

#include <KisStaticInitializer.h>
#include <qtextoption.h>
#include <qtversionchecks.h>

KIS_DECLARE_STATIC_INITIALIZER {
    qRegisterMetaType<KoSvgTextPropertyData>("KoSvgTextPropertyData");

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QMetaType::registerEqualsComparator<KoSvgTextPropertyData>();
    QMetaType::registerDebugStreamOperator<KoSvgTextPropertyData>();
#endif
}

QDebug operator<<(QDebug dbg, const KoSvgTextPropertyData &prop)
{
    dbg.nospace() << "TextPropertyData(";
    dbg.nospace() << " Common properties:" << prop.commonProperties.convertParagraphProperties() << prop.commonProperties.convertToSvgTextAttributes();
    dbg.nospace() << " Tristate:" << prop.tristate;
    dbg.nospace() << " SpanSelection:" << prop.spanSelection;
    dbg.nospace() << " Enabled:" << prop.enabled;
    dbg.nospace() << " )";
    return dbg.space();
}
