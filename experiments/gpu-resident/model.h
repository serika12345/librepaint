// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace poc {
constexpr uint32_t Tile = 64, TilePixels = Tile * Tile, Missing = ~0u;

inline uint32_t over(uint32_t dst, uint32_t src)
{
    const uint32_t inv = 255 - (src >> 24);
    uint32_t result = 0;
    for (unsigned c = 0; c < 32; c += 8) {
        const uint32_t value = ((src >> c) & 255) + (((dst >> c) & 255) * inv + 127) / 255;
        result |= value << c;
    }
    return result;
}

inline uint32_t shade(uint32_t dst, int x, int y, int cx, int cy,
                      uint32_t radius, uint32_t opacity, uint32_t color)
{
    const int dx = x - cx, dy = y - cy;
    const uint32_t distance = uint32_t(dx * dx + dy * dy), r2 = radius * radius;
    if (distance >= r2) return dst;
    const uint32_t alpha = (r2 - distance) * opacity / r2;
    uint32_t src = alpha << 24;
    for (unsigned c = 0; c < 24; c += 8)
        src |= ((((color >> c) & 255) * alpha + 127) / 255) << c;
    return over(dst, src);
}

struct Dab {
    uint32_t x, y, radius, opacity, color, reserved[3]{};
};
struct Job {
    uint32_t logical, physical, first, count;
};
struct Batch {
    std::vector<Dab> dabs;
    std::vector<Job> jobs;
    std::vector<uint32_t> indices;
    std::vector<uint32_t> commands(uint32_t grid, uint32_t layers) const;
};
struct Options {
    uint32_t size = 1024, layers = 1, diameter = 32, frames = 24, workers = 4;
    uint32_t samples = 8, warmup = 2;
    bool shape = false, varying = false, verify = false, timestamps = false;
    double intervalMs = 0;
};

// Fixed-capacity sparse pool: every path in the deterministic trace is reserved
// before measurement. CPU and GPU use identical physical offsets and tile maps.
struct Scene {
    Options options;
    uint32_t grid, gridTiles, activeTiles;
    std::vector<uint32_t> mapping, initial;
    std::vector<std::vector<Dab>> frames;
    std::vector<uint32_t> activeLogical, occupiedLogical;
    explicit Scene(Options options);
    Batch batch(const std::vector<Dab> &dabs, const std::vector<uint32_t> &alsoDirty = {}) const;
    Batch tiles(const std::vector<uint32_t> &logical) const;
};

class Workers {
public:
    explicit Workers(uint32_t count);
    ~Workers();
    void run(size_t count, const std::function<void(size_t)> &operation);
private:
    std::vector<std::thread> threads;
    std::mutex mutex;
    std::condition_variable start, finished;
    std::function<void(size_t)> function;
    std::atomic<size_t> next{0};
    size_t count = 0, generation = 0, pending = 0;
    bool stop = false;
};

struct CpuDocument {
    const Scene &scene;
    Workers workers;
    std::vector<uint32_t> pixels, projection;
    explicit CpuDocument(const Scene &scene, uint32_t workerCount);
    void reset();
    void paint(const Batch &batch);
    void compose(const Batch &batch);
};
double wallMs();
double cpuMs();
uint64_t hash(const std::vector<uint32_t> &pixels);
}
