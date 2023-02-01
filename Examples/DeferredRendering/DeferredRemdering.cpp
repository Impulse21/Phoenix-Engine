#include <PhxEngine/PhxEngine.h>
 
#include <PhxEngine/Graphics/ShaderFactory.h>
#include <PhxEngine/Core/Helpers.h>
#include <PhxEngine/Core/Platform.h>
#include <PhxEngine/Core/Span.h>
#include <PhxEngine/Core/VirtualFileSystem.h>
#include <PhxEngine/Scene/Scene.h>
#include <PhxEngine/Scene/Entity.h>
#include <PhxEngine/Scene/Components.h>
#include <PhxEngine/Graphics/TextureCache.h>
#include <PhxEngine/Renderer/Renderer.h>
#include <PhxEngine/Graphics/ShaderFactory.h>

using namespace PhxEngine;
using namespace PhxEngine::RHI;

namespace CubeGeometry
{
    static constexpr uint16_t kIndices[] = {
         0,  1,  2,   0,  3,  1, // front face
         4,  5,  6,   4,  7,  5, // left face
         8,  9, 10,   8, 11,  9, // right face
        12, 13, 14,  12, 15, 13, // back face
        16, 17, 18,  16, 19, 17, // top face
        20, 21, 22,  20, 23, 21, // bottom face
    };

    static constexpr DirectX::XMFLOAT3 kPositions[] = {
        {-0.5f,  0.5f, -0.5f}, // front face
        { 0.5f, -0.5f, -0.5f},
        {-0.5f, -0.5f, -0.5f},
        { 0.5f,  0.5f, -0.5f},

        { 0.5f, -0.5f, -0.5f}, // right side face
        { 0.5f,  0.5f,  0.5f},
        { 0.5f, -0.5f,  0.5f},
        { 0.5f,  0.5f, -0.5f},

        {-0.5f,  0.5f,  0.5f}, // left side face
        {-0.5f, -0.5f, -0.5f},
        {-0.5f, -0.5f,  0.5f},
        {-0.5f,  0.5f, -0.5f},

        { 0.5f,  0.5f,  0.5f}, // back face
        {-0.5f, -0.5f,  0.5f},
        { 0.5f, -0.5f,  0.5f},
        {-0.5f,  0.5f,  0.5f},

        {-0.5f,  0.5f, -0.5f}, // top face
        { 0.5f,  0.5f,  0.5f},
        { 0.5f,  0.5f, -0.5f},
        {-0.5f,  0.5f,  0.5f},

        { 0.5f, -0.5f,  0.5f}, // bottom face
        {-0.5f, -0.5f, -0.5f},
        { 0.5f, -0.5f, -0.5f},
        {-0.5f, -0.5f,  0.5f},
    };

    static constexpr DirectX::XMFLOAT2 kTexCoords[] = {
        {0.0f, 0.0f}, // front face
        {1.0f, 1.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},

        {0.0f, 1.0f}, // right side face
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 0.0f},

        {0.0f, 0.0f}, // left side face
        {1.0f, 1.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},

        {0.0f, 0.0f}, // back face
        {1.0f, 1.0f},
        {0.0f, 1.0f},
        {1.0f, 0.0f},

