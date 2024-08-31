#include "phxModelImporterGltf.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf/cgltf.h"

#include <Core/phxVirtualFileSystem.h>
#include <Core/phxLog.h>

#include <Renderer/phxConstantBuffers.h>
#include "phxTextureConvert.h"

#include <unordered_map>

using namespace phx;
using namespace phx::renderer;

namespace
{
	cgltf_result CgltfReadFile(const cgltf_memory_options* memory_options, const cgltf_file_options* file_options, const char* path, cgltf_size* size, void** Data)
	{
		CgltfContext* context = (CgltfContext*)file_options->user_data;

		std::unique_ptr<IBlob> dataBlob = context->FileSystem->ReadFile(path);
		if (!dataBlob)
		{
			return cgltf_result_file_not_found;
		}

		if (size)
		{
			*size = dataBlob->Size();
		}

		if (Data)
		{
			*Data = (void*)dataBlob->Data();  // NOLINT(clang-diagnostic-cast-qual)
		}

		context->Blobs.push_back(std::move(dataBlob));

		return cgltf_result_success;
	}

	static void CgltfReleaseFile(
		const struct cgltf_memory_options*,
		const struct cgltf_file_options*,
		void*)
	{
		// do nothing
	}

	inline void SetTextureOptions(std::unordered_map<std::string, uint8_t>& optionsMap, cgltf_texture* texture, uint8_t options)
	{
		if (texture && texture->image && optionsMap.find(texture->image->uri) == optionsMap.end())
			optionsMap[texture->image->uri] = options;
	}
}

bool phx::phxModelImporterGltf::Import(IFileSystem* fs, std::string const& filename, ModelData& outModel)
{
	// Load GLF File into memory
	this->m_cgltfContext =
	{
		.FileSystem = fs,
		.Blobs = {}
	};

	cgltf_options options = { };
	options.file.read = &CgltfReadFile;
	options.file.release = &CgltfReleaseFile;
	options.file.user_data = &this->m_cgltfContext;


	std::unique_ptr<IBlob> blob = fs->ReadFile(filename);
	if (!blob)
	{
		PHX_ERROR("Couldn't Read file %s", filename.c_str());
		return false;
	}

	this->m_gltfData = nullptr;
	cgltf_result res = cgltf_parse(&options, blob->Data(), blob->Size(), &this->m_gltfData);
	if (res != cgltf_result_success)
	{
		PHX_ERROR("Couldn't load glTF file %s", filename.c_str());
		return false;
	}

	res = cgltf_load_buffers(&options, this->m_gltfData, filename.c_str());
	if (res != cgltf_result_success)
	{
		PHX_ERROR("Couldn't load glTF Binary data %s", filename.c_str());
		return false;
	}

	this->BuildMaterials(outModel);

	outModel.SceneGraph.resize(this->m_gltfData->scene->nodes_count);
	const cgltf_scene* scene = this->m_gltfData->scene; // sceneIdx < 0 ? asset.m_scene : &asset.m_scenes[sceneIdx];
	if (scene == nullptr)
		return false;

	// Aggregate all of the vertex and index buffers in this unified buffer
	std::vector<uint8_t>& bufferMemory = outModel.GeometryData;

	outModel.BoundingSphere = {};
	outModel.BoundingBox = {};
	uint32_t numNodes = WalkGraphRec(
		outModel.SceneGraph,
		outModel.BoundingSphere,
		outModel.BoundingBox,
		outModel.Meshes,
		bufferMemory,
		scene->nodes,
		scene->nodes_count,
		0,
		DirectX::XMMatrixIdentity());

	outModel.SceneGraph.resize(numNodes);

    return true;
}

