#pragma once

#include <string>
#include <array>

#include <DirectXMath.h>

#include "Systems/Ecs.h"

#include "Graphics/Enums.h"
#include "Graphics/RHI/PhxRHI.h"

#include <Shaders/ShaderInteropStructures.h>

namespace PhxEngine::Scene
{
	// TODO: Move to a math library
	static constexpr DirectX::XMFLOAT4X4 cIdentityMatrix = DirectX::XMFLOAT4X4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

	// TODO: Find a home for this
	constexpr uint32_t AlignTo(uint32_t value, uint32_t alignment)
	{
		return ((value + alignment - 1) / alignment) * alignment;
	}

	constexpr uint64_t AlignTo(uint64_t value, uint64_t alignment)
	{
		return ((value + alignment - 1) / alignment) * alignment;
	}

	struct NameComponent
	{
		std::string Name;

		inline void operator=(const std::string& str) { Name = str; }
		inline void operator=(std::string&& str) { Name = std::move(str); }
		inline bool operator==(const std::string& str) const { return Name.compare(str) == 0; }

	};

	using NameComponentStore = ECS::ComponentStore<NameComponent>;

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

		DirectX::XMFLOAT3 GetPosition() const;
		DirectX::XMFLOAT4 GetRotation() const;
		DirectX::XMFLOAT3 GetScale() const;
		DirectX::XMVECTOR GetPositionV() const;
		DirectX::XMVECTOR GetRotationV() const;
		DirectX::XMVECTOR GetScaleV() const;

		// Computes the local space matrix from scale, rotation, translation and returns it
		DirectX::XMMATRIX GetLocalMatrix() const;

		// Applies the local space to the world space matrix. This overwrites world matrix
		void UpdateTransform();

		// Apply a parent transform relative to the local space. This overwrites world matrix
		void UpdateTransform(const TransformComponent& parent);

		// Apply the world matrix to the local space. This overwrites scale, rotation, translation
		void ApplyTransform();

		// Clears the local space. This overwrites scale, rotation, translation
		void ClearTransform();

