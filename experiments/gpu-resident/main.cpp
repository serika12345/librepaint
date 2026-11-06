// SPDX-License-Identifier: GPL-2.0-or-later
#include "gpu.h"
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <stdexcept>

using namespace poc;

static void expectPixels(const std::vector<uint32_t> &actual, const std::vector<uint32_t> &expected, const std::string &stage)
{
    if (actual.size() != expected.size()) throw std::runtime_error(stage + ": size mismatch");
    for (size_t i = 0; i < actual.size(); ++i) {
        if (actual[i] != expected[i]) {
            std::ostringstream out;
            out << stage << ": pixel " << i << " expected 0x" << std::hex << expected[i] << ", actual 0x" << actual[i];
            throw std::runtime_error(out.str());
        }
    }
}
static std::vector<uint32_t> linear(const Scene &scene, const std::vector<uint32_t> &tiles)
{
    const uint32_t size = scene.options.size;
    std::vector<uint32_t> pixels(size_t(size) * size);
    for (uint32_t y = 0; y < size; ++y)
        for (uint32_t x = 0; x < size; ++x)
            pixels[size_t(y) * size + x] = tiles[((y / Tile) * scene.grid + x / Tile) * TilePixels + (y % Tile) * Tile + x % Tile];
    return pixels;
}
static std::string hex(uint64_t value)
{
    std::ostringstream out; out << std::hex << std::setw(16) << std::setfill('0') << value; return out.str();
}
static double quantile(std::vector<double> values, double q)
{
    std::sort(values.begin(), values.end());
    return values[std::min(values.size() - 1, size_t(std::ceil(q * values.size()) - 1))];
}
static void array(const std::vector<double> &values)
{
    std::cout << '[';
    for (size_t i = 0; i < values.size(); ++i) std::cout << (i ? "," : "") << values[i];
    std::cout << ']';
}

static void verify(const Scene &scene, Mode mode)
{
    CpuDocument reference(scene, 1);
    CpuDocument parallel(scene, scene.options.workers);
    Gpu gpu(scene, false, mode);
    const auto initialProjection = reference.projection;
    gpu.frame(scene.tiles(scene.occupiedLogical), mode, &parallel, false); gpu.wait();
    expectPixels(gpu.readDisplay(), linear(scene, initialProjection), "initial display");
    std::vector<uint32_t> previous;
    for (size_t f = 0; f < scene.frames.size(); ++f) {
        auto batch = scene.batch(scene.frames[f], scene.options.shape ? previous : std::vector<uint32_t>{});
        if (scene.options.shape) { reference.reset(); parallel.reset(); }
        reference.paint(batch); reference.compose(batch);
        parallel.paint(batch); parallel.compose(batch);
        expectPixels(parallel.pixels, reference.pixels, "parallel paint");
        expectPixels(parallel.projection, reference.projection, "parallel composition");
        gpu.frame(batch, mode, mode == Mode::Gpu ? nullptr : &parallel, scene.options.shape); gpu.wait();
        if (mode != Mode::Cpu) expectPixels(gpu.readDocument(), reference.pixels, "document frame " + std::to_string(f));
        expectPixels(gpu.readProjection(), reference.projection, "projection frame " + std::to_string(f));
        expectPixels(gpu.readDisplay(), linear(scene, reference.projection), "display frame " + std::to_string(f));
        previous.clear(); for (auto &j : batch.jobs) previous.push_back(j.logical);
    }
    // Keeping the final working image is commit. The pre-operation checkpoint is
    // retained until the next operation. Cancellation and one-step Undo restore it.
    const auto committedPixels = reference.pixels;
    const auto committedProjection = reference.projection;
    parallel.reset(); parallel.compose(scene.tiles(scene.activeLogical));
    if (mode == Mode::Gpu) gpu.restore();
    gpu.frame(scene.tiles(scene.activeLogical), mode, mode == Mode::Gpu ? nullptr : &parallel, false); gpu.wait();
    if (mode != Mode::Cpu) expectPixels(gpu.readDocument(), scene.initial, "cancel/undo document");
    expectPixels(gpu.readDisplay(), linear(scene, initialProjection), "cancel/undo display");
    // Redo replays the retained deterministic command list in this minimal PoC.
    previous.clear();
    for (const auto &frame : scene.frames) {
        auto batch = scene.batch(frame, scene.options.shape ? previous : std::vector<uint32_t>{});
        if (scene.options.shape) parallel.reset();
        parallel.paint(batch); parallel.compose(batch);
        gpu.frame(batch, mode, mode == Mode::Gpu ? nullptr : &parallel, scene.options.shape);
        previous.clear(); for (auto &j : batch.jobs) previous.push_back(j.logical);
    }
    gpu.wait();
    if (mode != Mode::Cpu) expectPixels(gpu.readDocument(), committedPixels, "redo document");
    expectPixels(gpu.readDisplay(), linear(scene, committedProjection), "redo display");
    std::cout << "{\"verified\":true,\"frames\":" << scene.frames.size()
              << ",\"document_hash\":\"" << hex(hash(committedPixels))
              << "\",\"projection_hash\":\"" << hex(hash(committedProjection)) << "\"}\n";
}

