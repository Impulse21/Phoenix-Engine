#pragma once

#include <PhxEngine/Renderer/SceneDataTypes.h>
#include <DirectXMath.h>
#include <list>

namespace PhxEngine::Renderer
{
	class Node
	{
	public:
		virtual ~Node() = default;

		void SetTranslation(DirectX::XMFLOAT3 const& translation);
		void SetRotation(DirectX::XMFLOAT4 const& rotationQuat);
		void SetScale(DirectX::XMFLOAT3 const& scale);

		const DirectX::XMFLOAT3& GetTranslation() const { return this->m_translation; }
		const DirectX::XMFLOAT4& GetRotation() const { return this->m_rotation; }
		const DirectX::XMFLOAT3& GetScale() const { return this->m_scale; }

		const DirectX::XMMATRIX& GetLocalTransform();

		DirectX::XMMATRIX& GetWorldMatrix();
		void InvalidateWorldMatrix();

		Node* GetParentNode() { return this->m_parentNode; }

		void SetParentNode(Node* node);
		void AttachChild(std::shared_ptr<Node> node) { this->m_childiren.emplace_back(node); }
		const std::list<std::shared_ptr<Node>>& GetChildrent() const { return this->m_childiren; }

		void SetName(std::string const& name) { this->m_nodeName = name; }

	private:
		void UpdateWorldMatrix();

	private:
		std::string m_nodeName = "";
		Node* m_parentNode = nullptr;
		std::list<std::shared_ptr<Node>> m_childiren;

		DirectX::XMFLOAT3 m_translation = { 0.0f, 0.0f, 0.0f };
		DirectX::XMFLOAT4 m_rotation = { 1.0f, 0.0f, 0.0f, 0.0f };
		DirectX::XMFLOAT3 m_scale = { 1.0f, 1.0f, 1.0f };

		DirectX::XMMATRIX m_localTransform = DirectX::XMMatrixIdentity();
		DirectX::XMMATRIX m_worldMatrix = DirectX::XMMatrixIdentity();
		bool m_updateWorldMatrix = false;
		bool m_updateLocalTransform = false;
	};

	class LightNode : public Node
	{
	public:
		LightNode() = default;
		virtual ~LightNode() = default;

		void SetColour(DirectX::XMFLOAT3 const& colour) { this->m_colour = colour; }
		const DirectX::XMFLOAT3& GetColour() const { return this->m_colour; }

	protected:
		DirectX::XMFLOAT3 m_colour = { 1.0f, 1.0f, 1.0f };
	};

	class DirectionalLightNode final : public LightNode
	{
	public:
		DirectionalLightNode() = default;
		~DirectionalLightNode() = default;

		void SetDirection(DirectX::XMFLOAT3 const& direction) { this->m_direction = direction; }
		const DirectX::XMFLOAT3& GetDirection() const { return this->m_direction; }

		void SetIrradiance(float irradiance) { this->m_irradiance = irradiance; }
		float GetIrradiance() const { return this->m_irradiance; }

	private:
		float m_irradiance = 1.0f;
		DirectX::XMFLOAT3 m_direction = { 0.0f, 0.0f, -1.0f}; 
	};

	class CameraNode : public Node
	{
	public:
		virtual ~CameraNode() = default;

		virtual const DirectX::XMMATRIX& GetProjectionMatrix() = 0;
	};

	class PerspectiveCameraNode final : public CameraNode
	{
	public:
		PerspectiveCameraNode() = default;
		~PerspectiveCameraNode() = default;

		void SetZNear(float value) { this->m_zNear = value; this->m_isDirty = true; }
		float GetZNear() const { return this->m_zNear; }

		void SetZFar(float value) { this->m_zFar = value; this->m_isDirty = true; }
		float GetZFar() const { return this->m_zFar; }

		void SetAspectRatio(float value) { this->m_aspectRatio = value; this->m_isDirty = true; }
		float GetAspectRatio() const { return this->m_aspectRatio; }

		void SetYFov(float value) { this->m_yFov = value; this->m_isDirty = true; }
		float GetYFov() const { return this->m_yFov; }

		const DirectX::XMMATRIX& GetProjectionMatrix() override;

	private:
		float m_zNear = 1.f;
		float m_zFar = 1000.0f;
		float m_aspectRatio = 1.7f;
		float m_yFov = 1.f; // in radians

		bool m_isDirty = false;
		DirectX::XMMATRIX m_projectionMatrix = DirectX::XMMatrixIdentity();
	};

	class MeshInstanceNode : public Node
	{
	public:
		MeshInstanceNode() = default;
		virtual ~MeshInstanceNode() = default;

		void SetMeshData(std::shared_ptr<Mesh> value) { this->m_meshes = value; }
		std::shared_ptr<Mesh> GetMeshData() const { return this->m_meshes; }

		void SetInstanceIndex(size_t index) { this->m_instanceIndex = index; }
		size_t GetInstanceIndex() const { return this->m_instanceIndex; }

	private:
		std::shared_ptr<Mesh> m_meshes;
		uint32_t m_instanceIndex = ~0u;
	};

	class SceneForwardIterator
	{
	public:
		SceneForwardIterator() = default;
		SceneForwardIterator(Node* current)
			: m_current(current)
			, m_scope(current)
		{}

		[[nodiscard]] operator bool() const { return this->m_current != nullptr; }
		Node* operator->() const { return this->m_current; }

		[[nodiscard]] Node* Get() const { return this->m_current; }

		int Next(bool allowChildren);
		int Up();

	private:
		Node* m_current;
		Node* m_scope;
	};
}