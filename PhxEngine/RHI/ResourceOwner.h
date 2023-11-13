#pragma once

namespace PhxEngine::RHI
{
	template<typename T>
	class ResourceOwner : public T
	{
	public:
		ResourceOwner() = default;
		ResourceOwner()

		ResourceOwner(ResourceOwner&&) = delete;
		ResourceOwner& operator=(ResourceOwner&&) = delete;

		operator T* () const noexcept { return (T)this; }
	};
}