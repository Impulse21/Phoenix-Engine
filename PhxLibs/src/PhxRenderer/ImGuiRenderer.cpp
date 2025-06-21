#include "PhxRenderer/PhxRenderer_pch.h"

#if false
#include <DirectXMath.h>

#include <PhxCore/Memory/MemorySystem.h>

#include <PhxRhi/RHICore.h>

#include <PhxRenderer/shaders/PrecompiledShaders.h>
#include "PhxRenderer/ImGuiRenderer.h"

#include "ImGui/imgui_impl_win32.h"
#include "PhxCore/Span.h"
#include "PhxCore/VFS.h"

using namespace phx;
using namespace phx::gfx;
using namespace phx::rhi;

namespace
{
    enum RootParameters
    {
        PushConstant,           // cbuffer vertexBuffer : register(b0)
        BindlessResources,
        NumRootParameters
    };

    struct ImGuiDrawCommand
    {
		DescriptorIndex TextureDescriptorIndex;
		Rect ScissorRect;

        uint32_t IndexCount;
        uint32_t VertexOffset;
        uint32_t IndexOffset;

        const ImDrawList* DrawList;
        ImDrawCallback DrawCallback;
        void* DrawCallbackData;
    };

    struct ImGuiDrawList
    {
	    uint32_t VertexCount;
        ImDrawVert* Vertices;

        uint32_t IndexCount;
        ImDrawIdx* Indices;

        uint32_t CommandCount;
        ImGuiDrawCommand* Commands;

		DirectX::XMFLOAT4X4 Mvp;
        Viewport View;
    };

}

void ImGuiRenderSystem::Initialize(IFileSystem* /*fs*/, void* windowHandle, bool enableDocking)
{
    m_imguiContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(m_imguiContext);

    if (!ImGui_ImplWin32_Init(windowHandle))
    {
        throw std::runtime_error("Failed it initalize IMGUI");
    }

    ImGuiIO& io = ImGui::GetIO();

    io.Fonts->Clear();  // Remove the default font

    // Load a new font to replace the default one
   io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 30.0f); // Adjust size as needed

    // Rebuild the font atlas
    io.Fonts->Build();
    // ImGui::GetIO().FontGlobalScale = 1.5f; // Increase UI scale
    
    // Drive this based on configuration
    if (enableDocking)
    {
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    }

    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

    unsigned char* pixelData = nullptr;
    int width;
    int height;

    io.Fonts->GetTexDataAsRGBA32(&pixelData, &width, &height);

    MemInfo memInfo = {};
    // Bytes per pixel * width of the image. Since we are using an RGBA8, there is 4 bytes per pixel.
    memInfo.RowPitch = width * 4;
    memInfo.SlicePitch = memInfo.RowPitch * height;
    memInfo.Data = pixelData;

    // Create texture
    m_fontTexture = RHI::CreateTexture({
            .DebugName = "ImGui Font",
            .Format = RHI::Format::RGBA8_UNORM,
            .Width = static_cast<uint32_t>(width),
            .Height = static_cast<uint32_t>(height),
            .ArraySize = 1,
        }, &memInfo);
    
    this->m_fontTextureBindlessIndex = RHI::GetDescriptorIndex(this->m_fontTexture, SubresouceType::SRV);
    io.Fonts->SetTexID(static_cast<ImTextureID>(this->m_fontTextureBindlessIndex));
    
    Span vsOutBytesCode = Span(ImGuiVS::g_MainVS, sizeof(ImGuiVS::g_MainVS));
    Span psOutBytesCode = Span(ImGuiPS::g_MainPS, sizeof(ImGuiPS::g_MainPS));

	m_pipeline = RHI::CreatePipelineState({
            .VS = {.ByteCode = vsOutBytesCode, .EntryPoint = "MainVS" },
            .PS = {.ByteCode = psOutBytesCode, .EntryPoint = "MainPS" },
		    .BlendState = {
                .Targets {
                    {
                        .BlendEnable = true,
                        .SrcBlend = BlendFactor::SrcAlpha,
                        .DestBlend = BlendFactor::InvSrcAlpha,
                        .BlendOp = EBlendOp::Add,
                        .SrcBlendAlpha = BlendFactor::One,
                        .DestBlendAlpha = BlendFactor::InvSrcAlpha,
                        .BlendOpAlpha = EBlendOp::Add,
                        .ColorWriteMask = ColorMask::All,
                    }
                }
            },
		    .DepthStencilState = { .DepthEnable = false, .DepthWriteMask = DepthWriteMask::Zero },
		    .RasterState = { .CullMode = RasterCullMode::None, .DepthClipEnable = true, .ScissorEnable = true },
            .VertexBufferBindings = {
                {
                    .SemanticName = "POSITION",
                    .Format = Format::RG32_FLOAT,
                },
                {
                    .SemanticName = "TEXCOORD",
                    .Format = Format::RG32_FLOAT,
                },
                {
                    .SemanticName = "COLOR",
                    .Format = Format::RGBA8_UNORM,
                },
            },
            .RenderPassInfo = {.RTFormats { RHI::Format::UNKNOWN }}
        });

    m_indexFormat = sizeof(ImDrawIdx) == 2 ? Format::R16_UINT : Format::R32_UINT;
}

