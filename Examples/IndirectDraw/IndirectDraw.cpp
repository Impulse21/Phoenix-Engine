#include <PhxEngine/PhxEngine.h>
 
#include <PhxEngine/Graphics/ShaderFactory.h>
#include <PhxEngine/Core/Helpers.h>
#include <PhxEngine/Core/Platform.h>
#include <PhxEngine/Core/Span.h>
#include <PhxEngine/Core/VirtualFileSystem.h>
#include <PhxEngine/Scene/Scene.h>
#include <PhxEngine/Scene/Entity.h>
#include <PhxEngine/Scene/SceneLoader.h>
#include <PhxEngine/Scene/Components.h>
#include <PhxEngine/Graphics/TextureCache.h>
#include <PhxEngine/Renderer/Renderer.h>
#include <PhxEngine/Graphics/ShaderFactory.h>
#include <PhxEngine/Renderer/CommonPasses.h>
#include <PhxEngine/Renderer/ToneMappingPass.h>
#include <PhxEngine/Core/Math.h>
#include <PhxEngine/Engine/ApplicationBase.h>
#include <PhxEngine/Engine/CameraControllers.h>
#include <PhxEngine/Renderer/GeometryPasses.h>
#include <PhxEngine/Core/Math.h>

using namespace PhxEngine;
using namespace PhxEngine::RHI;
using namespace PhxEngine::Graphics;
using namespace PhxEngine::Renderer;


float GetRandomFloat(float min, float max)
{
    float scale = static_cast<float>(rand()) / RAND_MAX;
    float range = max - min;
    return scale * range + min;
}

struct IndirectCommand
{
    uint32_t SceneConstantBufferIndex = RHI::cInvalidDescriptorIndex;
    uint32_t LookupIndex = 0;
    RHI::IndirectDrawArgInstanced DrawArgs;
};

// Constant buffer definition.
struct SceneConstantBuffer
{
    XMFLOAT4 Velocity;
    XMFLOAT4 Offset;
    XMFLOAT4 Color;
    XMFLOAT4X4 Projection;

    // Constant buffers are 256-byte aligned. Add padding in the struct to allow multiple buffers
    // to be array-indexed.
    float Padding[36];
};

// Root constants for the compute shader.
struct CSRootConstants
{
    float XOffset;
    float ZOffset;
    float CullOffset;
    float CommandCount;
};


#define	D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT	( 4096 )
// We pack the UAV counter into the same buffer as the commands rather than create
// a separate 64K resource/heap for it. The counter must be aligned on 4K boundaries,
// so we pad the command buffer (if necessary) such that the counter will be placed
// at a valid location in the buffer.
constexpr uint32_t AlignForUavCounter(UINT bufferSize)
{
    const UINT alignment = D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT;
    return (bufferSize + (alignment - 1)) & ~(alignment - 1);
}

constexpr uint32_t TriangleCount = 1024;
constexpr uint32_t CommandSizePerFrame = TriangleCount * sizeof(IndirectCommand);
constexpr uint32_t CommandBufferCounterOffset = AlignForUavCounter(CommandSizePerFrame);
constexpr float TriangleHalfWidth = 0.05f;
constexpr float TriangleDepth = 1.0f;
constexpr float CullingCutoff = 0.5f;
constexpr uint32_t ComputeThreadBlockSize = 128;        // Should match the value in compute.hlsl.



class IndirectDraw : public EngineRenderPass
{
private:

public:
    IndirectDraw(IPhxEngineRoot* root)
        : EngineRenderPass(root)
    {
    }

