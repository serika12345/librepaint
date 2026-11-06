// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "model.h"
#include <array>
#include <memory>
#include <string>

namespace poc {
enum class Mode { Cpu, Hybrid, Gpu };
struct Counters {
    uint64_t pixelUpload = 0, commandUpload = 0, readback = 0;
    uint64_t copyBytes = 0, submissions = 0, writeCalls = 0;
    uint64_t paintTiles = 0, composeTiles = 0;
};

class Gpu {
public:
    Gpu(const Scene &scene, bool timestamps, Mode mode);
    ~Gpu();
    Gpu(const Gpu &) = delete;
    Gpu &operator=(const Gpu &) = delete;
    // CPU pixels are supplied only in Cpu/Hybrid mode. Gpu mode has no CPU document.
    void frame(const Batch &batch, Mode mode, const CpuDocument *cpu, bool restore);
    void reset();
    void checkpoint();
    void restore();
    void wait();
    std::vector<uint32_t> readDocument();
    std::vector<uint32_t> readProjection();
    std::vector<uint32_t> readDisplay();
    std::array<double, 3> gpuTimes();
    Counters counters;
    std::string adapter;
    uint64_t allocatedBytes = 0;
private:
    struct Impl;
    std::unique_ptr<Impl> d;
};
}
