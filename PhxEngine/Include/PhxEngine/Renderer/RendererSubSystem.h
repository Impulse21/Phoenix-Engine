#pragma once

#include <PhxEngine/Core/Handle.h>
#include <DirectXMath.h>

namespace PhxEngine
{
	struct DrawableInstanceDesc
	{
		StringHash DrawableId;
		DirectX::XMFLOAT4X4 TransformMatrix;
	};

	using DrawableInstanceHandle = Handle<struct DrawableInstance>;

	struct MaterialDesc
	{

	};

	using MaterialHandle = Handle<struct Material>;

	class RendererSubSystem
	{
	public:
		inline static RendererSubSystem* Ptr = nullptr;

	public:
		virtual void Startup() = 0;
		virtual void Shutdown() = 0;
		virtual void Update() = 0;
		virtual void Render() = 0;

	public:
		virtual DrawableInstanceHandle DrawableInstanceCreate(DrawableInstanceDesc const& desc) = 0;
		virtual void DrawableInstanceUpdateTransform(DirectX::XMFLOAT4X4 transformMatrix) = 0;
		virtual void DrawableInstanceOverrideMaterial(MaterialHandle handle) = 0;
		virtual void DrawableInstanceDelete(DrawableInstanceHandle handle) = 0;

		virtual void MaterialCreate(MaterialDesc const& desc) = 0;
		virtual void MaterialUpdate(MaterialHandle handle, MaterialDesc const& desc) = 0;
		virtual void MaterialDelete(MaterialHandle handle) = 0;
	};

}

