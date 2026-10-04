/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisVulkanFrameWindow.h"

#include <QApplication>
#include <QFile>
#include <QScopeGuard>
#include <QVulkanInstance>
#include <QVulkanFunctions>
#include <array>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {
template<class T>
T vulkanInfo(VkStructureType type)
{
    T value{};
    value.sType = type;
    return value;
}

void checked(VkResult result, const char *operation)
{
    if (result != VK_SUCCESS) {
        throw std::runtime_error(QString("%1: VkResult %2").arg(operation).arg(result).toStdString());
    }
}

class FrameRenderer final : public QVulkanWindowRenderer
{
public:
    explicit FrameRenderer(KisVulkanFrameWindow *window) : w(window) {}

    void initResources() override
    {
        device = w->device();
        functions = w->vulkanInstance()->deviceFunctions(device);
        try {
            w->vulkanInstance()->functions()->vkGetPhysicalDeviceMemoryProperties(w->physicalDevice(), &memoryProperties);
            VkDescriptorSetLayoutBinding binding{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                                1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
            auto layout = vulkanInfo<VkDescriptorSetLayoutCreateInfo>(VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO);
            layout.bindingCount = 1;
            layout.pBindings = &binding;
            checked(functions->vkCreateDescriptorSetLayout(device, &layout, nullptr, &descriptorLayout), "descriptor layout");
            auto pipelineInfo = vulkanInfo<VkPipelineLayoutCreateInfo>(VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO);
            pipelineInfo.setLayoutCount = 1;
            pipelineInfo.pSetLayouts = &descriptorLayout;
            checked(functions->vkCreatePipelineLayout(device, &pipelineInfo, nullptr, &pipelineLayout), "pipeline layout");
            frames.resize(w->concurrentFrameCount());
            VkDescriptorPoolSize poolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, uint32_t(frames.size())};
            auto pool = vulkanInfo<VkDescriptorPoolCreateInfo>(VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO);
            pool.maxSets = uint32_t(frames.size());
            pool.poolSizeCount = 1;
            pool.pPoolSizes = &poolSize;
            checked(functions->vkCreateDescriptorPool(device, &pool, nullptr, &descriptorPool), "descriptor pool");
            for (auto &frame : frames) {
                auto alloc = vulkanInfo<VkDescriptorSetAllocateInfo>(VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO);
                alloc.descriptorPool = descriptorPool;
                alloc.descriptorSetCount = 1;
                alloc.pSetLayouts = &descriptorLayout;
                checked(functions->vkAllocateDescriptorSets(device, &alloc, &frame.descriptor), "descriptor set");
            }
            auto samplerInfo = vulkanInfo<VkSamplerCreateInfo>(VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO);
            samplerInfo.magFilter = samplerInfo.minFilter = VK_FILTER_NEAREST;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            samplerInfo.addressModeU = samplerInfo.addressModeV = samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            checked(functions->vkCreateSampler(device, &samplerInfo, nullptr, &sampler), "sampler");
            qInfo() << "MoltenVK PoC device:" << w->physicalDeviceProperties()->deviceName;
        } catch (const std::exception &e) {
            fail(e.what());
        }
    }

