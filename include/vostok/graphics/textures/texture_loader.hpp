#pragma once

#include "vostok/graphics/gpu.hpp"
#include "vostok/graphics/textures/texture.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace vostok::graphics
{

class TextureLoader
{
public:
    static auto loadFromFile(
        GPUHandle *gpu,
        const fs::path &filePath,
        const TextureCreateInfo &options = {}
    ) -> std::expected<Texture, std::string>;

    static auto loadFromGLTF(
        GPUHandle *gpu,
        const fs::path &gltfPath,
        const TextureCreateInfo &options = {}
    ) -> std::expected<std::vector<Texture>, std::string>;

private:
    static auto loadImageData(
        const std::filesystem::path &filePath
    ) -> std::expected<std::tuple<std::vector<u8>, u32, u32, u32>, std::string>;

    static auto calculateMipLevels(u32 width, u32 height) -> u32;
};

} // namespace vostok::graphics