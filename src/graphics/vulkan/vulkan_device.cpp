#include "graphics/vulkan/vulkan_device.hpp"

#include "graphics/vulkan/core/instance.hpp"

#include <memory>
#include <vulkan/vulkan.h>

namespace vostok::graphics::vulkan
{

class VulkanDevice::Impl
{
public:
    Impl(const GPUDevice::CreateInfo &createInfo);
    ~Impl() = default;

    Impl(const Impl &) = delete;
    Impl &operator=(const Impl &) = delete;
    Impl(Impl &&) = delete;
    Impl &operator=(Impl &&) = delete;

    void waitIdle();

    std::expected<u32, std::string> beginFrame();
    std::expected<void, std::string> endFrame();

    std::expected<void, std::string> resize(FramebufferSize size);

    [[nodiscard]] bool isInitialized() const
    {
        return m_instance != nullptr;
    }

    [[nodiscard]] const std::string &getLastError() const
    {
        return m_lastError;
    }

private:
    bool initInstance(const GPUDevice::CreateInfo &createInfo);
    bool initSurface(void *windowHandle);

    std::unique_ptr<Instance> m_instance;

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

    // if (!initSurface(createInfo.windowHandle)) {
    //     return;
    // }
}

bool VulkanDevice::Impl::initInstance(const GPUDevice::CreateInfo &createInfo)
{
    Instance::CreateInfo instanceCreateInfo;
    instanceCreateInfo.appName = createInfo.appName;
    instanceCreateInfo.appVersion = createInfo.appVersion.toPackedInt();
    instanceCreateInfo.engineName = createInfo.engineName;
    instanceCreateInfo.engineVersion = createInfo.engineVersion.toPackedInt();
    instanceCreateInfo.enableValidationLayers = createInfo.enableValidationLayers;

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

    return true;
}

} // namespace vostok::graphics::vulkan