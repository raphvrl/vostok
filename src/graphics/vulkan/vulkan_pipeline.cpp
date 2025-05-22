#include "graphics/vulkan/vulkan_pipeline.hpp"

#include "core/logger/logger.hpp"
#include "graphics/vulkan/core/device.hpp"
#include "graphics/vulkan/core/frame_sync.hpp"
#include "graphics/vulkan/core/swapchain.hpp"
#include "graphics/vulkan/utils/vk_utils.hpp"
#include "graphics/vulkan/vulkan_device.hpp"

#include <expected>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <volk.h>

namespace vostok::graphics::vulkan
{

class VulkanPipeline::Impl
{
public:
    explicit Impl(VulkanDevice *device, VkPipeline pipeline, VkPipelineLayout layout);
    ~Impl();

    Impl(Impl &) = delete;
    Impl &operator=(const Impl &) = delete;
    Impl(Impl &&) = delete;
    Impl &operator=(Impl &&) = delete;

    [[nodiscard]] VkPipeline getHandle() const { return m_pipeline; }
    [[nodiscard]] VkPipelineLayout getLayout() const { return m_layout; }

    void bind();

private:
    VulkanDevice *m_ctx;

    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_layout = VK_NULL_HANDLE;

    friend class VulkanPipeline::Builder;
};

class VulkanPipeline::Builder::Impl
{
public:
    explicit Impl(VulkanDevice *device);
    ~Impl() = default;

    Impl(Impl &) = delete;
    Impl &operator=(const Impl &) = delete;
    Impl(Impl &&) = delete;
    Impl &operator=(Impl &&) = delete;

    Impl &setVertexShader(const fs::path &path);
    Impl &setFragmentShader(const fs::path &path);
    Impl &setGeometryShader(const fs::path &path);
    Impl &setTessellationControlShader(const fs::path &path);
    Impl &setTessellationEvaluationShader(const fs::path &path);
    Impl &setComputeShader(const fs::path &path);

    Impl &setPrimitiveTopology(const PrimitiveTopology &topology);

    Impl &setPolygonMode(const PolygonMode &mode);
    Impl &setCullMode(const CullMode &mode);
    Impl &setFrontFace(const FrontFace &face);
    Impl &setLineWidth(f32 width);

    Impl &setDepthTest(bool enable);
    Impl &setDepthWrite(bool enable);
    Impl &setDepthCompareOp(const CompareOp &op);
    Impl &setStencilTest(bool enable);
    Impl &
    setStencilOp(const StencilOp &failOp, const StencilOp &passOp, const StencilOp &depthFailOp);

    Impl &setBlend(bool enable);
    Impl &setBlendFactor(
        const BlendFactor &srcColor,
        const BlendFactor &dstColor,
        const BlendFactor &srcAlpha,
        const BlendFactor &dstAlpha
    );
    Impl &setBlendOp(const BlendOp &colorOp, const BlendOp &alphaOp);
    Impl &setColorWriteMask(const ColorComponentFlags &mask);

    Impl &addPushConstant(u32 size);

    Impl &setName(const std::string &name);

    std::expected<std::unique_ptr<Pipeline>, std::string> build();

private:
    static std::vector<char> readFile(const fs::path &path);
    std::expected<VkShaderModule, std::string> createShaderModule(const std::vector<char> &code);

    VulkanDevice *m_ctx;
    std::string m_name;

    struct ShaderStage
    {
        fs::path path;
        VkShaderStageFlagBits stage;
    };

    std::vector<ShaderStage> m_shaderStages;

    VkPrimitiveTopology m_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPolygonMode m_polygonMode = VK_POLYGON_MODE_FILL;
    VkCullModeFlags m_cullMode = VK_CULL_MODE_BACK_BIT;
    VkFrontFace m_frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    float m_lineWidth = 1.0F;

    bool m_depthTestEnable = true;
    bool m_depthWriteEnable = true;
    VkCompareOp m_depthCompareOp = VK_COMPARE_OP_LESS;
    bool m_stencilTestEnable = false;
    VkStencilOpState m_frontStencil = {};
    VkStencilOpState m_backStencil = {};

