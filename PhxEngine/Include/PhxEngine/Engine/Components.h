#pragma once


#include <DirectXMath.h>
#include <PhxEngine/Core/UUID.h>
#include <PhxEngine/Core/Primitives.h>
#include <PhxEngine/Assets/Assets.h>
#include <entt.hpp>

namespace PhxEngine::World
{
	static constexpr DirectX::XMFLOAT4X4 cIdentityMatrix = DirectX::XMFLOAT4X4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
	using EntityId = entt::entity;
	struct IDComponent
	{
		Core::UUID ID;
	};

	struct NameComponent
	{
		std::string Name;

		inline void operator=(const std::string& str) { this->Name = str; }
		inline void operator=(std::string&& str) { this->Name = std::move(str); }
		inline bool operator==(const std::string& str) const { return this->Name.compare(str) == 0; }

	};


	struct TransformComponent
	{
		enum Flags
		{
			kEmpty = 0,
			kDirty = 1 << 0,
		};

		uint32_t Flags;

		DirectX::XMFLOAT3 LocalScale = { 1.0f, 1.0f, 1.0f };
		DirectX::XMFLOAT4 LocalRotation = { 0.0f, 0.0f, 0.0f, 1.0f };
		DirectX::XMFLOAT3 LocalTranslation = { 0.0f, 0.0f, 0.0f };

		DirectX::XMFLOAT4X4 WorldMatrix = cIdentityMatrix;

		inline void SetDirty(bool value = true)
		{
			if (value)
			{
				Flags |= kDirty;
			}
			else
			{
				Flags &= ~kDirty;
			}
		}

		inline bool IsDirty() const { return this->Flags & kDirty; }

		void RotateRollPitchYaw(const DirectX::XMFLOAT3& value)
		{
			SetDirty();

			// This needs to be handled a bit differently
			DirectX::XMVECTOR quat = XMLoadFloat4(&this->LocalRotation);
			DirectX::XMVECTOR x = DirectX::XMQuaternionRotationRollPitchYaw(value.x, 0, 0);
			DirectX::XMVECTOR y = DirectX::XMQuaternionRotationRollPitchYaw(0, value.y, 0);
			DirectX::XMVECTOR z = DirectX::XMQuaternionRotationRollPitchYaw(0, 0, value.z);

			quat = DirectX::XMQuaternionMultiply(x, quat);
			quat = DirectX::XMQuaternionMultiply(quat, y);
			quat = DirectX::XMQuaternionMultiply(z, quat);
			quat = DirectX::XMQuaternionNormalize(quat);

			XMStoreFloat4(&this->LocalRotation, quat);
		}

		DirectX::XMFLOAT3 GetPosition() const
		{
			return *((DirectX::XMFLOAT3*)&this->WorldMatrix._41);
		}

		DirectX::XMFLOAT4 GetRotation() const
		{
			DirectX::XMFLOAT4 rotation;
			DirectX::XMStoreFloat4(&rotation, this->GetRotationV());
			return rotation;
		}

		DirectX::XMFLOAT3 GetScale() const
		{
			DirectX::XMFLOAT3 scale;
			DirectX::XMStoreFloat3(&scale, this->GetScaleV());
			return scale;
		}

		DirectX::XMVECTOR GetPositionV() const
		{
			return DirectX::XMLoadFloat3((DirectX::XMFLOAT3*)&this->WorldMatrix._41);
		}

		DirectX::XMVECTOR GetRotationV() const
		{
			DirectX::XMVECTOR S, R, T;
			DirectX::XMMatrixDecompose(&S, &R, &T, DirectX::XMLoadFloat4x4(&this->WorldMatrix));
			return R;
		}

		DirectX::XMVECTOR GetScaleV() const
		{
			DirectX::XMVECTOR S, R, T;
			DirectX::XMMatrixDecompose(&S, &R, &T, DirectX::XMLoadFloat4x4(&this->WorldMatrix));
			return S;
		}

		// TODO: Move to external functions that operate on the data type.
		inline void UpdateTransform()
		{
			if (this->IsDirty())
			{
				this->SetDirty(false);
				DirectX::XMStoreFloat4x4(&this->WorldMatrix, this->GetLocalMatrix());
			}
		}

		inline void UpdateTransform(TransformComponent const& parent)
		{
			DirectX::XMMATRIX world = this->GetLocalMatrix();
			DirectX::XMMATRIX worldParentworldParent = DirectX::XMLoadFloat4x4(&parent.WorldMatrix);
			world *= worldParentworldParent;

			XMStoreFloat4x4(&WorldMatrix, world);
		}

