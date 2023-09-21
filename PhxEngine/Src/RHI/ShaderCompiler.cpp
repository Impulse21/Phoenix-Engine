#include <PhxEngine/RHI/PhxShaderCompiler.h>
#include <PhxEngine/Core/Log.h>

#include <wrl.h>
#include <dxc/dxcapi.h>
using namespace PhxEngine::Core;
using namespace PhxEngine::RHI;
using namespace Microsoft::WRL;
namespace
{
	DxcCreateInstance2Proc m_dxcCreateInstance = nullptr;
}

void ShaderCompiler::Compile(std::string const& filename, PhxEngine::Core::IFileSystem* fileSystem)
{
	ComPtr<IDxcUtils> dxcUtils;

	ComPtr<IDxcUtils> pUtils;
	assert(
		SUCCEEDED(
			DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(pUtils.GetAddressOf()))
		));

	ComPtr<IDxcCompiler3> dxcCompiler;
	assert(
		SUCCEEDED(
			DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(pUtils.GetAddressOf()))
		));

	if (dxcCompiler == nullptr)
	{
		return;
	}


	ComPtr<IDxcBlobEncoding> pSource;

	if (!fileSystem->FileExists(filename))
	{
		return;
	}

	std::unique_ptr<IBlob> shaderSrcData= fileSystem->ReadFile(filename);
	// pUtils->CreateBlob(pShaderSource, shaderSourceSize, CP_UTF8, pSource.GetAddressOf());

	// https://github.com/microsoft/DirectXShaderCompiler/wiki/Using-dxc.exe-and-dxcompiler.dll#dxcompiler-dll-interface
	std::vector<LPCWSTR> args = {
		DXC_ARG_DEBUG,
		L"/Qembed_debug"
		//L"-res-may-alias",
		//L"-flegacy-macro-expansion",
		//L"-no-legacy-cbuf-layout",
		//L"-pack-optimized", // this has problem with tessellation shaders: https://github.com/microsoft/DirectXShaderCompiler/issues/3362
		//L"-all-resources-bound",
		//L"-Gis", // Force IEEE strictness
		//L"-Gec", // Enable backward compatibility mode
		//L"-Ges", // Enable strict mode
		//L"-O0", // Optimization Level 0
	};

	if (has_flag(input.flags, Flags::DISABLE_OPTIMIZATION))
	{
		args.push_back(L"-Od");
	}
	else
	{
		args.push_back(L"-Od");
	}

	switch (input.format)
	{
	case ShaderFormat::HLSL6:
		args.push_back(L"-D"); args.push_back(L"HLSL6");
		args.push_back(L"-rootsig-define"); args.push_back(L"WICKED_ENGINE_DEFAULT_ROOTSIGNATURE");
		break;
	case ShaderFormat::SPIRV:
		args.push_back(L"-D"); args.push_back(L"SPIRV");
		args.push_back(L"-spirv");
		args.push_back(L"-fspv-target-env=vulkan1.2");
		args.push_back(L"-fvk-use-dx-layout");
		args.push_back(L"-fvk-use-dx-position-w");
		//args.push_back(L"-fvk-b-shift"); args.push_back(L"0"); args.push_back(L"0");
		args.push_back(L"-fvk-t-shift"); args.push_back(L"1000"); args.push_back(L"0");
		args.push_back(L"-fvk-u-shift"); args.push_back(L"2000"); args.push_back(L"0");
		args.push_back(L"-fvk-s-shift"); args.push_back(L"3000"); args.push_back(L"0");
		break;
	default:
		assert(0);
		return;
	}

	args.push_back(L"-T");
	switch (input.stage)
	{
	case ShaderStage::MS:
		switch (input.minshadermodel)
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
	case ShaderStage::AS:
		switch (input.minshadermodel)
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
	case ShaderStage::VS:
		switch (input.minshadermodel)
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
	case ShaderStage::HS:
		switch (input.minshadermodel)
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
	case ShaderStage::DS:
		switch (input.minshadermodel)
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
	case ShaderStage::GS:
		switch (input.minshadermodel)
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
	case ShaderStage::PS:
		switch (input.minshadermodel)
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
	case ShaderStage::CS:
		switch (input.minshadermodel)
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
	case ShaderStage::LIB:
		switch (input.minshadermodel)
		{
		default:
			args.push_back(L"lib_6_5");
			break;
		case ShaderModel::SM_6_6:
			args.push_back(L"lib_6_6");
			break;
		case ShaderModel::SM_6_7:
			args.push_back(L"lib_6_7");
			break;
		}
		break;
	default:
		assert(0);
		return;
	}

}