    bool Initialize()
    {
        const std::filesystem::path appShaderPath = Core::Platform::GetExcecutableDir() / "Shaders/IndirectDraw/dxil";

        std::shared_ptr<Core::IFileSystem> nativeFS = Core::CreateNativeFileSystem();
        this->m_shaderFactory = std::make_unique<Graphics::ShaderFactory>(this->GetGfxDevice(), nativeFS, appShaderPath);
        this->m_vertexShader = this->m_shaderFactory->CreateShader(
            "InidrectDrawVS.hlsl",
            {
                .Stage = RHI::ShaderStage::Vertex,
                .DebugName = "InidrectDrawVS",
            });

        this->m_pixelShader = this->m_shaderFactory->CreateShader(
            "IndirectDrawPS.hlsl",
            {
                .Stage = RHI::ShaderStage::Pixel,
                .DebugName = "BasicTrianglePS",
            });

        this->m_computeShader = this->m_shaderFactory->CreateShader(
            "IndirectDrawCS.hlsl",
            {
                .Stage = RHI::ShaderStage::Compute,
                .DebugName = "BasicTrianglePS",
            });

        std::vector<VertexAttributeDesc> attributeDesc =
        {
            { "POSITION",   0, RHIFormat::RG32_FLOAT,  0, VertexAttributeDesc::SAppendAlignedElement, false},
        };

        this->m_inputLayout = this->GetGfxDevice()->CreateInputLayout(attributeDesc.data(), attributeDesc.size());

        this->m_computeConstants.XOffset = TriangleHalfWidth;
        this->m_computeConstants.ZOffset = TriangleDepth;
        this->m_computeConstants.CullOffset = CullingCutoff;
        this->m_computeConstants.CommandCount = TriangleCount;

        float center = this->GetRoot()->GetCanvasSize().x / 2.0f;
        this->m_cullingScissorRect.MinX = static_cast<LONG>(center - (center * CullingCutoff));
        this->m_cullingScissorRect.MaxX = static_cast<LONG>(center + (center * CullingCutoff));
        this->m_cullingScissorRect.MinY = 0.0f;
        this->m_cullingScissorRect.MaxY = static_cast<LONG>(this->GetRoot()->GetCanvasSize().y);


		for (int i = 0; i < this->m_indirectDrawBuffers.size(); i++)
		{
			this->m_indirectDrawBuffers[i] = this->GetGfxDevice()->CreateBuffer({
			   .MiscFlags = BufferMiscFlags::Structured,
			   .Binding = BindingFlags::UnorderedAccess,
			   .InitialState = ResourceStates::IndirectArgument,
			   .StrideInBytes = sizeof(IndirectCommand),
			   .SizeInBytes = sizeof(IndirectCommand) * TriangleCount + sizeof(uint32_t),
			   .AllowUnorderedAccess = true });
		}

        RHI::BufferDesc desc = {};
        desc.DebugName = "Instance Data";
        desc.Binding = RHI::BindingFlags::ShaderResource;
        desc.InitialState = ResourceStates::ShaderResource;
        desc.MiscFlags = RHI::BufferMiscFlags::Bindless | RHI::BufferMiscFlags::Structured;
        desc.CreateBindless = true;
        desc.StrideInBytes = sizeof(SceneConstantBuffer);
        desc.SizeInBytes = sizeof(SceneConstantBuffer) * TriangleCount;

        if (this->m_constantBufferData.IsValid())
        {
            IGraphicsDevice::GPtr->DeleteBuffer(this->m_constantBufferData);
        }
        this->m_constantBufferData = IGraphicsDevice::GPtr->CreateBuffer(desc);

        desc.DebugName = "Instance Upload";
        desc.Usage = RHI::Usage::Upload;
        desc.Binding = RHI::BindingFlags::None;
        desc.MiscFlags = RHI::BufferMiscFlags::None;
        desc.InitialState = ResourceStates::CopySource;
        desc.CreateBindless = false;
        for (int i = 0; i < this->m_constantUploadBuffers.size(); i++)
        {
            if (this->m_constantUploadBuffers[i].IsValid())
            {
                IGraphicsDevice::GPtr->DeleteBuffer(this->m_constantUploadBuffers[i]);
            }
            this->m_constantUploadBuffers[i] = IGraphicsDevice::GPtr->CreateBuffer(desc);
        }

        for (auto& uploadBuffer : this->m_constantUploadBuffers)
        {
            // Initialize the constant buffers for each of the triangles.
            SceneConstantBuffer* start = reinterpret_cast<SceneConstantBuffer*>(this->GetGfxDevice()->GetBufferMappedData(uploadBuffer));
            for (uint32_t n = 0; n < TriangleCount; n++)
            {
                auto data = start + n;
                data->Velocity = XMFLOAT4(GetRandomFloat(0.01f, 0.02f), 0.0f, 0.0f, 0.0f);
                data->Offset = XMFLOAT4(GetRandomFloat(-5.0f, -1.5f), GetRandomFloat(-1.0f, 1.0f), GetRandomFloat(0.0f, 2.0f), 0.0f);
                data->Color = XMFLOAT4(GetRandomFloat(0.5f, 1.0f), GetRandomFloat(0.5f, 1.0f), GetRandomFloat(0.5f, 1.0f), 1.0f);
                XMStoreFloat4x4(&data->Projection, XMMatrixTranspose(XMMatrixPerspectiveFovLH(XM_PIDIV4, 1.7, 0.01f, 20.0f)));
                data += 1;
            }
        }

        // Get structured buffer indiex;
        
        // Fill Commands
        ResourceUpload indirectDrawUpload = Renderer::CreateResourceUpload(sizeof(IndirectCommand) * TriangleCount + sizeof(uint32_t));
        for (UINT n = 0; n < TriangleCount; n++)
        {
            IndirectCommand command = {};
            command.SceneConstantBufferIndex = this->GetGfxDevice()->GetDescriptorIndex(this->m_constantBufferData, SubresouceType::SRV);
            command.LookupIndex = n;
            command.DrawArgs.VertexCount = 3;
            command.DrawArgs.InstanceCount = 1;
            command.DrawArgs.StartVertex= 0;
            command.DrawArgs.StartInstance= 0;

            indirectDrawUpload.SetData(&command, sizeof(IndirectCommand), sizeof(IndirectCommand));
        }

        ICommandList* commandList = this->GetGfxDevice()->BeginCommandRecording();
        assert(this->m_indirectDrawBuffers.size() == 3);
        // Upload data
        RHI::GpuBarrier preCopyBarriers[] =
        {
            RHI::GpuBarrier::CreateBuffer(this->m_indirectDrawBuffers[0], RHI::ResourceStates::ShaderResource, RHI::ResourceStates::CopyDest),
            RHI::GpuBarrier::CreateBuffer(this->m_indirectDrawBuffers[1], RHI::ResourceStates::ShaderResource, RHI::ResourceStates::CopyDest),
            RHI::GpuBarrier::CreateBuffer(this->m_indirectDrawBuffers[2], RHI::ResourceStates::ShaderResource, RHI::ResourceStates::CopyDest),
        };
        commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(preCopyBarriers, _countof(preCopyBarriers)));