		void Translate(DirectX::XMFLOAT3 const& value);
		void Translate(DirectX::XMVECTOR const&  value);
		void RotateRollPitchYaw(DirectX::XMFLOAT3 const&  value);
		void Rotate(DirectX::XMFLOAT4 const& quaternion);
		void Rotate(DirectX::XMVECTOR const&  quaternion);
		void Scale(DirectX::XMFLOAT3 const&  value);
		void Scale(DirectX::XMVECTOR const&  value);
		void MatrixTransform(DirectX::XMFLOAT4X4 const& matrix);
		void MatrixTransform(DirectX::XMMATRIX const& matrix);

	};

	using TransformComponentStore = ECS::ComponentStore<TransformComponent>;

	struct HierarchyComponent
	{
		ECS::Entity ParentId = ECS::InvalidEntity;
	};

	using HierarchyComponentStore = ECS::ComponentStore<HierarchyComponent>;

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

		void TransformCamera(TransformComponent const& transform);
		void UpdateCamera();

		// -- Custom View Matrix construction ---
		DirectX::XMMATRIX ConstructViewMatrixLH();
		DirectX::XMMATRIX ConstructViewMatrixRH();

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

	using CameraComponentStore = ECS::ComponentStore<CameraComponent>;

	struct LightComponent
	{
		enum Flags
		{
			kEmpty = 0,
			kCastShadow = 1 << 0,
			kEnabled = 1 << 1,
		};

		uint32_t Flags;

		DirectX::XMFLOAT4 Colour = { 1.0f, 1.0f, 1.0f, 1.0f};

		enum LightType : uint32_t
		{
			kDirectionalLight = Shader::ENTITY_TYPE_DIRECTIONALLIGHT,
			kOmniLight = Shader::ENTITY_TYPE_OMNILIGHT,
			kSpotLight = Shader::ENTITY_TYPE_SPOTLIGHT,
			kLightTypeCount
		} Type = kOmniLight;

		float Energy = 1.0f;
		float Range = 10.0f;
		float FoV = DirectX::XM_PIDIV4;

		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT3 Direction;
		DirectX::XMFLOAT3 LocalRotation;
		DirectX::XMFLOAT3 LocalScale;
		DirectX::XMFLOAT3 Front;
		DirectX::XMFLOAT3 Right;

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

	using LightComponentStore = ECS::ComponentStore<LightComponent>;

	struct MaterialComponent
	{
		enum Flags
		{
			kEmpty = 0,
			kCastShadow = 1 << 0,
		};

		uint32_t Flags;

		enum ShaderType : uint8_t
		{
			kPbr= 0,
			kUnlit = 1,
			kEmissive = 2,
			kNumShaderTypes
		} ShaderType = kPbr;

		PhxEngine::Graphics::BlendMode BlendMode = Graphics::BlendMode::Opaque;
		DirectX::XMFLOAT4 Albedo = { 0.0f, 0.0f, 0.0f, 1.0f };
		DirectX::XMFLOAT4 Emissive = { 0.0f, 0.0f, 0.0f, 1.0f };
		float Metalness = 1.0f;
		float Roughness = 1.0f;
		float Ao = 0.4f;
		bool IsDoubleSided = false;

		RHI::TextureHandle AlbedoTexture;
		RHI::TextureHandle MetalRoughnessTexture;
		RHI::TextureHandle AoTexture;
		RHI::TextureHandle NormalMapTexture;

		void PopulateShaderData(Shader::MaterialData& shaderData);
		uint32_t GetRenderTypes();
	};

	using MaterialComponentStore = ECS::ComponentStore<MaterialComponent>;

	struct MeshComponent
	{
		enum Flags
		{
			kEmpty = 0,
			kContainsNormals = 1 << 0,
			kContainsTexCoords = 1 << 1,
			kContainsTangents = 1 << 2,
		};

		uint32_t Flags;

		std::vector<DirectX::XMFLOAT3> VertexPositions;
		std::vector<DirectX::XMFLOAT2> VertexTexCoords;
		std::vector<DirectX::XMFLOAT3> VertexNormals;
		std::vector<DirectX::XMFLOAT4> VertexTangents;
		std::vector< DirectX::XMFLOAT3> VertexColour;
		std::vector<uint32_t> Indices;

		RHI::BufferHandle VertexGpuBuffer;
		RHI::BufferHandle IndexGpuBuffer;

		// -- TODO: Remove ---
		uint32_t IndexOffset = 0;
		uint32_t VertexOffset = 0;
		// -- TODO: End Remove ---

		uint32_t TotalIndices = 0;
		uint32_t TotalVertices = 0;

		struct MeshGeometry
		{
			ECS::Entity MaterialID = ECS::InvalidEntity;
			uint32_t IndexOffsetInMesh = 0;
			uint32_t VertexOffsetInMesh = 0;
			uint32_t NumVertices = 0;
			uint32_t NumIndices = 0;

			uint32_t GlobalGeometryIndex = ~0U;
		};

		std::vector<MeshGeometry> Geometry;

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

		[[nodiscard]] bool HasVertexAttribuite(VertexAttribute attr) const { return this->BufferRanges[(int)attr].SizeInBytes != 0; }
		RHI::BufferRange& GetVertexAttribute(VertexAttribute attr) { return this->BufferRanges[(int)attr]; }
		[[nodiscard]] const RHI::BufferRange& GetVertexAttribute(VertexAttribute attr) const { return this->BufferRanges[(int)attr]; }

		// Render Data

		// GPU Data functions
		void CreateRenderData(RHI::IGraphicsDevice* grahicsDevice, RHI::CommandListHandle commandList);

		// Utility Functions
		void ReverseWinding();
		void ComputeTangentSpace();

		bool IsTriMesh() const { return (this->Indices.size() % 3) == 0; }
	};

	using MeshComponentStore = ECS::ComponentStore<MeshComponent>;

	struct MeshInstanceComponent
	{
		enum Flags
		{
			kEmpty = 0,
			kCastShadow = 1 << 0,
		};

		uint32_t Flags;

		ECS::Entity MeshId = ECS::InvalidEntity;
		uint32_t RenderBucketMask = 0;
	};

	using MeshInstanceComponentStore = ECS::ComponentStore<MeshInstanceComponent>;
}