        {0.0f, 1.0f}, // top face
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 0.0f},

        {1.0f, 1.0f}, // bottom face
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {0.0f, 1.0f},
    };

    static constexpr DirectX::XMFLOAT3 kNormals[] = {
        { 0.0f, 0.0f, -1.0f }, // front face
        { 0.0f, 0.0f, -1.0f },
        { 0.0f, 0.0f, -1.0f },
        { 0.0f, 0.0f, -1.0f },

        { 1.0f, 0.0f, 0.0f }, // right side face
        { 1.0f, 0.0f, 0.0f },
        { 1.0f, 0.0f, 0.0f },
        { 1.0f, 0.0f, 0.0f },

        { -1.0f, 0.0f, 0.0f }, // left side face
        { -1.0f, 0.0f, 0.0f },
        { -1.0f, 0.0f, 0.0f },
        { -1.0f, 0.0f, 0.0f },

        { 0.0f, 0.0f, 1.0f }, // back face
        { 0.0f, 0.0f, 1.0f },
        { 0.0f, 0.0f, 1.0f },
        { 0.0f, 0.0f, 1.0f },

        { 0.0f, 1.0f, 0.0f }, // top face
        { 0.0f, 1.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f },

        { 0.0f, -1.0f, 0.0f }, // bottom face
        { 0.0f, -1.0f, 0.0f },
        { 0.0f, -1.0f, 0.0f },
        { 0.0f, -1.0f, 0.0f },
    };

    static constexpr DirectX::XMFLOAT4 kTangents[] = {
        { 1.0f, 0.0f, 0.0f, 1.0f }, // front face
        { 1.0f, 0.0f, 0.0f, 1.0f },
        { 1.0f, 0.0f, 0.0f, 1.0f },
        { 1.0f, 0.0f, 0.0f, 1.0f },
          
        { 0.0f, 0.0f, 1.0f, 1.0f }, // right side face
        { 0.0f, 0.0f, 1.0f, 1.0f },
        { 0.0f, 0.0f, 1.0f, 1.0f },
        { 0.0f, 0.0f, 1.0f, 1.0f },
          
        { 0.0f, 0.0f, -1.0f, 1.0f }, // left side face
        { 0.0f, 0.0f, -1.0f, 1.0f },
        { 0.0f, 0.0f, -1.0f, 1.0f },
        { 0.0f, 0.0f, -1.0f, 1.0f },
          
        { -1.0f, 0.0f, 0.0f, 1.0f }, // back face
        { -1.0f, 0.0f, 0.0f, 1.0f },
        { -1.0f, 0.0f, 0.0f, 1.0f },
        { -1.0f, 0.0f, 0.0f, 1.0f },
          
        { 1.0f, 0.0f, 0.0f, 1.0f }, // top face
        { 1.0f, 0.0f, 0.0f, 1.0f },
        { 1.0f, 0.0f, 0.0f, 1.0f },
        { 1.0f, 0.0f, 0.0f, 1.0f },
          
        { 1.0f, 0.0f, 0.0f, 1.0f }, // bottom face
        { 1.0f, 0.0f, 0.0f, 1.0f },
        { 1.0f, 0.0f, 0.0f, 1.0f },
        { 1.0f, 0.0f, 0.0f, 1.0f },
    };
}



class DeferredRendering : public EngineRenderPass
{
private:

public:
    DeferredRendering(IPhxEngineRoot* root)
        : EngineRenderPass(root)
    {
    }

    bool Initialize()
    {
        // Load Data in seperate thread?
        // Create File Sustem
        std::shared_ptr<Core::IFileSystem> nativeFS = Core::CreateNativeFileSystem();

        std::filesystem::path appShadersRoot = Core::Platform::GetExcecutableDir() / "Shaders";

        std::shared_ptr<Core::IRootFileSystem> rootFilePath = Core::CreateRootFileSystem();
        rootFilePath->Mount("PhxEngine\dxil", appShadersRoot);

        this->m_shaderFactory = std::make_unique<Graphics::ShaderFactory>(this->GetGfxDevice(), rootFilePath);

        auto textureCache = std::make_unique<Graphics::TextureCache>(nativeFS, this->GetGfxDevice());
        this->CreateSimpleScene(textureCache.get());

        return true;
    }

    void OnWindowResize(WindowResizeEvent const& e) override
    {
        this->GetGfxDevice()->DeleteGraphicsPipeline(this->m_pipeline);
        this->m_pipeline = {};
    }

    void Update(Core::TimeStep const& deltaTime) override
    {
        this->GetRoot()->SetInformativeWindowTitle("Basic Triangle", {});
    }

    void Render() override
    {
    }

private:
    void CreateSimpleScene(Graphics::TextureCache* textureCache)
    {
        ICommandList* commandList = this->GetGfxDevice()->BeginCommandRecording();

        Scene::Entity materialEntity = this->m_simpleScene.CreateEntity("Material");
        auto& mtl = materialEntity.AddComponent<Scene::MaterialComponent>();
        mtl.Roughness = 0.4;
        mtl.Ao = 0.4;
        mtl.Metalness = 0.4;
        mtl.BaseColour = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);