        for (int i = 0; i < this->m_indirectDrawBuffers.size(); i++)
        {
            commandList->CopyBuffer(
                this->m_indirectDrawBuffers[i],
                0,
                indirectDrawUpload.UploadBuffer,
                0,
                sizeof(IndirectCommand) * TriangleCount + sizeof(uint32_t));
        }

        RHI::GpuBarrier postCopyBarriers[] =
        {
            RHI::GpuBarrier::CreateBuffer(this->m_indirectDrawBuffers[0], RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource),
            RHI::GpuBarrier::CreateBuffer(this->m_indirectDrawBuffers[1], RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource),
            RHI::GpuBarrier::CreateBuffer(this->m_indirectDrawBuffers[2], RHI::ResourceStates::CopyDest, RHI::ResourceStates::ShaderResource),
        };
        commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(postCopyBarriers, _countof(postCopyBarriers)));

        commandList->Close();
        this->GetGfxDevice()->ExecuteCommandLists({ commandList }, true);
		return true;
    }

    void OnWindowResize(WindowResizeEvent const& e) override
    {
        this->GetGfxDevice()->DeleteGraphicsPipeline(this->m_gfxPipeline);
        this->m_gfxPipeline = {};
    }

    void Update(Core::TimeStep const& deltaTime) override
    {
        // Initialize the constant buffers for each of the triangles.
        SceneConstantBuffer* start = reinterpret_cast<SceneConstantBuffer*>(this->GetGfxDevice()->GetBufferMappedData(this->m_constantUploadBuffers[this->m_currentFrame]));
        for (UINT n = 0; n < TriangleCount; n++)
        {
            SceneConstantBuffer* data = start + n;
            const float offsetBounds = 2.5f;

            // Animate the triangles.
            data->Offset.x += data->Velocity.x;
            if (data->Offset.x > offsetBounds)
            {
                data->Velocity.x = GetRandomFloat(0.01f, 0.02f);
                data->Offset.x = -offsetBounds;
            }
        }

        this->GetRoot()->SetInformativeWindowTitle("Indirect Draw", {});
    }

    void Render() override
    {
        if (!this->m_gfxPipeline.IsValid())
        {
            this->m_gfxPipeline = this->GetGfxDevice()->CreateGraphicsPipeline(
                {
                    .InputLayout = this->m_inputLayout,
                    .VertexShader = this->m_vertexShader,
                    .PixelShader = this->m_pixelShader,
                    .DepthStencilRenderState = {
                        .DepthTestEnable = false
                    },
                    .RtvFormats = { this->GetGfxDevice()->GetTextureDesc(this->GetGfxDevice()->GetBackBuffer()).Format },
                });

            this->m_commandSignature = this->GetGfxDevice()->CreateCommandSignature<IndirectCommand>({
                    .ArgDesc =
                        {
                            {.Type = IndirectArgumentType::Constant, .Constant = {.RootParameterIndex = 0, .DestOffsetIn32BitValues = 0, .Num32BitValuesToSet = 1} },
                            {.Type = IndirectArgumentType::DrawIndex }
                        },
                    .PipelineType = PipelineType::Gfx,
                    .GfxHandle = this->m_gfxPipeline
                });
        }

        if (!this->m_computeShader.IsValid())
        {
            this->m_computePipeline = this->GetGfxDevice()->CreateComputePipeline(
                {
                    .ComputeShader = this->m_computeShader,
                });
        }

        RHI::ICommandList* commandList = this->GetGfxDevice()->BeginCommandRecording();
        {
            auto _ = commandList->BeginScopedMarker("Upload");
            // Upload data
            RHI::GpuBarrier preCopyBarriers[] =
            {
                RHI::GpuBarrier::CreateBuffer(this->m_constantBufferData, RHI::ResourceStates::ShaderResource, RHI::ResourceStates::CopyDest),
            };
            commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(preCopyBarriers, _countof(preCopyBarriers)));

            for (int i = 0; i < this->m_indirectDrawBuffers.size(); i++)
            {
                commandList->CopyBuffer(
                    this->m_constantBufferData,
                    0,
                    this->m_constantUploadBuffers[this->m_currentFrame],
                    0,
                    sizeof(IndirectCommand) * TriangleCount + sizeof(uint32_t));
            }

            RHI::GpuBarrier postCopyBarriers[] =
            {
                RHI::GpuBarrier::CreateBuffer(this->m_constantBufferData, RHI::ResourceStates::CopyDest,  RHI::ResourceStates::ShaderResource),
            };
            commandList->TransitionBarriers(Core::Span<RHI::GpuBarrier>(postCopyBarriers, _countof(postCopyBarriers)));
        }
        {
            auto _ = commandList->BeginScopedMarker("Compute");
            commandList->SetComputeState(this->m_computePipeline);
            commandList->Dispatch(static_cast<UINT>(ceil(TriangleCount / float(ComputeThreadBlockSize))), 1, 1);
        }
        {
            auto _ = commandList->BeginScopedMarker("Render");
            commandList->BeginRenderPassBackBuffer();

            commandList->SetGraphicsPipeline(this->m_gfxPipeline);
            auto canvas = this->GetRoot()->GetCanvasSize();
            RHI::Viewport v(canvas.x, canvas.y);
            commandList->SetViewports(&v, 1);

            RHI::Rect rec(LONG_MAX, LONG_MAX);
            commandList->SetScissors(&rec, 1);

            commandList->BindDynamicVertexBuffer(0, this->m_triangleVertices);

            if (false)
            {

            }
            else
            {
                commandList->ExecuteIndirect(
                    this->m_commandSignature,
                    this->m_indirectDrawBuffers[this->m_currentFrame],
                    0,);
            }
            commandList->EndRenderPass();
        }

        commandList->Close();
        this->GetGfxDevice()->ExecuteCommandLists({ commandList });
    }

