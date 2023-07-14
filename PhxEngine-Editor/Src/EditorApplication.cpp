#include <PhxEngine/PhxEngine.h>

#include <PhxEngine/Engine/ApplicationBase.h>
#include <imgui.h>

#include <conio.h>

#include <iostream>
#include <PhxEngine/Engine/ImguiRenderer.h>
#include <PhxEngine/Core/Window.h>
#include <PhxEngine/Core/Helpers.h>
#include <DirectXMath.h>

using namespace PhxEngine;
using namespace PhxEngine::Core;
using namespace PhxEngine::RHI;
using namespace PhxEngine::Graphics;
using namespace PhxEngine::Renderer;

using namespace DirectX;

// Construct mesh
struct MeshAsset
{
    constexpr static uint64_t kVertexBufferAlignment = 16ull;
    enum Flags : uint8_t
    {
        kEmpty = 0,
        kContainsNormals = 1 << 0,
        kContainsTexCoords = 1 << 1,
        kContainsTangents = 1 << 2,
        kContainsColour = 1 << 3,
    };

    uint32_t Flags = kEmpty;

    size_t TotalVerts = 0;
    size_t TotalIndices = 0;
    DirectX::XMFLOAT3* Positions = nullptr;
    DirectX::XMFLOAT2* TexCoords = nullptr;
    DirectX::XMFLOAT3* Normals = nullptr;
    DirectX::XMFLOAT4* Tangents = nullptr;
    DirectX::XMFLOAT3* Colour = nullptr;
    uint8_t* VertexBufferData = nullptr;
    size_t VertexBufferSize = 0;

    uint32_t* Indices = nullptr;
    enum class VertexAttribute : uint8_t
    {
        Position = 0,
        TexCoord,
        Normal,
        Tangent,
        Colour,
        Count,
    };

    std::array<RHI::BufferRange, (int)VertexAttribute::Count> BufferRanges;

    // -- Helper Functions ---
    [[nodiscard]] bool HasVertexAttribuite(VertexAttribute attr) const { return this->BufferRanges[(int)attr].SizeInBytes != 0; }
    RHI::BufferRange& GetVertexAttribute(VertexAttribute attr) { return this->BufferRanges[(int)attr]; }
    [[nodiscard]] const RHI::BufferRange& GetVertexAttribute(VertexAttribute attr) const { return this->BufferRanges[(int)attr]; }
    void AllocateCpuData(size_t totalVerts, size_t totalIndices, IAllocator& allocator)
    {
        this->TotalVerts = totalVerts;
        this->TotalIndices = totalIndices;

        this->Positions = allocator.Allocate<DirectX::XMFLOAT3>(16);
        this->TexCoords = allocator.Allocate<DirectX::XMFLOAT2>(16);
        this->Normals = allocator.Allocate<DirectX::XMFLOAT3>(16);
        this->Tangents = allocator.Allocate<DirectX::XMFLOAT4>(16);
        this->Colour = allocator.Allocate<DirectX::XMFLOAT3>(16);
        this->Indices = allocator.Allocate<uint32_t>(16);

    }

    MeshDesc CreateMeshDesc()
    {

    };

