/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_PAINTING_ENGINE_TEST_SCOPE_H
#define KIS_PAINTING_ENGINE_TEST_SCOPE_H

#include <QByteArray>
#include <QtGlobal>

namespace TestUtil {

/** Keeps reference fixtures independent of the application's default painting engine. */
class PaintingEngineScope
{
public:
    explicit PaintingEngineScope(bool gpu)
        : m_previous(qgetenv("LIBREPAINT_GPU_ENGINE"))
    {
        qputenv("LIBREPAINT_GPU_ENGINE", gpu ? "1" : "0");
    }
    ~PaintingEngineScope()
    {
        if (m_previous.isNull()) qunsetenv("LIBREPAINT_GPU_ENGINE");
        else qputenv("LIBREPAINT_GPU_ENGINE", m_previous);
    }
    PaintingEngineScope(const PaintingEngineScope &) = delete;
    PaintingEngineScope &operator=(const PaintingEngineScope &) = delete;
private:
    const QByteArray m_previous;
};

}

#endif
