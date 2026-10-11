/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuInstance_p.h"
#include <stdexcept>

namespace {
/** API initialization outlives replaceable GPU devices and their driver resources. */
struct ProcessInstance {
    WGPUInstance instance = nullptr;
    ProcessInstance() {
        if (wgpuGetVersion() != 0x1b000400) throw std::runtime_error("GPU document requires wgpu-native 27.0.4.0");
        if (wgpuLibrePaintRecoveryRevision() != 3) throw std::runtime_error("GPU document requires recovery revision 3");
        WGPUInstanceExtras extras{};
        extras.chain.sType = static_cast<WGPUSType>(WGPUSType_InstanceExtras);
#ifdef __APPLE__
        extras.backends = WGPUInstanceBackend_Metal;
#else
        extras.backends = WGPUInstanceBackend_Vulkan;
#endif
        extras.flags = WGPUInstanceFlag_Validation;
        WGPUInstanceDescriptor descriptor{};
        descriptor.nextInChain = &extras.chain;
        instance = wgpuCreateInstance(&descriptor);
        if (!instance) throw std::runtime_error("Cannot create GPU instance");
    }
    ~ProcessInstance() { wgpuInstanceRelease(instance); }
};
}

WGPUInstance KisGpuTileStorage::processInstance()
{
    static ProcessInstance owner;
    return owner.instance;
}
