#pragma once

#include <DirectXMath.h>
#include <PhxEngine/Core/Span.h>
#include <PhxEngine/Core/Primitives.h>
#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/Core/Helpers.h>


namespace PhxEngine::Scene
{
	struct CameraComponent;
	class Scene;
}

namespace PhxEngine::Renderer
{
	struct ResourceUpload
	{
		RHI::BufferHandle UploadBuffer;
		void* Data = nullptr;
		uint64_t Offset = 0;
		
		uint64_t SetData(void* srcData, uint64_t sizeInBytes, uint64_t alignment = 0)
		{
			uint64_t currentOffset = this->Offset;

			std::memcpy(static_cast<uint8_t*>(this->Data) + this->Offset, srcData, sizeInBytes);
			this->Offset += Core::Helpers::AlignTo(sizeInBytes, alignment);

			return currentOffset;
		}

		void Free()
		{
			RHI::IGraphicsDevice::GPtr->DeleteBuffer(this->UploadBuffer);
		}
	};

	static ResourceUpload CreateResourceUpload(uint64_t sizeInBytes, std::string const& debugName = "Resource Upload")
	{
		ResourceUpload retVal = {};
		retVal.UploadBuffer = RHI::IGraphicsDevice::GPtr->CreateBuffer(
			{
				.MiscFlags = RHI::BufferMiscFlags::Raw,
				.Usage = RHI::Usage::Upload,
				.Binding = RHI::BindingFlags::None,
				.InitialState = RHI::ResourceStates::CopySource,
				.SizeInBytes = sizeInBytes,
				.CreateBindless = false,
				.DebugName = "Resource Upload",
			});

		retVal.Data = RHI::IGraphicsDevice::GPtr->GetBufferMappedData(retVal.UploadBuffer);
		return retVal;
	}

	struct RenderCam
	{
		DirectX::XMMATRIX ViewProjection;
		Core::Frustum Frustum;

		RenderCam() = default;
		RenderCam(DirectX::XMFLOAT3 const& eyePos, DirectX::XMFLOAT4 const& rotation, float nearPlane, float farPlane, float fov)
		{
			const DirectX::XMVECTOR eye = DirectX::XMLoadFloat3(&eyePos);
			const DirectX::XMVECTOR qRot = DirectX::XMQuaternionNormalize(DirectX::XMLoadFloat4(&rotation));
			const DirectX::XMMATRIX rot = DirectX::XMMatrixRotationQuaternion(qRot);
			const DirectX::XMVECTOR to = DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f), rot);
			const DirectX::XMVECTOR up = DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), rot);
			const DirectX::XMMATRIX V = DirectX::XMMatrixLookToRH(eye, to, up);

			// Note the farPlane is passed in as near, this is to support reverseZ
			const DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovRH(fov, 1, farPlane, nearPlane);
			this->ViewProjection = XMMatrixMultiply(V, P);
		}
	};

	inline void CreateCubemapCameras(DirectX::XMFLOAT3 position, float nearZ, float farZ, Core::Span<RenderCam> cameras)
	{
		assert(cameras.Size() == 6);
		/*
		cameras[0] = RenderCam(position, DirectX::XMFLOAT4(0, -0.707f, 0, 0.707f), nearZ, farZ, DirectX::XM_PIDIV2); // +x
		cameras[1] = RenderCam(position, DirectX::XMFLOAT4(0, 0.707f, 0, 0.707f), nearZ, farZ, DirectX::XM_PIDIV2); // -x
		cameras[2] = RenderCam(position, DirectX::XMFLOAT4(0.707f, 0, 0, 0.707f), nearZ, farZ, DirectX::XM_PIDIV2); // +y
		cameras[3] = RenderCam(position, DirectX::XMFLOAT4(-0.707f, 0, 0, 0.707f), nearZ, farZ, DirectX::XM_PIDIV2); // -y
		cameras[4] = RenderCam(position, DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f), nearZ, farZ, DirectX::XM_PIDIV2); // +z
		cameras[5] = RenderCam(position, DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f), nearZ, farZ, DirectX::XM_PIDIV2); // -z
		*/
	}

	// -- Enums ---
	enum class BlendMode
	{
		Opaque = 0,
		Alpha,
		Count
	};

}