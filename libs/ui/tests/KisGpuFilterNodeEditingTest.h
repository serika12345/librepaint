/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_FILTER_NODE_EDITING_TEST_H
#define KIS_GPU_FILTER_NODE_EDITING_TEST_H
#include <kis_paintop_preset.h>
namespace FilterNodeEditingTest {
void addData();
void addConvolutionData();
void addConvolutionChainData();
void runConvolutionChain();
void run(const KisPaintOpPresetSP &preset);
}
#endif