    void initSwapChainResources() override
    {
        if (failed) return;
        try {
            if (w->colorFormat() != VK_FORMAT_B8G8R8A8_UNORM &&
                w->colorFormat() != VK_FORMAT_R8G8B8A8_UNORM) {
                throw std::runtime_error("Display-encoded frames require an UNORM surface");
            }
            const auto vert = shader("frame.vert.spv");
            const auto releaseVertex = qScopeGuard([&] { functions->vkDestroyShaderModule(device, vert, nullptr); });
            const auto frag = shader("frame.frag.spv");
            const auto releaseFragment = qScopeGuard([&] { functions->vkDestroyShaderModule(device, frag, nullptr); });
            std::array<VkPipelineShaderStageCreateInfo, 2> stages{};
            stages[0] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
                         VK_SHADER_STAGE_VERTEX_BIT, vert, "main", nullptr};
            stages[1] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0,
                         VK_SHADER_STAGE_FRAGMENT_BIT, frag, "main", nullptr};
            auto vertex = vulkanInfo<VkPipelineVertexInputStateCreateInfo>(VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO);
            auto assembly = vulkanInfo<VkPipelineInputAssemblyStateCreateInfo>(VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO);
            assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            auto viewport = vulkanInfo<VkPipelineViewportStateCreateInfo>(VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO);
            viewport.viewportCount = viewport.scissorCount = 1;
            auto raster = vulkanInfo<VkPipelineRasterizationStateCreateInfo>(VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO);
            raster.polygonMode = VK_POLYGON_MODE_FILL;
            raster.lineWidth = 1;
            auto multisample = vulkanInfo<VkPipelineMultisampleStateCreateInfo>(VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO);
            multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
            auto depth = vulkanInfo<VkPipelineDepthStencilStateCreateInfo>(VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO);
            VkPipelineColorBlendAttachmentState attachment{};
            attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            auto blend = vulkanInfo<VkPipelineColorBlendStateCreateInfo>(VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO);
            blend.attachmentCount = 1;
            blend.pAttachments = &attachment;
            const VkDynamicState dynamicStates[]{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
            auto dynamic = vulkanInfo<VkPipelineDynamicStateCreateInfo>(VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO);
            dynamic.dynamicStateCount = 2;
            dynamic.pDynamicStates = dynamicStates;
            auto info = vulkanInfo<VkGraphicsPipelineCreateInfo>(VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO);
            info.stageCount = uint32_t(stages.size());
            info.pStages = stages.data();
            info.pVertexInputState = &vertex;
            info.pInputAssemblyState = &assembly;
            info.pViewportState = &viewport;
            info.pRasterizationState = &raster;
            info.pMultisampleState = &multisample;
            info.pDepthStencilState = &depth;
            info.pColorBlendState = &blend;
            info.pDynamicState = &dynamic;
            info.layout = pipelineLayout;
            info.renderPass = w->defaultRenderPass();
            const VkResult result = functions->vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline);
            checked(result, "graphics pipeline");
        } catch (const std::exception &e) {
            fail(e.what());
        }
    }

    void releaseSwapChainResources() override
    {
        if (pipeline) functions->vkDestroyPipeline(device, pipeline, nullptr);
        pipeline = VK_NULL_HANDLE;
    }

    void releaseResources() override
    {
        if (!device) return;
        functions->vkDeviceWaitIdle(device);
        releaseSwapChainResources();
        for (auto &frame : frames) releaseFrame(frame);
        frames.clear();
        if (sampler) functions->vkDestroySampler(device, sampler, nullptr);
        if (descriptorPool) functions->vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        if (pipelineLayout) functions->vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        if (descriptorLayout) functions->vkDestroyDescriptorSetLayout(device, descriptorLayout, nullptr);
        sampler = VK_NULL_HANDLE;
        descriptorPool = VK_NULL_HANDLE;
        pipelineLayout = VK_NULL_HANDLE;
        descriptorLayout = VK_NULL_HANDLE;
        device = VK_NULL_HANDLE;
    }

    void startNextFrame() override
    {
        const auto cmd = w->currentCommandBuffer();
        Frame *frame = nullptr;
        try {
            const QImage image = w->pendingFrame();
            if (!failed && !image.isNull()) {
                frame = &frames.at(w->currentFrame());
                upload(cmd, *frame, image);
            }
        } catch (const std::exception &e) {
            fail(e.what());
            frame = nullptr;
        }
        VkClearValue clear[2]{};
        clear[0].color.float32[3] = 1;
        clear[1].depthStencil.depth = 1;
        const QSize extent = w->swapChainImageSize();
        auto pass = vulkanInfo<VkRenderPassBeginInfo>(VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO);
        pass.renderPass = w->defaultRenderPass();
        pass.framebuffer = w->currentFramebuffer();
        pass.renderArea.extent = {uint32_t(extent.width()), uint32_t(extent.height())};
        pass.clearValueCount = 2;
        pass.pClearValues = clear;
        functions->vkCmdBeginRenderPass(cmd, &pass, VK_SUBPASS_CONTENTS_INLINE);
        if (frame && pipeline && !failed) {
            VkViewport viewport{0, 0, float(extent.width()), float(extent.height()), 0, 1};
            const VkRect2D scissor{{0, 0}, pass.renderArea.extent};
            functions->vkCmdSetViewport(cmd, 0, 1, &viewport);
            functions->vkCmdSetScissor(cmd, 0, 1, &scissor);
            functions->vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            functions->vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &frame->descriptor, 0, nullptr);
            functions->vkCmdDraw(cmd, 3, 1, 0, 0);
        }
        functions->vkCmdEndRenderPass(cmd);
        w->frameReady();
        Q_EMIT w->frameSubmitted();
    }

    void logicalDeviceLost() override { fail("Vulkan logical device lost"); }
    void physicalDeviceLost() override { fail("Vulkan physical device lost"); }