    void ComputeTangentSpace()
    {
        std::vector<DirectX::XMVECTOR> computedTangents(this->TotalVerts);
        std::vector<DirectX::XMVECTOR> computedBitangents(this->TotalVerts);

        for (int i = 0; i < this->TotalIndices; i += 3)
        {
            auto& index0 = this->Indices[i + 0];
            auto& index1 = this->Indices[i + 1];
            auto& index2 = this->Indices[i + 2];

            // Vertices
            DirectX::XMVECTOR pos0 = DirectX::XMLoadFloat3(&this->Positions[index0]);
            DirectX::XMVECTOR pos1 = DirectX::XMLoadFloat3(&this->Positions[index1]);
            DirectX::XMVECTOR pos2 = DirectX::XMLoadFloat3(&this->Positions[index2]);

            // UVs
            DirectX::XMVECTOR uvs0 = DirectX::XMLoadFloat2(&this->TexCoords[index0]);
            DirectX::XMVECTOR uvs1 = DirectX::XMLoadFloat2(&this->TexCoords[index1]);
            DirectX::XMVECTOR uvs2 = DirectX::XMLoadFloat2(&this->TexCoords[index2]);

            DirectX::XMVECTOR deltaPos1 = DirectX::XMVectorSubtract(pos1, pos0);
            DirectX::XMVECTOR deltaPos2 = DirectX::XMVectorSubtract(pos2, pos0);

            DirectX::XMVECTOR deltaUV1 = DirectX::XMVectorSubtract(uvs1, uvs0);
            DirectX::XMVECTOR deltaUV2 = DirectX::XMVectorSubtract(uvs2, uvs0);

            // TODO: Take advantage of SIMD better here
            float r = 1.0f / (DirectX::XMVectorGetX(deltaUV1) * DirectX::XMVectorGetY(deltaUV2) - DirectX::XMVectorGetY(deltaUV1) * DirectX::XMVectorGetX(deltaUV2));

            DirectX::XMVECTOR tangent = (deltaPos1 * DirectX::XMVectorGetY(deltaUV2) - deltaPos2 * DirectX::XMVectorGetY(deltaUV1)) * r;
            DirectX::XMVECTOR bitangent = (deltaPos2 * DirectX::XMVectorGetX(deltaUV1) - deltaPos1 * DirectX::XMVectorGetX(deltaUV2)) * r;

            computedTangents[index0] += tangent;
            computedTangents[index1] += tangent;
            computedTangents[index2] += tangent;

            computedBitangents[index0] += bitangent;
            computedBitangents[index1] += bitangent;
            computedBitangents[index2] += bitangent;
        }

        for (int i = 0; i < this->TotalVerts; i++)
        {
            const DirectX::XMVECTOR normal = DirectX::XMLoadFloat3(&this->Normals[i]);
            const DirectX::XMVECTOR& tangent = computedTangents[i];
            const DirectX::XMVECTOR& bitangent = computedBitangents[i];

            // Gram-Schmidt orthogonalize
            DirectX::XMVECTOR orthTangent = DirectX::XMVector3Normalize(tangent - normal * DirectX::XMVector3Dot(normal, tangent));
            float sign = DirectX::XMVectorGetX(DirectX::XMVector3Dot(DirectX::XMVector3Cross(normal, tangent), bitangent)) > 0
                ? -1.0f
                : 1.0f;

            orthTangent = DirectX::XMVectorSetW(orthTangent, sign);
            DirectX::XMStoreFloat4(&this->Tangents[i], orthTangent);
        }
    }
    
    void ReverseWinding()
    {
        assert((this->TotalIndices % 3) == 0);
        for (size_t i = 0; i < this->TotalIndices; i += 3)
        {
            std::swap(this->Indices[i + 1], this->Indices[i + 2]);
        }
    }
    void FlipZ()
    {
        // Flip Z
        for (int i = 0; i < this->TotalVerts; i++)
        {
            this->Positions[i].z *= -1.0f;
        }
        for (int i = 0; i < this->TotalVerts; i++)
        {
            this->Normals[i].z *= -1.0f;
        }
        for (int i = 0; i < this->TotalVerts; i++)
        {
            this->Tangents[i].z *= -1.0f;
        }
    }

