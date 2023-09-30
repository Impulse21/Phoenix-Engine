#include <PhxEngine/RHI/PhxShaderCompiler.h>
#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/String.h>

#include <wrl.h>
#include <dxc/dxcapi.h>
using namespace PhxEngine::Core;
using namespace PhxEngine::RHI;
using namespace Microsoft::WRL;
namespace
{
	struct IncludeHandler : public IDxcIncludeHandler
	{
		const ShaderCompiler::CompilerInput* Input = nullptr;
		ShaderCompiler::CompilerResult* Result = nullptr;
		Microsoft::WRL::ComPtr<IDxcIncludeHandler> dxcIncludeHandler;

		HRESULT STDMETHODCALLTYPE LoadSource(
			_In_z_ LPCWSTR pFilename,                                 // Candidate filename.
			_COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource  // Resultant source object for included file, nullptr if not found.
		) override
		{
			HRESULT hr = dxcIncludeHandler->LoadSource(pFilename, ppIncludeSource);
			if (SUCCEEDED(hr))
			{
				std::string& filename = Result->Dependencies.emplace_back();
				StringConvert(pFilename, filename);
			}
			return hr;
		}
		HRESULT STDMETHODCALLTYPE QueryInterface(
			/* [in] */ REFIID riid,
			/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override
		{
			return dxcIncludeHandler->QueryInterface(riid, ppvObject);
		}

		ULONG STDMETHODCALLTYPE AddRef(void) override
		{
			return 0;
		}
		ULONG STDMETHODCALLTYPE Release(void) override
		{
			return 0;
		}
	};
}

ShaderCompiler::CompilerResult ShaderCompiler::Compile(CompilerInput const& input)
{
	CompilerResult result;

	ComPtr<IDxcUtils> dxcUtils;
	assert(
		SUCCEEDED(
			DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(dxcUtils.GetAddressOf()))
		));

