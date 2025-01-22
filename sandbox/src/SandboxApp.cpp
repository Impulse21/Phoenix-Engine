
#include <Phoenix.h>
#include <phx/core/EntryPoint.h>

class Sandbox final : public phx::IApplication
{
public:
	static Sandbox* Instance() { return ms_instance; }

public:
	Sandbox(const phx::ApplicationDescriptor& desc)
		: m_desc(desc)
	{
		ms_instance = this;
	}

	~Sandbox() { ms_instance = nullptr; }


	void Startup() override
	{
		PHX_INFO("Sandbox app is starting up");
	}

	void Shutdown() override
	{
		PHX_INFO("Sandbox app is starting up");
	}

	void Tick() override
	{
		phx::rhi::Present();
	}

	const char* GetName() const override { return this->m_desc.Name.c_str(); }
	void GetDefaultWindowSize(uint32_t& outWidth, uint32_t& outHeight) const override
	{
		outWidth = m_desc.Width;
		outHeight = m_desc.Height;
	}

private:
	inline static Sandbox* ms_instance = nullptr;

private:
	const phx::ApplicationDescriptor m_desc;
};

phx::IApplication* phx::CreateApplication()
{
	ApplicationDescriptor desc = {
		.Name = "Sandbox",
	};

	return new Sandbox(desc);
}
