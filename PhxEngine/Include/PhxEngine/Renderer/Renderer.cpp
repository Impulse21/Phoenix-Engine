#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include "Renderer.h"

using namespace PhxEngine;
using namespace PhxEngine::Renderer;

using namespace DirectX;

void PhxEngine::Renderer::CreateCubemapCameras(DirectX::XMFLOAT3 position, float nearZ, float farZ, Core::Span<RenderCam> cameras)
{
	assert(cameras.Size() == 6);
	cameras[0] = RenderCam(position, XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f), nearZ, farZ, XM_PIDIV2); // -z
	cameras[1] = RenderCam(position, XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f), farZ, farZ, XM_PIDIV2); // +z
	cameras[2] = RenderCam(position, XMFLOAT4(0, 0.707f, 0, 0.707f), nearZ, farZ, XM_PIDIV2); // -x
	cameras[3] = RenderCam(position, XMFLOAT4(0, -0.707f, 0, 0.707f), nearZ, farZ, XM_PIDIV2); // +x
	cameras[4] = RenderCam(position, XMFLOAT4(0.707f, 0, 0, 0.707f), nearZ, farZ, XM_PIDIV2); // -y
	cameras[5] = RenderCam(position, XMFLOAT4(-0.707f, 0, 0, 0.707f), nearZ, farZ, XM_PIDIV2); // +y
}
