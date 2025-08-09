#include "graphics/backends/vulkan/vulkan_pipeline.hpp"

#include "core/logger/logger.hpp"
#include "graphics/backends/vulkan/core/vulkan_device.hpp"
#include "graphics/backends/vulkan/core/vulkan_frame_sync.hpp"
#include "graphics/backends/vulkan/core/vulkan_swapchain.hpp"
#include "graphics/backends/vulkan/utils/vk_utils.hpp"
#include "graphics/backends/vulkan/vulkan_gpu.hpp"

#include <expected>
#include <memory>
#include <vector>
#include <volk.h>
#include <vulkan/vulkan.h>

namespace vostok::graphics::vulkan
{

class VulkanPipeline::Impl
{
public:
    explicit Impl(VulkanGPU *gpu, VkPipeline pipeline, VkPipelineLayout layout);
    ~Impl();

    Impl(const Impl &) = delete;
    auto operator=(const Impl &) -> Impl & = delete;
    Impl(Impl &&) = delete;
    auto operator=(Impl &&) -> Impl & = delete;

    [[nodiscard]] auto getHandle() const -> VkPipeline;
    [[nodiscard]] auto getLayout() const -> VkPipelineLayout;

    void bind() const;

    auto pushRaw(std::span<const std::byte> data, u32 offset = 0)
        -> std::expected<void, std::string>;

private:
    VulkanGPU *m_gpu = nullptr;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_layout = VK_NULL_HANDLE;
};

auto VulkanPipeline::create(VulkanGPU *gpu, const PipelineCreateInfo &info)
    -> std::expected<std::unique_ptr<VulkanPipeline>, std::string>
{
    if (gpu == nullptr) {
        return std::unexpected("GPU cannot be null");
    }

    if (info.name.empty()) {
        return std::unexpected("Pipeline name cannot be empty");
    }

    Logger::debug("Creating Vulkan pipeline: {}", info.name);

    auto *device = gpu->getDevice();

    std::vector<VkShaderModule> shaderModules;
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

    if (info.vertexShader) {
        Logger::debug("Loading vertex shader: {}", info.vertexShader->string());
        auto vertexModule = utils::createShaderModule(
            device->getHandle(),
            info.vertexShader.value()
        );

        if (!vertexModule) {
            Logger::error(
                "Failed to create vertex shader module: {}",
                vertexModule.error()
            );
            return std::unexpected(vertexModule.error());
        }

        shaderModules.push_back(vertexModule.value());

        VkPipelineShaderStageCreateInfo stageInfo{};
        stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stageInfo.pNext = nullptr;
        stageInfo.flags = 0;
        stageInfo.module = vertexModule.value();
        stageInfo.pName = "main";
        stageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

        shaderStages.push_back(stageInfo);
        Logger::debug("Vertex shader stage created successfully");
    }

    if (info.fragmentShader) {
        Logger::debug(
            "Loading fragment shader: {}",
            info.fragmentShader->string()
        );
        auto fragmentModule = utils::createShaderModule(
            device->getHandle(),
            info.fragmentShader.value()
        );

        if (!fragmentModule) {
            Logger::error(
                "Failed to create fragment shader module: {}",
                fragmentModule.error()
            );
            return std::unexpected(fragmentModule.error());
        }

        shaderModules.push_back(fragmentModule.value());

        VkPipelineShaderStageCreateInfo stageInfo{};
        stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stageInfo.pNext = nullptr;
        stageInfo.flags = 0;
        stageInfo.module = fragmentModule.value();
        stageInfo.pName = "main";
        stageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;

        shaderStages.push_back(stageInfo);
        Logger::debug("Fragment shader stage created successfully");
    }

    if (info.geometryShader) {
        Logger::debug(
            "Loading geometry shader: {}",
            info.geometryShader->string()
        );
        auto geometryModule = utils::createShaderModule(
            device->getHandle(),
            info.geometryShader.value()
        );

        if (!geometryModule) {
            Logger::error(
                "Failed to create geometry shader module: {}",
                geometryModule.error()
            );
            return std::unexpected(geometryModule.error());
        }

        shaderModules.push_back(geometryModule.value());

        VkPipelineShaderStageCreateInfo stageInfo{};
        stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stageInfo.pNext = nullptr;
        stageInfo.flags = 0;
        stageInfo.module = geometryModule.value();
        stageInfo.pName = "main";
        stageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;

        shaderStages.push_back(stageInfo);
        Logger::debug("Geometry shader stage created successfully");
    }

    Logger::debug("Creating pipeline layout");
    std::vector<VkPushConstantRange> pushConstantRanges;
    if (info.pushConstantSize > 0) {
        VkPushConstantRange range{};
        range.stageFlags = VK_SHADER_STAGE_ALL;
        range.offset = 0;
        range.size = static_cast<u32>(info.pushConstantSize);
        pushConstantRanges.push_back(range);
        Logger::debug(
            "Adding push constant range: size={} bytes, stages=ALL",
            info.pushConstantSize
        );
    }

    auto layout = utils::createPipelineLayout(
        device->getHandle(),
        {},
        pushConstantRanges
    );

    if (!layout) {
        Logger::error("Failed to create pipeline layout: {}", layout.error());
        return std::unexpected(layout.error());
    }

    Logger::debug("Pipeline layout created successfully");

    VkFormat colorFormat = VK_FORMAT_UNDEFINED;
    if (auto *swapchain = gpu->getSwapchain()) {
        colorFormat = swapchain->getFormat();
        Logger::debug(
            "Using swapchain color format: {}",
            static_cast<int>(colorFormat)
        );
    } else {
        Logger::debug("No swapchain available, using undefined color format");
    }

    Logger::debug(
        "Creating graphics pipeline with {} shader stages",
        shaderStages.size()
    );
    auto pipeline = utils::createGraphicsPipeline(
        device->getHandle(),
        info,
        shaderStages,
        layout.value(),
        VK_NULL_HANDLE,
        0,
        colorFormat
    );

    if (!pipeline) {
        Logger::error(
            "Failed to create graphics pipeline: {}",
            pipeline.error()
        );
        return std::unexpected(pipeline.error());
    }

    Logger::info(
        "Vulkan pipeline '{}' created successfully with {} shader stages",
        info.name,
        shaderStages.size()
    );

    return std::make_unique<VulkanPipeline>(
        gpu,
        pipeline.value(),
        layout.value()
    );
}

VulkanPipeline::Impl::Impl(
    VulkanGPU *gpu,
    VkPipeline pipeline,
    VkPipelineLayout layout
)
    : m_gpu(gpu),
      m_pipeline(pipeline),
      m_layout(layout)
{
    Logger::debug("VulkanPipeline::Impl constructor called");
}

VulkanPipeline::Impl::~Impl()
{
    Logger::debug("VulkanPipeline::Impl destructor called");
    auto *device = m_gpu->getDevice();

    if (m_pipeline != VK_NULL_HANDLE) {
        Logger::debug("Destroying Vulkan pipeline");
        vkDestroyPipeline(device->getHandle(), m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }

    if (m_layout != VK_NULL_HANDLE) {
        Logger::debug("Destroying Vulkan pipeline layout");
        vkDestroyPipelineLayout(device->getHandle(), m_layout, nullptr);
        m_layout = VK_NULL_HANDLE;
    }
}

auto VulkanPipeline::Impl::getHandle() const -> VkPipeline
{
    return m_pipeline;
}

auto VulkanPipeline::Impl::getLayout() const -> VkPipelineLayout
{
    return m_layout;
}

void VulkanPipeline::Impl::bind() const
{
    Logger::trace("Binding Vulkan pipeline to command buffer");
    auto *frameSync = m_gpu->getFrameSync();
    auto *commandBuffer = frameSync->getCommandBuffer();

    vkCmdBindPipeline(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_pipeline
    );
}

auto VulkanPipeline::Impl::pushRaw(std::span<const std::byte> data, u32 offset)
    -> std::expected<void, std::string>
{
    if (m_gpu == nullptr || m_gpu->getFrameSync() == nullptr) {
        Logger::error("GPU or frame sync is not initialized");
        return std::unexpected("GPU or frame sync is not initialized");
    }

    auto *frameSync = m_gpu->getFrameSync();
    auto *commandBuffer = frameSync->getCommandBuffer();

    vkCmdPushConstants(
        commandBuffer,
        m_layout,
        VK_SHADER_STAGE_ALL,
        offset,
        static_cast<u32>(data.size()),
        data.data()
    );

    return {};
}

VulkanPipeline::VulkanPipeline(
    VulkanGPU *gpu,
    VkPipeline pipeline,
    VkPipelineLayout layout
)
    : m_impl(std::make_unique<Impl>(gpu, pipeline, layout))
{
    Logger::debug("VulkanPipeline constructor called");
}

VulkanPipeline::~VulkanPipeline() = default;

void VulkanPipeline::bind()
{
    m_impl->bind();
}

auto VulkanPipeline::pushRaw(std::span<const std::byte> data, u32 offset)
    -> std::expected<void, std::string>
{
    return m_impl->pushRaw(data, offset);
}

} // namespace vostok::graphics::vulkan