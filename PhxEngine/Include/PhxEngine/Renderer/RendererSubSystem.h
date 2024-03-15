#pragma once

#include <PhxEngine/Core/Handle.h>
#include <DirectXMath.h>

namespace PhxEngine
{
	struct DrawableDesc
	{
		DirectX::XMFLOAT4X4 TransformMatrix;
	};

	using DrawableHandle = Handle<struct Drawable>;

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
		virtual DrawableHandle DrawableCreate(DrawableDesc const& desc) = 0;
		virtual void DrawableUpdateTransform(DirectX::XMFLOAT4X4 transformMatrix) = 0;
		virtual void DrawableOverrideMaterial(MaterialHandle handle) = 0;
		virtual void DrawableDelete(DrawableHandle handle) = 0;

		virtual void MaterialCreate(MaterialDesc const& desc) = 0;
		virtual void MaterialUpdate(MaterialHandle handle, MaterialDesc const& desc) = 0;
		virtual void MaterialDelete(MaterialHandle handle) = 0;

	};

}

