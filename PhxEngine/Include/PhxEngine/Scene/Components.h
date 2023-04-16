#pragma once

#include <string>
#include <DirectXMath.h>
#include "entt.hpp"
#include <PhxEngine/Core/UUID.h>
#include <PhxEngine/Core/Helpers.h>
#include <PhxEngine/Scene/Assets.h>
#include <PhxEngine/Core/Primitives.h>
#include <PhxEngine/Shaders/ShaderInteropStructures_NEW.h>
#include <DirectXMesh.h>
#include <PhxEngine/Graphics/RectPacker.h>

#define LH

namespace PhxEngine::Core
{
	class IAllocator;
}

// Required for Operators - Move to CPP please
using namespace DirectX;
namespace PhxEngine::Scene
{
	// TODO: Move to a math library
	static constexpr DirectX::XMFLOAT4X4 cIdentityMatrix = DirectX::XMFLOAT4X4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

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

		void RotateRollPitchYaw(const XMFLOAT3& value)
		{
			SetDirty();

			// This needs to be handled a bit differently
			XMVECTOR quat = XMLoadFloat4(&this->LocalRotation);
			XMVECTOR x = XMQuaternionRotationRollPitchYaw(value.x, 0, 0);
			XMVECTOR y = XMQuaternionRotationRollPitchYaw(0, value.y, 0);
			XMVECTOR z = XMQuaternionRotationRollPitchYaw(0, 0, value.z);

			quat = XMQuaternionMultiply(x, quat);
			quat = XMQuaternionMultiply(quat, y);
			quat = XMQuaternionMultiply(z, quat);
			quat = XMQuaternionNormalize(quat);

			XMStoreFloat4(&this->LocalRotation, quat);
		}

		DirectX::XMFLOAT3 GetPosition() const
		{
			return *((XMFLOAT3*)&this->WorldMatrix._41);
		}

		DirectX::XMFLOAT4 GetRotation() const
		{
			XMFLOAT4 rotation;
			XMStoreFloat4(&rotation, this->GetRotationV());
			return rotation;
		}

		DirectX::XMFLOAT3 GetScale() const
		{
			XMFLOAT3 scale;
			XMStoreFloat3(&scale, this->GetScaleV());
			return scale;
		}

		DirectX::XMVECTOR GetPositionV() const
		{
			return XMLoadFloat3((XMFLOAT3*)&this->WorldMatrix._41);
		}

		DirectX::XMVECTOR GetRotationV() const
		{
			XMVECTOR S, R, T;
			XMMatrixDecompose(&S, &R, &T, XMLoadFloat4x4(&this->WorldMatrix));
			return R;
		}

		DirectX::XMVECTOR GetScaleV() const
		{
			XMVECTOR S, R, T;
			XMMatrixDecompose(&S, &R, &T, XMLoadFloat4x4(&this->WorldMatrix));
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
			kDirectionalLight = Shader::ENTITY_TYPE_DIRECTIONALLIGHT,
			kOmniLight = Shader::ENTITY_TYPE_OMNILIGHT,
			kSpotLight = Shader::ENTITY_TYPE_SPOTLIGHT,
			kLightTypeCount
		} Type = kOmniLight;

		DirectX::XMFLOAT4 Colour = { 1.0f, 1.0f, 1.0f, 1.0f };
		PhxEngine::Graphics::PackerRect ShadowRect;

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

		// TODO: Remove
		float FoV = DirectX::XM_PIDIV4;
		// End TODO

		bool CastShadows() const { return Flags & Flags::kCastShadow; }
		inline void SetCastShadows(bool value = true)
		{
			if (value)
			{
				Flags |= kCastShadow;
			}
			else
			{
				Flags &= ~kCastShadow;
			}
		}

		bool IsEnabled() const { return Flags & Flags::kEnabled; }

