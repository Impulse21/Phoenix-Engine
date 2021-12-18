#pragma once

#include <PhxEngine/Renderer/SceneDataTypes.h>
#include <DirectXMath.h>

namespace PhxEngine::Renderer
{
	class Node
	{
	public:
		virtual ~Node() = default;

	private:
		DirectX::XMFLOAT3 m_translation = { 0.0f, 0.0f, 0.0f };
		DirectX::XMFLOAT4 m_rotation = { 1.0f, 0.0f, 0.0f, 0.0f };
		DirectX::XMFLOAT3 m_scale = { 1.0f, 1.0f, 1.0f };

		DirectX::XMMATRIX m_worldMatrix = DirectX::XMMatrixIdentity();
		bool updateWorldMatrix = false;

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
		CameraNode() = default;
		virtual ~CameraNode() = default;

	};

	class PerspectiveCameraNode final : public CameraNode
	{
	public:
		PerspectiveCameraNode() = default;
		~PerspectiveCameraNode() = default;


		void SetZNear(float value) { this->m_zNear = value; }
		float GetZNear() const { return this->m_zNear; }

		void SetZFar(float value) { this->m_zFar = value; }
		float GetZFar() const { return this->m_zFar; }

		void SetAspectRatio(float value) { this->m_aspectRatio = value; }
		float GetAspectRatio() const { return this->m_aspectRatio; }

		void SetYFov(float value) { this->m_yFov = value; }
		float GetYFov() const { return this->m_yFov; }

	private:
		float m_zNear = 1.f;
		float m_zFar = 1000.0f;
		float m_aspectRatio = 1.7f;
		float m_yFov = 1.f; // in radians
	};

	class MeshInstanceNode : public Node
	{
	public:
		MeshInstanceNode() = default;
		virtual ~MeshInstanceNode() = default;

		void SetMeshData(std::shared_ptr<Mesh> value) { this->m_mesh = value; }
		std::shared_ptr<Mesh> GetMeshData() const { return this->m_mesh; }

	private:
		std::shared_ptr<Mesh> m_mesh;
	};
}