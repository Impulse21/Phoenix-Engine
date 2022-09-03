#pragma once

#include <string>
#include <DirectXMath.h>
#include "entt.hpp"
#include <PhxEngine/Core/UUID.h>
#include <PhxEngine/Core/RefPtr.h>
#include <PhxEngine/Core/Helpers.h>
#include <PhxEngine/Scene/Assets.h>

// Required for Operators - Move to CPP please
using namespace DirectX;
namespace PhxEngine::Scene
{
	namespace New
	{
		// TODO: Move to a math library
		static constexpr DirectX::XMFLOAT4X4 cIdentityMatrix = DirectX::XMFLOAT4X4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

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

			inline void TransformCamera(TransformComponent const& transform)
			{
				DirectX::XMVECTOR scale, rotation, translation;
				DirectX::XMMatrixDecompose(
					&scale,
					&rotation,
					&translation,
					DirectX::XMLoadFloat4x4(&transform.WorldMatrix));

				DirectX::XMVECTOR eye = translation;
				DirectX::XMVECTOR at = DirectX::XMVectorSet(0, 0, -1, 0);
				DirectX::XMVECTOR up = DirectX::XMVectorSet(0, 1, 0, 0);

				DirectX::XMMATRIX rot = DirectX::XMMatrixRotationQuaternion(rotation);
				at = DirectX::XMVector3TransformNormal(at, rot);
				up = DirectX::XMVector3TransformNormal(up, rot);
				// DirectX::XMStoreFloat3x3(&rotationMatrix, _Rot);

				DirectX::XMStoreFloat3(&this->Eye, eye);
				DirectX::XMStoreFloat3(&this->Forward, at);
				DirectX::XMStoreFloat3(&this->Up, up);
			}

			inline void UpdateCamera()
			{
				auto viewMatrix = this->ConstructViewMatrixRH();
				// auto viewMatrix = this->ConstructViewMatrixLH();

				DirectX::XMStoreFloat4x4(&this->View, viewMatrix);
				DirectX::XMStoreFloat4x4(&this->ViewInv, DirectX::XMMatrixInverse(nullptr, viewMatrix));

				auto projectionMatrix = DirectX::XMMatrixPerspectiveFovRH(this->FoV, 1.7f, this->ZNear, this->ZFar);
				// auto projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(this->FoV, 1.7f, this->ZNear, this->ZFar);

				DirectX::XMStoreFloat4x4(&this->Projection, projectionMatrix);
				DirectX::XMStoreFloat4x4(&this->ProjectionInv, DirectX::XMMatrixInverse(nullptr, projectionMatrix));

				auto viewProjectionMatrix = viewMatrix * projectionMatrix;

				DirectX::XMStoreFloat4x4(&this->ViewProjection, viewProjectionMatrix);

				DirectX::XMStoreFloat4x4(&this->ViewProjectionInv, DirectX::XMMatrixInverse(nullptr, viewProjectionMatrix));
			}

			inline DirectX::XMMATRIX ConstructViewMatrixLH()
			{
				const DirectX::XMVECTOR forward = -DirectX::XMLoadFloat3(&this->Forward);
				const DirectX::XMVECTOR axisZ = DirectX::XMVector3Normalize(forward);

				// axisX == right vector
				const DirectX::XMVECTOR up = DirectX::XMLoadFloat3(&this->Up);
				const DirectX::XMVECTOR axisX = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(up, forward));

				// Axisy == Up vector ( forward cross with right)
				const DirectX::XMVECTOR axisY = DirectX::XMVector3Cross(axisZ, axisX);

				// --- Construct View matrix ---
				const DirectX::XMVECTOR negEye = DirectX::XMVectorNegate(DirectX::XMLoadFloat3(&this->Eye));

				// Not sure I get this bit.
				const DirectX::XMVECTOR d0 = DirectX::XMVector3Dot(axisX, negEye);
				const DirectX::XMVECTOR d1 = DirectX::XMVector3Dot(axisY, negEye);
				const DirectX::XMVECTOR d2 = DirectX::XMVector3Dot(axisZ, negEye);

				// Construct column major view matrix;
				DirectX::XMMATRIX m;
				m.r[0] = DirectX::XMVectorSelect(d0, axisX, g_XMSelect1110.v);
				m.r[1] = DirectX::XMVectorSelect(d1, axisY, g_XMSelect1110.v);
				m.r[2] = DirectX::XMVectorSelect(d2, axisZ, g_XMSelect1110.v);
				m.r[3] = g_XMIdentityR3.v;

				return DirectX::XMMatrixTranspose(m);
			}

			inline DirectX::XMMATRIX ConstructViewMatrixRH()
			{
				const DirectX::XMVECTOR forward = -DirectX::XMLoadFloat3(&this->Forward);
				const DirectX::XMVECTOR axisZ = DirectX::XMVector3Normalize(forward);

				// axisX == right vector
				const DirectX::XMVECTOR up = DirectX::XMLoadFloat3(&this->Up);
				const DirectX::XMVECTOR axisX = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(up, forward));

				// Axisy == Up vector ( forward cross with right)
				const DirectX::XMVECTOR axisY = DirectX::XMVector3Cross(axisZ, axisX);

				// --- Construct View matrix ---
				const DirectX::XMVECTOR negEye = DirectX::XMVectorNegate(DirectX::XMLoadFloat3(&this->Eye));

				// Not sure I get this bit.
				const DirectX::XMVECTOR d0 = DirectX::XMVector3Dot(axisX, negEye);
				const DirectX::XMVECTOR d1 = DirectX::XMVector3Dot(axisY, negEye);
				const DirectX::XMVECTOR d2 = DirectX::XMVector3Dot(axisZ, negEye);

				// Construct column major view matrix;
				DirectX::XMMATRIX m;
				m.r[0] = DirectX::XMVectorSelect(d0, axisX, g_XMSelect1110.v);
				m.r[1] = DirectX::XMVectorSelect(d1, axisY, g_XMSelect1110.v);
				m.r[2] = DirectX::XMVectorSelect(d2, axisZ, g_XMSelect1110.v);
				m.r[3] = g_XMIdentityR3.v;

				return DirectX::XMMatrixTranspose(m);
			}
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

		struct MeshRenderComponent
		{
			enum RenderType
			{
				RenderType_Void = 0,
				RenderType_Opaque = 1 << 0,
				RenderType_Transparent = 1 << 1,
				RenderType_All = RenderType_Opaque | RenderType_Transparent
			};

			std::shared_ptr<Assets::Mesh> Mesh;
			uint32_t RenderBucketMask = RenderType::RenderType_Opaque;
		};
	}
}
