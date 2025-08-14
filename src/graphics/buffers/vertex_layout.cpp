#include "vostok/graphics/buffers/vertex_layout.hpp"

namespace vostok::graphics
{

void VertexLayout::addAttribute(
    u32 location,
    u32 offset,
    u32 size,
    VertexFormat format
)
{
    attributes.emplace_back(location, offset, size, format);
    calculateStride();
}

void VertexLayout::calculateStride()
{
    if (attributes.empty()) {
        stride = 0;
        return;
    }

    stride = 0;
    for (const auto &attr : attributes) {
        stride += attr.size;
    }
}

auto VertexLayout::isValid() const -> bool
{
    if (attributes.empty()) {
        return false;
    }

    for (size_t i = 0; i < attributes.size(); ++i) {
        for (size_t j = i + 1; j < attributes.size(); ++j) {
            const auto &attr1 = attributes[i];
            const auto &attr2 = attributes[j];

            if (attr1.offset < attr2.offset + attr2.size &&
                attr2.offset < attr1.offset + attr1.size) {
                return false;
            }
        }
    }

    return true;
}

auto getFormatSize(VertexFormat format) -> u32
{
    switch (format) {
        case VertexFormat::FLOAT1:
        case VertexFormat::INT1:
        case VertexFormat::UINT1:
            return 4;
        case VertexFormat::FLOAT2:
        case VertexFormat::INT2:
        case VertexFormat::UINT2:
            return 8;
        case VertexFormat::FLOAT3:
        case VertexFormat::INT3:
        case VertexFormat::UINT3:
            return 12;
        case VertexFormat::FLOAT4:
        case VertexFormat::INT4:
        case VertexFormat::UINT4:
            return 16;
        default:
            return 4;
    }
}

auto createVertexLayout(std::initializer_list<VertexFormat> formats)
    -> VertexLayout
{
    std::vector<VertexAttribute> attrs;
    size_t offset = 0;
    u32 location = 0;

    for (VertexFormat format : formats) {
        u32 size = getFormatSize(format);
        attrs.emplace_back(location, offset, size, format);
        location++;
        offset += size;
    }

    VertexLayout layout;
    layout.attributes = std::move(attrs);
    layout.calculateStride();

    return layout;
}

} // namespace vostok::graphics