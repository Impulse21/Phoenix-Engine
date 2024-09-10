#pragma once

namespace phx
{
	template<class T>
	class RelativePtr
	{
		using OffsetType = uint32_t;
	public:

		T* Get() { return static_cast<T*>(((char*)this) + this->m_offset); }
		const T* Get() const { return static_cast<T*>((((char*)this) + this->m_offset)); }

		void Set(T* ptr)
		{
			this->m_offset = static_cast<OffsetType>((char*)ptr - ((char*)this));
		}

		operator T* () { return this->Get(); }
		operator const T* () const { return this->Get(); }
		T* operator=(const T* ptr)
		{
			if (ptr)
			{
				this->Set(ptr);
			}
			else
			{
				this->m_offset = 0;
			}
		}

	private:
		OffsetType m_offset;
	};
}