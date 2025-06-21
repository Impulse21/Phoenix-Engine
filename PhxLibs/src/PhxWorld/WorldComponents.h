#pragma once

#include <PhxCore/Base.h>
#include <PhxCore/UUID.h>
#include <PhxCore/Math.h>

#include <PhxResource/Resource.h>

#include <entt/entt.hpp>

#include <string>

#include <DirectXMath.h>
#include <hlsl++.h>


namespace phx
{
	struct IDComponent
	{
		UUID ID;
	};

	struct NameComponent
	{
		std::string Name;

		inline void operator=(const std::string& str) { Name = str; }
		inline void operator=(std::string&& str) { Name = std::move(str); }
		inline bool operator==(const std::string& str) const { return Name.compare(str) == 0; }
	};

	struct HierarchyComponent
	{
		entt::entity ParentID = entt::null;
	};

	struct CameraComponent
	{
		float Width = 0.0f;
		float Height = 0.0f;
		float ZNear = 0.1f;
		float ZFar = 5000.0f;
		float FoV = 1.0f; // Radians

		hlslpp::float3 Eye = { 0.0f, 0.0f, 0.0f };
#ifdef LH
		DirectX::XMFLOAT3 Forward = { 0.0f, 0.0f, 1.0f };
#else
		hlslpp::float3 Forward = { 0.0f, 0.0f, -1.0f };
#endif
		hlslpp::float3 Up = { 0.0f, 1.0f, 0.0f };

		bool Active : 1 = false;
	};

	struct MeshComponent
	{
		phx::RefCountPtr<Resource> Mesh;
		
		static void Reflect()
		{
			using namespace entt;
			entt::meta<MeshComponent>()
				.data<&MeshComponent::Mesh>("Mesh"_hs);
		}
	};

	struct TransformComponent
	{
		enum Flags
		{
			kEmpty = 0,
			kDirty = BIT(1),
		};

		static void Reflect()
		{
			using namespace entt;
			entt::meta<TransformComponent>()
				.data<&TransformComponent::Scale>("Scale"_hs)
				.data<&TransformComponent::Scale>("Rotation"_hs)
				.data<&TransformComponent::Scale>("Translation"_hs);
		}

		uint32_t Flags = kEmpty;

		DirectX::XMFLOAT3 Scale = { 1.0f, 1.0f, 1.0f };
		DirectX::XMFLOAT4 Rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
		DirectX::XMFLOAT3 Translation = { 0.0f, 0.0f, 0.0f };

		DirectX::XMFLOAT4X4 WorldMatrix = {};

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

		inline bool IsDirty() const { return Flags & kDirty; }
		
		DirectX::XMFLOAT3 GetPosition() const
		{
			return *((DirectX::XMFLOAT3*)&WorldMatrix._41);
		}

		DirectX::XMFLOAT4 GetRotation() const
		{
			DirectX::XMFLOAT4 rotation;
			DirectX::XMStoreFloat4(&rotation, GetRotationV());
			return rotation;
		}

		DirectX::XMFLOAT3 GetScale() const
		{
			DirectX::XMFLOAT3 scale;
			DirectX::XMStoreFloat3(&scale, GetScaleV());
			return scale;
		}

		DirectX::XMVECTOR GetPositionV() const
		{
			return DirectX::XMLoadFloat3((DirectX::XMFLOAT3*)&WorldMatrix._41);
		}

		DirectX::XMVECTOR GetRotationV() const
		{
			DirectX::XMVECTOR S, R, T;
			XMMatrixDecompose(&S, &R, &T, DirectX::XMLoadFloat4x4(&WorldMatrix));
			return R;
		}

		DirectX::XMVECTOR GetScaleV() const
		{
			DirectX::XMVECTOR S, R, T;
			XMMatrixDecompose(&S, &R, &T, DirectX::XMLoadFloat4x4(&WorldMatrix));
			return S;
		}

		// TODO: Move to external functions that operate on the data type.
		inline void UpdateTransform()
		{
			if (IsDirty())
			{
				SetDirty(false);
				DirectX::XMStoreFloat4x4(&WorldMatrix, GetMatrix());
			}
		}

		inline void UpdateTransform(TransformComponent const& parent)
		{
			DirectX::XMMATRIX world = GetMatrix();
			DirectX::XMMATRIX worldParentworldParent = DirectX::XMLoadFloat4x4(&parent.WorldMatrix);
			world *= worldParentworldParent;

			DirectX::XMStoreFloat4x4(&WorldMatrix, world);
		}

		inline void ApplyTransform()
		{
			SetDirty();

			DirectX::XMVECTOR scalar, rotation, translation;
			DirectX::XMMatrixDecompose(&scalar, &rotation, &translation, DirectX::XMLoadFloat4x4(&WorldMatrix));
			DirectX::XMStoreFloat3(&Scale, scalar);
			DirectX::XMStoreFloat4(&Rotation, rotation);
			DirectX::XMStoreFloat3(&Translation, translation);
		}

		inline DirectX::XMMATRIX GetMatrix()
		{
			DirectX::XMVECTOR ScaleV = DirectX::XMLoadFloat3(&Scale);
			DirectX::XMVECTOR RotationV = DirectX::XMLoadFloat4(&Rotation);
			DirectX::XMVECTOR TranslationV = DirectX::XMLoadFloat3(&Translation);
			return
				DirectX::XMMatrixScalingFromVector(ScaleV) *
				DirectX::XMMatrixRotationQuaternion(RotationV) *
				DirectX::XMMatrixTranslationFromVector(TranslationV);
		}

		inline void MatrixTransform(const DirectX::XMFLOAT4X4& matrix)
		{
			MatrixTransform(DirectX::XMLoadFloat4x4(&matrix));
		}

		inline void MatrixTransform(const DirectX::XMMATRIX& matrix)
		{
			SetDirty();

			DirectX::XMVECTOR scale;
			DirectX::XMVECTOR rotate;
			DirectX::XMVECTOR translate;
			DirectX::XMMatrixDecompose(&scale, &rotate, &translate, GetMatrix() * matrix);

			DirectX::XMStoreFloat3(&Scale, scale);
			DirectX::XMStoreFloat4(&Rotation, rotate);
			DirectX::XMStoreFloat3(&Translation, translate);
		}
	};


	// Not sure about these yet
	struct MeshResourceComponent
	{

	};

	struct MaterialComponent
	{

	};
}