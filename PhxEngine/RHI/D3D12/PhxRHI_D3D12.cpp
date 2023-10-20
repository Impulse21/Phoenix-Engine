#include <PhxEngine.h>

using namespace PhxEngine::RHI;

// -- D3D12 Implementation

bool PhxEngine::RHI::Initialize(RHIParams const& params)
{
    return false;
}

bool PhxEngine::RHI::Finalize()
{
    return false;
}

void PhxEngine::RHI::RunGarbageCollection()
{
}

GfxContext& PhxEngine::RHI::BeginGfxCtx()
{
    return {};
}

ComputeContext& PhxEngine::RHI::BeginComputeCtx()
{
    return {};
}

CopyContext& PhxEngine::RHI::BeginCopyCtx()
{
    return {};
}

SubmitRecipt PhxEngine::RHI::Submit(Core::Span<CommandContext> context)
{
    return SubmitRecipt();
}

// Resource Creation Functions
template<typename T>
bool PhxEngine::RHI::CreateCommandSignature(CommandSignatureDesc const& desc, CommandSignature& out)
{
	static_assert(sizeof(T) % sizeof(uint32_t) == 0);
	return this->CreateCommandSignature(desc, sizeof(T), out);
}
bool PhxEngine::RHI::CreateCommandSignature(CommandSignatureDesc const& desc, size_t byteStride, CommandSignature& out)
{
    return false;
}

bool PhxEngine::RHI::CreateShader(ShaderDesc const& desc, Core::Span<uint8_t> shaderByteCode, Shader& out)
{
    return false;
}

bool PhxEngine::RHI::CreateInputLayout(InputLayoutDesc const& desc, uint32_t attributeCount, InputLayout& out)
{
    return false;
}

bool PhxEngine::RHI::CreateGfxPipeline(GfxPipelineDesc const& desc, Texture& out)
{
    return false;
}

bool PhxEngine::RHI::CreateComputePipeline(ComputePipelineDesc const& desc, Texture& out)
{
    return false;
}

bool PhxEngine::RHI::CreateMeshPipeline(MeshPipelineDesc const& desc, Texture& out)
{
    return false;
}

bool PhxEngine::RHI::CreateGpuBuffer(GpuBufferDesc const& desc, Texture& out, void* initalData = nullptr)
{
    return false;
}

bool PhxEngine::RHI::CreateTexture(TextureDesc const& desc, Texture& out, void* initalData = nullptr)
{
    return false;
}