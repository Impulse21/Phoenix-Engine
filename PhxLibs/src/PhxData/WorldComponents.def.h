#pragma once

#include <PhxCore/Base.h>
#include <PhxCore/UUID.h>
#include <PhxCore/Math.h>
#include <DirectXMath.h>

#include <entt/entt.hpp>

#define REFLECT_CLASS(T) \
	
#define PROPERTY(...)

namespace phx
{
	struct IDComponent
	{
		PROPERTY()
		UUID ID;
	};

	struct NameComponent
	{
		PROPERTY()
		std::string Name;

		inline void operator=(const std::string& str) { Name = str; }
		inline void operator=(std::string&& str) { Name = std::move(str); }
		inline bool operator==(const std::string& str) const { return Name.compare(str) == 0; }
	};

	struct HierarchyComponent
	{
		entt::entity ParentID = entt::null;
	};

	struct TransformComponent
	{
		enum Flags
		{
			kEmpty = 0,
			kDirty = BIT(1),
		};

		uint32_t Flags;

        PROPERTY(name="Scale", tooltip="Local Scale")
		DirectX::XMFLOAT3 LocalScale = { 1.0f, 1.0f, 1.0f };
        
        PROPERTY(name="Rotation", tooltip="Local Rotation")
		DirectX::XMFLOAT4 LocalRotation = { 0.0f, 0.0f, 0.0f, 1.0f };
        
        PROPERTY(name="Translation", tooltip="Local Translation")
		DirectX::XMFLOAT3 LocalTranslation = { 0.0f, 0.0f, 0.0f };

		DirectX::XMFLOAT4X4 WorldMatrix = math::cIdentityMatrix;


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
			XMStoreFloat4(&rotation, GetRotationV());
			return rotation;
		}

		DirectX::XMFLOAT3 GetScale() const
		{
			DirectX::XMFLOAT3 scale;
			XMStoreFloat3(&scale, GetScaleV());
			return scale;
		}

		DirectX::XMVECTOR GetPositionV() const
		{
			return XMLoadFloat3((DirectX::XMFLOAT3*)&WorldMatrix._41);
		}

		DirectX::XMVECTOR GetRotationV() const
		{
			DirectX::XMVECTOR S, R, T;
			XMMatrixDecompose(&S, &R, &T, XMLoadFloat4x4(&WorldMatrix));
			return R;
		}

		DirectX::XMVECTOR GetScaleV() const
		{
			DirectX::XMVECTOR S, R, T;
			XMMatrixDecompose(&S, &R, &T, XMLoadFloat4x4(&WorldMatrix));
			return S;
		}

		// TODO: Move to external functions that operate on the data type.
		inline void UpdateTransform()
		{
			if (IsDirty())
			{
				SetDirty(false);
				DirectX::XMStoreFloat4x4(&WorldMatrix, GetLocalMatrix());
			}
		}

		inline void UpdateTransform(TransformComponent const& parent)
		{
			DirectX::XMMATRIX world = GetLocalMatrix();
			DirectX::XMMATRIX worldParentworldParent = XMLoadFloat4x4(&parent.WorldMatrix);
			world *= worldParentworldParent;

			XMStoreFloat4x4(&WorldMatrix, world);
		}

		inline void ApplyTransform()
		{
			SetDirty();

			DirectX::XMVECTOR scalar, rotation, translation;
			DirectX::XMMatrixDecompose(&scalar, &rotation, &translation, DirectX::XMLoadFloat4x4(&WorldMatrix));
			DirectX::XMStoreFloat3(&LocalScale, scalar);
			DirectX::XMStoreFloat4(&LocalRotation, rotation);
			DirectX::XMStoreFloat3(&LocalTranslation, translation);
		}

		inline DirectX::XMMATRIX GetLocalMatrix()
		{
			DirectX::XMVECTOR localScale = XMLoadFloat3(&LocalScale);
			DirectX::XMVECTOR localRotation = XMLoadFloat4(&LocalRotation);
			DirectX::XMVECTOR localTranslation = XMLoadFloat3(&LocalTranslation);
			return
				DirectX::XMMatrixScalingFromVector(localScale) *
				DirectX::XMMatrixRotationQuaternion(localRotation) *
				DirectX::XMMatrixTranslationFromVector(localTranslation);
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
			DirectX::XMMatrixDecompose(&scale, &rotate, &translate, GetLocalMatrix() * matrix);

			DirectX::XMStoreFloat3(&LocalScale, scale);
			DirectX::XMStoreFloat4(&LocalRotation, rotate);
			DirectX::XMStoreFloat3(&LocalTranslation, translate);
		}
    };
}