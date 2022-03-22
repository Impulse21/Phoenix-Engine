#include "GltfViewer.h"

#include <PhxEngine/Renderer/SceneLoader.h>
#include <PhxEngine/RHI/Dx12/RootSignatureBuilder.h>

#include <Shaders/MeshRender_CommonVS_compiled.h>
#include <Shaders/MeshRender_BrdfPS_compiled.h>

#include <iostream>
#include <fstream>

using namespace PhxEngine;
using namespace PhxEngine::Core;
using namespace PhxEngine::Renderer;
using namespace PhxEngine::RHI;
using namespace DirectX;


namespace PBRBindingSlots
{
    enum : uint8_t
    {
        DrawPushConstant = 0,
        FrameCB,
        CameraCB,
        // InstanceSB,
        BindlessDescriptorTable
    };
}

void GltfViewer::LoadContent()
{
    this->GetCommandList()->Open();

    // TODO: Move this into the base Application or Renderer
    this->m_fs = std::make_shared<NativeFileSystem>();

    std::shared_ptr<IFileSystem> texturesFS= std::make_shared<RelativeFileSystem>(this->m_fs, "Assets\\Textures");

    this->m_textureCache = std::make_shared<TextureCache>(
        this->GetGraphicsDevice(),
        texturesFS);

    this->m_scene.GetGlobalCamera().At = { 0.0f, 0.0f, 0.0f };
    this->m_scene.GetGlobalCamera().Eye = { 0.0f, 4.0f, -6.0f };
    this->m_scene.GetGlobalCamera().Width = WindowWidth;
    this->m_scene.GetGlobalCamera().Height = WindowHeight;
    this->m_scene.GetGlobalCamera().UpdateCamera();

    std::shared_ptr<IFileSystem> modelFileSystem = std::make_shared<RelativeFileSystem>(this->m_fs, "Assets\\Models");
    auto loader = CreateGltfSceneLoader(modelFileSystem, this->m_textureCache, this->GetGraphicsDevice());
    bool success = loader->LoadScene("Sponza\\glTF\\Sponza.gltf", this->GetCommandList(), this->m_scene);

    if (!success)
    {
        LOG_ERROR("Failed to load Scene");
        return;
    }

    this->m_scene.CreateGpuBuffers(this->GetGraphicsDevice(), this->GetCommandList());

    this->CreateRenderTargets();
    this->CreatePipelineStateObjects();


    this->m_scene.SkyboxTexture = this->m_textureCache->LoadTexture("IBL\\Serpentine_Valley\\output_skybox.dds", true, this->GetCommandList());
    this->m_scene.IrradanceMap = this->m_textureCache->LoadTexture("IBL\\Serpentine_Valley\\output_irradiance.dds", true, this->GetCommandList());
    this->m_scene.PrefilteredMap = this->m_textureCache->LoadTexture("IBL\\Serpentine_Valley\\output_radiance.dds", true, this->GetCommandList());
    this->m_scene.BrdfLUT = this->m_textureCache->LoadTexture("IBL\\BrdfLut.dds", true, this->GetCommandList());

    // DEBUG
    this->WriteSeceneData();

    this->GetCommandList()->Close();
    this->GetGraphicsDevice()->ExecuteCommandLists(this->GetCommandList(), true);
}

void GltfViewer::RenderScene()
{
    TextureHandle backBuffer = this->GetGraphicsDevice()->GetBackBuffer();
    this->GetCommandList()->Open();
    {
        this->GetCommandList()->BeginScropedMarker("Clear Render Target");
        this->GetCommandList()->TransitionBarrier(backBuffer, ResourceStates::Present, ResourceStates::RenderTarget);
        this->GetCommandList()->ClearTextureFloat(backBuffer, { 0.0f, 0.0f, 0.0f, 1.0f });
        this->GetCommandList()->ClearDepthStencilTexture(this->m_depthBuffer, true, 1.0f, false, 0);

        // Transition to Write State
        // this->GetCommandList()->TransitionBarrier(this->m_shadowMap, ResourceStates::ShaderResource, ResourceStates::DepthWrite);
        // this->GetCommandList()->ClearDepthStencilTexture(this->m_shadowMap, true, 1.0f, false, 0);
    }

    // Set up frame data
    Shader::Frame frameData = {};
    frameData.BrdfLUTTexIndex = this->m_scene.BrdfLUT->GetDescriptorIndex();
    this->m_scene.PopulateShaderSceneData(frameData.Scene);


    {
        ScopedMarker m = this->GetCommandList()->BeginScropedMarker("Main Render Pass");

        // Set PSO
        this->GetCommandList()->SetGraphicsPSO(this->m_geomtryPassPso);
        this->GetCommandList()->SetRenderTargets({ backBuffer }, this->m_depthBuffer);
        Viewport v(WindowWidth, WindowHeight);
        this->GetCommandList()->SetViewports(&v, 1);
        Rect rec(LONG_MAX, LONG_MAX);
        this->GetCommandList()->SetScissors(&rec, 1);

        Shader::Camera cameraData = {};
        cameraData.CameraPosition = this->m_scene.GetGlobalCamera().Eye;
        cameraData.ViewProjection = this->m_scene.GetGlobalCamera().ViewProjection;

        this->GetCommandList()->BindDynamicConstantBuffer(PBRBindingSlots::FrameCB, frameData);
        this->GetCommandList()->BindDynamicConstantBuffer(PBRBindingSlots::CameraCB, cameraData);

        this->GetCommandList()->BindResourceTable(PBRBindingSlots::BindlessDescriptorTable);

        // Draw the Meshes
        for (int i = 0; i < this->m_scene.MeshInstances.GetCount(); i++)
        {
            auto& meshInstanceComponent = this->m_scene.MeshInstances[i];
            ECS::Entity e = this->m_scene.MeshInstances.GetEntity(i);
            auto& meshComponent = *this->m_scene.Meshes.GetComponent(meshInstanceComponent.MeshId);
            auto& transformComponent = *this->m_scene.Transforms.GetComponent(e);
            this->GetCommandList()->BindIndexBuffer(meshComponent.IndexGpuBuffer);

            for (size_t i = 0; i < meshComponent.Geometry.size(); i++)
            {
                Shader::MeshRenderPushConstant pushConstant;
                pushConstant.MeshIndex = this->m_scene.Meshes.GetIndex(meshInstanceComponent.MeshId);
                pushConstant.GeometryIndex = meshComponent.Geometry[i].GlobalGeometryIndex;
                pushConstant.WorldTransform = transformComponent.WorldMatrix;

                this->GetCommandList()->BindPushConstant(PBRBindingSlots::DrawPushConstant, pushConstant);

                this->GetCommandList()->DrawIndexed(
                    meshComponent.Geometry[i].NumIndices,
                    1,
                    meshComponent.Geometry[i].IndexOffsetInMesh);
            }
        }
    }

    {
        this->GetCommandList()->TransitionBarrier(backBuffer, ResourceStates::RenderTarget, ResourceStates::Present);
    }

    this->GetCommandList()->Close();
    this->GetGraphicsDevice()->ExecuteCommandLists(this->GetCommandList());
}

