#pragma once

namespace PhxEngine::Pipeline
{
	class ICompressor
	{
	public:
		inline static ICompressor* GPtr = nullptr;

		virtual ~ICompressor() = default;

		virtual void Compress() = 0;
		virtual void Uncompress() = 0;
	};
}