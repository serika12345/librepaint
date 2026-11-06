/*
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *
 *  Experimental pen-latency probe. Records timestamped markers when the
 *  LIBREPAINT_LATENCY_LOG environment variable names an output file.
 */
#ifndef KIS_LATENCY_PROBE_H
#define KIS_LATENCY_PROBE_H

#include "kritaglobal_export.h"
#include <QtGlobal>

class QCoreApplication;

namespace KisLatencyProbe
{
KRITAGLOBAL_EXPORT bool enabled();
KRITAGLOBAL_EXPORT void mark(const char *event, double x = 0.0, double y = 0.0, qint64 extra = 0);
KRITAGLOBAL_EXPORT void installNativeFilter(QCoreApplication *app);
}

#endif
