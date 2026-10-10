/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_TEST_DEVICE_H
#define KIS_GPU_TEST_DEVICE_H

#include "KisGpuTileStore.h"
#include "KisGpuDevice.h"
#include <QByteArray>
#include <QString>

/** Hardware device and explicit readback owned by GPU tests and measurements. */
struct KisGpuTestDevice {
    explicit KisGpuTestDevice(quint64 storageBindingLimit = 0, bool timestamps = false);
    ~KisGpuTestDevice();
    KisGpuTestDevice(const KisGpuTestDevice &) = delete;
    KisGpuTestDevice &operator=(const KisGpuTestDevice &) = delete;

    QByteArray read(const KisGpuTileStore::Version &version, QRect bounds);
    KisGpuDevice owner;
    WGPUDevice device;
    WGPUQueue queue;
    QString name;
};

#endif
