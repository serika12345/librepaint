/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KISSTROKEPREDICTIONRENDERBENCHMARK_H
#define KISSTROKEPREDICTIONRENDERBENCHMARK_H

#include <simpletest.h>

class KisStrokePredictionRenderBenchmark : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void benchmarkPredictionRender();
};

#endif // KISSTROKEPREDICTIONRENDERBENCHMARK_H