    void BuildResourceData()
    {
        size_t vertexBufferSize = Helpers::AlignTo(this->TotalVerts * sizeof(DirectX::XMFLOAT3), kVertexBufferAlignment);

        if (this->Normals && (this->Flags & Flags::kContainsNormals) == Flags::kContainsNormals)
        {
            vertexBufferSize += Helpers::AlignTo(this->TotalVerts * sizeof(DirectX::XMFLOAT3), kVertexBufferAlignment);
        }

        if (this->TexCoords && (this->Flags & Flags::kContainsTexCoords) == Flags::kContainsTexCoords)
        {
            vertexBufferSize += Helpers::AlignTo(this->TotalVerts * sizeof(DirectX::XMFLOAT2), kVertexBufferAlignment);
        }

        if (this->Tangents && (this->Flags & Flags::kContainsTangents) == Flags::kContainsTangents)
        {
            vertexBufferSize += Helpers::AlignTo(this->TotalVerts * sizeof(DirectX::XMFLOAT4), kVertexBufferAlignment);
        }
        this->VertexBufferSize = vertexBufferSize;
        this->VertexBufferData = static_cast<uint8_t*>(MemoryService::GetInstance().GetSystemAllocator().Allocate(vertexBufferSize, 0));

        uint64_t bufferOffset = 0ull;
        auto WriteDataToGpuBuffer = [&](VertexAttribute attr, void* data, uint64_t sizeInBytes)
        {
            auto& bufferRange = this->GetVertexAttribute(attr);
            bufferRange.ByteOffset = bufferOffset;
            bufferRange.SizeInBytes = sizeInBytes;
            bufferOffset += Helpers::AlignTo(bufferRange.SizeInBytes, kVertexBufferAlignment);
            std:memcpy(this->VertexBufferData + bufferRange.ByteOffset, data, bufferRange.SizeInBytes);
        };

        if (this->Positions)
        {
            WriteDataToGpuBuffer(
                VertexAttribute::Position,
                this->Positions,
                this->TotalVerts * sizeof(DirectX::XMFLOAT3));
        }

        if (this->TexCoords)
        {
            WriteDataToGpuBuffer(
                VertexAttribute::TexCoord,
                this->TexCoords,
                this->TotalVerts * sizeof(DirectX::XMFLOAT2));
        }

        if (this->Normals)
        {
            WriteDataToGpuBuffer(
                VertexAttribute::Normal,
                this->Normals,
                this->TotalVerts * sizeof(DirectX::XMFLOAT3));
        }

        if (this->Tangents)
        {
            WriteDataToGpuBuffer(
                VertexAttribute::Tangent,
                this->Tangents,
                this->TotalVerts * sizeof(DirectX::XMFLOAT4));
        }

    }

};

MeshAsset* CreateCube(
    float size,
    bool rhsCoord)
{
    // A cube has six faces, each one pointing in a different direction.
    constexpr int FaceCount = 6;

    constexpr XMVECTORF32 faceNormals[FaceCount] =
    {
        { 0,  0,  1 },
        { 0,  0, -1 },
        { 1,  0,  0 },
        { -1,  0,  0 },
        { 0,  1,  0 },
        { 0, -1,  0 },
    };

    constexpr XMFLOAT3 faceColour[] =
    {
        { 1.0f,  0.0f,  0.0f },
        { 0.0f,  1.0f,  0.0f },
        { 0.0f,  0.0f,  1.0f },
    };

    constexpr XMFLOAT2 textureCoordinates[4] =
    {
        { 1, 0 },
        { 1, 1 },
        { 0, 1 },
        { 0, 0 },
    };

    MeshAsset* retVal = MemoryService::GetInstance().GetSystemAllocator().Allocate<MeshAsset>();
    size /= 2;
    retVal->AllocateCpuData (FaceCount * 4, FaceCount * 6, MemoryService::GetInstance().GetSystemAllocator());

    size_t vbase = 0;
    size_t ibase = 0;
    for (int i = 0; i < FaceCount; i++)
    {
        XMVECTOR normal = faceNormals[i];

        // Get two vectors perpendicular both to the face normal and to each other.
        XMVECTOR basis = (i >= 4) ? g_XMIdentityR2 : g_XMIdentityR1;

        XMVECTOR side1 = XMVector3Cross(normal, basis);
        XMVECTOR side2 = XMVector3Cross(normal, side1);

        // Six indices (two triangles) per face.
        retVal->Indices[ibase + 0] = static_cast<uint16_t>(vbase + 0);
        retVal->Indices[ibase + 1] = static_cast<uint16_t>(vbase + 1);
        retVal->Indices[ibase + 2] = static_cast<uint16_t>(vbase + 2);

        retVal->Indices[ibase + 3] = static_cast<uint16_t>(vbase + 0);
        retVal->Indices[ibase + 4] = static_cast<uint16_t>(vbase + 2);
        retVal->Indices[ibase + 5] = static_cast<uint16_t>(vbase + 3);

        XMFLOAT3 positon;
        XMStoreFloat3(&positon, (normal - side1 - side2) * size);
        retVal->Positions[vbase + 0] = positon;

        XMStoreFloat3(&positon, (normal - side1 + side2) * size);
        retVal->Positions[vbase + 1] = positon;

        XMStoreFloat3(&positon, (normal + side1 + side2) * size);
        retVal->Positions[vbase + 2] = positon;

        XMStoreFloat3(&positon, (normal + side1 - side2) * size);
        retVal->Positions[vbase + 3] = positon;

        retVal->TexCoords[vbase + 0] = textureCoordinates[0];
        retVal->TexCoords[vbase + 1] = textureCoordinates[1];
        retVal->TexCoords[vbase + 2] = textureCoordinates[2];
        retVal->TexCoords[vbase + 3] = textureCoordinates[3];

        retVal->Colour[vbase + 0] = faceColour[0];
        retVal->Colour[vbase + 1] = faceColour[1];
        retVal->Colour[vbase + 2] = faceColour[2];
        retVal->Colour[vbase + 3] = faceColour[3];

        DirectX::XMStoreFloat3((retVal->Normals + vbase + 0), normal);
        DirectX::XMStoreFloat3((retVal->Normals + vbase + 1), normal);
        DirectX::XMStoreFloat3((retVal->Normals + vbase + 2), normal);
        DirectX::XMStoreFloat3((retVal->Normals + vbase + 3), normal);

        vbase += 4;
        ibase += 6;
    }

    retVal->Flags =
        MeshAsset::Flags::kContainsNormals |
        MeshAsset::Flags::kContainsTexCoords |
        MeshAsset::Flags::kContainsTangents |
        MeshAsset::Flags::kContainsColour;

    retVal->ComputeTangentSpace();

    if (rhsCoord)
    {
        retVal->ReverseWinding();
        retVal->FlipZ();
    }

    // Calculate AABB
    // Calculate Meshlet Data
    return retVal;
}

