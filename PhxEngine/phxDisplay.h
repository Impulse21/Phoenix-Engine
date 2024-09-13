#pragma once


namespace phx
{
	namespace Display
	{
		void Initialize();
		void Finalize();

		void Resize(uint32_t width, uint32_t height);
		void Preset();
	}

	namespace gfx
	{
		extern uint32_t g_DisplayWidth;
		extern uint32_t g_DisplayHeight;
		extern bool g_EnableHDROutput;

		// Returns the number of elapsed frames since application start
		uint64_t GetFrameCount();

		// The amount of time elapsed during the last completed frame.  The CPU and/or
		// GPU may be idle during parts of the frame.  The frame time measures the time
		// between calls to present each frame.
		float GetFrameTime();

		// The total number of frames per second
		float GetFrameRate();
	}
}