#include "ModelImporterFactory.h"

#include <array>
#include <functional>
#include <string_view>

#include "ModelImporter_Gltf.h"

namespace
{
    using ModelImporterCreatorPtr = std::unique_ptr<IModelImporter>(*)();

    constexpr std::pair<std::string_view, ModelImporterCreatorPtr> MakeFactory(std::string_view ext, ModelImporterCreatorPtr creator)
    {
        return { ext, creator };
    }

    constexpr auto Importer_Factories = std::to_array({
           MakeFactory(".gltf", []() -> std::unique_ptr<IModelImporter> { // Explicit return type for lambda clarity
               return std::make_unique<GltfModelImporter>();
           }),
        });
}

bool ModelImporterFactory::IsSupported(std::string const& extension)
{
    auto it = std::find_if(Importer_Factories.begin(), Importer_Factories.end(),
        [&](const auto& entry) {
            return extension == entry.first;
        });

    return it != Importer_Factories.end();
}

std::unique_ptr<IModelImporter> ModelImporterFactory::Create(std::string const& extension)
{
    auto it = std::find_if(Importer_Factories.begin(), Importer_Factories.end(),
        [&](const auto& entry) {
            return extension == entry.first;
        });

    if (it != Importer_Factories.end()) 
    {
        return it->second();
    }

    return nullptr;
}
