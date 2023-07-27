#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include <PhxEngine/Renderer/GeometryPasses.h>
#include <PhxEngine/Scene/Scene.h>
#include <PhxEngine/Scene/Components.h>

using namespace PhxEngine::Core;
using namespace PhxEngine::Renderer;
using namespace PhxEngine::RHI;

void PhxEngine::Renderer::RenderViews(RHI::ICommandList* commandList, IGeometryPass* geometryPass, Scene::Scene& scene, DrawQueue& drawQueue, bool markMeshes)
{
    auto _ = commandList->BeginScopedMarker("Render View");

    GPUAllocation instanceBufferAlloc =
        commandList->AllocateGpu( 
            sizeof(Shader::ShaderMeshInstancePointer) * drawQueue.Size(),
            sizeof(Shader::ShaderMeshInstancePointer));

    // See how this data is copied over.
    const DescriptorIndex instanceBufferDescriptorIndex = IGraphicsDevice::GPtr->GetDescriptorIndex(instanceBufferAlloc.GpuBuffer, SubresouceType::SRV);

    Shader::ShaderMeshInstancePointer* pInstancePointerData = (Shader::ShaderMeshInstancePointer*)instanceBufferAlloc.CpuData;

    struct InstanceBatch
    {
        entt::entity MeshEntity = entt::null;
        uint32_t NumInstance;
        uint32_t DataOffset;
    } instanceBatch = {};

    auto batchFlush = [&]()
    {
        if (instanceBatch.NumInstance == 0)
        {
            return;
        }

        auto [meshComponent, nameComponent] = scene.GetRegistry().get<Scene::MeshComponent, Scene::NameComponent>(instanceBatch.MeshEntity);

		if (markMeshes)
		{
			std::string modelName = nameComponent.Name;
			auto scrope = commandList->BeginScopedMarker(modelName);
		}

		auto& materiaComp = scene.GetRegistry().get<Scene::MaterialComponent>(meshComponent.Material);

		Shader::GeometryPassPushConstants pushConstant = {};
		pushConstant.GeometryIndex = meshComponent.GlobalIndexOffsetGeometryBuffer;
		pushConstant.MaterialIndex = materiaComp.GlobalBufferIndex;
		pushConstant.InstancePtrBufferDescriptorIndex = instanceBufferDescriptorIndex;
		pushConstant.InstancePtrDataOffset = instanceBatch.DataOffset;

        // TODO: FIX OFFSET IN BUFFER
        assert(false);
		geometryPass->BindPushConstant(commandList, pushConstant);
		commandList->DrawIndexed(
			meshComponent.TotalIndices,
			instanceBatch.NumInstance,
			meshComponent.GlobalOffsetIndexBuffer);
	};

    commandList->BindIndexBuffer(scene.GetGlobalIndexBuffer());
    uint32_t instanceCount = 0;
    for (const DrawPacket& drawBatch : drawQueue.DrawItem)
    {
        entt::entity meshEntityHandle = (entt::entity)drawBatch.GetMeshEntityHandle();

        // Flush if we are dealing with a new Mesh
        if (instanceBatch.MeshEntity != meshEntityHandle)
        {
            // TODO: Flush draw
            batchFlush();

            instanceBatch.MeshEntity = meshEntityHandle;
            instanceBatch.NumInstance = 0;
            instanceBatch.DataOffset = (uint32_t)(instanceBufferAlloc.Offset + instanceCount * sizeof(Shader::ShaderMeshInstancePointer));
        }

        auto& instanceComp = scene.GetRegistry().get<Scene::MeshInstanceComponent>((entt::entity)drawBatch.GetInstanceEntityHandle());

        for (uint32_t renderCamIndex = 0; renderCamIndex < 1; renderCamIndex++)
        {
            Shader::ShaderMeshInstancePointer shaderMeshPtr = {};
            shaderMeshPtr.Create(instanceComp.GlobalBufferIndex, renderCamIndex);

            // Write into actual GPU-buffer:
            std::memcpy(pInstancePointerData + instanceCount, &shaderMeshPtr, sizeof(shaderMeshPtr)); // memcpy whole structure into mapped pointer to avoid read from uncached memory

            instanceBatch.NumInstance++;
            instanceCount++;
        }
    }

    // Flush what ever is left over.
    batchFlush();

    commandList->EndRenderPass();
}
