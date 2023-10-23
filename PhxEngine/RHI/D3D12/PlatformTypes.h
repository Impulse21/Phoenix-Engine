#pragma once

#include <Core/Memory.h>
#include <D3D12Context.h>

namespace PhxEngine::RHI
{
	struct D3D12RHIResource
	{
	private:
		std::atomic<unsigned long> m_refCount = 1;

	public:
		unsigned long AddRef()
		{
			return ++m_refCount;
		}

		unsigned long Release()
		{
			unsigned long result = --m_refCount;
			if (result == 0)
			{
				phx_delete(this);
			}
			return result;
		}
	};

	struct D3D12GpuBuffer
	{
	};
	using PlatformGpuBuffer = D3D12GpuBuffer;


	struct D3D12Texture
	{
	};
	using PlatformTexture = D3D12Texture;


	struct D3D12SubmitRecipt
	{

	};
	using PlatformSubmitRecipt = D3D12SubmitRecipt;

	struct D3D12GfxPipeline
	{
	};
	using PlatformGfxPipeline = D3D12GfxPipeline;

	struct D3D12ComputePipeline
	{
	};
	using PlatformComputePipeline = D3D12ComputePipeline;

	
	struct D3D12MeshPipeline
	{
	};
	using PlatformMeshPipeline = D3D12MeshPipeline;

	struct D3D12InputLayout
	{
	};
	using PlatformInputLayout = D3D12InputLayout;

	struct D3D12Shader
	{
	};
	using PlatformShader = D3D12Shader;

	struct D3D12CommandSignature
	{
	};
	using PlatformCommandSignature = D3D12CommandSignature;

	struct D3D12SwapChain : D3D12RHIResource
	{
		IDXGISwapChain4* DxgiSwapchain;
		~D3D12SwapChain()
		{
			// TODO: Enqueue Resource Deletion
			D3D12::D3D12Context::DeleteItem d =
			{
				this->m_frameCount,
				[=]()
				{
					DxgiSwapchain->Release();
					// Release Handles
				}
			};

			auto& deleteQueue = D3D12Ctx::DeferredDeleteQueue;
			ScopedSpinLock _(deleteQueue.Lock);
			deleteQueue.Queue.push_back(d);

		}
	};
	using PlatformSwapChain = D3D12SwapChain;

}

