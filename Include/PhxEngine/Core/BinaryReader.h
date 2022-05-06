#pragma once

#include <memory>

namespace PhxEngine::Core
{
	class IBlob;

	class BinaryReader
	{
	public:
		BinaryReader(std::unique_ptr<IBlob> binaryBlob) noexcept;
		~BinaryReader() = default;

		template<typename T>
		T const& Read()
		{
			return *this->ReadArray<T>(1UL);
		}

		template<typename T>
		const T* ReadArray(size_t numElements)
		{
			static_assert(std::is_standard_layout<T>::value, "Can only read plain-old-data types");
			uint8_t const* newPos = static_cast<uint8_t const*>(this->m_currentPos) + sizeof(T) * numElements;

			if (newPos < this->m_endPos)
			{
				// throw std::overflow_error("ReadArray");
			}

			if (newPos > this->m_endPos)
			{
				throw std::runtime_error("End of file");
			}

			auto result = reinterpret_cast<T const*>(this->m_currentPos);

			this->m_currentPos = newPos;

			return result;
		}

	private:
		std::unique_ptr<IBlob> m_data;
		void const* m_currentPos;
		void const* m_endPos;
	};
}