void ImGuiRenderSystem::Finalize()
{
    RHI::DeleteTexture(m_fontTexture);
    RHI::DeletePipeline(m_pipeline);
}

void ImGuiRenderSystem::EnableDarkThemeColours()
{
    auto& colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_WindowBg] = ImVec4{ 0.1f, 0.105f, 0.11f, 1.0f };

    // Headers
    colors[ImGuiCol_Header] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
    colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
    colors[ImGuiCol_HeaderActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

    // Buttons
    colors[ImGuiCol_Button] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
    colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
    colors[ImGuiCol_ButtonActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

    // Frame BG
    colors[ImGuiCol_FrameBg] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
    colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
    colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

    // Tabs
    colors[ImGuiCol_Tab] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    colors[ImGuiCol_TabHovered] = ImVec4{ 0.38f, 0.3805f, 0.381f, 1.0f };
    colors[ImGuiCol_TabSelected] = ImVec4{ 0.28f, 0.2805f, 0.281f, 1.0f };
    colors[ImGuiCol_TabDimmed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    colors[ImGuiCol_TabDimmedSelected] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };

    // Title
    colors[ImGuiCol_TitleBg] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
}

void ImGuiRenderSystem::BeginFrame()
{
    ImGui::SetCurrentContext(m_imguiContext);
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void phx::gfx::ImGuiRenderSystem::EndFrame()
{
    ImGui::Render();
}


void* ImGuiRenderSystem::OnPreRender()
{
    ImDrawData* drawData = ImGui::GetDrawData();
    // Check if there is anything to render.
    if (!drawData || drawData->CmdListsCount == 0)
    {
        return nullptr;
    }

    ImVec2 displayPos = drawData->DisplayPos;
    
    Memory::FrameAllocator& allocator = Memory::GetFrameAllocator();
    auto imguiDrawList = allocator.Alloc<ImGuiDrawList>();

    imguiDrawList->VertexCount = drawData->TotalVtxCount;
    imguiDrawList->Vertices = allocator.AllocArray<ImDrawVert>(imguiDrawList->VertexCount);

	imguiDrawList->IndexCount = drawData->TotalIdxCount;
	imguiDrawList->Indices = allocator.AllocArray<ImDrawIdx>(imguiDrawList->IndexCount);

	imguiDrawList->CommandCount = 0;
	for (int i = 0; i < drawData->CmdListsCount; i++)
	{
		imguiDrawList->CommandCount += drawData->CmdLists[i]->CmdBuffer.size();
	}

    imguiDrawList->Commands = allocator.AllocArray<ImGuiDrawCommand>(imguiDrawList->CommandCount);
    

    // Set root arguments.
    //    DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixOrthographicRH( drawData->DisplaySize.x, drawData->DisplaySize.y, 0.0f, 1.0f );
	{
		const float L = drawData->DisplayPos.x;
		const float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
		const float T = drawData->DisplayPos.y;
		const float B = drawData->DisplayPos.y + drawData->DisplaySize.y;

		imguiDrawList->Mvp = DirectX::XMFLOAT4X4(
			2.0f / (R - L),     0.0f,               0.0f,       0.0f,
            0.0f,               2.0f / (T - B),     0.0f,       0.0f,
            0.0f,               0.0f,               0.5f,       0.0f,
            (R + L) / (L - R),  (T + B) / (B - T),  0.5f,       1.0f);

		imguiDrawList->View = Viewport(drawData->DisplaySize.x, drawData->DisplaySize.y);
    }

    uint32_t vertexOffset = 0;
    uint32_t indexOffset = 0;
    for (int i = 0; i < drawData->CmdListsCount; ++i)
    {
        const ImDrawList* cmdList = drawData->CmdLists[i];

        if (cmdList->VtxBuffer.size() > 0)
            std::memcpy(imguiDrawList->Vertices + vertexOffset, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.size() * sizeof(ImDrawVert));

		if (cmdList->IdxBuffer.size() > 0)
			std::memcpy(imguiDrawList->Indices + indexOffset, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.size() * sizeof(ImDrawIdx));

        for (int j = 0; j < cmdList->CmdBuffer.size(); ++j)
        {
			const ImDrawCmd& cmdBuffer = cmdList->CmdBuffer[j];

			const ImVec4& clipRect = cmdBuffer.ClipRect;

			ImGuiDrawCommand& command = imguiDrawList->Commands[j];

			command.TextureDescriptorIndex = static_cast<DescriptorIndex>(cmdBuffer.GetTexID());
			command.IndexCount = cmdBuffer.ElemCount;

			command.ScissorRect.MinX = static_cast<int>(clipRect.x - displayPos.x);
			command.ScissorRect.MinY = static_cast<int>(clipRect.y - displayPos.y);
			command.ScissorRect.MaxX = static_cast<int>(clipRect.z - displayPos.x);
			command.ScissorRect.MaxY = static_cast<int>(clipRect.w - displayPos.y);
			command.IndexCount = cmdBuffer.ElemCount;
			command.VertexOffset = vertexOffset;
			command.IndexOffset = indexOffset;
			command.DrawList = cmdList;
			command.DrawCallback = cmdBuffer.UserCallback;
			command.DrawCallbackData = cmdBuffer.UserCallbackData;

            indexOffset += cmdBuffer.ElemCount;
        }

        vertexOffset += cmdList->VtxBuffer.size();
    }

    return imguiDrawList;
}

void ImGuiRenderSystem::OnRender(RHI::CommandCtx* ctx, void* cachedData)
{
    if (!cachedData)
        return;

    auto drawList = static_cast<ImGuiDrawList*>(cachedData);
    if (drawList->IndexCount == 0)
        return;

    ctx->SetPipelineState(m_pipeline);

    // TODO: I am here
	struct ImguiDrawInfo
	{
		DirectX::XMFLOAT4X4 Mvp;
		uint32_t TextureIndex;
	} push = {};
	push.Mvp = drawList->Mvp;

	ctx->SetViewports({ drawList->View });

	const Format indexFormat = sizeof(ImDrawIdx) == 2 ? Format::R16_UINT : Format::R32_UINT;

	ctx->SetDynamicVertexBuffer(0, drawList->VertexCount, sizeof(ImDrawVert), drawList->Vertices);
	ctx->SetDynamicIndexBuffer(drawList->IndexCount, indexFormat, drawList->Indices);

	for (uint32_t i = 0; i < drawList->CommandCount; i++)
	{
        auto command = drawList->Commands[i];

#if false
        // TODO
		if (command.DrawCallback)
		{
			ImDrawCmd cmd;
			cmd.ElemCount = command.IndexCount;
			// cmd.ClipRect cannot restore cliprect here
			cmd.TextureId = command.TextureDescriptorIndex;
			cmd.UserCallback = command.DrawCallback;
			cmd.UserCallbackData = command.DrawCallbackData;
			command.DrawCallback(command.DrawList, &cmd);
		}
		else
#endif
		{
			Rect& scissorRect = command.ScissorRect;
			if (scissorRect.MaxX - scissorRect.MinX > 0 &&
				scissorRect.MaxY - scissorRect.MinY > 0)
			{
				push.TextureIndex = command.TextureDescriptorIndex;

				ctx->SetPushConstant(RootParameters::PushConstant, sizeof(ImguiDrawInfo), &push);
				ctx->SetScissors({ &scissorRect, 1 });
				ctx->DrawIndexed(command.IndexCount, 1, command.IndexOffset, command.VertexOffset, 0);
			}
		}
	}

	// cmd->TransitionBarriers(Span<GpuBarrier>(postBarriers.data(), postBarriers.size()));
}

void ImGuiRenderSystem::Render(CommandCtx* ctx)
{
    ImGui::SetCurrentContext(m_imguiContext);
    ImGui::Render();

    ImDrawData* drawData = ImGui::GetDrawData();

    // Check if there is anything to render.
    if (!drawData || drawData->CmdListsCount == 0)
    {
        return;
    }

    ImVec2 displayPos = drawData->DisplayPos;

    {
        ctx->SetPipelineState(m_pipeline);

        // Set root arguments.
        //    DirectX::XMMATRIX projectionMatrix = DirectX::XMMatrixOrthographicRH( drawData->DisplaySize.x, drawData->DisplaySize.y, 0.0f, 1.0f );
        float L = drawData->DisplayPos.x;
        float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
        float T = drawData->DisplayPos.y;
        float B = drawData->DisplayPos.y + drawData->DisplaySize.y;
        static const float mvp[4][4] =
        {
            { 2.0f / (R - L),   0.0f,           0.0f,       0.0f },
            { 0.0f,         2.0f / (T - B),     0.0f,       0.0f },
            { 0.0f,         0.0f,           0.5f,       0.0f },
            { (R + L) / (L - R),  (T + B) / (B - T),    0.5f,       1.0f },
        };

        struct ImguiDrawInfo
        {
            DirectX::XMFLOAT4X4 Mvp;
            uint32_t TextureIndex;
        } push = {};
        push.Mvp = DirectX::XMFLOAT4X4(&mvp[0][0]);

        Viewport v(drawData->DisplaySize.x, drawData->DisplaySize.y);
        ctx->SetViewports({ v });

        const Format indexFormat = sizeof(ImDrawIdx) == 2 ? Format::R16_UINT : Format::R32_UINT;

        for (int i = 0; i < drawData->CmdListsCount; ++i)
        {
            const ImDrawList* drawList = drawData->CmdLists[i];
            ctx->SetDynamicVertexBuffer(0, drawList->VtxBuffer.size(), sizeof(ImDrawVert), drawList->VtxBuffer.Data);
            ctx->SetDynamicIndexBuffer(drawList->IdxBuffer.size(), indexFormat, drawList->IdxBuffer.Data);

            int indexOffset = 0;
            for (int j = 0; j < drawList->CmdBuffer.size(); ++j)
            {
                const ImDrawCmd& drawCmd = drawList->CmdBuffer[j];

                if (drawCmd.UserCallback)
                {
                    drawCmd.UserCallback(drawList, &drawCmd);
                }
                else
                {
                    ImVec4 clipRect = drawCmd.ClipRect;
                    Rect scissorRect;
                    // TODO: Validate
                    scissorRect.MinX = static_cast<int>(clipRect.x - displayPos.x);
                    scissorRect.MinY = static_cast<int>(clipRect.y - displayPos.y);
                    scissorRect.MaxX = static_cast<int>(clipRect.z - displayPos.x);
                    scissorRect.MaxY = static_cast<int>(clipRect.w - displayPos.y);

                    if (scissorRect.MaxX - scissorRect.MinX > 0 &&
                        scissorRect.MaxY - scissorRect.MinY > 0)
                    {
                        auto desciptorIndex = static_cast<DescriptorIndex>(drawCmd.GetTexID());
                        push.TextureIndex = desciptorIndex;
                        ctx->SetPushConstant(RootParameters::PushConstant, sizeof(ImguiDrawInfo), &push);
                        ctx->SetScissors({ &scissorRect, 1 });
                        ctx->DrawIndexed(drawCmd.ElemCount, 1, indexOffset, 0, 0);
                    }
                }
                indexOffset += drawCmd.ElemCount;
            }
        }

        // cmd->TransitionBarriers(Span<GpuBarrier>(postBarriers.data(), postBarriers.size()));
        ImGui::EndFrame();
    }
}
#endif