		inline void ApplyTransform()
		{
			this->SetDirty();

			DirectX::XMVECTOR scalar, rotation, translation;
			DirectX::XMMatrixDecompose(&scalar, &rotation, &translation, DirectX::XMLoadFloat4x4(&this->WorldMatrix));
			DirectX::XMStoreFloat3(&this->LocalScale, scalar);
			DirectX::XMStoreFloat4(&this->LocalRotation, rotation);
			DirectX::XMStoreFloat3(&this->LocalTranslation, translation);
		}

		inline DirectX::XMMATRIX GetLocalMatrix()
		{
			DirectX::XMVECTOR localScale = XMLoadFloat3(&this->LocalScale);
			DirectX::XMVECTOR localRotation = XMLoadFloat4(&this->LocalRotation);
			DirectX::XMVECTOR localTranslation = XMLoadFloat3(&this->LocalTranslation);
			return
				DirectX::XMMatrixScalingFromVector(localScale) *
				DirectX::XMMatrixRotationQuaternion(localRotation) *
				DirectX::XMMatrixTranslationFromVector(localTranslation);
		}

		inline void MatrixTransform(const DirectX::XMFLOAT4X4& matrix)
		{
			this->MatrixTransform(DirectX::XMLoadFloat4x4(&matrix));
		}

		inline void MatrixTransform(const DirectX::XMMATRIX& matrix)
		{
			this->SetDirty();

			DirectX::XMVECTOR scale;
			DirectX::XMVECTOR rotate;
			DirectX::XMVECTOR translate;
			DirectX::XMMatrixDecompose(&scale, &rotate, &translate, this->GetLocalMatrix() * matrix);

			DirectX::XMStoreFloat3(&this->LocalScale, scale);
			DirectX::XMStoreFloat4(&this->LocalRotation, rotate);
			DirectX::XMStoreFloat3(&this->LocalTranslation, translate);
		}
	};

	struct HierarchyComponent
	{
		EntityId ParentID = entt::null;
	};

	struct CameraComponent
	{
		enum Flags
		{
			kEmpty = 0,
			kDirty = 1 << 0,
			kCustomProjection = 1 << 1,
		};

		uint32_t Flags;

		float Width = 0.0f;
		float Height = 0.0f;
		float ZNear = 0.1f;
		float ZFar = 5000.0f;
		float FoV = 1.0f; // Radians

		DirectX::XMFLOAT3 Eye = { 0.0f, 0.0f, 0.0f };
#ifdef LH
		DirectX::XMFLOAT3 Forward = { 0.0f, 0.0f, 1.0f };
#else
		DirectX::XMFLOAT3 Forward = { 0.0f, 0.0f, -1.0f };
#endif
		DirectX::XMFLOAT3 Up = { 0.0f, 1.0f, 0.0f };

		DirectX::XMFLOAT4X4 View;
		DirectX::XMFLOAT4X4 Projection;
		DirectX::XMFLOAT4X4 ViewProjection;

		DirectX::XMFLOAT4X4 ViewInv;
		DirectX::XMFLOAT4X4 ProjectionInv;
		DirectX::XMFLOAT4X4 ViewProjectionInv;

		Core::Frustum FrustumWS;
		Core::Frustum FrustumVS;

		inline DirectX::XMMATRIX GetInvViewProjMatrix() const { return DirectX::XMLoadFloat4x4(&this->ViewProjectionInv); }

		inline void SetDirty(bool value = true)
		{
			if (value)
			{
				Flags |= kDirty;
			}
			else
			{
				Flags &= ~kDirty;
			}
		}
		inline bool IsDirty() const { return this->Flags & kDirty; }

		inline void TransformCamera(TransformComponent const& transform)
		{
			DirectX::XMVECTOR scale, rotation, translation;
			DirectX::XMMatrixDecompose(
				&scale,
				&rotation,
				&translation,
				DirectX::XMLoadFloat4x4(&transform.WorldMatrix));

			DirectX::XMVECTOR eye = translation;
			DirectX::XMVECTOR at = DirectX::XMVectorSet(0, 0, 1, 0);
			DirectX::XMVECTOR up = DirectX::XMVectorSet(0, 1, 0, 0);

			DirectX::XMMATRIX rot = DirectX::XMMatrixRotationQuaternion(rotation);
			at = DirectX::XMVector3TransformNormal(at, rot);
			up = DirectX::XMVector3TransformNormal(up, rot);
			// DirectX::XMStoreFloat3x3(&rotationMatrix, _Rot);

			DirectX::XMStoreFloat3(&this->Eye, eye);
			DirectX::XMStoreFloat3(&this->Forward, at);
			DirectX::XMStoreFloat3(&this->Up, up);
			this->SetDirty();
		}