int main(int argc, char **argv)
{
    try {
        Options options;
        Mode mode = Mode::Gpu;
        std::string modeName = "gpu", dump;
        uint32_t inFlight = 1;
        for (int i = 1; i < argc; ++i) {
            const std::string key = argv[i];
            if (key == "--verify") { options.verify = true; continue; }
            if (key == "--timestamps") { options.timestamps = true; continue; }
            if (key == "--shape") { options.shape = true; continue; }
            if (key == "--varying") { options.varying = true; continue; }
            if (i + 1 == argc) throw std::runtime_error("missing value for " + key);
            const std::string value = argv[++i];
            if (key == "--mode") {
                modeName = value;
                if (value == "cpu") mode = Mode::Cpu;
                else if (value == "hybrid") mode = Mode::Hybrid;
                else if (value == "gpu") mode = Mode::Gpu;
                else throw std::runtime_error("unknown mode");
            } else if (key == "--size") options.size = std::stoul(value);
            else if (key == "--layers") options.layers = std::stoul(value);
            else if (key == "--diameter") options.diameter = std::stoul(value);
            else if (key == "--frames") options.frames = std::stoul(value);
            else if (key == "--samples") options.samples = std::stoul(value);
            else if (key == "--warmup") options.warmup = std::stoul(value);
            else if (key == "--workers") options.workers = std::stoul(value);
            else if (key == "--in-flight") inFlight = std::stoul(value);
            else if (key == "--interval-ms") options.intervalMs = std::stod(value);
            else if (key == "--dump") dump = value;
            else throw std::runtime_error("unknown option " + key);
        }
        if (options.size < 128 || options.size > 4096 || options.size % Tile ||
            options.layers < 1 || options.layers > 24 || options.diameter < 2 || options.diameter > 1024 ||
            options.diameter > options.size || options.frames < 2 || options.frames > 256 ||
            options.samples < 1 || options.samples > 1000 || options.warmup > 100 ||
            options.workers < 1 || options.workers > 16 || inFlight < 1 || inFlight > 3 ||
            !std::isfinite(options.intervalMs) || options.intervalMs < 0 || options.intervalMs > 100 ||
            (options.timestamps && inFlight != 1)) throw std::runtime_error("unsupported experiment dimensions/options");
        Scene scene(options);
        if (options.verify) { verify(scene, mode); return 0; }

        uint64_t expectedDocument, expectedProjection;
        // Serial oracle is computed before timing and released before the GPU run.
        {
            CpuDocument reference(scene, 1);
            for (auto &frame : scene.frames) {
                if (options.shape) reference.reset();
                reference.paint(scene.batch(frame));
            }
            reference.compose(scene.tiles(scene.occupiedLogical));
            expectedDocument = hash(reference.pixels); expectedProjection = hash(reference.projection);
        }
        std::unique_ptr<CpuDocument> cpu;
        if (mode != Mode::Gpu) cpu = std::make_unique<CpuDocument>(scene, options.workers);
        Gpu gpu(scene, options.timestamps, mode);
        const uint64_t initialBytes = scene.initial.size() * 4;
        const auto all = scene.tiles(scene.occupiedLogical), active = scene.tiles(scene.activeLogical);
        // Initial source data has been copied to its owner; release the seed in
        // every mode so the CPU path retains only its live document/projection.
        scene.initial.clear(); scene.initial.shrink_to_fit();
        std::vector<double> elapsed, cpuTimes, frameTimes, tailTimes, paintTimes, composeTimes, displayTimes;
        Counters measured;
        double binMs = 0, paintMs = 0, composeMs = 0, dispatchMs = 0;
        for (uint32_t sample = 0; sample < options.samples + options.warmup; ++sample) {
            if (cpu) { cpu->reset(); cpu->compose(all); }
            gpu.reset();
            gpu.frame(all, mode, cpu.get(), false); gpu.wait();
            gpu.counters = {};
            std::vector<uint32_t> previous;
            const bool measuredSample = sample >= options.warmup;
            double lastInput = 0;
            const double startCpu = cpuMs(), start = wallMs();
            for (uint32_t f = 0; f < options.frames; ++f) {
                if (options.intervalMs > 0) {
                    const double remaining = start + f * options.intervalMs - wallMs();
                    if (remaining > 0) std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(remaining));
                }
                const double frameStart = wallMs(); lastInput = frameStart;
                auto batch = scene.batch(scene.frames[f], options.shape ? previous : std::vector<uint32_t>{});
                const double afterBin = wallMs();
                if (cpu) { if (options.shape) cpu->reset(); cpu->paint(batch); }
                const double afterPaint = wallMs();
                if (mode == Mode::Cpu) cpu->compose(batch);
                const double afterCompose = wallMs();
                gpu.frame(batch, mode, cpu.get(), options.shape);
                const double afterDispatch = wallMs();
                if ((f + 1) % inFlight == 0 || f + 1 == options.frames) gpu.wait();
                const double completed = wallMs();
                if (measuredSample) {
                    if (inFlight == 1) frameTimes.push_back(completed - frameStart);
                    binMs += afterBin - frameStart; paintMs += afterPaint - afterBin;
                    composeMs += afterCompose - afterPaint; dispatchMs += afterDispatch - afterCompose;
                }
                if (options.timestamps) {
                    const auto times = gpu.gpuTimes();
                    if (measuredSample) { paintTimes.push_back(times[0]); composeTimes.push_back(times[1]); displayTimes.push_back(times[2]); }
                }
                previous.clear();
                // Retain only this frame's footprint, rather than growing the
                // invalidation union across all previous shape revisions.
                if (options.shape) {
                    for (auto &j : batch.jobs) if (j.count) previous.push_back(j.logical);
                }
            }
            gpu.wait();
            const double end = wallMs(), endCpu = cpuMs();
            if (measuredSample) {
                elapsed.push_back(end - start); cpuTimes.push_back(endCpu - startCpu);
                tailTimes.push_back(end - lastInput); measured = gpu.counters;
            }
        }
        const auto document = mode == Mode::Cpu ? cpu->pixels : gpu.readDocument();
        const auto projection = gpu.readProjection();
        if (hash(document) != expectedDocument || hash(projection) != expectedProjection)
            throw std::runtime_error("final image differs from serial reference");
        const auto display = gpu.readDisplay();
        expectPixels(display, linear(scene, projection), "final display");
        if (mode == Mode::Gpu && (measured.pixelUpload || measured.readback))
            throw std::runtime_error("GPU-resident timing path transferred pixels");
        if (!dump.empty()) {
            std::ofstream out(dump, std::ios::binary); out << "P6\n" << options.size << ' ' << options.size << "\n255\n";
            for (auto p : display) {
                const uint32_t white = 255 - (p >> 24);
                for (unsigned c = 0; c < 24; c += 8) out.put(char(((p >> c) & 255) + white));
            }
        }
        const double frames = double(options.samples) * options.frames;
        std::cout << std::fixed << std::setprecision(6)
            << "{\"mode\":\"" << modeName << "\",\"verified\":true,\"size\":" << options.size
            << ",\"layers\":" << options.layers << ",\"diameter\":" << options.diameter
            << ",\"shape\":" << (options.shape ? "true" : "false")
            << ",\"varying\":" << (options.varying ? "true" : "false")
            << ",\"timestamps\":" << (options.timestamps ? "true" : "false")
            << ",\"in_flight\":" << inFlight << ",\"frames\":" << options.frames
            << ",\"workers\":" << options.workers << ",\"interval_ms\":" << options.intervalMs
            << ",\"adapter\":" << std::quoted(gpu.adapter)
            << ",\"samples_ms\":"; array(elapsed);
        std::cout << ",\"cpu_samples_ms\":"; array(cpuTimes);
        std::cout << ",\"frame_completion_ms\":"; array(frameTimes);
        std::cout << ",\"last_input_tail_ms\":"; array(tailTimes);
        std::cout << ",\"median_ms\":" << quantile(elapsed, .5)
            << ",\"p95_ms\":" << quantile(elapsed, .95)
            << ",\"median_cpu_ms\":" << quantile(cpuTimes, .5)
            << ",\"mean_frame_host_stages_wall_ms\":[" << binMs / frames << ',' << paintMs / frames << ',' << composeMs / frames << ',' << dispatchMs / frames << ']'
            << ",\"pixel_upload_bytes_per_sample\":" << measured.pixelUpload
            << ",\"command_upload_bytes_per_sample\":" << measured.commandUpload
            << ",\"pixel_readback_bytes_per_sample\":" << measured.readback
            << ",\"gpu_copy_bytes_per_sample\":" << measured.copyBytes
            << ",\"submissions_per_sample\":" << measured.submissions
            << ",\"write_calls_per_sample\":" << measured.writeCalls
            << ",\"document_bytes\":" << initialBytes << ",\"active_tile_count\":" << scene.activeTiles
            << ",\"gpu_allocated_bytes\":" << gpu.allocatedBytes
            << ",\"document_hash\":\"" << hex(expectedDocument) << "\",\"projection_hash\":\"" << hex(expectedProjection) << '"';
        if (options.timestamps) std::cout << ",\"median_gpu_pass_ms\":[" << quantile(paintTimes, .5) << ',' << quantile(composeTimes, .5) << ',' << quantile(displayTimes, .5) << ']';
        std::cout << "}\n";
    } catch (const std::exception &error) {
        std::cerr << "PoC failed: " << error.what() << '\n'; return 1;
    }
}
