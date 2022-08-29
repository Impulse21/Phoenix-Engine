#pragma once

#include <string>
#include <DirectXMath.h>

#include <PhxEngine/Core/UUID.h>

namespace PhxEngine::Scene
{
	namespace New
	{
		struct IDComponent
		{
			Core::UUID ID;

			IDComponent() = default;
			IDComponent(const IDComponent&) = default;
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

			// TODO: Move to external functions that operate on the data type.
			inline void UpdateTransform()
			{
				if (this->IsDirty())
				{
					this->SetDirty(false);
					DirectX::XMStoreFloat4x4(&this->WorldMatrix, this->GetLocalMatrix());
				}
			}

			inline void UpdateTransform(New::TransformComponent const& parent)
			{
				DirectX::XMMATRIX world = this->GetLocalMatrix();
				DirectX::XMMATRIX worldParentworldParent = XMLoadFloat4x4(&parent.WorldMatrix);
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

			inline void TransformComponent::MatrixTransform(const DirectX::XMFLOAT4X4& matrix)
			{
				this->MatrixTransform(DirectX::XMLoadFloat4x4(&matrix));
			}

			void TransformComponent::MatrixTransform(const DirectX::XMMATRIX& matrix)
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
			float ZFar = 1000.0f;
			float FoV = 1.0f; // Radians

			DirectX::XMFLOAT3 Eye = { 0.0f, 0.0f, 0.0f };
			DirectX::XMFLOAT3 Forward = { 0.0f, 0.0f, -1.0f };
			DirectX::XMFLOAT3 Up = { 0.0f, 1.0f, 0.0f };

			DirectX::XMFLOAT4X4 View;
			DirectX::XMFLOAT4X4 Projection;
			DirectX::XMFLOAT4X4 ViewProjection;

			DirectX::XMFLOAT4X4 ViewInv;
			DirectX::XMFLOAT4X4 ProjectionInv;
			DirectX::XMFLOAT4X4 ViewProjectionInv;

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
		};

		struct DirectionalLightComponent
		{

			DirectX::XMFLOAT4 Colour = { 1.0f, 1.0f, 1.0f, 1.0f };
		};

		struct OmniLightComponent
		{

			DirectX::XMFLOAT4 Colour = { 1.0f, 1.0f, 1.0f, 1.0f };
		};

		struct SpotLightComponent
		{

			DirectX::XMFLOAT4 Colour = { 1.0f, 1.0f, 1.0f, 1.0f };
		};

		struct SkyLightComponent
		{

			DirectX::XMFLOAT4 Colour = { 1.0f, 1.0f, 1.0f, 1.0f };
		};

		struct StaticMeshComponent
		{
			uint32_t TotalIndices = 0;
			uint32_t TotalVertices = 0;
			// Handle to Mesh Asset
		};
	}
}
