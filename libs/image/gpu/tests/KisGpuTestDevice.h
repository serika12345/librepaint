/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_TEST_DEVICE_H
#define KIS_GPU_TEST_DEVICE_H

#include "KisGpuTileStore.h"
#include <QByteArray>
#include <QString>
#include <atomic>

/** Hardware device and explicit readback owned by GPU tests and measurements. */
struct KisGpuTestDevice {
    KisGpuTestDevice();
    ~KisGpuTestDevice();
    KisGpuTestDevice(const KisGpuTestDevice &) = delete;
    KisGpuTestDevice &operator=(const KisGpuTestDevice &) = delete;

    QByteArray read(const KisGpuTileStore::Version &version, QRect bounds);
    WGPUInstance instance = nullptr;
    WGPUAdapter adapter = nullptr;
    WGPUDevice device = nullptr;
    WGPUQueue queue = nullptr;
    std::atomic<int> errors{0};
    QString name;
private:
    void release();
};

#endif
