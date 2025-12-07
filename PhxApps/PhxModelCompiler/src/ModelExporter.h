#pragma once

#include <fstream>

#include "CompiledResource.h"
#include "ModelBuildData.h"

#include <PhxCore/IO/FileUtils.h>
#include <PhxCore/IO/MemoryRegion.h>

namespace phx::compiler
{
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
        CompiledResource m_compiled_resource;

        std::ostream& m_out;
        const ModelData& m_model_data;

        const ExportOptions& m_options;
    };

}