class PhxEngineEditorApp : public IEngineApplication
{
private:

public:
    PhxEngineEditorApp() = default;

    bool Initialize() override
    {
        this->m_profile = {};

        LOG_CORE_INFO("Initializing Editor App");
        WindowSpecification windowSpec = {};
        windowSpec.Width = 1280;
        windowSpec.Height = 720;
        windowSpec.Title = "Phx Engine Editor";
        windowSpec.VSync = false;

        this->m_window = WindowFactory::CreateGlfwWindow(windowSpec);
        this->m_window->Initialize();
        this->m_window->SetResizeable(false);
        this->m_window->SetVSync(windowSpec.VSync);
        this->m_window->SetEventCallback(
            [this](Event& e) {
                this->ProcessEvent(e);
            });

        this->m_windowVisibile = true;

       GfxDevice::Impl->CreateViewport(
            {
                .WindowHandle = static_cast<Platform::WindowHandle>(m_window->GetNativeWindowHandle()),
                .Width = this->m_window->GetWidth(),
                .Height = this->m_window->GetHeight(),
                .Format = RHI::RHIFormat::R10G10B10A2_UNORM,
            });

       DirectX::XMFLOAT2 m_canvasSize;


       this->m_cubeMesh = CreateCube(1.0f, false);
       this->m_cubeMesh->BuildResourceData();
       return true;
    }

    bool Finalize() override
    {
        LOG_CORE_INFO("PhxEngine is Finalizing.");
        if (this->m_cubeMesh)
        {
        }

        return true;
    }

    bool ShuttingDown() const { return this->m_shuttingDown; }

