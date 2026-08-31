/*
 *  SPDX-FileCopyrightText: 2015 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_time_span.h"

#include <QDebug>
#include <KisStaticInitializer.h>

KIS_DECLARE_STATIC_INITIALIZER {
    qRegisterMetaType<KisTimeSpan>("KisTimeSpan");
}

QDebug operator<<(QDebug dbg, const KisTimeSpan &r)
{
    dbg.nospace() << "KisTimeSpan(" << r.start() << ", " << r.end() << ")";

    return dbg.space();
}

namespace KisDomUtils {

void saveValue(QDomElement *parent, const QString &tag, const KisTimeSpan &range)
{
    QDomDocument doc = parent->ownerDocument();
    QDomElement e = doc.createElement(tag);
    parent->appendChild(e);

    e.setAttribute("type", "timerange");

    if (range.isValid()) {
        e.setAttribute("from", toString(range.start()));

        if (!range.isInfinite()) {
            e.setAttribute("to", toString(range.end()));
        }
    }
}


bool loadValue(const QDomElement &parent, const QString &tag, KisTimeSpan *range)
{
    QDomElement e;
    if (!findOnlyElement(parent, tag, &e)) return false;

    if (!Private::checkType(e, "timerange")) return false;

    int start = toInt(e.attribute("from", "-1"));
    int end = toInt(e.attribute("to", "-1"));

    if (start == -1) {
        *range = KisTimeSpan();
    } else if (end == -1) {
        *range = KisTimeSpan::infinite(start);
    } else {
        *range = KisTimeSpan::fromTimeToTime(start, end);
    }
    return true;
}

}
