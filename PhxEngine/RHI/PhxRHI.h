#pragma once

#include "Core/CommonInclude.h"
#include "PhxRHIDescriptors.h"
#include "D3D12/PlatformTypes.h"

namespace phx::rhi
{
	class IDeviceNotify;

	void Initialize(RHIDesc const& desc)
	{
		PlatformContext::Initialize(desc);
	}

	void RegisterDeviceNotify(IDeviceNotify* deviceNotify)
	{
		PlatformContext::RegisterDeviceNotify(deviceNotify);
	}

	void WaitForGpu()
	{
		PlatformContext::WaitForGpu();
	}
	void Finalize()
	{
		PlatformContext::Finalize();
	}

	struct GpuBufferDesc
	{

	};

	class GpuBuffer : NonCopyable
	{
		PlatformBuffer m_PlatformBuffer;
		GpuBufferDesc m_Desc;
	public:

		inline bool Init(const GpuBufferDesc& desc);

		const GpuBufferDesc& Desc() const { return m_Desc; }

		void Release()
		{
			this->m_PlatformBuffer.Release();
		}

		phx::rhi::PlatformBuffer& PlatformBuffer() { return this->m_PlatformBuffer; }
		const phx::rhi::PlatformBuffer& PlatformBuffer() const { return this->m_PlatformBuffer; }

#if false
		template<typename T> bool CreateConstantBufferView(
			const BindingSchema binding_schema,
			const ConstantBufferViewBindPoint<T> bind_point,
			ResourceOwner<ConstantBufferView>* view) const
		{
			return m_PlatformBuffer.CreateConstantBufferView(m_Desc, binding_schema, bind_point.PlatformBindPoint, view->ReleaseAndGetAddressOf());
		}
		bool CreateShaderView(ResourceOwner<BufferShaderView>* view) const
		{
			return m_PlatformBuffer.CreateShaderView(m_Desc, view->ReleaseAndGetAddressOf());
		}

		bool CreateShaderView(ResourceOwner<BufferTypedShaderView>* view) const
		{
			return m_PlatformBuffer.CreateShaderView(m_Desc, view->ReleaseAndGetAddressOf());
		}

		bool CreateUnorderedView(ResourceOwner<BufferUnorderedView>* view) const
		{
			return m_PlatformBuffer.CreateUnorderedView(m_Desc, view->ReleaseAndGetAddressOf());
		}

		bool CreateUnorderedView(ResourceOwner<BufferTypedUnorderedView>* view) const
		{
			return m_PlatformBuffer.CreateUnorderedView(m_Desc, view->ReleaseAndGetAddressOf());
		}

		uint8_t* Lock(const LockType type)
		{
			return this->m_PlatformBuffer.Lock(m_Desc, type);
		}

		void Unlock()
		{
			return this->m_PlatformBuffer.Unlock(m_Desc);
		}

		explicit operator bool() const
		{
			return !!this->m_PlatformBuffer;
		}

#endif
	};
}

