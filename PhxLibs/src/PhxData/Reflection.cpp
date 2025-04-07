#include "PhxData_pch.h"
#include "Reflection.h"

#include <PhxCore/VFS.h>

#include "yaml-cpp/yaml.h"

using namespace phx;
using namespace phx::rft;



template<typename T>
bool rft::Serialize(IFileSystem* fs, TypeInfo<T>& typeInfo)
{

    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "World" << YAML::Value << "Untitled";
    out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

    for (auto& entityId : world.GetRegistry().view<entt::entity>())
    {
        Entity entity(entityId, &world);
        if (!entity)
            continue;

        Yaml::SerializeEntity(out, entity, world);
    };

    out << YAML::EndSeq;
    out << YAML::EndMap;
    const char* strData = out.c_str();
    fs->WriteFile(filename, Span(strData, strlen(strData)));

    return true;
}