        std::filesystem::path texturePath = Core::Platform::GetExcecutableDir().parent_path().parent_path() / "Assets/Textures/TestTexture.png";
        mtl.BaseColourTexture = textureCache->LoadTexture(texturePath, true, commandList);

        Scene::Entity meshEntity = this->m_simpleScene.CreateEntity("Cube");
        auto& mesh = meshEntity.AddComponent<Scene::MeshComponent>();
        
        mesh.Indices.resize(ARRAYSIZE(CubeGeometry::kIndices));
        std::memcpy(mesh.Indices.data(), CubeGeometry::kIndices, sizeof(CubeGeometry::kIndices[0]) * ARRAYSIZE(CubeGeometry::kIndices));

        mesh.VertexPositions.resize(ARRAYSIZE(CubeGeometry::kPositions));
        std::memcpy(mesh.VertexPositions.data(), CubeGeometry::kPositions, sizeof(CubeGeometry::kPositions[0]) * ARRAYSIZE(CubeGeometry::kPositions));

        mesh.VertexNormals.resize(ARRAYSIZE(CubeGeometry::kNormals));
        std::memcpy(mesh.VertexNormals.data(), CubeGeometry::kNormals, sizeof(CubeGeometry::kNormals[0]) * ARRAYSIZE(CubeGeometry::kNormals));

        mesh.VertexTangents.resize(ARRAYSIZE(CubeGeometry::kTangents));
        std::memcpy(mesh.VertexTangents.data(), CubeGeometry::kTangents, sizeof(CubeGeometry::kTangents[0]) * ARRAYSIZE(CubeGeometry::kTangents));

        mesh.VertexTexCoords.resize(ARRAYSIZE(CubeGeometry::kTexCoords));
        std::memcpy(mesh.VertexTexCoords.data(), CubeGeometry::kTexCoords, sizeof(CubeGeometry::kTexCoords[0]) * ARRAYSIZE(CubeGeometry::kTexCoords));

        mesh.VertexPositions.resize(ARRAYSIZE(CubeGeometry::kPositions));
        std::memcpy(mesh.VertexPositions.data(), CubeGeometry::kPositions, sizeof(CubeGeometry::kPositions[0]) * ARRAYSIZE(CubeGeometry::kPositions));

        mesh.Flags = ~0u;

        auto& meshGeometry = mesh.Surfaces.emplace_back();
        {
            meshGeometry.Material = materialEntity;
        }

        meshGeometry.IndexOffsetInMesh = 0;
        meshGeometry.VertexOffsetInMesh = 9;
        meshGeometry.NumIndices = ARRAYSIZE(CubeGeometry::kIndices);
        meshGeometry.NumVertices = ARRAYSIZE(CubeGeometry::kPositions);

        mesh.TotalIndices = meshGeometry.NumIndices;
        mesh.TotalVertices = meshGeometry.NumVertices;

        // Add a Mesh Instance
        Scene::Entity meshInstanceEntity = this->m_simpleScene.CreateEntity("Cube Instance");
        auto& meshInstance = meshInstanceEntity.AddComponent<Scene::MeshInstanceComponent>();
        meshInstance.Mesh = meshEntity;

        Renderer::ResourceUpload indexUpload;
        Renderer::ResourceUpload vertexUpload;
        this->m_simpleScene.ConstructRenderData(commandList, indexUpload, vertexUpload);
        indexUpload.Free();
        vertexUpload.Free();

        commandList->Close();
        this->GetGfxDevice()->ExecuteCommandLists({ commandList }, true);
    }

private:
    std::unique_ptr<Graphics::ShaderFactory> m_shaderFactory;
    RHI::GraphicsPipelineHandle m_pipeline;
    Scene::Scene m_simpleScene;
};

#ifdef WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main(int __argc, const char** __argv)
#endif
{
    std::unique_ptr<IPhxEngineRoot> root = CreateEngineRoot();

    EngineParam params = {};
    params.Name = "PhxEngine Example: Basic Triangle";
    params.GraphicsAPI = RHI::GraphicsAPI::DX12;
    root->Initialize(params);

    {
        DeferredRendering example(root.get());
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
