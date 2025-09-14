#pragma once

#include <fstream>

#include "ModelBuildData.h"

#include <PhxCore/IO/FileUtils.h>
#include <PhxCore/IO/MemoryRegion.h>

struct ExportOptions
{

};

class ModelExporter
{
public:
    static void Export(
        std::ostream& out,
        ModelData const& model_data,
        ExportOptions const& options)
    {
        ModelExporter exporter(out, model_data, options);
        exporter.Export();
    }

private:
    ModelExporter(
        std::ostream& out,
        ModelData const& model_data,
        ExportOptions const& options) 
        : m_out(out)
        , m_model_data(model_data)
        , m_options(options)
    {

    }

    void Export();

private:
    struct CompiledResource
    {
        std::string name;
        std::string ext;

        // Keep metadata chunk separate as they are stored differently in pak files.
        phx::MemoryBuffer metadata_chunk;
        std::vector<phx::MemoryBuffer> chunks;
    } m_compiled_resource;

    std::ostream& m_out;
    const ModelData& m_model_data;

    const ExportOptions& m_options;
};

