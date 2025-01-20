
#include <Phoenix.h>
#include <phx/core/EntryPoint.h>



class Sandbox final : public phx::IApplication
{
public:
	static Sandbox* Instance() { return ms_instance; }

public:

	Sandbox(const phx::ApplicationSpecification& specification)
		: m_spec(specification)
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
		PHX_INFO("Tick");
	}

private:
	inline static Sandbox* ms_instance = nullptr;

private:
	const phx::ApplicationSpecification m_spec;
};

phx::IApplication* phx::CreateApplication()
{
	ApplicationSpecification spec;
	spec.Name = "Sandbox";
	spec.WorkingDirectory = "../Hazelnut";

	return new Sandbox(spec);
}
