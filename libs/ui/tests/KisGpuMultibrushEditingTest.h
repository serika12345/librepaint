/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_MULTIBRUSH_EDITING_TEST_H
#define KIS_GPU_MULTIBRUSH_EDITING_TEST_H

#include <memory>
namespace LibrePaint::Image::Gpu { class Execution; }
namespace MultibrushEditingTest {
void addData();
void run(std::shared_ptr<LibrePaint::Image::Gpu::Execution> &sharedExecution);
}

#endif
