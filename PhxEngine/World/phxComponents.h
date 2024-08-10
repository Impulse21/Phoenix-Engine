#pragma once

#include "Core/phxUUID.h"
#include "Core/phxStringHash.h"
#include "Core/phxMath.h"

#include "entt/entt.hpp"

namespace phx
{

	struct IDComponent
	{
		UUID ID;
	};

	struct NameComponent
	{
		std::string Name;
		StringHash Hash;

		inline void operator=(const std::string& str) { this->Name = str; this->Hash = str.c_str(); }
		inline void operator=(std::string&& str) { this->Name = std::move(str); this->Hash = this->Name.c_str();}
		inline bool operator==(const std::string& str) const { return this->Name.compare(str) == 0; }

	};

	struct HierarchyComponent
	{
		entt::entity ParentID = entt::null;
	};


	struct TransformComponent
	{
		union
		{
			struct
			{
				bool IsDirty : 8;
			};
			uint8_t Flags;
		};

		DirectX::XMFLOAT3 LocalScale = { 1.0f, 1.0f, 1.0f };
		DirectX::XMFLOAT4 LocalRotation = { 0.0f, 0.0f, 0.0f, 1.0f };
		DirectX::XMFLOAT3 LocalTranslation = { 0.0f, 0.0f, 0.0f };

		DirectX::XMFLOAT4X4 WorldMatrix = math::cIdentityMatrix;

		inline void UpdateTransform()
		{
			if (this->IsDirty)
			{
				this->IsDirty = false;
				DirectX::XMStoreFloat4x4(&this->WorldMatrix, this->GetLocalMatrix());
			}
		}

		inline void UpdateTransform(TransformComponent const& parent)
		{
			DirectX::XMMATRIX world = this->GetLocalMatrix();
			DirectX::XMMATRIX worldParentworldParent = XMLoadFloat4x4(&parent.WorldMatrix);
			world *= worldParentworldParent;

			XMStoreFloat4x4(&WorldMatrix, world);
		}

		inline void ApplyTransform()
		{
			IsDirty = true;

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
			this->IsDirty = true;

			DirectX::XMVECTOR scale;
			DirectX::XMVECTOR rotate;
			DirectX::XMVECTOR translate;
			DirectX::XMMatrixDecompose(&scale, &rotate, &translate, this->GetLocalMatrix() * matrix);

			DirectX::XMStoreFloat3(&this->LocalScale, scale);
			DirectX::XMStoreFloat4(&this->LocalRotation, rotate);
			DirectX::XMStoreFloat3(&this->LocalTranslation, translate);
		}
	};
}