		inline void SetEnabled(bool value = true)
		{
			if (value)
			{
				Flags |= kEnabled;
			}
			else
			{
				Flags &= ~kEnabled;
			}
		}
	};

	struct SkyLightComponent
	{

		DirectX::XMFLOAT4 Colour = { 1.0f, 1.0f, 1.0f, 1.0f };
	};

	struct WorldEnvironmentComponent
	{
		enum class IndirectLightingMode : uint8_t
		{
			GeneratedEnvMap = 0,
			IBL,
		} IndirectLightingMode = IndirectLightingMode::GeneratedEnvMap;

		enum SkyType
		{
			Simple = 0,
		};

		DirectX::XMFLOAT3 ZenithColour = { 0.39, 0.57, 1.0 };
		DirectX::XMFLOAT3 HorizonColour = { 0.0f, 0.0f, 0.0f };
		DirectX::XMFLOAT3 AmbientColour = { 0.0, 0.0, 0.0 };


		enum IBLTextures
		{
			IrradanceMap,
			PreFilteredEnvMap,
			EnvMap,
			NumIblTextures
		};
		std::array<std::shared_ptr<Assets::Texture>, NumIblTextures> IblTextures;

	};

	struct EnvProbeComponent
	{
		int textureIndex = -1;
		DirectX::XMFLOAT3 Position = {};
		float Range;
		DirectX::XMFLOAT4X4 InverseMatrix = {};

	};


	struct MeshComponent
	{
		enum RenderType
		{
			RenderType_Void = 0,
			RenderType_Opaque = 1 << 0,
			RenderType_Transparent = 1 << 1,
			RenderType_All = RenderType_Opaque | RenderType_Transparent
		};

		enum Flags : uint8_t
		{
			kEmpty = 0,
			kContainsNormals = 1 << 0,
			kContainsTexCoords = 1 << 1,
			kContainsTangents = 1 << 2,
			kContainsColour = 1 << 3,
		};

		uint32_t Flags = kEmpty;

		// -- CPU Data ---
		entt::entity Material;

		Core::AABB Aabb;
		Core::Sphere BoundingSphere;

		uint32_t TotalVertices = 0;
		DirectX::XMFLOAT3* Positions = nullptr;
		DirectX::XMFLOAT2* TexCoords = nullptr;
		DirectX::XMFLOAT3* Normals = nullptr;
		DirectX::XMFLOAT4* Tangents = nullptr;
		DirectX::XMFLOAT3* Colour = nullptr;

		uint32_t TotalIndices = 0;
		uint32_t* Indices = nullptr;
		
		std::vector<DirectX::Meshlet> Meshlets;
		std::vector<uint8_t> UniqueVertexIB;
		std::vector<DirectX::MeshletTriangle> MeshletTriangles;

		Shader::New::MeshletPackedVertexData* PackedVertexData = nullptr;
		DirectX::CullData* MeshletCullData = nullptr;

		// -- GPU Data --
		size_t GlobalByteOffsetVertexBuffer;
		size_t GlobalOffsetIndexBuffer;

		size_t GlobalOffsetMeshletBuffer;
		size_t GlobalOffsetUnqiueVertexIBBuffer;
		size_t GlobalOffsetMeshletPrimitiveBuffer;
		size_t GlobalOffsetMeshletCullDataBuffer;

		size_t GlobalIndexOffsetGeometryBuffer;

		RHI::BufferHandle IndexBuffer;
		RHI::BufferHandle VertexBuffer;
		RHI::BufferHandle MeshletBuffer;
		RHI::BufferHandle UniqueVertexIBBuffer;
		RHI::BufferHandle MeshletPrimitivesBuffer;
		RHI::BufferHandle MeshletCullDataBuffer;

		// TODO: Is this needed or is there a better way to do this.
		RHI::BufferHandle PackedVertexDataBuffer;

		enum class VertexAttribute : uint8_t
		{
			Position = 0,
			TexCoord,
			Normal,
			Tangent,
			Colour,
			Count,
		};

		std::array<RHI::BufferRange, (int)VertexAttribute::Count> BufferRanges;

		// -- Helper Functions ---
		[[nodiscard]] bool HasVertexAttribuite(VertexAttribute attr) const { return this->BufferRanges[(int)attr].SizeInBytes != 0; }
		RHI::BufferRange& GetVertexAttribute(VertexAttribute attr) { return this->BufferRanges[(int)attr]; }
		[[nodiscard]] const RHI::BufferRange& GetVertexAttribute(VertexAttribute attr) const { return this->BufferRanges[(int)attr]; }

		void BuildRenderData(
			Core::IAllocator* allocator,
			RHI::IGraphicsDevice* gfxDevice);


		// Helpers
		void InitializeCpuBuffers(Core::IAllocator* allocator);
		void ReverseWinding();
		void ComputeTangentSpace();
		void FlipZ();
		void ComputeBounds();
		void ComputeMeshletData(Core::IAllocator* allocator);
	};

	struct MeshInstanceComponent
	{
		enum RenderType
		{
			RenderType_Void = 0,
			RenderType_Opaque = 1 << 0,
			RenderType_Transparent = 1 << 1,
			RenderType_All = RenderType_Opaque | RenderType_Transparent
		};
		uint32_t RenderBucketMask = RenderType::RenderType_Opaque;

		entt::entity Mesh = entt::null;
		DirectX::XMFLOAT4 Color = DirectX::XMFLOAT4(1, 1, 1, 1);
		DirectX::XMFLOAT4 EmissiveColor = DirectX::XMFLOAT4(1, 1, 1, 1);

		size_t GlobalBufferIndex = ~0ull;
	};

	struct AABBComponent
	{
		Core::AABB BoundingData;

		 operator Core::AABB&() { return this->BoundingData; }
	};

	struct MaterialComponent
	{
		PhxEngine::Renderer::BlendMode BlendMode = Renderer::BlendMode::Opaque;
		DirectX::XMFLOAT4 BaseColour = { 0.0f, 0.0f, 0.0f, 1.0f };
		DirectX::XMFLOAT4 Emissive = { 0.0f, 0.0f, 0.0f, 1.0f };

		float Metalness = 1.0f;
		float Roughness = 1.0f;
		float Ao = 1.0f;
		bool IsDoubleSided = false;

		std::shared_ptr<Assets::Texture> BaseColourTexture;
		std::shared_ptr<Assets::Texture> MetalRoughnessTexture;
		std::shared_ptr<Assets::Texture> AoTexture;
		std::shared_ptr<Assets::Texture> NormalMapTexture;

		size_t GlobalBufferIndex = ~0ull;
	};

}
