#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"

#include <PhxEngine/Scene/Scene.h>
#include <PhxEngine/Scene/Entity.h>
#include <PhxEngine/Scene/Components.h>
#include <PhxEngine/Scene/SceneWriter.h>

#include <cereal/archives/json.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>

#include <rapidjson/document.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>

#include <fstream>

using namespace PhxEngine; 
using namespace PhxEngine::Core;
using namespace PhxEngine::Scene;

namespace PhxEngine::Scene::New
{

    template<typename Archive>
    void serialize(Archive& archive, PhxEngine::Scene::New::NameComponent& name)
    {
        archive("NameComponent", CEREAL_NVP(name.Name));
    }

    template<typename Archive>
    void serialize(Archive& archive, PhxEngine::Scene::New::TransformComponent& transform)
    {
        archive(CEREAL_NVP(transform.LocalTranslation));
    }
}

namespace DirectX
{
    template<typename Archive>
    void serialize(Archive& archive, DirectX::XMFLOAT3& v)
    {
        std::vector<float> float3 = { v.x , v.y , v.z };
        archive(CEREAL_NVP(float3));
    }
}

namespace
{
    class CerealJsonSceneWriter : public ISceneWriter
    {
    public:
        CerealJsonSceneWriter() = default;
        bool Write(std::string const& filePath, New::Scene& scene);
    };

    bool CerealJsonSceneWriter::Write(std::string const& filePath, New::Scene& scene)
    {
        std::ofstream os(filePath.c_str(), std::ofstream::out);

        assert(!os.fail());

        {
            // output finishes flushing its contents when it goes out of scope
            cereal::JSONOutputArchive output{ os };
            entt::snapshot{ scene.GetRegistry()}.entities(output).component<New::NameComponent, New::TransformComponent, DirectX::XMFLOAT3>(output);
        }

        os.close();
        return !os.fail();
    }

    class RapidJsonSceneWriter : public ISceneWriter
    {
    public:
        RapidJsonSceneWriter() = default;
        bool Write(std::string const& filePath, New::Scene& scene);

    private:
        void WriteEntity(rapidjson::Document& d, rapidjson::Value& entitiesV, Entity entity);
    };

    bool RapidJsonSceneWriter::Write(std::string const& filePath, New::Scene& scene)
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
        assert(entity.HasComponent<New::IDComponent>());

        // TODO: I am here
        Value name = 
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
}

std::unique_ptr<ISceneWriter> PhxEngine::Scene::ISceneWriter::Create()
{
    return std::make_unique<CerealJsonSceneWriter>();
}
