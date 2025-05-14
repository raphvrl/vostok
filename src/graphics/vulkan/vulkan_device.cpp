#include "graphics/vulkan/vulkan_device.hpp"

#include "graphics/gpu_device.hpp"
#include "graphics/vulkan/core/instance.hpp"
#include "graphics/vulkan/core/physical_device.hpp"
#include "graphics/vulkan/platform/glfw_platform.hpp"

#include <memory>
#include <vulkan/vulkan.h>

namespace vostok::graphics::vulkan
{

class VulkanDevice::Impl
{
public:
    Impl(const GPUDevice::CreateInfo &createInfo);
    ~Impl();

    Impl(const Impl &) = delete;
    Impl &operator=(const Impl &) = delete;
    Impl(Impl &&) = delete;
    Impl &operator=(Impl &&) = delete;

    void waitIdle();

    std::expected<u32, std::string> beginFrame();
    std::expected<void, std::string> endFrame();

    std::expected<void, std::string> resize(const FramebufferSize &size);

    [[nodiscard]] bool isInitialized() const { return m_instance != nullptr; }

    [[nodiscard]] const std::string &getLastError() const { return m_lastError; }

private:
    bool initInstance(const GPUDevice::CreateInfo &createInfo);
    bool initSurface(void *windowHandle);
    bool initPhysicalDevice();

    std::unique_ptr<Instance> m_instance;
    std::unique_ptr<PhysicalDevice> m_physicalDevice;

    VkSurfaceKHR m_surface = VK_NULL_HANDLE;

    std::string m_lastError;
};

std::expected<std::unique_ptr<GPUDevice>, std::string>
VulkanDevice::create(const CreateInfo &createInfo)
{
    auto device = Factory::create();

    device->m_impl = std::make_unique<Impl>(createInfo);

    if (!device->m_impl->isInitialized()) {
        return std::unexpected(device->m_impl->getLastError());
    }

    return device;
}

VulkanDevice::Impl::Impl(const GPUDevice::CreateInfo &createInfo)
{
    if (!initInstance(createInfo)) {
        return;
    }

    if (!initSurface(createInfo.windowHandle)) {
        return;
    }

    if (!initPhysicalDevice()) {
        return;
    }
}

VulkanDevice::Impl::~Impl()
{
    if (m_surface != VK_NULL_HANDLE) {
        m_instance->destroySurface(m_surface);
    }
}

bool VulkanDevice::Impl::initInstance(const GPUDevice::CreateInfo &createInfo)
{
    auto platform = std::make_unique<platform::GlfwPlatform>();

    Instance::CreateInfo instanceCreateInfo;
    instanceCreateInfo.appName = createInfo.appName;
    instanceCreateInfo.appVersion = createInfo.appVersion.toPackedInt();
    instanceCreateInfo.engineName = createInfo.engineName;
    instanceCreateInfo.engineVersion = createInfo.engineVersion.toPackedInt();
    instanceCreateInfo.enableValidationLayers = createInfo.enableValidationLayers;
    instanceCreateInfo.platform = std::move(platform);

    auto result = Instance::create(instanceCreateInfo);
    if (!result) {
        m_lastError = result.error();
        return false;
    }

    m_instance = std::move(result.value());

    return true;
}

bool VulkanDevice::Impl::initSurface(void *windowHandle)
{
    auto result = m_instance->createSurface(windowHandle);
    if (!result) {
        m_lastError = result.error();
        return false;
    }

    m_surface = result.value();
    return true;
}

bool VulkanDevice::Impl::initPhysicalDevice()
{
    auto result = PhysicalDevice::create(m_instance->getHandle(), m_surface);
    if (!result) {
        m_lastError = result.error();
        return false;
    }

    m_physicalDevice = std::move(result.value());

    return true;
}

void VulkanDevice::Impl::waitIdle()
{
    // TODO: Implement waitIdle
}

std::expected<u32, std::string> VulkanDevice::Impl::beginFrame()
{
    if (!m_instance) {
        return std::unexpected("Instance is not initialized");
    }

    // TODO: Implement beginFrame
    return std::unexpected("Not implemented");
}

std::expected<void, std::string> VulkanDevice::Impl::endFrame()
{
    if (!m_instance) {
        return std::unexpected("Instance is not initialized");
    }

    // TODO: Implement endFrame
    return std::unexpected("Not implemented");
}

std::expected<void, std::string> VulkanDevice::Impl::resize(const FramebufferSize &size)
{
    if (size.width == 0 || size.height == 0) {
        return std::unexpected("Invalid framebuffer size");
    }

    if (!m_instance) {
        return std::unexpected("Instance is not initialized");
    }

    // TODO: Implement resize
    return std::unexpected("Not implemented");
}

VulkanDevice::VulkanDevice() : m_impl(nullptr) {}

VulkanDevice::~VulkanDevice()
{
    if (m_impl) {
        m_impl->waitIdle();
    }
}

void VulkanDevice::waitIdle()
{
    if (m_impl) {
        m_impl->waitIdle();
    }
}

std::expected<u32, std::string> VulkanDevice::beginFrame()
{
    if (m_impl) {
        return m_impl->beginFrame();
    }
    return std::unexpected("VulkanDevice is not initialized");
}

std::expected<void, std::string> VulkanDevice::endFrame()
{
    if (m_impl) {
        return m_impl->endFrame();
    }
    return std::unexpected("VulkanDevice is not initialized");
}

std::expected<void, std::string> VulkanDevice::resize(const FramebufferSize &size)
{
    if (m_impl) {
        return m_impl->resize(size);
    }
    return std::unexpected("VulkanDevice is not initialized");
}

} // namespace vostok::graphics::vulkan