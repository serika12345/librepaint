// SPDX-License-Identifier: GPL-2.0-or-later
#include "model.h"
#include <chrono>
#include <cmath>
#include <cstring>
#include <map>
#include <set>
#include <stdexcept>
#include <sys/resource.h>

namespace poc {
double wallMs()
{
    return std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
double cpuMs()
{
    rusage usage{};
    getrusage(RUSAGE_SELF, &usage);
    return (usage.ru_utime.tv_sec + usage.ru_stime.tv_sec) * 1000.0
        + (usage.ru_utime.tv_usec + usage.ru_stime.tv_usec) / 1000.0;
}
uint64_t hash(const std::vector<uint32_t> &pixels)
{
    uint64_t result = 14695981039346656037ull;
    for (auto pixel : pixels) {
        for (unsigned c = 0; c < 32; c += 8) {
            result ^= (pixel >> c) & 255;
            result *= 1099511628211ull;
        }
    }
    return result;
}

static std::vector<uint32_t> footprint(const Dab &dab, uint32_t size)
{
    const int x0 = std::max(0, int(dab.x) - int(dab.radius)) / Tile;
    const int y0 = std::max(0, int(dab.y) - int(dab.radius)) / Tile;
    const int x1 = std::min(int(size) - 1, int(dab.x + dab.radius)) / Tile;
    const int y1 = std::min(int(size) - 1, int(dab.y + dab.radius)) / Tile;
    std::vector<uint32_t> result;
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x) result.push_back(y * (size / Tile) + x);
    return result;
}

Scene::Scene(Options o) : options(o), grid(o.size / Tile), gridTiles(grid * grid)
{
    const double span = std::min(o.size * 0.5, 1024.0);
    std::set<uint32_t> active;
    for (uint32_t f = 0; f < o.frames; ++f) {
        std::vector<Dab> dabs;
        const uint32_t count = o.shape ? 64 : 4;
        for (uint32_t i = 0; i < count; ++i) {
            double x, y;
            if (o.shape) {
                const double angle = i * 6.283185307179586 / count;
                const double scale = 0.6 + 0.35 * double(f) / std::max(1u, o.frames - 1);
                x = o.size / 2.0 + span * 0.5 * scale * std::cos(angle);
                y = o.size / 2.0 + span * 0.3 * scale * std::sin(angle);
            } else {
                const double t = double(f * count + i) / std::max(1u, o.frames * count - 1);
                x = o.size / 2.0 + span * (t - 0.5);
                y = o.size / 2.0 + span * 0.2 * std::sin(t * 6.283185307179586);
            }
            Dab dab{uint32_t(x), uint32_t(y), o.diameter / 2,
                    100 + ((f * count + i) * 17) % 156,
                    0x005020d0u + (i % 3) * 0x00101000u, {}};
            if (o.varying) dab.radius = std::max(1u, dab.radius * (6 + f % 3) / 8);
            dabs.push_back(dab);
            for (auto tile : footprint(dab, o.size)) active.insert(tile);
        }
        frames.push_back(std::move(dabs));
    }
    activeLogical.assign(active.begin(), active.end());
    activeTiles = activeLogical.size();
    mapping.assign(size_t(o.layers) * gridTiles, Missing);
    uint32_t physical = 0;
    for (auto tile : activeLogical) mapping[(o.layers - 1) * gridTiles + tile] = physical++;
    const uint32_t patch = std::min(o.size / 2, 1024u) / Tile;
    const uint32_t begin = (grid - patch) / 2;
    std::set<uint32_t> occupied = active;
    for (uint32_t l = 0; l + 1 < o.layers; ++l) {
        for (uint32_t y = begin; y < begin + patch; ++y) {
            for (uint32_t x = begin; x < begin + patch; ++x) {
                const uint32_t tile = y * grid + x;
                mapping[l * gridTiles + tile] = physical++;
                occupied.insert(tile);
            }
        }
    }
    occupiedLogical.assign(occupied.begin(), occupied.end());
    initial.resize(size_t(physical) * TilePixels, 0);
    for (uint32_t l = 0; l + 1 < o.layers; ++l) {
        for (uint32_t logical : occupiedLogical) {
            const auto slot = mapping[l * gridTiles + logical];
            if (slot == Missing) continue;
            for (uint32_t p = 0; p < TilePixels; ++p) {
                const uint32_t alpha = 24 + (l * 19 + p % Tile) % 96;
                uint32_t color = alpha << 24;
                for (unsigned c = 0; c < 24; c += 8)
                    color |= (((l * 37 + p * (c + 1) + logical * 13) % 256) * alpha / 255) << c;
                initial[size_t(slot) * TilePixels + p] = color;
            }
        }
    }
}

Batch Scene::batch(const std::vector<Dab> &dabs, const std::vector<uint32_t> &alsoDirty) const
{
    Batch result;
    result.dabs = dabs;
    std::map<uint32_t, std::vector<uint32_t>> bins;
    for (auto logical : alsoDirty) bins[logical];
    for (uint32_t i = 0; i < dabs.size(); ++i)
        for (auto logical : footprint(dabs[i], options.size)) bins[logical].push_back(i);
    for (const auto &[logical, candidates] : bins) {
        result.jobs.push_back({logical, mapping[(options.layers - 1) * gridTiles + logical],
                               uint32_t(result.indices.size()), uint32_t(candidates.size())});
        result.indices.insert(result.indices.end(), candidates.begin(), candidates.end());
    }
    return result;
}
Batch Scene::tiles(const std::vector<uint32_t> &logical) const { return batch({}, logical); }

std::vector<uint32_t> Batch::commands(uint32_t grid, uint32_t layers) const
{
    const uint32_t dabOffset = 8 + 4 * jobs.size();
    const uint32_t indexOffset = dabOffset + 8 * dabs.size();
    std::vector<uint32_t> data(indexOffset + indices.size(), 0);
    data[0] = jobs.size(); data[1] = layers; data[2] = grid;
    data[3] = dabOffset; data[4] = indexOffset;
    if (!jobs.empty()) std::memcpy(data.data() + 8, jobs.data(), jobs.size() * sizeof(Job));
    if (!dabs.empty()) std::memcpy(data.data() + dabOffset, dabs.data(), dabs.size() * sizeof(Dab));
    if (!indices.empty()) std::memcpy(data.data() + indexOffset, indices.data(), indices.size() * 4);
    return data;
}

Workers::Workers(uint32_t n)
{
    if (n == 1) return;
    for (uint32_t i = 0; i < n; ++i) threads.emplace_back([this] {
        size_t seen = 0;
        std::unique_lock lock(mutex);
        for (;;) {
            start.wait(lock, [&] { return stop || generation != seen; });
            if (stop) return;
            seen = generation;
            lock.unlock();
            for (size_t i; (i = next.fetch_add(1, std::memory_order_relaxed)) < count;) function(i);
            lock.lock();
            if (--pending == 0) finished.notify_one();
        }
    });
}
Workers::~Workers()
{
    { std::lock_guard lock(mutex); stop = true; }
    start.notify_all();
    for (auto &thread : threads) thread.join();
}
void Workers::run(size_t n, const std::function<void(size_t)> &operation)
{
    if (threads.empty()) { for (size_t i = 0; i < n; ++i) operation(i); return; }
    std::unique_lock lock(mutex);
    function = operation; count = n; next = 0; pending = threads.size(); ++generation;
    start.notify_all();
    finished.wait(lock, [&] { return pending == 0; });
}

CpuDocument::CpuDocument(const Scene &s, uint32_t n)
    : scene(s), workers(n), pixels(s.initial), projection(size_t(s.gridTiles) * TilePixels, 0)
{
    compose(s.tiles(s.occupiedLogical));
}
void CpuDocument::reset()
{
    std::fill_n(pixels.begin(), size_t(scene.activeTiles) * TilePixels, 0u);
}
void CpuDocument::paint(const Batch &batch)
{
    workers.run(batch.jobs.size(), [&](size_t index) {
        const auto &job = batch.jobs[index];
        if (!job.count) return;
        const int x0 = int(job.logical % scene.grid * Tile), y0 = int(job.logical / scene.grid * Tile);
        auto *tile = pixels.data() + size_t(job.physical) * TilePixels;
        for (uint32_t k = 0; k < job.count; ++k) {
            const auto &d = batch.dabs[batch.indices[job.first + k]];
            const int xmin = std::max(x0, int(d.x) - int(d.radius));
            const int xmax = std::min(x0 + int(Tile), int(d.x + d.radius));
            const int ymin = std::max(y0, int(d.y) - int(d.radius));
            const int ymax = std::min(y0 + int(Tile), int(d.y + d.radius));
            for (int y = ymin; y < ymax; ++y)
                for (int x = xmin; x < xmax; ++x) {
                    auto &pixel = tile[(y - y0) * Tile + x - x0];
                    pixel = shade(pixel, x, y, d.x, d.y, d.radius, d.opacity, d.color);
                }
        }
    });
}
void CpuDocument::compose(const Batch &batch)
{
    workers.run(batch.jobs.size(), [&](size_t index) {
        const uint32_t logical = batch.jobs[index].logical;
        auto *dst = projection.data() + size_t(logical) * TilePixels;
        std::fill_n(dst, TilePixels, 0u);
        for (uint32_t l = 0; l < scene.options.layers; ++l) {
            const auto slot = scene.mapping[l * scene.gridTiles + logical];
            if (slot == Missing) continue;
            const auto *src = pixels.data() + size_t(slot) * TilePixels;
            for (uint32_t p = 0; p < TilePixels; ++p) dst[p] = over(dst[p], src[p]);
        }
    });
}
}
