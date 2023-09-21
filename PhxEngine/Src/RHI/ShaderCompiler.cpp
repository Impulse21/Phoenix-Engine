#include <PhxEngine/RHI/PhxShaderCompiler.h>

#include <dxcapi.h>
using namespace PhxEngine::RHI;

namespace
{
	DxcCreateInstance2Proc m_dxcCreateInstance = nullptr;
	void Initialize()
	{

	}
}

void ShaderCompiler::Compile(std::string const& filename)
{
	if (!m_dxcCreateInstance)
	{
		Initialize();
	}
	// TODO
}