		inline void UpdateCamera(bool reverseZ = false)
		{
			const float nearZ = reverseZ ? this->ZFar : this->ZNear;
			const float farZ = reverseZ ? this->ZNear : this->ZFar;
#ifdef LH
			auto viewMatrix = DirectX::XMMatrixLookToLH(
				DirectX::XMLoadFloat3(&this->Eye),
				DirectX::XMLoadFloat3(&this->Forward),
				DirectX::XMLoadFloat3(&this->Up));
#else
			auto viewMatrix = DirectX::XMMatrixLookToRH(
				DirectX::XMLoadFloat3(&this->Eye),
				DirectX::XMLoadFloat3(&this->Forward),
				DirectX::XMLoadFloat3(&this->Up));
#endif
			// auto viewMatrix = this->ConstructViewMatrixLH();

			DirectX::XMStoreFloat4x4(&this->View, viewMatrix);
			DirectX::XMStoreFloat4x4(&this->ViewInv, DirectX::XMMatrixInverse(nullptr, viewMatrix));

			float aspectRatio = this->Width / this->Height;
#ifdef LH
			auto projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(this->FoV, aspectRatio, nearZ, farZ);
#else
			auto projectionMatrix = DirectX::XMMatrixPerspectiveFovRH(this->FoV, aspectRatio, nearZ, farZ);
#endif
			DirectX::XMStoreFloat4x4(&this->Projection, projectionMatrix);

			this->UpdateCache();
		}

		inline void UpdateCache()
		{
			DirectX::XMMATRIX viewMatrix = DirectX::XMLoadFloat4x4(&this->View);
			DirectX::XMMATRIX projectionMatrix = DirectX::XMLoadFloat4x4(&this->Projection);

			auto viewProjectionMatrix = viewMatrix * projectionMatrix;
			DirectX::XMStoreFloat4x4(&this->ViewProjection, viewProjectionMatrix);

			DirectX::XMStoreFloat4x4(&this->ProjectionInv, DirectX::XMMatrixInverse(nullptr, projectionMatrix));

			auto viewProjectionInv = DirectX::XMMatrixInverse(nullptr, viewProjectionMatrix);
			DirectX::XMStoreFloat4x4(&this->ViewProjectionInv, viewProjectionInv);

			this->FrustumWS = Core::Frustum(viewProjectionMatrix, false);
			this->FrustumVS = Core::Frustum(projectionMatrix, false);
			DirectX::XMStoreFloat4x4(&this->ViewInv, DirectX::XMMatrixInverse(nullptr, viewMatrix));
		}
	};

	struct MeshRendererComponent
	{
		enum RenderType
		{
			RenderType_Void = 0,
			RenderType_Opaque = 1 << 0,
			RenderType_Transparent = 1 << 1,
			RenderType_CastShadows = 1 << 2,
			RenderType_All = RenderType_Opaque | RenderType_CastShadows
		};
		uint32_t RenderBucketMask = RenderType::RenderType_All;

		Assets::MeshRef Mesh = nullptr;
		DirectX::XMFLOAT4 Color = DirectX::XMFLOAT4(1, 1, 1, 1);
		DirectX::XMFLOAT4 EmissiveColor = DirectX::XMFLOAT4(1, 1, 1, 1);

		size_t GlobalBufferIndex = ~0ull;
	};

	struct LightComponent
	{
		enum Flags
		{
			kEmpty = 0,
			kCastShadow = 1 << 0,
			kEnabled = 1 << 1,
		};

		uint32_t Flags = kEmpty;

		enum LightType : uint32_t
		{
			kDirectionalLight = 0,
			kOmniLight,
			kSpotLight,
			kLightTypeCount
		} Type = kOmniLight;

		DirectX::XMFLOAT4 Colour = { 1.0f, 1.0f, 1.0f, 1.0f };
		// PhxEngine::Graphics::PackerRect ShadowRect;

		// helper data
		DirectX::XMFLOAT3 Direction;
		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT4 Rotation;
		DirectX::XMFLOAT3 Scale;
		uint32_t GlobalBufferIndex = 0;
		// end Helper data

		float Intensity = 10.0f;
		float Range = 60.0f;
		float OuterConeAngle = DirectX::XM_PIDIV4;
		float InnerConeAngle = 0;

	};
}