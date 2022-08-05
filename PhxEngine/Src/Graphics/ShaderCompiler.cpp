#include "phxpch.h"
#include "Graphics/ShaderCompiler.h"

#include <wrl.h>
#include "Core/Helpers.h"
#include <dxc/dxcapi.h>
#include "Graphics/RHI/Dx12/Common.h"

using namespace PhxEngine::RHI;
using namespace PhxEngine::RHI::Dx12;
using namespace PhxEngine::Core;

namespace
{
	std::wstring GetStagePrefix(ShaderStage stage)
	{
		switch (stage)
		{
		case ShaderStage::Vertex:
			return L"vs_";
		case ShaderStage::Pixel:
			return L"ps_";
		case ShaderStage::Compute:
			return L"cs_";
		default:
			throw std::runtime_error("Unsupported Shader Stage");
		}
	}

	std::wstring GetShaderModelPostfix(ShaderModel model)
	{
		switch (model)
		{
		case ShaderModel::SM_6_0:
			return L"6_0";
			break;
		case ShaderModel::SM_6_1:
			return L"6_1";
			break;
		case ShaderModel::SM_6_2:
			return L"6_2";
			break;
		case ShaderModel::SM_6_3:
			return L"6_3";
			break;
		case ShaderModel::SM_6_4:
			return L"6_4";
			break;
		case ShaderModel::SM_6_5:
			return L"6_5";
			break;
		case ShaderModel::SM_6_6:
			return L"6_6";
			break;
		case ShaderModel::SM_6_7:
			return L"6_7";
			break;
		default:
			throw std::runtime_error("Unsupported Shader Model");
		}
	}

	std::wstring ConstructProfileString(ShaderStage stage, ShaderModel shaderModel)
	{
		return GetStagePrefix(stage) + GetShaderModelPostfix(shaderModel);
	}
}