private:
    struct Frame {
        QSize size;
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory bufferMemory = VK_NULL_HANDLE;
        void *mapped = nullptr;
        bool coherent = false;
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory imageMemory = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkDescriptorSet descriptor = VK_NULL_HANDLE;
        bool uploaded = false;
    };

    uint32_t memoryType(uint32_t bits, VkMemoryPropertyFlags required)
    {
        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
            if ((bits & (1u << i)) && (memoryProperties.memoryTypes[i].propertyFlags & required) == required) return i;
        }
        throw std::runtime_error("Vulkan memory type unavailable");
    }

    VkShaderModule shader(const char *name)
    {
        QFile file(QString(":/librepaint/vulkan-poc/%1").arg(name));
        if (!file.open(QIODevice::ReadOnly)) throw std::runtime_error("SPIR-V resource unavailable");
        const QByteArray bytes = file.readAll();
        if (bytes.isEmpty() || bytes.size() % 4) throw std::runtime_error("Invalid SPIR-V resource");
        std::vector<uint32_t> words(size_t(bytes.size() / 4));
        std::memcpy(words.data(), bytes.constData(), size_t(bytes.size()));
        auto info = vulkanInfo<VkShaderModuleCreateInfo>(VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO);
        info.codeSize = size_t(bytes.size());
        info.pCode = words.data();
        VkShaderModule module = VK_NULL_HANDLE;
        checked(functions->vkCreateShaderModule(device, &info, nullptr, &module), "shader module");
        return module;
    }

    void releaseFrame(Frame &frame)
    {
        if (frame.mapped) functions->vkUnmapMemory(device, frame.bufferMemory);
        if (frame.view) functions->vkDestroyImageView(device, frame.view, nullptr);
        if (frame.image) functions->vkDestroyImage(device, frame.image, nullptr);
        if (frame.imageMemory) functions->vkFreeMemory(device, frame.imageMemory, nullptr);
        if (frame.buffer) functions->vkDestroyBuffer(device, frame.buffer, nullptr);
        if (frame.bufferMemory) functions->vkFreeMemory(device, frame.bufferMemory, nullptr);
        const auto descriptor = frame.descriptor;
        frame = Frame{};
        frame.descriptor = descriptor;
    }

    void allocateFrame(Frame &frame, QSize size)
    {
        if (uint32_t(size.width()) > w->physicalDeviceProperties()->limits.maxImageDimension2D ||
            uint32_t(size.height()) > w->physicalDeviceProperties()->limits.maxImageDimension2D) {
            throw std::runtime_error("Frame exceeds Vulkan image limits");
        }
        // Qt waits for this frame slot before startNextFrame; other slots stay live.
        releaseFrame(frame);
        auto buffer = vulkanInfo<VkBufferCreateInfo>(VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO);
        buffer.size = VkDeviceSize(size.width()) * VkDeviceSize(size.height()) * 4;
        buffer.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        checked(functions->vkCreateBuffer(device, &buffer, nullptr, &frame.buffer), "staging buffer");
        VkMemoryRequirements requirements;
        functions->vkGetBufferMemoryRequirements(device, frame.buffer, &requirements);
        const uint32_t type = memoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        frame.coherent = memoryProperties.memoryTypes[type].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        auto allocation = vulkanInfo<VkMemoryAllocateInfo>(VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO);
        allocation.allocationSize = requirements.size;
        allocation.memoryTypeIndex = type;
        checked(functions->vkAllocateMemory(device, &allocation, nullptr, &frame.bufferMemory), "staging memory");
        checked(functions->vkBindBufferMemory(device, frame.buffer, frame.bufferMemory, 0), "staging binding");
        checked(functions->vkMapMemory(device, frame.bufferMemory, 0, VK_WHOLE_SIZE, 0, &frame.mapped), "staging mapping");
        auto image = vulkanInfo<VkImageCreateInfo>(VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO);
        image.imageType = VK_IMAGE_TYPE_2D;
        image.format = VK_FORMAT_R8G8B8A8_UNORM;
        image.extent = {uint32_t(size.width()), uint32_t(size.height()), 1};
        image.mipLevels = image.arrayLayers = 1;
        image.samples = VK_SAMPLE_COUNT_1_BIT;
        image.tiling = VK_IMAGE_TILING_OPTIMAL;
        image.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        checked(functions->vkCreateImage(device, &image, nullptr, &frame.image), "frame image");
        functions->vkGetImageMemoryRequirements(device, frame.image, &requirements);
        allocation.allocationSize = requirements.size;
        allocation.memoryTypeIndex = memoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        checked(functions->vkAllocateMemory(device, &allocation, nullptr, &frame.imageMemory), "frame memory");
        checked(functions->vkBindImageMemory(device, frame.image, frame.imageMemory, 0), "frame binding");
        auto view = vulkanInfo<VkImageViewCreateInfo>(VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
        view.image = frame.image;
        view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view.format = image.format;
        view.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        checked(functions->vkCreateImageView(device, &view, nullptr, &frame.view), "frame view");
        VkDescriptorImageInfo descriptor{sampler, frame.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        auto write = vulkanInfo<VkWriteDescriptorSet>(VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET);
        write.dstSet = frame.descriptor;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo = &descriptor;
        functions->vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
        frame.size = size;
    }

    void upload(VkCommandBuffer cmd, Frame &frame, const QImage &image)
    {
        if (frame.size != image.size()) allocateFrame(frame, image.size());
        const size_t stride = size_t(image.width()) * 4;
        for (int y = 0; y < image.height(); ++y) {
            std::memcpy(static_cast<char *>(frame.mapped) + size_t(y) * stride, image.constScanLine(y), stride);
        }
        if (!frame.coherent) {
            auto range = vulkanInfo<VkMappedMemoryRange>(VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE);
            range.memory = frame.bufferMemory;
            range.size = VK_WHOLE_SIZE;
            checked(functions->vkFlushMappedMemoryRanges(device, 1, &range), "staging flush");
        }
        auto barrier = vulkanInfo<VkImageMemoryBarrier>(VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER);
        barrier.oldLayout = frame.uploaded ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcAccessMask = frame.uploaded ? VK_ACCESS_SHADER_READ_BIT : 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = frame.image;
        barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        functions->vkCmdPipelineBarrier(cmd, frame.uploaded ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        VkBufferImageCopy copy{};
        copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        copy.imageExtent = {uint32_t(image.width()), uint32_t(image.height()), 1};
        functions->vkCmdCopyBufferToImage(cmd, frame.buffer, frame.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        functions->vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &barrier);
        frame.uploaded = true;
    }

    void fail(const QString &message)
    {
        if (!failed) w->reportFailure(message);
        failed = true;
    }

    KisVulkanFrameWindow *w;
    VkDevice device = VK_NULL_HANDLE;
    QVulkanDeviceFunctions *functions = nullptr;
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    VkDescriptorSetLayout descriptorLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    std::vector<Frame> frames;
    bool failed = false;
};
}

KisVulkanFrameWindow::KisVulkanFrameWindow(QVulkanInstance *instance, QWidget *receiver)
    : m_inputReceiver(receiver)
{
    setVulkanInstance(instance);
    setPreferredColorFormats({VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM});
    setDeviceExtensions({"VK_KHR_portability_subset"});
    setEnabledFeaturesModifier([](VkPhysicalDeviceFeatures &features) { features = {}; });
}

void KisVulkanFrameWindow::setFrame(const QImage &frame)
{
    if (frame.isNull()) return;
    m_frame = frame.convertToFormat(QImage::Format_RGBA8888);
    requestUpdate();
}

void KisVulkanFrameWindow::reportFailure(const QString &message)
{
    qWarning() << "MoltenVK PoC failed:" << message;
    Q_EMIT renderFailed(message);
}

QVulkanWindowRenderer *KisVulkanFrameWindow::createRenderer()
{
    return new FrameRenderer(this);
}

bool KisVulkanFrameWindow::event(QEvent *event)
{
    if (m_inputReceiver) {
        switch (event->type()) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseMove:
        case QEvent::Wheel:
        case QEvent::KeyPress:
        case QEvent::KeyRelease:
        case QEvent::FocusIn:
        case QEvent::FocusOut:
        case QEvent::Enter:
        case QEvent::Leave:
        case QEvent::TabletPress:
        case QEvent::TabletMove:
        case QEvent::TabletRelease: {
            std::unique_ptr<QEvent> copy(event->clone());
            QApplication::sendEvent(m_inputReceiver, copy.get());
            event->setAccepted(copy->isAccepted());
            return true;
        }
        default:
            break;
        }
    }
    return QVulkanWindow::event(event);
}
