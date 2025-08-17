#pragma once

#include "vostok/graphics/buffers/ibo.hpp"
#include "vostok/graphics/buffers/vbo.hpp"
#include "vostok/graphics/gpu.hpp"

namespace vostok::graphics
{

template <graphics::VertexType VertexType, typename IndexType>
class MeshHandle
{
public:
    using VertexData = std::vector<VertexType>;
    using IndexData = std::vector<IndexType>;

    static auto
    create(GPUHandle *gpu, const VertexData &vertices, const IndexData &indices)
        -> std::expected<std::unique_ptr<MeshHandle>, std::string>
    {
        auto vboResult = gpu->createVBO<VertexType>(vertices);
        if (!vboResult) {
            return std::unexpected(vboResult.error());
        }

        auto iboResult = gpu->createIBO<IndexType>(indices);
        if (!iboResult) {
            return std::unexpected(iboResult.error());
        }

        return std::make_unique<MeshHandle>(
            gpu,
            std::move(vboResult.value()),
            std::move(iboResult.value())
        );
    }

    explicit MeshHandle(GPUHandle *gpu, VBO<VertexType> vbo, IBO<IndexType> ibo)
        : m_gpu(gpu),
          m_vertexBuffer(std::move(vbo)),
          m_indexBuffer(std::move(ibo))
    {}

    [[nodiscard]] auto getVertexBuffer() const -> const VBO<VertexType> &
    {
        return *m_vertexBuffer;
    }
    [[nodiscard]] auto getIndexBuffer() const -> const IBO<IndexType> &
    {
        return *m_indexBuffer;
    }
    [[nodiscard]] auto getVertexCount() const -> size_t
    {
        return m_vertexBuffer.get()->getVertexCount();
    }
    [[nodiscard]] auto getIndexCount() const -> size_t
    {
        return m_indexBuffer.get()->getIndexCount();
    }
    [[nodiscard]] auto getLayout() const -> const VertexLayout &
    {
        return m_vertexBuffer.get()->getLayout();
    }

    void draw() const
    {
        m_vertexBuffer->bind();
        m_indexBuffer->bind();
        m_gpu->drawIndexed(getIndexCount());
    }

private:
    GPUHandle *m_gpu;
    VBO<VertexType> m_vertexBuffer;
    IBO<IndexType> m_indexBuffer;
};

template <typename VertexType, typename IndexType>
struct Mesh : public std::unique_ptr<MeshHandle<VertexType, IndexType>>
{
    using Base = std::unique_ptr<MeshHandle<VertexType, IndexType>>;
    using Base::Base;

    Mesh() = default;
    ~Mesh() = default;

    Mesh(Mesh &&) = default;
    auto operator=(Mesh &&) -> Mesh & = default;
    Mesh(const Mesh &) = delete;
    auto operator=(const Mesh &) -> Mesh & = delete;

    explicit Mesh(std::unique_ptr<MeshHandle<VertexType, IndexType>> &&ptr)
        : Base(std::move(ptr))
    {}

    static auto create(
        GPUHandle *gpu,
        const std::vector<VertexType> &vertices,
        const std::vector<IndexType> &indices
    ) -> std::expected<Mesh, std::string>
    {
        auto result =
            MeshHandle<VertexType, IndexType>::create(gpu, vertices, indices);
        if (!result) {
            return std::unexpected(result.error());
        }
        return Mesh(std::move(result.value()));
    }
};

} // namespace vostok::graphics