private:
    uint32_t m_currentFrame = 0;
    CSRootConstants m_computeConstants;
    RHI::Rect m_cullingScissorRect = {};

    std::vector<DirectX::XMFLOAT3> m_triangleVertices =
    {
        DirectX::XMFLOAT3( 0.0f, TriangleHalfWidth, TriangleDepth ),
        DirectX::XMFLOAT3( TriangleHalfWidth, -TriangleHalfWidth, TriangleDepth ),
        DirectX::XMFLOAT3( - TriangleHalfWidth, -TriangleHalfWidth, TriangleDepth )
    };

    std::array<RHI::BufferHandle, 3> m_indirectDrawBuffers;
    std::array<RHI::BufferHandle, 3> m_constantUploadBuffers;
    RHI::BufferHandle m_constantBufferData;

    RHI::CommandSignatureHandle m_commandSignature;
    RHI::InputLayoutHandle m_inputLayout;
    RHI::ShaderHandle m_vertexShader;
    RHI::ShaderHandle m_pixelShader;
    RHI::ShaderHandle m_computeShader;
    std::unique_ptr<Graphics::ShaderFactory> m_shaderFactory;
    RHI::GraphicsPipelineHandle m_gfxPipeline;
    RHI::ComputePipelineHandle m_computePipeline;
};

#ifdef WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main(int __argc, const char** __argv)
#endif
{
    std::unique_ptr<IPhxEngineRoot> root = CreateEngineRoot();

    EngineParam params = {};
    params.Name = "PhxEngine Example: Indirect Draw";
    params.GraphicsAPI = RHI::GraphicsAPI::DX12;
    root->Initialize(params);

    {
        IndirectDraw example(root.get());
        if (example.Initialize())
        {
            root->AddPassToBack(&example);
            root->Run();
            root->RemovePass(&example);
        }
    }

    root->Finalizing();
    root.reset();
    RHI::ReportLiveObjects();

    return 0;
}
