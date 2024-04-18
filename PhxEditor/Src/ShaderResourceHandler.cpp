#include "ShaderResourceHandler.h"

#include <PhxEngine/RHI/PhxShaderCompiler.h>

using namespace PhxEngine;

PhxEngine::RefCountPtr<PhxEngine::Resource> PhxEditor::ShaderResourceHandler::Load(std::unique_ptr<PhxEngine::IBlob>&& fileData)
{
	// Compile the shader if we haven't already Compiled it.

	RHI::ShaderCompiler::CompilerInput input = {};
	auto output = PhxEngine::RHI::ShaderCompiler::Compile(input);

	if (!output.ErrorMessage.empty())
	{
		// Report errors

	}

	// Create TODO: I am here.
	return nullptr;
}

void PhxEditor::ShaderResourceHandler::LoadDispatch(std::string const& path)
{
}
