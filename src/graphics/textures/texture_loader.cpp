#include "graphics/textures/texture_loader.hpp"

#include <cstddef>
#include <stb_image.h>
#include <stb_image_write.h>
#include <tiny_gltf.h>

namespace vostok::graphics
{

auto TextureLoader::loadFromFile(
    GPUHandle *gpu,
    const fs::path &filePath,
    const TextureCreateInfo &options
) -> std::expected<Texture, std::string>
{
    if (!std::filesystem::exists(filePath)) {
        return std::unexpected{
            std::format("Texture file not found: {}", filePath.string())
        };
    }

    auto imageResult = loadImageData(filePath);
    if (!imageResult) {
        return std::unexpected{
            std::format("Failed to load image data: {}", imageResult.error())
        };
    }

    auto [imageData, width, height, channels] = imageResult.value();

    TextureCreateInfo textureOptions = options;
    textureOptions.imageData = std::span<const std::byte>(
        std::bit_cast<const std::byte *>(imageData.data()),
        imageData.size()
    );

    textureOptions.width = width;
    textureOptions.height = height;

    textureOptions.format = ImageFormat::R8G8B8A8_UNORM;

    if (textureOptions.generateMipmaps) {
        textureOptions.mipLevel = calculateMipLevels(width, height);
    } else {
        textureOptions.mipLevel = 1;
    }

    return gpu->createTexture(textureOptions);
}

auto TextureLoader::loadFromGLTF(
    GPUHandle *gpu,
    const fs::path &gltfPath,
    const TextureCreateInfo &options
) -> std::expected<std::vector<Texture>, std::string>
{
    if (!std::filesystem::exists(gltfPath)) {
        return std::unexpected{
            std::format("GLTF file not found: {}", gltfPath.string())
        };
    }

    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, gltfPath.string());
    if (!ret) {
        return std::unexpected{
            std::format("Failed to load GLTF file: {}", err)
        };
    }

    if (!warn.empty()) {
        Logger::warning("{}", warn);
    }

    std::vector<Texture> textures;
    std::filesystem::path baseDir = gltfPath.parent_path();

    for (const auto &image : model.images) {
        std::filesystem::path imagePath;

        if (image.uri.empty()) {
            continue;
        }

        imagePath = baseDir / image.uri;

        auto textureResult = loadFromFile(gpu, imagePath, options);
        if (!textureResult) {
            Logger::warning(
                "Failed to load texture {}: {}",
                imagePath.string(),
                textureResult.error()
            );
            continue;
        }

        textures.push_back(std::move(textureResult.value()));
    }

    Logger::info("Loaded {} textures from GLTF file", textures.size());
    return textures;
}

auto TextureLoader::loadImageData(const std::filesystem::path &filePath)
    -> std::expected<std::tuple<std::vector<u8>, u32, u32, u32>, std::string>
{
    u32 width = 0;
    u32 height = 0;
    u32 channels = 0;

    u8 *data = stbi_load(
        filePath.string().c_str(),
        std::bit_cast<int *>(&width),
        std::bit_cast<int *>(&height),
        std::bit_cast<int *>(&channels),
        0
    );

    if (data == nullptr) {
        return std::unexpected{
            std::format("Failed to load image: {}", stbi_failure_reason())
        };
    }

    std::vector<u8> imageData;
    imageData.resize(
        static_cast<size_t>(width) * static_cast<size_t>(height) * 4
    );

    if (channels == 1) {
        for (size_t i = 0; i < static_cast<size_t>(width) * height; ++i) {
            u8 gray = data[i];
            imageData[(i * 4) + 0] = gray;
            imageData[(i * 4) + 1] = gray;
            imageData[(i * 4) + 2] = gray;
            imageData[(i * 4) + 3] = 255;
        }
    } else if (channels == 3) {
        for (size_t i = 0; i < static_cast<size_t>(width) * height; ++i) {
            imageData[(i * 4) + 0] = data[(i * 3) + 0];
            imageData[(i * 4) + 1] = data[(i * 3) + 1];
            imageData[(i * 4) + 2] = data[(i * 3) + 2];
            imageData[(i * 4) + 3] = 255;
        }
    } else if (channels == 4) {
        std::memcpy(
            imageData.data(),
            data,
            static_cast<size_t>(width) * static_cast<size_t>(height) * 4
        );
    } else {
        stbi_image_free(data);
        return std::unexpected{ "Unsupported number of channels" };
    }

    stbi_image_free(data);

    return std::make_tuple(std::move(imageData), width, height, 4);
}

auto TextureLoader::calculateMipLevels(u32 width, u32 height) -> u32
{
    if (width == 0 || height == 0) {
        return 1;
    }

    u32 maxDimension = std::max(width, height);

    u32 mipLevels = static_cast<u32>(std::floor(std::log2(maxDimension))) + 1;

    mipLevels = std::max(mipLevels, 1U);

    return mipLevels;
}

} // namespace vostok::graphics