void phx::phxModelImporterGltf::BuildMaterials(ModelData& outModel)
{
	static_assert((_alignof(MaterialConstants) & 255) == 0, "CBVs need 256 byte alignment");

	// Replace texture filename extensions with "DDS" in the string table
	outModel.TextureNames.resize(this->m_gltfData->textures_count);
	for (size_t i = 0; i < this->m_gltfData->textures_count; ++i)
	{
		outModel.TextureNames[i] = this->m_gltfData->textures[i].image->name;
	}

	std::unordered_map<std::string, uint8_t> textureOptions;
	const uint32_t numMaterials = (uint32_t)this->m_gltfData->materials_count;

	outModel.MaterialConstants.resize(numMaterials);
	outModel.MaterialTextures.resize(numMaterials);

	auto SetTextureData = [&outModel](cgltf_data* gltfData, cgltf_texture* texture, int mtlIdx, int texIdx) {
		MaterialTextureData& dstTexture = outModel.MaterialTextures[mtlIdx];
		dstTexture.StringIdx[texIdx];

		if (!texture)
		{
			dstTexture.AddressModes |= 0x5 << (texIdx * 4);
			return;
		}

		dstTexture.StringIdx[texIdx] = uint16_t(texture - gltfData->textures);

		if (texture->sampler)
		{
			dstTexture.AddressModes |= texture->sampler->wrap_s << (texIdx * 4);
			dstTexture.AddressModes |= texture->sampler->wrap_t << (texIdx * 4 + 2);
		}
		else 
		{
			dstTexture.AddressModes |= 0x5 << (texIdx * 4);
		}

	};

	for (size_t i = 0; i < this->m_gltfData->materials_count; i++)
	{
		MaterialTextureData& dstTexture = outModel.MaterialTextures[i];
		dstTexture.AddressModes = 0;

		const cgltf_material& srcMat = this->m_gltfData->materials[i];
		MaterialConstantData& material = outModel.MaterialConstants[i];

		material.Flags = 0u;
		material.AlphaCutoff = DirectX::XMConvertFloatToHalf(0.5f);

		if (srcMat.has_pbr_metallic_roughness)
		{
			const cgltf_pbr_metallic_roughness& pbr = srcMat.pbr_metallic_roughness;
			material.BaseColour[0] = pbr.base_color_factor[0];
			material.BaseColour[1] = pbr.base_color_factor[1];
			material.BaseColour[2] = pbr.base_color_factor[2];
			material.BaseColour[3] = pbr.base_color_factor[3];
			material.Metalness = pbr.metallic_factor;
			material.Rougness = pbr.roughness_factor;

			material.BaseColorUV = pbr.base_color_texture.texcoord;
			SetTextureData(this->m_gltfData, pbr.base_color_texture.texture, i, kBaseColor);

			material.MetallicRoughnessUV = pbr.metallic_roughness_texture.texcoord;
			SetTextureData(this->m_gltfData, pbr.metallic_roughness_texture.texture, i, kMetallicRoughness);
		}

		material.EmissiveColour[0] = srcMat.emissive_factor[0];
		material.EmissiveColour[1] = srcMat.emissive_factor[1];
		material.EmissiveColour[2] = srcMat.emissive_factor[2];
		material.EmissiveUV = srcMat.emissive_texture.texcoord;
		SetTextureData(this->m_gltfData, srcMat.emissive_texture.texture, i, kEmissive);

		material.NormalUV = srcMat.normal_texture.texcoord;
		SetTextureData(this->m_gltfData, srcMat.normal_texture.texture, i, kNormal);

		material.OcclusionUV = srcMat.occlusion_texture.texcoord;
		SetTextureData(this->m_gltfData, srcMat.occlusion_texture.texture, i, kOcclusion);

		if (srcMat.alpha_mode == cgltf_alpha_mode_blend)
			material.AlphaBlend = true;
		else if (srcMat.alpha_mode == cgltf_alpha_mode_mask)
			material.AlphaTest = true;

		material.AlphaCutoff = DirectX::XMConvertFloatToHalf((float)material.AlphaCutoff);
		material.TwoSided = srcMat.double_sided;

		// -- Set Texture Conversion options ---
		if (srcMat.has_pbr_metallic_roughness)
		{
			const cgltf_pbr_metallic_roughness& pbr = srcMat.pbr_metallic_roughness;
			SetTextureOptions(textureOptions, pbr.base_color_texture.texture, TextureOptions(true, material.AlphaBlend | material.AlphaTest));
			SetTextureOptions(textureOptions, pbr.metallic_roughness_texture.texture, TextureOptions(false));
		}

		SetTextureOptions(textureOptions, srcMat.occlusion_texture.texture, TextureOptions(false));
		SetTextureOptions(textureOptions, srcMat.emissive_texture.texture, TextureOptions(true));
		SetTextureOptions(textureOptions, srcMat.normal_texture.texture, TextureOptions(false));
	}

	const bool compileTextures = true;
	outModel.TextureOptions.clear();
	for (auto& name : outModel.TextureNames)
	{
		auto iter = textureOptions.find(name);
		if (iter != textureOptions.end())
		{
			outModel.TextureOptions.push_back(iter->second);
			if (compileTextures)
				TextureCompiler::CompileOnDemand(*this->m_fs, iter->first, iter->second);
		}
		else
		{
			outModel.TextureOptions.push_back(0xFF);
		}
	}

	assert(outModel.TextureOptions.size() == outModel.TextureNames.size());
}

