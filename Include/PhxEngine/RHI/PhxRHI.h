#pragma once

#include <stdint.h>
#include <optional>
#include <string>

#include <PhxEngine/RHI/RefCountPtr.h>

namespace PhxEngine::RHI
{
    struct Color
    {
        float R;
        float G;
        float B;
        float A;

        Color()
            : R(0.f), G(0.f), B(0.f), A(0.f)
        { }

        Color(float c)
            : R(c), G(c), B(c), A(c)
        { }

        Color(float r, float g, float b, float a)
            : R(r), G(g), B(b), A(a) { }

        bool operator ==(const Color& other) const { return R == other.R && G == other.G && B == other.B && A == other.A; }
        bool operator !=(const Color& other) const { return !(*this == other); }
    };

    struct Viewport
    {
        float MinX, MaxX;
        float MinY, MaxY;
        float MinZ, MaxZ;

        Viewport() : MinX(0.f), MaxX(0.f), MinY(0.f), MaxY(0.f), MinZ(0.f), MaxZ(1.f) { }

        Viewport(float width, float height) : MinX(0.f), MaxX(width), MinY(0.f), MaxY(height), MinZ(0.f), MaxZ(1.f) { }

        Viewport(float _minX, float _maxX, float _minY, float _maxY, float _minZ, float _maxZ)
            : MinX(_minX), MaxX(_maxX), MinY(_minY), MaxY(_maxY), MinZ(_minZ), MaxZ(_maxZ)
        { }

        bool operator ==(const Viewport& b) const
        {
            return MinX == b.MinX
                && MinY == b.MinY
                && MinZ == b.MinZ
                && MaxX == b.MaxX
                && MaxY == b.MaxY
                && MaxZ == b.MaxZ;
        }
        bool operator !=(const Viewport& b) const { return !(*this == b); }

        float GetWidth() const { return MaxX - MinX; }
        float GetHeight() const { return MaxY - MinY; }
    };

    struct Rect
    {
        int MinX, MaxX;
        int MinY, MaxY;

        Rect() : MinX(0), MaxX(0), MinY(0), MaxY(0) { }
        Rect(int width, int height) : MinX(0), MaxX(width), MinY(0), MaxY(height) { }
        Rect(int _minX, int _maxX, int _minY, int _maxY) : MinX(_minX), MaxX(_maxX), MinY(_minY), MaxY(_maxY) { }
        explicit Rect(const Viewport& viewport)
            : MinX(int(floorf(viewport.MinX)))
            , MaxX(int(ceilf(viewport.MaxX)))
            , MinY(int(floorf(viewport.MinY)))
            , MaxY(int(ceilf(viewport.MaxY)))
        {
        }

        bool operator ==(const Rect& b) const {
            return MinX == b.MinX && MinY == b.MinY && MaxX == b.MaxX && MaxY == b.MaxY;
        }
        bool operator !=(const Rect& b) const { return !(*this == b); }

        int GetWidth() const { return MaxX - MinX; }
        int GetHeight() const { return MaxY - MinY; }
    };

    enum class EFormat : uint8_t
    {
        UNKNOWN = 0,
        R8_UINT,
        R8_SINT,
        R8_UNORM,
        R8_SNORM,
        RG8_UINT,
        RG8_SINT,
        RG8_UNORM,
        RG8_SNORM,
        R16_UINT,
        R16_SINT,
        R16_UNORM,
        R16_SNORM,
        R16_FLOAT,
        BGRA4_UNORM,
        B5G6R5_UNORM,
        B5G5R5A1_UNORM,
        RGBA8_UINT,
        RGBA8_SINT,
        RGBA8_UNORM,
        RGBA8_SNORM,
        BGRA8_UNORM,
        SRGBA8_UNORM,
        SBGRA8_UNORM,
        R10G10B10A2_UNORM,
        R11G11B10_FLOAT,
        RG16_UINT,
        RG16_SINT,
        RG16_UNORM,
        RG16_SNORM,
        RG16_FLOAT,
        R32_UINT,
        R32_SINT,
        R32_FLOAT,
        RGBA16_UINT,
        RGBA16_SINT,
        RGBA16_FLOAT,
        RGBA16_UNORM,
        RGBA16_SNORM,
        RG32_UINT,
        RG32_SINT,
        RG32_FLOAT,
        RGB32_UINT,
        RGB32_SINT,
        RGB32_FLOAT,
        RGBA32_UINT,
        RGBA32_SINT,
        RGBA32_FLOAT,

        D16,
        D24S8,
        X24G8_UINT,
        D32,
        D32S8,
        X32G8_UINT,

        BC1_UNORM,
        BC1_UNORM_SRGB,
        BC2_UNORM,
        BC2_UNORM_SRGB,
        BC3_UNORM,
        BC3_UNORM_SRGB,
        BC4_UNORM,
        BC4_SNORM,
        BC5_UNORM,
        BC5_SNORM,
        BC6H_UFLOAT,
        BC6H_SFLOAT,
        BC7_UNORM,
        BC7_UNORM_SRGB,

        COUNT,
    };

    enum class CommandQueueType : uint8_t
    {
        Graphics = 0,
        Compute,
        Copy,

        Count
    };

    enum class TextureDimension : uint8_t
    {
        Unknown,
        Texture1D,
        Texture1DArray,
        Texture2D,
        Texture2DArray,
        TextureCube,
        TextureCubeArray,
        Texture2DMS,
        Texture2DMSArray,
        Texture3D
    };