    bool m_blendEnable = false;
    VkBlendFactor m_srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    VkBlendFactor m_dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    VkBlendFactor m_srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    VkBlendFactor m_dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    VkBlendOp m_colorBlendOp = VK_BLEND_OP_ADD;
    VkBlendOp m_alphaBlendOp = VK_BLEND_OP_ADD;
    VkColorComponentFlags m_colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                             VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    std::vector<VkVertexInputBindingDescription> m_vertexBindings;
    std::vector<VkVertexInputAttributeDescription> m_vertexAttributes;

    std::vector<VkPushConstantRange> m_pushConstants;
};

VulkanPipeline::Impl::Impl(VulkanDevice *device, VkPipeline pipeline, VkPipelineLayout layout)
    : m_ctx(device),
      m_pipeline(pipeline),
      m_layout(layout)
{
    Logger::info(
        "Created VulkanPipeline [handle: {:#x}, layout: {:#x}]",
        std::bit_cast<u64>(m_pipeline),
        std::bit_cast<u64>(m_layout)
    );
}

VulkanPipeline::Impl::~Impl()
{
    auto *device = m_ctx->getDevice();
    if (device == nullptr) {
        return;
    }

    if (m_pipeline != VK_NULL_HANDLE) {
        Logger::debug("Destroying VulkanPipeline [handle: {:#x}]", std::bit_cast<u64>(m_pipeline));
        vkDestroyPipeline(device->getHandle(), m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }

    if (m_layout != VK_NULL_HANDLE) {
        Logger::debug(
            "Destroying VulkanPipelineLayout [handle: {:#x}]",
            std::bit_cast<u64>(m_layout)
        );
        vkDestroyPipelineLayout(device->getHandle(), m_layout, nullptr);
        m_layout = VK_NULL_HANDLE;
    }
}

void VulkanPipeline::Impl::bind()
{
    auto *device = m_ctx->getDevice();
    if (device == nullptr) {
        Logger::warning("Cannot bind pipeline: device is null");
        return;
    }

    auto *syncFrame = m_ctx->getFrameSync();
    if (syncFrame == nullptr) {
        Logger::warning("Cannot bind pipeline: frame sync is null");
        return;
    }

    Logger::trace("Binding pipeline [handle: {:#x}]", std::bit_cast<u64>(m_pipeline));
    vkCmdBindPipeline(syncFrame->getCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
}

VulkanPipeline::Builder::Impl::Impl(VulkanDevice *device)
    : m_ctx(device)
{
    Logger::debug("Created VulkanPipeline Builder");
}

VulkanPipeline::Builder::Impl &VulkanPipeline::Builder::Impl::setVertexShader(const fs::path &path)
{
    Logger::debug("Setting vertex shader: {}", path.string());
    m_shaderStages.push_back({ path, VK_SHADER_STAGE_VERTEX_BIT });
    return *this;
}

VulkanPipeline::Builder::Impl &
VulkanPipeline::Builder::Impl::setFragmentShader(const fs::path &path)
{
    Logger::debug("Setting fragment shader: {}", path.string());
    m_shaderStages.push_back({ path, VK_SHADER_STAGE_FRAGMENT_BIT });
    return *this;
}

VulkanPipeline::Builder::Impl &
VulkanPipeline::Builder::Impl::setGeometryShader(const fs::path &path)
{
    Logger::debug("Setting geometry shader: {}", path.string());
    m_shaderStages.push_back({ path, VK_SHADER_STAGE_GEOMETRY_BIT });
    return *this;
}

VulkanPipeline::Builder::Impl &
VulkanPipeline::Builder::Impl::setTessellationControlShader(const fs::path &path)
{
    Logger::debug("Setting tessellation control shader: {}", path.string());
    m_shaderStages.push_back({ path, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT });
    return *this;
}

VulkanPipeline::Builder::Impl &
VulkanPipeline::Builder::Impl::setTessellationEvaluationShader(const fs::path &path)
{
    Logger::debug("Setting tessellation evaluation shader: {}", path.string());
    m_shaderStages.push_back({ path, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT });
    return *this;
}

VulkanPipeline::Builder::Impl &VulkanPipeline::Builder::Impl::setComputeShader(const fs::path &path)
{
    Logger::debug("Setting compute shader: {}", path.string());
    m_shaderStages.push_back({ path, VK_SHADER_STAGE_COMPUTE_BIT });
    return *this;
}

VulkanPipeline::Builder::Impl &
VulkanPipeline::Builder::Impl::setPrimitiveTopology(const PrimitiveTopology &topology)
{
    static const std::unordered_map<PrimitiveTopology, VkPrimitiveTopology> TOPOLOGY_MAP = {
        { PrimitiveTopology::POINT_LIST, VK_PRIMITIVE_TOPOLOGY_POINT_LIST },
        { PrimitiveTopology::LINE_LIST, VK_PRIMITIVE_TOPOLOGY_LINE_LIST },
        { PrimitiveTopology::LINE_STRIP, VK_PRIMITIVE_TOPOLOGY_LINE_STRIP },
        { PrimitiveTopology::TRIANGLE_LIST, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST },
        { PrimitiveTopology::TRIANGLE_STRIP, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP },
    };

    auto it = TOPOLOGY_MAP.find(topology);
    if (it != TOPOLOGY_MAP.end()) {
        m_topology = it->second;
    } else {
        Logger::error("Invalid primitive topology");
    }

    Logger::debug("Setting primitive topology: {}", utils::vkPrimitiveTopologyToString(m_topology));

    return *this;
}

VulkanPipeline::Builder::Impl &
VulkanPipeline::Builder::Impl::setPolygonMode(const PolygonMode &mode)
{
    static const std::unordered_map<PolygonMode, VkPolygonMode> POLYGON_MODE_MAP = {
        { PolygonMode::FILL, VK_POLYGON_MODE_FILL },
        { PolygonMode::LINE, VK_POLYGON_MODE_LINE },
        { PolygonMode::POINT, VK_POLYGON_MODE_POINT },
        { PolygonMode::FILL_LINE, VK_POLYGON_MODE_FILL },
        { PolygonMode::SOLID, VK_POLYGON_MODE_FILL },
    };

    auto it = POLYGON_MODE_MAP.find(mode);
    if (it != POLYGON_MODE_MAP.end()) {
        m_polygonMode = it->second;
    } else {
        Logger::error("Invalid polygon mode");
    }

    Logger::debug("Setting polygon mode: {}", utils::vkPolygonModeToString(m_polygonMode));

    return *this;
}

VulkanPipeline::Builder::Impl &VulkanPipeline::Builder::Impl::setCullMode(const CullMode &mode)
{
    static const std::unordered_map<CullMode, VkCullModeFlags> CULL_MODE_MAP = {
        { CullMode::NONE, VK_CULL_MODE_NONE },
        { CullMode::FRONT, VK_CULL_MODE_FRONT_BIT },
        { CullMode::BACK, VK_CULL_MODE_BACK_BIT },
        { CullMode::FRONT_AND_BACK, VK_CULL_MODE_FRONT_AND_BACK },
    };

    auto it = CULL_MODE_MAP.find(mode);
    if (it != CULL_MODE_MAP.end()) {
        m_cullMode = it->second;
    } else {
        Logger::error("Invalid cull mode");
    }

    Logger::debug("Setting cull mode: {}", utils::vkCullModeToString(m_cullMode));

    return *this;
}

VulkanPipeline::Builder::Impl &VulkanPipeline::Builder::Impl::setFrontFace(const FrontFace &face)
{
    static const std::unordered_map<FrontFace, VkFrontFace> FRONT_FACE_MAP = {
        { FrontFace::COUNTER_CLOCKWISE, VK_FRONT_FACE_COUNTER_CLOCKWISE },
        { FrontFace::CLOCKWISE, VK_FRONT_FACE_CLOCKWISE },
    };

    auto it = FRONT_FACE_MAP.find(face);
    if (it != FRONT_FACE_MAP.end()) {
        m_frontFace = it->second;
    } else {
        Logger::error("Invalid front face");
    }

    Logger::debug("Setting front face: {}", utils::vkFrontFaceToString(m_frontFace));

    return *this;
}

VulkanPipeline::Builder::Impl &VulkanPipeline::Builder::Impl::setLineWidth(f32 width)
{
    Logger::debug("Setting line width: {}", width);

    m_lineWidth = width;
    return *this;
}

VulkanPipeline::Builder::Impl &VulkanPipeline::Builder::Impl::setDepthTest(bool enable)
{
    Logger::debug("Setting depth test: {}", enable ? "true" : "false");

    m_depthTestEnable = enable;
    return *this;
}

VulkanPipeline::Builder::Impl &VulkanPipeline::Builder::Impl::setDepthWrite(bool enable)
{
    Logger::debug("Setting depth write: {}", enable ? "true" : "false");

    m_depthWriteEnable = enable;
    return *this;
}

VulkanPipeline::Builder::Impl &VulkanPipeline::Builder::Impl::setDepthCompareOp(const CompareOp &op)
{
    static const std::unordered_map<CompareOp, VkCompareOp> COMPARE_OP_MAP = {
        { CompareOp::NEVER, VK_COMPARE_OP_NEVER },
        { CompareOp::LESS, VK_COMPARE_OP_LESS },
        { CompareOp::EQUAL, VK_COMPARE_OP_EQUAL },
        { CompareOp::LESS_OR_EQUAL, VK_COMPARE_OP_LESS_OR_EQUAL },
        { CompareOp::GREATER, VK_COMPARE_OP_GREATER },
        { CompareOp::NOT_EQUAL, VK_COMPARE_OP_NOT_EQUAL },
        { CompareOp::GREATER_OR_EQUAL, VK_COMPARE_OP_GREATER_OR_EQUAL },
        { CompareOp::ALWAYS, VK_COMPARE_OP_ALWAYS },
    };

    auto it = COMPARE_OP_MAP.find(op);
    if (it != COMPARE_OP_MAP.end()) {
        m_depthCompareOp = it->second;
    } else {
        Logger::error("Invalid depth compare operation");
    }

    Logger::debug(
        "Setting depth compare operation: {}",
        utils::vkCompareOpToString(m_depthCompareOp)
    );

    return *this;
}

VulkanPipeline::Builder::Impl &VulkanPipeline::Builder::Impl::setStencilTest(bool enable)
{
    Logger::debug("Setting stencil test: {}", enable ? "true" : "false");

    m_stencilTestEnable = enable;
    return *this;
}

VulkanPipeline::Builder::Impl &VulkanPipeline::Builder::Impl::setStencilOp(
    const StencilOp &failOp,
    const StencilOp &passOp,
    const StencilOp &depthFailOp
)
{
    static const std::unordered_map<StencilOp, VkStencilOp> STENCIL_OP_MAP = {
        { StencilOp::KEEP, VK_STENCIL_OP_KEEP },
        { StencilOp::ZERO, VK_STENCIL_OP_ZERO },
        { StencilOp::REPLACE, VK_STENCIL_OP_REPLACE },
        { StencilOp::INCREMENT_AND_CLAMP, VK_STENCIL_OP_INCREMENT_AND_CLAMP },
        { StencilOp::DECREMENT_AND_CLAMP, VK_STENCIL_OP_DECREMENT_AND_CLAMP },
        { StencilOp::INVERT, VK_STENCIL_OP_INVERT },
        { StencilOp::INCREMENT_AND_WRAP, VK_STENCIL_OP_INCREMENT_AND_WRAP },
        { StencilOp::DECREMENT_AND_WRAP, VK_STENCIL_OP_DECREMENT_AND_WRAP },
    };

    auto it = STENCIL_OP_MAP.find(failOp);
    if (it != STENCIL_OP_MAP.end()) {
        m_frontStencil.failOp = it->second;
    } else {
        Logger::error("Invalid stencil fail operation");
    }

    it = STENCIL_OP_MAP.find(passOp);
    if (it != STENCIL_OP_MAP.end()) {
        m_frontStencil.passOp = it->second;
    } else {
        Logger::error("Invalid stencil pass operation");
    }

    it = STENCIL_OP_MAP.find(depthFailOp);
    if (it != STENCIL_OP_MAP.end()) {
        m_frontStencil.depthFailOp = it->second;
    } else {
        Logger::error("Invalid stencil depth fail operation");
    }

    Logger::debug(
        "Setting stencil operations: failOp: {}, passOp: {}, depthFailOp: {}",
        utils::vkStencilOpToString(m_frontStencil.failOp),
        utils::vkStencilOpToString(m_frontStencil.passOp),
        utils::vkStencilOpToString(m_frontStencil.depthFailOp)
    );

    return *this;
}

VulkanPipeline::Builder::Impl &VulkanPipeline::Builder::Impl::setBlend(bool enable)
{
    Logger::debug("Setting blend: {}", enable ? "true" : "false");

    m_blendEnable = enable;
    return *this;
}

VulkanPipeline::Builder::Impl &VulkanPipeline::Builder::Impl::setBlendFactor(
    const BlendFactor &srcColor,
    const BlendFactor &dstColor,
    const BlendFactor &srcAlpha,
    const BlendFactor &dstAlpha
)
{
    static const std::unordered_map<BlendFactor, VkBlendFactor> BLEND_FACTOR_MAP = {
        { BlendFactor::ZERO, VK_BLEND_FACTOR_ZERO },
        { BlendFactor::ONE, VK_BLEND_FACTOR_ONE },
        { BlendFactor::SRC_COLOR, VK_BLEND_FACTOR_SRC_COLOR },
        { BlendFactor::ONE_MINUS_SRC_COLOR, VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR },
        { BlendFactor::DST_COLOR, VK_BLEND_FACTOR_DST_COLOR },
        { BlendFactor::ONE_MINUS_DST_COLOR, VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR },
        { BlendFactor::SRC_ALPHA, VK_BLEND_FACTOR_SRC_ALPHA },
        { BlendFactor::ONE_MINUS_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA },
        { BlendFactor::DST_ALPHA, VK_BLEND_FACTOR_DST_ALPHA },
        { BlendFactor::ONE_MINUS_DST_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA },
        { BlendFactor::CONSTANT_COLOR, VK_BLEND_FACTOR_CONSTANT_COLOR },
        { BlendFactor::ONE_MINUS_CONSTANT_COLOR, VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR },
        { BlendFactor::CONSTANT_ALPHA, VK_BLEND_FACTOR_CONSTANT_ALPHA },
        { BlendFactor::ONE_MINUS_CONSTANT_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA },
    };

    auto it = BLEND_FACTOR_MAP.find(srcColor);
    if (it != BLEND_FACTOR_MAP.end()) {
        m_srcColorBlendFactor = it->second;
    } else {
        Logger::error("Invalid source color blend factor");
    }

    it = BLEND_FACTOR_MAP.find(dstColor);
    if (it != BLEND_FACTOR_MAP.end()) {
        m_dstColorBlendFactor = it->second;
    } else {
        Logger::error("Invalid destination color blend factor");
    }

    it = BLEND_FACTOR_MAP.find(srcAlpha);
    if (it != BLEND_FACTOR_MAP.end()) {
        m_srcAlphaBlendFactor = it->second;
    } else {
        Logger::error("Invalid source alpha blend factor");
    }

    it = BLEND_FACTOR_MAP.find(dstAlpha);
    if (it != BLEND_FACTOR_MAP.end()) {
        m_dstAlphaBlendFactor = it->second;
    } else {
        Logger::error("Invalid destination alpha blend factor");
    }

    Logger::debug(
        "Setting blend factors: srcColor: {}, dstColor: {}, srcAlpha: {}, dstAlpha: {}",
        utils::vkBlendFactorToString(m_srcColorBlendFactor),
        utils::vkBlendFactorToString(m_dstColorBlendFactor),
        utils::vkBlendFactorToString(m_srcAlphaBlendFactor),
        utils::vkBlendFactorToString(m_dstAlphaBlendFactor)
    );

    return *this;
}

VulkanPipeline::Builder::Impl &
VulkanPipeline::Builder::Impl::setBlendOp(const BlendOp &colorOp, const BlendOp &alphaOp)
{
    static const std::unordered_map<BlendOp, VkBlendOp> BLEND_OP_MAP = {
        { BlendOp::ADD, VK_BLEND_OP_ADD },
        { BlendOp::SUBTRACT, VK_BLEND_OP_SUBTRACT },
        { BlendOp::REVERSE_SUBTRACT, VK_BLEND_OP_REVERSE_SUBTRACT },
        { BlendOp::MIN, VK_BLEND_OP_MIN },
        { BlendOp::MAX, VK_BLEND_OP_MAX },
    };

    auto it = BLEND_OP_MAP.find(colorOp);
    if (it != BLEND_OP_MAP.end()) {
        m_colorBlendOp = it->second;
    } else {
        Logger::error("Invalid color blend operation");
    }

    it = BLEND_OP_MAP.find(alphaOp);
    if (it != BLEND_OP_MAP.end()) {
        m_alphaBlendOp = it->second;
    } else {
        Logger::error("Invalid alpha blend operation");
    }

    Logger::debug(
        "Setting blend operations: colorOp: {}, alphaOp: {}",
        utils::vkBlendOpToString(m_colorBlendOp),
        utils::vkBlendOpToString(m_alphaBlendOp)
    );

    return *this;
}

VulkanPipeline::Builder::Impl &
VulkanPipeline::Builder::Impl::setColorWriteMask(const ColorComponentFlags &mask)
{
    static const std::unordered_map<ColorComponentFlags, VkColorComponentFlags> COLOR_MASK_MAP = {
        { ColorComponentFlags::RED, VK_COLOR_COMPONENT_R_BIT },
        { ColorComponentFlags::GREEN, VK_COLOR_COMPONENT_G_BIT },
        { ColorComponentFlags::BLUE, VK_COLOR_COMPONENT_B_BIT },
        { ColorComponentFlags::ALPHA, VK_COLOR_COMPONENT_A_BIT },
        { ColorComponentFlags::ALL,
          VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
              VK_COLOR_COMPONENT_A_BIT },
    };

    auto it = COLOR_MASK_MAP.find(mask);
    if (it != COLOR_MASK_MAP.end()) {
        m_colorWriteMask = it->second;
    } else {
        Logger::error("Invalid color write mask");
    }

    Logger::debug(
        "Setting color write mask: {}",
        utils::vkColorComponentFlagsToString(m_colorWriteMask)
    );

    return *this;
}

VulkanPipeline::Builder::Impl &VulkanPipeline::Builder::Impl::addPushConstant(u32 size)
{
    Logger::debug("Adding push constant: size = {}", size);

    m_pushConstants.push_back({ VK_SHADER_STAGE_ALL, 0, size });
    return *this;
}

VulkanPipeline::Builder::Impl &VulkanPipeline::Builder::Impl::setName(const std::string &name)
{
    Logger::debug("Setting pipeline name: {}", name);

    m_name = name;
    return *this;
}

std::expected<std::unique_ptr<Pipeline>, std::string> VulkanPipeline::Builder::Impl::build()
{
    auto *device = m_ctx->getDevice();
    if (device == nullptr) {
        return std::unexpected("Vulkan device is not initialized");
    }

    auto *swapchain = m_ctx->getSwapchain();

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    std::vector<VkShaderModule> shaderModules;

    for (const auto &shaderStage : m_shaderStages) {
        auto code = readFile(shaderStage.path);
        if (code.empty()) {
            return std::unexpected("Failed to read shader file: " + shaderStage.path.string());
        }

        auto shaderModuleResult = createShaderModule(code);
        if (!shaderModuleResult) {
            return std::unexpected(shaderModuleResult.error());
        }

        shaderModules.push_back(shaderModuleResult.value());

        VkPipelineShaderStageCreateInfo stageInfo = {};
        stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stageInfo.stage = shaderStage.stage;
        stageInfo.module = shaderModules.back();
        stageInfo.pName = "main";

        shaderStages.push_back(stageInfo);
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<u32>(m_vertexBindings.size());
    vertexInputInfo.pVertexBindingDescriptions =
        m_vertexBindings.empty() ? nullptr : m_vertexBindings.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<u32>(m_vertexAttributes.size());
    vertexInputInfo.pVertexAttributeDescriptions =
        m_vertexAttributes.empty() ? nullptr : m_vertexAttributes.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = m_topology;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = m_polygonMode;
    rasterizer.cullMode = m_cullMode;
    rasterizer.frontFace = m_frontFace;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.lineWidth = m_lineWidth;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = m_depthTestEnable ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = m_depthWriteEnable ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp = m_depthCompareOp;
    depthStencil.stencilTestEnable = m_stencilTestEnable ? VK_TRUE : VK_FALSE;
    depthStencil.front = m_frontStencil;
    depthStencil.back = m_backStencil;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.blendEnable = m_blendEnable ? VK_TRUE : VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = m_srcColorBlendFactor;
    colorBlendAttachment.dstColorBlendFactor = m_dstColorBlendFactor;
    colorBlendAttachment.colorBlendOp = m_colorBlendOp;
    colorBlendAttachment.srcAlphaBlendFactor = m_srcAlphaBlendFactor;
    colorBlendAttachment.dstAlphaBlendFactor = m_dstAlphaBlendFactor;
    colorBlendAttachment.alphaBlendOp = m_alphaBlendOp;
    colorBlendAttachment.colorWriteMask = m_colorWriteMask;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT,
                                                  VK_DYNAMIC_STATE_SCISSOR };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<u32>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    auto format = swapchain->getFormat();

    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachmentFormats = &format;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pushConstantRangeCount = static_cast<u32>(m_pushConstants.size());
    pipelineLayoutInfo.pPushConstantRanges =
        m_pushConstants.empty() ? nullptr : m_pushConstants.data();

    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkResult result =
        vkCreatePipelineLayout(device->getHandle(), &pipelineLayoutInfo, nullptr, &pipelineLayout);
    if (result != VK_SUCCESS) {
        return std::unexpected(
            "Failed to create pipeline layout: " + utils::vkResultToString(result)
        );
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &renderingInfo;
    pipelineInfo.stageCount = static_cast<u32>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;

    VkPipeline pipeline = VK_NULL_HANDLE;
    result = vkCreateGraphicsPipelines(
        device->getHandle(),
        VK_NULL_HANDLE,
        1,
        &pipelineInfo,
        nullptr,
        &pipeline
    );
    if (result != VK_SUCCESS) {
        vkDestroyPipelineLayout(device->getHandle(), pipelineLayout, nullptr);
        return std::unexpected(
            "Failed to create graphics pipeline: " + utils::vkResultToString(result)
        );
    }

    for (auto *shaderModule : shaderModules) {
        vkDestroyShaderModule(device->getHandle(), shaderModule, nullptr);
    }

    return VulkanPipeline::Factory::create(m_ctx, pipeline, pipelineLayout);
}

std::vector<char> VulkanPipeline::Builder::Impl::readFile(const fs::path &path)
{
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        return {};
    }

    auto fileSize = file.tellg();
    std::vector<char> buffer(static_cast<size_t>(fileSize));

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    return buffer;
}

std::expected<VkShaderModule, std::string>
VulkanPipeline::Builder::Impl::createShaderModule(const std::vector<char> &code)
{
    auto *device = m_ctx->getDevice();
    if (device == nullptr) {
        return std::unexpected("Vulkan device is not initialized");
    }

    if (code.size() % sizeof(u32) != 0) {
        return std::unexpected("Shader code size is not a multiple of sizeof(u32)");
    }

    auto pCode = utils::vectorCharToU32(code);

    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = pCode.size() * sizeof(u32);
    createInfo.pCode = pCode.data();

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    auto result = vkCreateShaderModule(device->getHandle(), &createInfo, nullptr, &shaderModule);
    if (result != VK_SUCCESS) {
        return std::unexpected(
            "Failed to create shader module: " + utils::vkResultToString(result)
        );
    }

    return shaderModule;
}

VulkanPipeline::VulkanPipeline(VulkanDevice *device, VkPipeline pipeline, VkPipelineLayout layout)
    : m_impl(std::make_unique<Impl>(device, pipeline, layout))
{}

VulkanPipeline::Builder::Builder(VulkanDevice *device)
    : m_impl(std::make_unique<Impl>(device))
{}

std::expected<std::unique_ptr<Pipeline::Builder>, std::string>
VulkanPipeline::Builder::create(VulkanDevice *device)
{
    if (device == nullptr) {
        return std::unexpected("Null device pointer provided to VulkanPipeline::Builder::create");
    }

    if (device->getDevice() == nullptr) {
        return std::unexpected("VulkanDevice is not properly initialized");
    }

    try {
        auto builder = std::make_unique<VulkanPipeline::Builder>(device);

        return std::unique_ptr<Pipeline::Builder>(std::move(builder));
    } catch (const std::exception &e) {
        return std::unexpected(std::string("Failed to create pipeline builder: ") + e.what());
    } catch (...) {
        return std::unexpected("Unknown error occurred when creating pipeline builder");
    }
}

VulkanPipeline::Builder::~Builder() = default;

VulkanPipeline::Builder &VulkanPipeline::Builder::setVertexShader(const fs::path &path)
{
    m_impl->setVertexShader(path);
    return *this;
}

VulkanPipeline::Builder &VulkanPipeline::Builder::setFragmentShader(const fs::path &path)
{
    m_impl->setFragmentShader(path);
    return *this;
}

VulkanPipeline::Builder &VulkanPipeline::Builder::setGeometryShader(const fs::path &path)
{
    m_impl->setGeometryShader(path);
    return *this;
}

VulkanPipeline::Builder &VulkanPipeline::Builder::setTessellationControlShader(const fs::path &path)
{
    m_impl->setTessellationControlShader(path);
    return *this;
}

VulkanPipeline::Builder &
VulkanPipeline::Builder::setTessellationEvaluationShader(const fs::path &path)
{
    m_impl->setTessellationEvaluationShader(path);
    return *this;
}

VulkanPipeline::Builder &VulkanPipeline::Builder::setComputeShader(const fs::path &path)
{
    m_impl->setComputeShader(path);
    return *this;
}

VulkanPipeline::Builder &
VulkanPipeline::Builder::setPrimitiveTopology(const PrimitiveTopology &topology)
{
    m_impl->setPrimitiveTopology(topology);
    return *this;
}

VulkanPipeline::Builder &VulkanPipeline::Builder::setPolygonMode(const PolygonMode &mode)
{
    m_impl->setPolygonMode(mode);
    return *this;
}

VulkanPipeline::Builder &VulkanPipeline::Builder::setCullMode(const CullMode &mode)
{
    m_impl->setCullMode(mode);
    return *this;
}

VulkanPipeline::Builder &VulkanPipeline::Builder::setFrontFace(const FrontFace &face)
{
    m_impl->setFrontFace(face);
    return *this;
}

VulkanPipeline::Builder &VulkanPipeline::Builder::setLineWidth(f32 width)
{
    m_impl->setLineWidth(width);
    return *this;
}

VulkanPipeline::Builder &VulkanPipeline::Builder::setDepthTest(bool enable)
{
    m_impl->setDepthTest(enable);
    return *this;
}

VulkanPipeline::Builder &VulkanPipeline::Builder::setDepthWrite(bool enable)
{
    m_impl->setDepthWrite(enable);
    return *this;
}

VulkanPipeline::Builder &VulkanPipeline::Builder::setDepthCompareOp(const CompareOp &op)
{
    m_impl->setDepthCompareOp(op);
    return *this;
}

VulkanPipeline::Builder &VulkanPipeline::Builder::setStencilTest(bool enable)
{
    m_impl->setStencilTest(enable);
    return *this;
}

VulkanPipeline::Builder &VulkanPipeline::Builder::setStencilOp(
    const StencilOp &failOp,
    const StencilOp &passOp,
    const StencilOp &depthFailOp
)
{
    m_impl->setStencilOp(failOp, passOp, depthFailOp);
    return *this;
}

VulkanPipeline::Builder &VulkanPipeline::Builder::setBlend(bool enable)
{
    m_impl->setBlend(enable);
    return *this;
}

VulkanPipeline::Builder &VulkanPipeline::Builder::setBlendFactor(
    const BlendFactor &srcColor,
    const BlendFactor &dstColor,
    const BlendFactor &srcAlpha,
    const BlendFactor &dstAlpha
)
{
    m_impl->setBlendFactor(srcColor, dstColor, srcAlpha, dstAlpha);
    return *this;
}

VulkanPipeline::Builder &
VulkanPipeline::Builder::setBlendOp(const BlendOp &colorOp, const BlendOp &alphaOp)
{
    m_impl->setBlendOp(colorOp, alphaOp);
    return *this;
}

VulkanPipeline::Builder &VulkanPipeline::Builder::setColorWriteMask(const ColorComponentFlags &mask)
{
    m_impl->setColorWriteMask(mask);
    return *this;
}

VulkanPipeline::Builder &VulkanPipeline::Builder::addPushConstant(u32 size)
{
    m_impl->addPushConstant(size);
    return *this;
}

VulkanPipeline::Builder &VulkanPipeline::Builder::setName(const std::string &name)
{
    m_impl->setName(name);
    return *this;
}

std::expected<std::unique_ptr<Pipeline>, std::string> VulkanPipeline::Builder::build()
{
    return m_impl->build();
}

VulkanPipeline::~VulkanPipeline() = default;

VkPipeline VulkanPipeline::getHandle() const
{
    return m_impl->getHandle();
}

VkPipelineLayout VulkanPipeline::getLayout() const
{
    return m_impl->getLayout();
}

void VulkanPipeline::bind()
{
    if (m_impl) {
        m_impl->bind();
    }
}

} // namespace vostok::graphics::vulkan