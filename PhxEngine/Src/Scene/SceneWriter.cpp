#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"

#include <PhxEngine/Scene/Scene.h>
#include <PhxEngine/Scene/Entity.h>
#include <PhxEngine/Scene/Components.h>
#include <PhxEngine/Scene/SceneWriter.h>

#define ENABLE_CEREAL 1

#if ENABLE_CEREAL
#include <cereal/archives/json.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>

#else

#include <rapidjson/document.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#endif

#include <fstream>

using namespace PhxEngine; 
using namespace PhxEngine::Core;
using namespace PhxEngine::Scene;

namespace PhxEngine::Scene
{
	template<typename Archive>
	void serialize(Archive& archive, PhxEngine::Scene::Entity& entity)
	{
		assert(entity.HasComponent<IDComponent>());
		assert(entity.HasComponent<NameComponent>());
		archive(
			cereal::make_nvp("UUID", (uint64_t)entity.GetUUID()));
		archive(
			cereal::make_nvp("Name", entity.GetName()));

		if (entity.HasComponent<TransformComponent>())
		{
			archive(
				cereal::make_nvp("Transform Component", entity.GetComponent<TransformComponent>()));
		}
	}

	template<typename Archive>
	void serialize(Archive& archive, PhxEngine::Scene::NameComponent& name)
	{
		archive(cereal::make_nvp("Name", name.Name));
	}

	template<typename Archive>
	void serialize(Archive& archive, PhxEngine::Scene::TransformComponent& transform)
	{
		archive(
			cereal::make_nvp("Translation", transform.LocalTranslation));
		archive(
			cereal::make_nvp("Rotation", transform.LocalRotation));
		archive(
			cereal::make_nvp("Scale", transform.LocalScale));
	}

	template<typename Archive>
	void serialize(Archive& archive, PhxEngine::Scene::MeshRenderComponent& transform)
	{
		// Get The Static Mesh's Path and save it out.
		archive(
			cereal::make_nvp("Translation", transform.LocalTranslation));
		archive(
			cereal::make_nvp("Rotation", transform.LocalRotation));
		archive(
			cereal::make_nvp("Scale", transform.LocalScale));
	}
}

namespace DirectX
{
    template<typename Archive>
    void serialize(Archive& archive, DirectX::XMFLOAT2& v)
    {
        std::vector<float> float2 = { v.x , v.y};
        archive(CEREAL_NVP(float2));
    }

    template<typename Archive>
    void serialize(Archive& archive, DirectX::XMFLOAT3& v)
    {
        std::vector<float> float3 = { v.x , v.y , v.z };
        archive(CEREAL_NVP(float3));
    }

    template<typename Archive>
    void serialize(Archive& archive, DirectX::XMFLOAT4& v)
    {
        std::vector<float> float4 = { v.x , v.y , v.z, v.w};
        archive(CEREAL_NVP(float4));
    }
}

namespace
{
#if ENABLE_CEREAL
    class CerealJsonSceneWriter : public ISceneWriter
    {
    public:
        CerealJsonSceneWriter() = default;
        bool Write(std::string const& filePath, PhxEngine::Scene::Scene& scene);
    };

    bool CerealJsonSceneWriter::Write(std::string const& filePath, PhxEngine::Scene::Scene& scene)
    {
        std::ofstream os(filePath.c_str(), std::ofstream::out);

        assert(!os.fail());

        {
            // output finishes flushing its contents when it goes out of scope
            cereal::JSONOutputArchive archive{ os };
            std::vector<Entity> entities;
            entities.reserve(scene.GetRegistry().size());
            scene.GetRegistry().each([&](auto entityID)
                {
                    Entity entity = { entityID, &scene };
                    if (entity)
                    {
                        entities.push_back(entity);
                    }
                });

            archive(CEREAL_NVP(entities));
        }

        os.close();
        return !os.fail();
    }

#else
    class RapidJsonSceneWriter : public ISceneWriter
    {
    public:
        RapidJsonSceneWriter() = default;
        bool Write(std::string const& filePath, Scene& scene);

    private:
        void WriteEntity(rapidjson::Document& d, rapidjson::Value& entitiesV, Entity entity);
    };

    bool RapidJsonSceneWriter::Write(std::string const& filePath, Scene& scene)
    {
        rapidjson::Document d;
        {
            rapidjson::Value entitiesV(rapidjson::kArrayType);
            scene.GetRegistry().each([&](auto entityID)
                {
                    Entity entity = { entity, &scene };
                    if (entity)
                    {
                        this->WriteEntity(d, entitiesV, entity);
                    }
                });
            d.AddMember("entities", entitiesV, d.GetAllocator());
        }

        std::ofstream ofs(filePath.c_str(), std::ofstream::out);
        rapidjson::OStreamWrapper osw(ofs);

        rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
        d.Accept(writer);

        ofs.close();
        return !ofs.fail();
    }
    void RapidJsonSceneWriter::WriteEntity(rapidjson::Document& d, rapidjson::Value& entitiesV, Entity entity)
    {
        assert(entity.HasComponent<IDComponent>());

        // TODO: I am here
        rapidjson::Value uuidValue(r)
            out << YAML::BeginMap; // Entity
        out << YAML::Key << "Entity" << YAML::Value << entity.GetUUID();

        if (entity.HasComponent<TagComponent>())
        {
            out << YAML::Key << "TagComponent";
            out << YAML::BeginMap; // TagComponent

            auto& tag = entity.GetComponent<TagComponent>().Tag;
            out << YAML::Key << "Tag" << YAML::Value << tag;

            out << YAML::EndMap; // TagComponent
        }
    }
#endif
}

std::unique_ptr<ISceneWriter> PhxEngine::Scene::ISceneWriter::Create()
{
#if ENABLE_CEREAL
    return std::make_unique<CerealJsonSceneWriter>();
#else
    return std::make_unique<RapidJsonSceneWriter>();
#endif
}
