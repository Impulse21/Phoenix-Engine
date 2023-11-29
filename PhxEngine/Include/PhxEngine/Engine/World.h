#pragma once

#include <PhxEngine/Core/Object.h>
#include <PhxEngine/Core/VirtualFileSystem.h>

#include <DirectXMath.h>
#include <memory>
namespace PhxEngine
{

	class Node : public Core::Object, public std::enable_shared_from_this<Node>
	{
	public:
		Node() = default;
		virtual ~Node() = default;

		const DirectX::XMFLOAT3& GetPostion() const { return this->m_position; };
		const DirectX::XMFLOAT4& GetRotation() const { return this->m_rotation; };
		const DirectX::XMFLOAT3& GetScale() const { return this->m_scale; };

	protected:
		std::weak_ptr<Node> m_parent;
		std::shared_ptr<Node> m_children;

		DirectX::XMFLOAT3 m_position;
		DirectX::XMFLOAT4 m_rotation;
		DirectX::XMFLOAT3 m_scale;
	};

	class MeshInstanceNode : public Node
	{
	public:

	private:

	};

	class World
	{
	public:
		World() = default;
		~World() = default; 

	public:
		void AttachRoot(std::shared_ptr<Node> root);

	private:
		std::shared_ptr<Node> RootNode;
	};

	class IWorldLoader
	{
	public:
		virtual ~IWorldLoader() = default;
		
		virtual bool LoadWorld(std::string const& filename, Core::IFileSystem* fileSystem, World& outWorld) = 0;
	};

}
