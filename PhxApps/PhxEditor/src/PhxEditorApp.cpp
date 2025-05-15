
#include <PhxCore/Base.h>
#include <PhxCore/VFS.h>
#include <PhxEngine/EntryPoint.h>

#include "fast_obj/fast_obj.h"
#include "meshoptimizer/meshoptimizer.h"

#include "Generated/GlobalVariables.h"

namespace
{
	void CompileObjAndMaterials(const char* filename, const char*)
	{
		fastObjMesh* mesh = fast_obj_read(filename);
		if (!mesh)
		{
			PHX_ERROR("Failed to Load. \n\tError {0}\n\tWarn {1}");
			return;
		}

		fast_obj_destroy(mesh);
	}
}

class PhxEditor final : public phx::IApplication
{
public:
	static PhxEditor* Instance() { return ms_instance; }

	PhxEditor(const phx::ApplicationDescriptor& desc)
		: m_desc(desc)
	{
		ms_instance = this;
	}

	~PhxEditor() { ms_instance = nullptr; }

	void Startup() override;
	void Shutdown() override;

	void OnPreRender() override;
	void OnUpdate_Threaded(float deltaTime) override;
	void OnRender_Threaded() override;

	const char* GetName() const override { return this->m_desc.Name.c_str(); }
	void GetDefaultWindowSize(uint32_t& outWidth, uint32_t& outHeight) const override
	{
		outWidth = m_desc.Width;
		outHeight = m_desc.Height;
	}

	void SetWindowHandle(void* handle) override { m_windowHandle = handle; }
	void* GetWindowHandle() const override { return m_windowHandle; }

private:
	inline static PhxEditor* ms_instance = nullptr;
	const phx::ApplicationDescriptor m_desc;
	void* m_windowHandle;
};

phx::IApplication* phx::CreateApplication()
{
	ApplicationDescriptor desc = {
		.Name = "PhxEditor",
		.WorkingDirectory = phx::VFS::GetDirectoryWithExecutable()
	};

	return new PhxEditor(desc);
}

void PhxEditor::Startup()
{
	// phx::FileSystem::Mount("native://", "");
	// phx::FileSystem::Mount("res://", phx::GlobalPaths::AssetsDirectory);
	// Import Resource
	const char* filename = "C:/Users/dipao/OneDrive/Documents/Art/SM_Chest_01.obj";
	CompileObjAndMaterials(filename, "res://modulardungeoncollection");

	// Compile mesh and save it to disk
}

void PhxEditor::Shutdown()
{
}

void PhxEditor::OnPreRender()
{
}

void PhxEditor::OnUpdate_Threaded(float /*deltaTime*/)
{
}

void PhxEditor::OnRender_Threaded()
{
}