ShaderCompiler::CompileResult ShaderCompiler::CompileShader(
	std::string const& shaderSourceFile,
	CompileOptions const& compileOptions)
{
	Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils;
	ThrowIfFailed(
		DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils)));

	Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler;
	ThrowIfFailed(
		DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler)));

	Microsoft::WRL::ComPtr<IDxcIncludeHandler> defaultIncludeHandler;
	ThrowIfFailed(
		dxcUtils->CreateDefaultIncludeHandler(&defaultIncludeHandler));

	// https://developer.nvidia.com/dx12-dos-and-donts
	std::vector<LPCWSTR> arguments = {
		// Use the /all_resources_bound / D3DCOMPILE_ALL_RESOURCES_BOUND compile flag if possible
		// This allows for the compiler to do a better job at optimizing texture accesses. We have
		// seen frame rate improvements of > 1 % when toggling this flag on.
		L"-all_resources_bound",
		DXC_ARG_WARNINGS_ARE_ERRORS, // L"-WX"
		DXC_ARG_DEBUG, // L"-Zi"
		L"-Fd",
		// PdbPath.c_str(), // Shader Pdb
#ifdef _DEBUG
		L"-Od", // Disable optimization
#else
		L"-O3", // Optimization level 3
#endif
		L"-Zss", // Compute shader hash based on source
	};

	std::wstring wStrSourceFile;
	Helpers::StringConvert(shaderSourceFile, wStrSourceFile);

	std::wstring entryPointwStr;
	Helpers::StringConvert(compileOptions.EntryPoint, entryPointwStr);

	std::vector<std::wstring> wstrings;
	wstrings.reserve(compileOptions.Defines.size() + compileOptions.IncludeDirectories.size());

	std::vector<DxcDefine> dxcDefines;
	dxcDefines.reserve(compileOptions.Defines.size());
	for (const auto& define : compileOptions.Defines)
	{
		auto& wStr = wstrings.emplace_back();
		Helpers::StringConvert(define, wStr);

		DxcDefine& define = dxcDefines.emplace_back();
		define.Name = wStr.c_str();
		define.Value = nullptr;
	}

	for (const auto& includeDir : compileOptions.IncludeDirectories)
	{
		auto& wStr = wstrings.emplace_back();
		Helpers::StringConvert(includeDir, wStr);
		arguments.push_back(L"-I");
		arguments.push_back(wStr.c_str());
	}

	Microsoft::WRL::ComPtr<IDxcCompilerArgs> dxcCompilerArgs;
	ThrowIfFailed(
		dxcUtils->BuildArguments(
			wStrSourceFile.c_str(),
			entryPointwStr.c_str(),
			ConstructProfileString(compileOptions.Stage, compileOptions.Model).c_str(),
			arguments.data(),
			static_cast<UINT32>(arguments.size()),
			dxcDefines.data(),
			static_cast<UINT32>(dxcDefines.size()),
			&dxcCompilerArgs));


	Microsoft::WRL::ComPtr<IDxcBlobEncoding> dxcBlobSource;
	ThrowIfFailed(
		dxcUtils->LoadFile(wStrSourceFile.c_str(), nullptr, &dxcBlobSource));

	DxcBuffer dxcSourceBuffer = {};
	dxcSourceBuffer.Ptr = dxcBlobSource->GetBufferPointer();
	dxcSourceBuffer.Size = dxcBlobSource->GetBufferSize();
	dxcSourceBuffer.Encoding = DXC_CP_ACP;

	Microsoft::WRL::ComPtr<IDxcResult> pResults;
	auto hr = dxcCompiler->Compile(
		&dxcSourceBuffer,						// Source buffer.
		dxcCompilerArgs->GetArguments(),   // Array of pointers to arguments.
		dxcCompilerArgs->GetCount(),			// Number of arguments.
		defaultIncludeHandler.Get(),   // &includehandler,		// User-provided interface to handle #include directives (optional).
		IID_PPV_ARGS(&pResults) // Compiler output status, buffer, and errors.
	);

	assert(SUCCEEDED(hr));

	ShaderCompiler::CompileResult retVal = {};

	Microsoft::WRL::ComPtr<IDxcBlobUtf8> pErrors = nullptr;
	hr = pResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr);
	assert(SUCCEEDED(hr));
	if (pErrors != nullptr && pErrors->GetStringLength() != 0)
	{
		retVal.ErrorMsg = pErrors->GetStringPointer();
	}

	retVal.Successful = true;
	HRESULT hrStatus;
	hr = pResults->GetStatus(&hrStatus);
	assert(SUCCEEDED(hr));
	if (FAILED(hrStatus))
	{
		retVal.Successful = false;
		return retVal;
	}

	Microsoft::WRL::ComPtr<IDxcBlob> pShader = nullptr;
	hr = pResults->GetOutput(
		DXC_OUT_OBJECT,
		IID_PPV_ARGS(&pShader),
		nullptr);

	assert(SUCCEEDED(hr));
	if (pShader != nullptr)
	{
		retVal.Dependencies.push_back(shaderSourceFile);
		retVal.ShaderData = (const uint8_t*)pShader->GetBufferPointer();
		retVal.ShaderSize = pShader->GetBufferSize();

		// keep the blob alive == keep shader pointer valid!
		auto internalState = std::make_shared<Microsoft::WRL::ComPtr<IDxcBlob>>();
		*internalState = pShader;
		retVal._InternalData = internalState;
	}

	if (pResults->HasOutput(DXC_OUT_PDB))
	{
		Microsoft::WRL::ComPtr<IDxcBlob> pPdb = nullptr;
		hr = pResults->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(&pPdb), nullptr);
		assert(SUCCEEDED(hr));
		if (pPdb != nullptr)
		{
			retVal.ShaderPDB.resize(pPdb->GetBufferSize());
			std::memcpy(retVal.ShaderPDB.data(), pPdb->GetBufferPointer(), pPdb->GetBufferSize());
		}
	}

	if (compileOptions.Type == RHI::ShaderType::HLSL6 && pResults->HasOutput(DXC_OUT_SHADER_HASH))
	{
		Microsoft::WRL::ComPtr<IDxcBlob> pHash = nullptr;
		hr = pResults->GetOutput(DXC_OUT_SHADER_HASH, IID_PPV_ARGS(&pHash), nullptr);
		assert(SUCCEEDED(hr));
		if (pHash != nullptr)
		{
			DxcShaderHash* pHashBuf = (DxcShaderHash*)pHash->GetBufferPointer();
			retVal.ShaderHash.reserve(pHash->GetBufferSize());
			for (int i = 0; i < _countof(pHashBuf->HashDigest); i++)
			{
				retVal.ShaderHash.push_back(pHashBuf->HashDigest[i]);
			}
		}
	}

	return retVal;
}