	ComPtr<IDxcCompiler3> dxcCompiler;
	assert(
		SUCCEEDED(
			DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(dxcCompiler.GetAddressOf()))
		));

	if (dxcCompiler == nullptr)
	{
		return result;
	}


	ComPtr<IDxcBlobEncoding> pSource;

	IFileSystem* fileSystem = input.FileSystem;
	assert(fileSystem);

	if (!fileSystem->FileExists(input.Filename))
	{
		return result;
	}

	std::unique_ptr<IBlob> shaderSrcData = fileSystem->ReadFile(input.Filename);
	// pUtils->CreateBlob(pShaderSource, shaderSourceSize, CP_UTF8, pSource.GetAddressOf());

	// https://github.com/microsoft/DirectXShaderCompiler/wiki/Using-dxc.exe-and-dxcompiler.dll#dxcompiler-dll-interface
	std::vector<LPCWSTR> args = {
		DXC_ARG_DEBUG,
		L"-res-may-alias",
		L"-flegacy-macro-expansion",
		L"-no-legacy-cbuf-layout",
		L"-pack-optimized", // this has problem with tessellation shaders: https://github.com/microsoft/DirectXShaderCompiler/issues/3362
		L"-all-resources-bound",
		L"-Gis", // Force IEEE strictness
		L"-Gec", // Enable backward compatibility mode
		L"-Ges", // Enable strict mode
		L"-O0", // Optimization Level 0
	};

	if (input.Flags & CompilerFlags::DisableOptimization)
	{
		args.push_back(L"-Od");
	}

	if (input.Flags & CompilerFlags::EmbedDebug)
	{
		args.push_back(L"/Qembed_debug");
	}

	if (input.Flags & CompilerFlags::EmbedDebug)
	{
	}

	switch (input.ShaderType)
	{
	case ShaderType::HLSL6:
		args.push_back(L"-D"); args.push_back(L"HLSL6");
		args.push_back(L"-rootsig-define"); args.push_back(L"PHX_ENGINE_DEFAULT_ROOTSIGNATURE");
		break;
	default:
		assert(0);
		return result;
	}

	args.push_back(L"-T");
	switch (input.ShaderStage)
	{
	case ShaderStage::Mesh:
		switch (input.ShaderModel)
		{
		default:
			args.push_back(L"ms_6_5");
			break;
		case ShaderModel::SM_6_6:
			args.push_back(L"ms_6_6");
			break;
		case ShaderModel::SM_6_7:
			args.push_back(L"ms_6_7");
			break;
		}
		break;
	case ShaderStage::Amplification:
		switch (input.ShaderModel)
		{
		default:
			args.push_back(L"as_6_5");
			break;
		case ShaderModel::SM_6_6:
			args.push_back(L"as_6_6");
			break;
		case ShaderModel::SM_6_7:
			args.push_back(L"as_6_7");
			break;
		}
		break;
	case ShaderStage::Vertex:
		switch (input.ShaderModel)
		{
		default:
			args.push_back(L"vs_6_0");
			break;
		case ShaderModel::SM_6_1:
			args.push_back(L"vs_6_1");
			break;
		case ShaderModel::SM_6_2:
			args.push_back(L"vs_6_2");
			break;
		case ShaderModel::SM_6_3:
			args.push_back(L"vs_6_3");
			break;
		case ShaderModel::SM_6_4:
			args.push_back(L"vs_6_4");
			break;
		case ShaderModel::SM_6_5:
			args.push_back(L"vs_6_5");
			break;
		case ShaderModel::SM_6_6:
			args.push_back(L"vs_6_6");
			break;
		case ShaderModel::SM_6_7:
			args.push_back(L"vs_6_7");
			break;
		}
		break;
	case ShaderStage::Hull:
		switch (input.ShaderModel)
		{
		default:
			args.push_back(L"hs_6_0");
			break;
		case ShaderModel::SM_6_1:
			args.push_back(L"hs_6_1");
			break;
		case ShaderModel::SM_6_2:
			args.push_back(L"hs_6_2");
			break;
		case ShaderModel::SM_6_3:
			args.push_back(L"hs_6_3");
			break;
		case ShaderModel::SM_6_4:
			args.push_back(L"hs_6_4");
			break;
		case ShaderModel::SM_6_5:
			args.push_back(L"hs_6_5");
			break;
		case ShaderModel::SM_6_6:
			args.push_back(L"hs_6_6");
			break;
		case ShaderModel::SM_6_7:
			args.push_back(L"hs_6_7");
			break;
		}
		break;
	case ShaderStage::Domain:
		switch (input.ShaderModel)
		{
		default:
			args.push_back(L"ds_6_0");
			break;
		case ShaderModel::SM_6_1:
			args.push_back(L"ds_6_1");
			break;
		case ShaderModel::SM_6_2:
			args.push_back(L"ds_6_2");
			break;
		case ShaderModel::SM_6_3:
			args.push_back(L"ds_6_3");
			break;
		case ShaderModel::SM_6_4:
			args.push_back(L"ds_6_4");
			break;
		case ShaderModel::SM_6_5:
			args.push_back(L"ds_6_5");
			break;
		case ShaderModel::SM_6_6:
			args.push_back(L"ds_6_6");
			break;
		case ShaderModel::SM_6_7:
			args.push_back(L"ds_6_7");
			break;
		}
		break;
	case ShaderStage::Geometry:
		switch (input.ShaderModel)
		{
		default:
			args.push_back(L"gs_6_0");
			break;
		case ShaderModel::SM_6_1:
			args.push_back(L"gs_6_1");
			break;
		case ShaderModel::SM_6_2:
			args.push_back(L"gs_6_2");
			break;
		case ShaderModel::SM_6_3:
			args.push_back(L"gs_6_3");
			break;
		case ShaderModel::SM_6_4:
			args.push_back(L"gs_6_4");
			break;
		case ShaderModel::SM_6_5:
			args.push_back(L"gs_6_5");
			break;
		case ShaderModel::SM_6_6:
			args.push_back(L"gs_6_6");
			break;
		case ShaderModel::SM_6_7:
			args.push_back(L"gs_6_7");
			break;
		}
		break;
	case ShaderStage::Pixel:
		switch (input.ShaderModel)
		{
		default:
			args.push_back(L"ps_6_0");
			break;
		case ShaderModel::SM_6_1:
			args.push_back(L"ps_6_1");
			break;
		case ShaderModel::SM_6_2:
			args.push_back(L"ps_6_2");
			break;
		case ShaderModel::SM_6_3:
			args.push_back(L"ps_6_3");
			break;
		case ShaderModel::SM_6_4:
			args.push_back(L"ps_6_4");
			break;
		case ShaderModel::SM_6_5:
			args.push_back(L"ps_6_5");
			break;
		case ShaderModel::SM_6_6:
			args.push_back(L"ps_6_6");
			break;
		case ShaderModel::SM_6_7:
			args.push_back(L"ps_6_7");
			break;
		}
		break;
	case ShaderStage::Compute:
		switch (input.ShaderModel)
		{
		default:
			args.push_back(L"cs_6_0");
			break;
		case ShaderModel::SM_6_1:
			args.push_back(L"cs_6_1");
			break;
		case ShaderModel::SM_6_2:
			args.push_back(L"cs_6_2");
			break;
		case ShaderModel::SM_6_3:
			args.push_back(L"cs_6_3");
			break;
		case ShaderModel::SM_6_4:
			args.push_back(L"cs_6_4");
			break;
		case ShaderModel::SM_6_5:
			args.push_back(L"cs_6_5");
			break;
		case ShaderModel::SM_6_6:
			args.push_back(L"cs_6_6");
			break;
		case ShaderModel::SM_6_7:
			args.push_back(L"cs_6_7");
			break;
		}
		break;
	default:
		assert(0);
		return result;
	}

	std::vector<std::wstring> wstrings;
	wstrings.reserve(input.Defines.Size() + input.IncludeDirs.Size());

	for (auto& x : input.Defines)
	{
		std::wstring& wstr = wstrings.emplace_back();
		StringConvert(x, wstr);
		args.push_back(L"-D");
		args.push_back(wstr.c_str());
	}

	for (auto& x : input.IncludeDirs)
	{
		std::wstring& wstr = wstrings.emplace_back();
		StringConvert(x, wstr);
		args.push_back(L"-I");
		args.push_back(wstr.c_str());
	}

	// Entry point parameter:
	std::wstring wentry;
	StringConvert(input.EntryPoint, wentry);
	args.push_back(L"-E");
	args.push_back(wentry.c_str());

	// Add source file name as last parameter. This will be displayed in error messages
	std::wstring wsource;
	std::filesystem::path filenamePath = input.Filename;
	StringConvert(filenamePath.filename().string(), wsource);
	args.push_back(wsource.c_str());


	DxcBuffer Source;
	Source.Ptr = shaderSrcData->Data();
	Source.Size = shaderSrcData->Size();
	Source.Encoding = DXC_CP_ACP;

	IncludeHandler includehandler = {};
	includehandler.Input = &input;
	includehandler.Result = &result;

	assert(SUCCEEDED(
		dxcUtils->CreateDefaultIncludeHandler(&includehandler.dxcIncludeHandler)));

	Microsoft::WRL::ComPtr<IDxcResult> pResults;
	assert(SUCCEEDED(
		dxcCompiler->Compile(
			&Source,                // Source buffer.
			args.data(),            // Array of pointers to arguments.
			(uint32_t)args.size(),	// Number of arguments.
			&includehandler,		// User-provided interface to handle #include directives (optional).
			IID_PPV_ARGS(&pResults) // Compiler output status, buffer, and errors.
	)));

	Microsoft::WRL::ComPtr<IDxcBlobUtf8> pErrors = nullptr;
	assert(SUCCEEDED(
		pResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr)));

	if (pErrors != nullptr && pErrors->GetStringLength() != 0)
	{
		result.ErrorMessage = pErrors->GetStringPointer();
	}

	HRESULT hrStatus;
	assert(SUCCEEDED(
		pResults->GetStatus(&hrStatus)));

	if (FAILED(hrStatus))
	{
		return result;
	}

	Microsoft::WRL::ComPtr<IDxcBlob> pShader = nullptr;
	assert(SUCCEEDED(
		pResults->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pShader), nullptr)));
	if (pShader != nullptr)
	{
		result.Dependencies.push_back(input.Filename);
		result.ShaderData = Core::Span((const uint8_t*)pShader->GetBufferPointer(), pShader->GetBufferSize());

		// keep the blob alive == keep shader pointer valid!
		auto internalState = std::make_shared<Microsoft::WRL::ComPtr<IDxcBlob>>();
		*internalState = pShader;
		result.InternalResource = internalState;
	}

	Microsoft::WRL::ComPtr<IDxcBlob> pShaderSymbols = nullptr;
	assert(SUCCEEDED(
		pResults->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(&pShaderSymbols), nullptr)));
	if (pShaderSymbols != nullptr)
	{
		result.ShaderSymbols = Core::Span((const uint8_t*)pShaderSymbols->GetBufferPointer(), pShaderSymbols->GetBufferSize());

		// keep the blob alive == keep shader pointer valid!
		auto internalState = std::make_shared<Microsoft::WRL::ComPtr<IDxcBlob>>();
		*internalState = pShaderSymbols;
		result.InternalResourceSymbols = internalState;
	}

	if (input.ShaderType == ShaderType::HLSL6)
	{
		Microsoft::WRL::ComPtr<IDxcBlob> pHash = nullptr;
		assert(SUCCEEDED(
			pResults->GetOutput(DXC_OUT_SHADER_HASH, IID_PPV_ARGS(&pHash), nullptr)));
		if (pHash != nullptr)
		{
			DxcShaderHash* pHashBuf = (DxcShaderHash*)pHash->GetBufferPointer();
			for (int i = 0; i < _countof(pHashBuf->HashDigest); i++)
			{
				result.ShaderHash.push_back(pHashBuf->HashDigest[i]);
			}
		}
	}

	return result;
}
