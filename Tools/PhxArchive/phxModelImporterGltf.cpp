#include "phxModelImporterGltf.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf/cgltf.h"

#include <Core/phxVirtualFileSystem.h>
#include <Core/phxLog.h>
#include <Core/phxMath.h>
#include <Core/phxMemory.h>
#include <Core/phxBinaryBuilder.h>

#include <RHI/PhxRHI.h>

#include <Renderer/phxConstantBuffers.h>
#include "phxTextureConvert.h"
#include "phxMeshConvert.h"

#include <unordered_map>

using namespace phx;
using namespace phx::renderer;
using namespace DirectX;

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

		this->m_materialIndexLut[this->m_gltfData->materials + i] = i;
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

		if (curNode->has_matrix)
		{
			std::memcpy((float*)&thisGraphNode.XForm, curNode->matrix, sizeof(curNode->matrix));
		}
		else
		{

			DirectX::XMVECTOR localScale = XMLoadFloat3(&thisGraphNode.Scale);
			DirectX::XMVECTOR localRotation = XMLoadFloat4(&thisGraphNode.Rotation);
			DirectX::XMVECTOR localTranslation = {
				curNode->translation[0],
				curNode->translation[1],
				curNode->translation[2],
				1.0f };

			DirectX::XMMATRIX result = 
				DirectX::XMMatrixScalingFromVector(localScale) *
				DirectX::XMMatrixRotationQuaternion(localRotation) *
				DirectX::XMMatrixTranslationFromVector(localTranslation);

			DirectX::XMStoreFloat4x4(&thisGraphNode.XForm, result);
		}

		const DirectX::XMMATRIX localXform = xform * DirectX::XMLoadFloat4x4(&thisGraphNode.XForm);

		if (!curNode->camera && curNode->mesh != nullptr)
		{
			const size_t skinIndex = curNode->skin != nullptr ? curNode->skin - this->m_gltfData->skins : ~0ul;
			Sphere sphereOS;
			AABB boxOS;
			CompileMesh(meshList, bufferMemory, *curNode->mesh, curPos, localXform, skinIndex,sphereOS, boxOS);
			modelBSphere = modelBSphere.Union(sphereOS);
			modelBBox = AABB::Merge(modelBBox, boxOS);
		}

		uint32_t nextPos = curPos + 1;

		if (curNode->children_count > 0)
		{
			thisGraphNode.HasChildren = 1;
			nextPos = WalkGraphRec(
				sceneGraph,
				modelBSphere,
				modelBBox,
				meshList,
				bufferMemory,
				curNode->children,
				curNode->children_count,
				nextPos,
				localXform);
		}

		// Are there more siblings?
		if (i + 1 < numSiblings)
		{
			thisGraphNode.HasSibling = 1;
		}

		curPos = nextPos;
	}

	return curPos;
}

void phx::phxModelImporterGltf::CompileMesh(
	std::vector<Mesh*>& meshList,
	std::vector<uint8_t>& bufferMemory,
	cgltf_mesh& srcMesh,
	uint32_t matrixIdx,
	const DirectX::XMMATRIX& localToObject,
	size_t skinIndex,
	Sphere& boundingSphere,
	AABB& boundingBox)
{
	size_t totalVertexSize = 0;
	size_t totalIndexSize = 0;

	Sphere sphereOS;
	AABB bboxOS;

	std::vector<MeshConverter::Primitive> primitives(srcMesh.primitives_count);

	for (size_t i = 0; i < srcMesh.primitives_count; i++)
	{
		MeshConverter::OptimizeMesh(primitives[i], srcMesh.primitives[i], localToObject);
		sphereOS = sphereOS.Union(primitives[i].BoundsOS);
		bboxOS = AABB::Merge(bboxOS, primitives[i].BBoxOS);
		primitives[i].MaterialIdx = this->m_materialIndexLut[srcMesh.primitives[i].material];
	}
	boundingSphere = sphereOS;
	boundingBox = bboxOS;

	std::unordered_map<uint32_t, std::vector<MeshConverter::Primitive*>> renderMeshes;
	for (auto& prim : primitives)
	{
		const uint32_t hash = prim.Hash;
		renderMeshes[hash].push_back(&prim);
		totalVertexSize += prim.VertexSizeInBytes;
		totalIndexSize += MemoryAlign(prim.NumIndices, 4);
	}
	const uint32_t totalBufferSize = (uint32_t)(totalVertexSize + totalIndexSize);
	std::vector<uint8_t> stagggingBuffer(totalBufferSize);

	uint32_t curVBOffset = 0;
	uint32_t curIBOffset = totalVertexSize;

	for (auto& [hash, drawables] : renderMeshes)
	{
		const size_t numDraws = drawables.size();
		Mesh* mesh = (Mesh*)malloc(sizeof(Mesh) * sizeof(Mesh::Draw) * (numDraws - 1));
		size_t vbSize = 0;
		size_t ibSize = 0;

		// TODO: I am here Need to sort out this logic.
		// local space of all sub meshes
		Sphere collectiveSphereLS;
		for (auto& draw : drawables)
		{
			vbSize += draw->VertexSizeInBytes;
			ibSize += draw->NumIndices;
			collectiveSphereLS = collectiveSphereLS.Union(draw->BoundsLS);
		}

		mesh->Bounds[0] = collectiveSphereLS.Centre.x;
		mesh->Bounds[1] = collectiveSphereLS.Centre.y;
		mesh->Bounds[2] = collectiveSphereLS.Centre.z;
		mesh->Bounds[3] = collectiveSphereLS.Radius;
		mesh->VbOffset = (uint32_t)bufferMemory.size() + curVBOffset;
		mesh->VbSize = 0;
		mesh->VbStride = 0;
		mesh->IbFormat = uint8_t(drawables[0]->Index32 ? rhi::Format::R32_UINT : rhi::Format::R16_UINT);
		mesh->MeshCBV = (uint16_t)matrixIdx;
		mesh->MaterialCBV = drawables[0]->MaterialIdx;
		mesh->PsoFlags = drawables[0]->PsoFlags;
		mesh->Pso = 0xFFFF;

		if (skinIndex != ~0ul)
		{
			mesh->NumJoints = 0xFFFF;
			mesh->StartJoint = (uint16_t)skinIndex;
		}
		else
		{
			mesh->NumJoints = 0;
			mesh->StartJoint = 0xFFFF;
		}

		mesh->NumDraws = (uint16_t)numDraws;
		uint32_t drawIdx = 0;
		for (auto& draw : drawables)
		{
			Mesh::DrawInfo& d = mesh->Draw[drawIdx++];
			d.PrimCount = draw->PrimCount;
			d.BaseVertex = curVertOffset;
			d.StartIndex = curIndexOffset;
			std::memcpy(uploadMem + curVBOffset + curVertOffset, draw->VB->data(), draw->VB->size());
			curVertOffset += (uint32_t)draw->VB->size() / draw->vertexStride;
			std::memcpy(uploadMem + curDepthVBOffset, draw->DepthVB->data(), draw->DepthVB->size());
			std::memcpy(uploadMem + curIBOffset + curIndexOffset, draw->IB->data(), draw->IB->size());
			curIndexOffset += (uint32_t)draw->IB->size() >> (draw->index32 + 1);
		}
		meshList.push_back(mesh);
	}

}