    enum ResourceStates : uint32_t
    {
        Unknown = 0,
        Common = 0x00000001,
        ConstantBuffer = 0x00000002,
        VertexBuffer = 0x00000004,
        IndexBuffer = 0x00000008,
        IndirectArgument = 0x00000010,
        ShaderResource = 0x00000020,
        UnorderedAccess = 0x00000040,
        RenderTarget = 0x00000080,
        DepthWrite = 0x00000100,
        DepthRead = 0x00000200,
        StreamOut = 0x00000400,
        CopyDest = 0x00000800,
        CopySource = 0x00001000,
        ResolveDest = 0x00002000,
        ResolveSource = 0x00004000,
        Present = 0x00008000,
        AccelStructRead = 0x00010000,
        AccelStructWrite = 0x00020000,
        AccelStructBuildInput = 0x00040000,
        AccelStructBuildBlas = 0x00080000,
        ShadingRateSurface = 0x00100000,
    };

    struct SwapChainDesc
    {
        uint32_t Width;
        uint32_t Height;
        uint32_t BufferCount = 3;
        EFormat Format = EFormat::UNKNOWN;
        bool VSync = false;
        void* WindowHandle;
    };

    class IResource
    {
    protected:
        IResource() = default;
        virtual ~IResource() = default;

    public:
        virtual unsigned long AddRef() = 0;
        virtual unsigned long Release() = 0;

        // Non-copyable and non-movable
        IResource(const IResource&) = delete;
        IResource(const IResource&&) = delete;
        IResource& operator=(const IResource&) = delete;
        IResource& operator=(const IResource&&) = delete;
    };

    struct TextureDesc
    {
        TextureDimension Dimension = TextureDimension::Unknown;
        EFormat Format = EFormat::UNKNOWN;

        uint32_t Width;
        uint32_t Height;

        union
        {
            uint16_t ArraySize = 1;
            uint16_t Depth;
        };

        uint16_t MipLevels = 0;

        std::optional<Color> OptmizedClearValue;
        std::string DebugName;
    };

    class ITexture : public IResource
    {
    public:
        virtual ~ITexture() = default;
        virtual const TextureDesc& GetDesc() const = 0;
    };

    using TextureHandle = RefCountPtr<ITexture>;

    class IBuffer : public IResource
    {
    public:
        virtual ~IBuffer() = default;
    };

    using BufferHandle = RefCountPtr<IBuffer>;

    struct CommandListDesc
    {
        CommandQueueType QueueType = CommandQueueType::Graphics;
    };

    class ScopedMarker;

    class ICommandList : public IResource
    {
    public:
        virtual ~ICommandList() = default;

        virtual void Open() = 0;
        virtual void Close() = 0;

        virtual ScopedMarker BeginScropedMarker(std::string name) = 0;
        virtual void BeginMarker(std::string name) = 0;
        virtual void EndMarker() = 0;

        virtual void TransitionBarrier(ITexture* texture, ResourceStates beforeState, ResourceStates afterState) = 0;
        virtual void TransitionBarrier(IBuffer* buffer, ResourceStates beforeState, ResourceStates afterState) = 0;
        virtual void ClearTextureFloat(ITexture* texture, Color const& clearColour) = 0 ;
        virtual void ClearDepthStencilTexture(ITexture* depthStencil, bool clearDepth, float depth, bool clearStencil, uint8_t stencil) = 0;
    };

    using CommandListHandle = RefCountPtr<ICommandList>;

    class ScopedMarker
    {
    public:
        ScopedMarker(ICommandList* context)
            : m_commandList(context) {}

        ~ScopedMarker() { this->m_commandList->EndMarker(); }

    private:
        ICommandList* m_commandList;
    };

    class IGraphicsDevice
    {
    public:
        virtual ~IGraphicsDevice() = default;

        // -- Create Functions ---
    public:
        virtual void CreateSwapChain(SwapChainDesc const& swapChainDesc) = 0;
        virtual CommandListHandle CreateCommandList(CommandListDesc const& desc = {}) = 0;

        virtual TextureHandle CreateDepthStencil(TextureDesc const& desc) = 0;

        // -- Create Functions End ---
        virtual ITexture* GetBackBuffer() = 0;

        virtual void Present() = 0;
        virtual void WaitForIdle() = 0;

        virtual uint64_t ExecuteCommandLists(
            ICommandList* const* pCommandLists,
            size_t numCommandLists,
            CommandQueueType executionQueue = CommandQueueType::Graphics) = 0;

        virtual uint64_t ExecuteCommandLists(
            ICommandList* const* pCommandLists,
            size_t numCommandLists,
            bool waitForCompletion,
            CommandQueueType executionQueue = CommandQueueType::Graphics) = 0;

        // Front-end for executeCommandLists(..., 1) for compatibility and convenience
        uint64_t ExecuteCommandLists(ICommandList* commandList, CommandQueueType executionQueue = CommandQueueType::Graphics)
        {
            return this->ExecuteCommandLists(&commandList, 1, executionQueue);
        }

        uint64_t ExecuteCommandLists(ICommandList* commandList, bool waitForCompletion, CommandQueueType executionQueue = CommandQueueType::Graphics)
        {
            return this->ExecuteCommandLists(&commandList, 1, waitForCompletion, executionQueue);
        }

    };
}