    bool RunFrame()
    {
        TimeStep deltaTime = this->m_frameTimer.Elapsed();
        this->m_frameTimer.Begin();

        // polls events.
        this->m_window->OnUpdate();

        if (this->m_windowVisibile)
        {
            // Run Sub-System updates

            // auto updateLoopId = this->m_frameProfiler->BeginRangeCPU("Update Loop");
            // this->Update(deltaTime);
            //this->m_frameProfiler->EndRangeCPU(updateLoopId);

            // auto renderLoopId = this->m_frameProfiler->BeginRangeCPU("Render Loop");
            // this->Render();
            // this->m_frameProfiler->EndRangeCPU(renderLoopId);
        }

        this->m_profile.UpdateAverageFrameTime(deltaTime);
        this->SetInformativeWindowTitle("Phx Editor", {});
        return true;
    }

private:
    void SetInformativeWindowTitle(std::string_view appName, std::string_view extraInfo)
    {
        std::stringstream ss;
        ss << appName;
        ss << "(D3D12)";

        double frameTime = this->m_profile.AverageFrameTime;
        if (frameTime > 0)
        {
            // ss << "=>" << std::setprecision(4) << frameTime << " CPU (ms) - " << std::setprecision(4) << (1.0f / frameTime) << " FPS";
            ss << "- " << std::setprecision(4) << (1.0f / frameTime) << " FPS";
        }
        const RHI::MemoryUsage memoryUsage = GfxDevice::Impl->GetMemoryUsage();

        ss << " [ " << PhxToMB(memoryUsage.Usage) << "/" << PhxToMB(memoryUsage.Budget) << " (MB) ]";
        if (!extraInfo.empty())
        {
            ss << extraInfo;
        }

        this->m_window->SetWindowTitle(ss.str());
    }

    void ProcessEvent(Event& e)
    {
        if (e.GetEventType() == WindowCloseEvent::GetStaticType())
        {
            this->m_shuttingDown = true;
            e.IsHandled = true;

            return;
        }

        if (e.GetEventType() == WindowResizeEvent::GetStaticType())
        {
            WindowResizeEvent& resizeEvent = static_cast<WindowResizeEvent&>(e);
            if (resizeEvent.GetWidth() == 0 && resizeEvent.GetHeight() == 0)
            {
                this->m_windowVisibile = true;
            }
            else
            {
                this->m_windowVisibile = false;
                // Notify GFX to resize!

                // Trigger Resize event on RHI;
                /*
                for (auto& pass : this->m_renderPasses)
                {
                    pass->OnWindowResize(resizeEvent);
                }
                */
            }
        }
    }
    private:
        class SimpleProfiler
        {
        public:
            double AverageFrameTime = 0.0f;

            void UpdateAverageFrameTime(double elapsedTime)
            {
                this->m_frameTimeSum += elapsedTime;
                this->m_numAccumulatedFrames += 1;

                if (this->m_frameTimeSum > this->m_averageTimeUpdateInterval && this->m_numAccumulatedFrames > 0)
                {
                    this->AverageFrameTime = m_frameTimeSum / double(this->m_numAccumulatedFrames);
                    this->m_numAccumulatedFrames = 0;
                    this->m_frameTimeSum = 0.0;
                }
            }

        private:
            double m_frameTimeSum = 0.0f;
            uint64_t m_numAccumulatedFrames = 0;
            double m_averageTimeUpdateInterval = 0.5f;
        } m_profile = {};
private:
    std::unique_ptr<IWindow> m_window;

    bool m_windowVisibile = false;
    bool m_shuttingDown = false;
    Core::StopWatch m_frameTimer; // TODO: Move to a profiler and scope

    // Testing
    MeshAsset* m_cubeMesh = nullptr;
};


class PhxEngineEditorUI : public PhxEngine::ImGuiRenderer
{
private:

public:
    PhxEngineEditorUI(PhxEngineEditorApp* app)
        : m_app(app)
    {
    }

    void BuildUI() override
    {
    }

private:
    PhxEngineEditorApp* m_app;
};


#if 0
#ifdef WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main(int __argc, const char** __argv)
#endif
#endif 

int main(int __argc, const char** __argv)
{
    EngineParam params = {};
    params.Name = "Phx Editor";
    params.GraphicsAPI = RHI::GraphicsAPI::DX12;
    params.WindowWidth = 2000;
    params.WindowHeight = 1200;
    params.MaximumDynamicSize = PhxGB(1);


    PhxEngine::EngineInitialize(params);

    {
        PhxEngineEditorApp engineApp;
        PhxEngine::EngineRun(engineApp);
    }

    PhxEngine::EngineFinalize();

    return 0;
}