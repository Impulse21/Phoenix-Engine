#pragma once

#include "phx/core/UUID.h"
#include "phx/core/Math.h"

#include <string>
#include <DirectXMath.h>

namespace phx
{
	struct IDComponent
	{
		UUID ID;
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

		inline bool IsDirty() const { return this->Flags & kDirty; }

		void RotateRollPitchYaw(const DirectX::XMFLOAT3& value)
		{
			SetDirty();

			// This needs to be handled a bit differently
			DirectX::XMVECTOR quat = DirectX::XMLoadFloat4(&this->LocalRotation);
			DirectX::XMVECTOR x = DirectX::XMQuaternionRotationRollPitchYaw(value.x, 0, 0);
			DirectX::XMVECTOR y = DirectX::XMQuaternionRotationRollPitchYaw(0, value.y, 0);
			DirectX::XMVECTOR z = DirectX::XMQuaternionRotationRollPitchYaw(0, 0, value.z);

			quat = DirectX::XMQuaternionMultiply(x, quat);
			quat = DirectX::XMQuaternionMultiply(quat, y);
			quat = DirectX::XMQuaternionMultiply(z, quat);
			quat = DirectX::XMQuaternionNormalize(quat);

			DirectX::XMStoreFloat4(&this->LocalRotation, quat);
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

			DirectX::XMStoreFloat4x4(&WorldMatrix, world);
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
			DirectX::XMVECTOR localScale = DirectX::XMLoadFloat3(&this->LocalScale);
			DirectX::XMVECTOR localRotation = DirectX::XMLoadFloat4(&this->LocalRotation);
			DirectX::XMVECTOR localTranslation = DirectX::XMLoadFloat3(&this->LocalTranslation);
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
		entt::entity ParentID = entt::null;
	};

}