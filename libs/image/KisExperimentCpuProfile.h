/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef KIS_EXPERIMENT_CPU_PROFILE_H
#define KIS_EXPERIMENT_CPU_PROFILE_H
#include <kritaimage_export.h>
#include <QtGlobal>
#include <array>
#include <map>

// Issue #87 experiment: counters are enabled only during measured inputs.
namespace KisExperimentCpuProfile {
enum Stage { Dab, BrushApply, Composite, TileAccess, TileAllocate, TileCopy,
             DeviceBlend, ImageExtract, ReadPixels, WritePixels, WorkerLoop, StrokeJob, ProjectionJob, GpuFrame, GuiLoop, DabRaster, DabCacheLookup, DabCacheHit, Count };
struct Value { qint64 cpu = 0, exclusiveCpu = 0, wall = 0, calls = 0, units = 0, peakUnits = 0; };
using Snapshot = std::array<Value, Count>;
KRITAIMAGE_EXPORT void start();
KRITAIMAGE_EXPORT Snapshot stop();
using ThreadCpu = std::map<quint64,qint64>;
KRITAIMAGE_EXPORT ThreadCpu threadCpu();
class KRITAIMAGE_EXPORT Scope {
public:
    explicit Scope(Stage stage, qint64 units = 0);
    ~Scope();
    Scope(const Scope &) = delete;
    Scope &operator=(const Scope &) = delete;
private:
    Stage m_stage;
    bool m_enabled;
    qint64 m_cpu = 0, m_wall = 0, m_children = 0, m_units;
    Scope *m_parent = nullptr;
};
}
#endif