void GltfViewer::Update(double elapsedTime)
{
}

void GltfViewer::WriteSeceneData()
{
    std::ofstream sceneFile;
    try
    {
        sceneFile.open("SceneData.txt");

        std::stringstream stream;
        New::PrintScene(this->m_scene, stream);

        sceneFile << stream.str();
        LOG_CORE_INFO("Wrinting out Scene Data");
    }
    catch (...)
    {
        LOG_ERROR("Failed to write out scene");
    }

    sceneFile.close();
    return;
}

void GltfViewer::CreatePipelineStateObjects()
{
    // Create Pipeline State
    {
        ShaderDesc shaderDesc = {};
        shaderDesc.DebugName = "PBR Vertex Shader";
        shaderDesc.ShaderType = EShaderType::Vertex;

        ShaderHandle vs = this->GetGraphicsDevice()->CreateShader(shaderDesc, gMeshRender_CommonVS, sizeof(gMeshRender_CommonVS));

        shaderDesc.DebugName = "PBR Pixel Shader";
        shaderDesc.ShaderType = EShaderType::Pixel;
        ShaderHandle ps = this->GetGraphicsDevice()->CreateShader(shaderDesc, gMeshRender_BrdfPS, sizeof(gMeshRender_BrdfPS));

        GraphicsPSODesc psoDesc = {};
        psoDesc.VertexShader = vs;
        psoDesc.PixelShader = ps;
        psoDesc.InputLayout = nullptr;
        psoDesc.DsvFormat = this->m_depthBuffer->GetDesc().Format;
        psoDesc.RtvFormats.push_back(this->GetGraphicsDevice()->GetBackBuffer()->GetDesc().Format);

        // TODO: Work around to get things working. I really shouldn't even care making my stuff abstract. But here we are.
        Dx12::RootSignatureBuilder builder = {};

        builder.Add32BitConstants<999, 0>(sizeof(Shader::MeshRenderPushConstant) / 4);
        builder.AddConstantBufferView<0, 0>(); // FrameCB
        builder.AddConstantBufferView<1, 0>(); // Camera CB
        // builder.AddShaderResourceView<0, 0>(); // Instance SB

        // Default Sampler
        builder.AddStaticSampler<50, 0>(
            D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            1U);

        // BRDF Sampler
        builder.AddStaticSampler<51, 0>(
            D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

        // Shadow Map Sampler
        builder.AddStaticSampler<52, 0>(
            D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            16u,
            D3D12_COMPARISON_FUNC_LESS);


        Dx12::DescriptorTable textureTable = {};
        textureTable.AddSRVRange<10, 0>(1, D3D12_DESCRIPTOR_RANGE_FLAG_NONE, 0);
        builder.AddDescriptorTable(textureTable);

        Dx12::DescriptorTable bindlessDescriptorTable = {};
        auto range = this->GetGraphicsDevice()->GetNumBindlessDescriptors();
        bindlessDescriptorTable.AddSRVRange<0, 100>(
            range,
            D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE,
            0);

        bindlessDescriptorTable.AddSRVRange<0, 101>(
            range,
            D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE,
            0);

        bindlessDescriptorTable.AddSRVRange<0, 102>(
            range,
            D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE,
            0);

        builder.AddDescriptorTable(bindlessDescriptorTable);

        psoDesc.RootSignatureBuilder = &builder;

        this->m_geomtryPassPso = this->GetGraphicsDevice()->CreateGraphicsPSOHandle(psoDesc);
    }
}

void GltfViewer::CreateRenderTargets()
{
    TextureDesc desc = {};
    desc.Format = EFormat::D32;
    desc.Width = WindowWidth;
    desc.Height = WindowHeight;
    desc.Dimension = TextureDimension::Texture2D;
    desc.DebugName = "Depth Buffer";
    Color clearValue = { 1.0f, 0.0f, 0.0f, 0.0f };
    desc.OptmizedClearValue = std::make_optional<Color>(clearValue);

    this->m_depthBuffer = this->GetGraphicsDevice()->CreateDepthStencil(desc);
}