uint32_t phx::phxModelImporterGltf::WalkGraphRec(
	std::vector<GraphNode>& sceneGraph,
	Sphere& modelBSphere,
	AABB& modelBBox,
	std::vector<Mesh*>& meshList,
	std::vector<uint8_t>& bufferMemory,
	cgltf_node** siblings,
	uint32_t numSiblings,
	uint32_t curPos,
	DirectX::XMMATRIX const& xform)
{
	for (size_t i = 0; i < numSiblings; ++i)
	{
		cgltf_node* curNode = siblings[i];
		GraphNode& thisGraphNode = sceneGraph[curPos];
		thisGraphNode.HasSibling = 0;
		thisGraphNode.MatrixIdx = curPos;
		thisGraphNode.SkeletonRoot = static_cast<uint32_t>(curNode->skin - this->m_gltfData->skins);

		// They might not be used, but we have space to hold the neutral values which could be
		// useful when updating the matrix via animation.
		std::memcpy((float*)&thisGraphNode.Scale, curNode->scale, sizeof(curNode->scale));
		std::memcpy((float*)&thisGraphNode.Rotation, curNode->rotation, sizeof(curNode->rotation));
		std::memcpy((float*)&thisGraphNode.Trans, curNode->rotation, sizeof(curNode->rotation));

		if (curNode->has_matrix)
		{
			std::memcpy((float*)&thisGraphNode.XForm, curNode->matrix, sizeof(curNode->matrix));
		}
		else
		{

			DirectX::XMVECTOR localScale = XMLoadFloat3(&thisGraphNode.Scale);
			DirectX::XMVECTOR localRotation = XMLoadFloat4(&thisGraphNode.Rotation);

			DirectX::XMMATRIX result = 
				DirectX::XMMatrixScalingFromVector(localScale) *
				DirectX::XMMatrixRotationQuaternion(localRotation) *
				DirectX::XMMatrixTranslationFromVector(localTranslation);

			DirectX::XMStoreFloat4x4(&thisGraphNode.XForm, result);
		}

		const Matrix4 LocalXform = xform * thisGraphNode.XForm;

		if (!curNode->pointsToCamera && curNode->mesh != nullptr)
		{
			BoundingSphere sphereOS;
			AxisAlignedBox boxOS;
			CompileMesh(meshList, bufferMemory, *curNode->mesh, curPos, LocalXform, sphereOS, boxOS);
			modelBSphere = modelBSphere.Union(sphereOS);
			modelBBox.AddBoundingBox(boxOS);
		}

		uint32_t nextPos = curPos + 1;

		if (curNode->children.size() > 0)
		{
			thisGraphNode.hasChildren = 1;
			nextPos = WalkGraphRec(sceneGraph, modelBSphere, modelBBox, meshList, bufferMemory, curNode->children, nextPos, LocalXform);
		}

		// Are there more siblings?
		if (i + 1 < numSiblings)
		{
			thisGraphNode.hasSibling = 1;
		}

		curPos = nextPos;
	}

	return curPos;
}
