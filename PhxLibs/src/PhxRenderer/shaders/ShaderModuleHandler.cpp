#include "PhxRenderer/PhxRenderer_pch.h"
#include "ShaderModuleHandler.h"

#include <PhxCore/IO/FileUtils.h>
#include <PhxCore/IVirtualFileSystem.h>

#include <PhxRenderer/shaders/ShaderModuleResource.h>

using namespace phx;
using namespace phx::renderer;


namespace
{
    enum InternalState
    {
        State_Init = ResourceState::Loading,
        State_Wait_For_Load = ResourceState::Loading + 1,
        State_Parse_MTL = ResourceState::Loading + 2,
        State_Wait_For_Parse = ResourceState::Loading + 3,
        State_Check_Dependencies = ResourceState::Waiting_dependencies
    };
}

LoaderStepResult phx::renderer::ShaderModuleHandler::Step(LoadContext& ctx) const
{
    // TODO: I am here.
    RefCountPtr<MaterialResource> mat_handle = ctx.handle.As<MaterialResource>();
    auto state = ctx.GetInternalState<InternalState>();

    return LoaderStepResult();
}
