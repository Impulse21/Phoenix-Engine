#pragma once

#include <stdint.h>

#include "Platform.h"

namespace PhxEngine::Core
{
	struct Canvas
	{
		uint32_t Width = 0;
		uint32_t Height = 0;
		float Dpi = 96.0f;
		float Scaling = 1.0f;

		// How many pixels there are per inch
		inline float GetDPI() const { return this->Dpi * this->Scaling; }
		inline float GetDPIScaling() const { return GetDPI() / 96.f; }

		inline uint32_t GetPhysicalWidth() const { return this->Width; }
		inline uint32_t GetPhysicalHeight() const { return this->Height; }

		inline float GetLogicalWidth() const { return GetPhysicalWidth() / GetDPIScaling(); }
		inline float GetLogicalHeight() const { return GetPhysicalHeight() / GetDPIScaling(); }

		// Returns projection matrix that maps logical to physical space
		//	Use this to render to a graphics viewport
		/*
		inline XMMATRIX GetProjection() const
		{
			return XMMatrixOrthographicOffCenterLH(0, (float)GetLogicalWidth(), (float)GetLogicalHeight(), 0, -1, 1);
		}
		*/
	};

	inline void InitializeCanvas(Platform::WindowHandle windowHandle, Canvas& outCanvas)
	{
		Platform::WindowProperties windowProps;
		Platform::GetWindowProperties(windowHandle, windowProps);
		outCanvas.Width = static_cast<uint32_t>(windowProps.Width);
		outCanvas.Height = static_cast<uint32_t>(windowProps.Height);
		outCanvas.Dpi = windowProps.Dpi;
	}
}

