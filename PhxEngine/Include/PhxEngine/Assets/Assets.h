#pragma once

#include <memory>
#include <PhxEngine/Core/Object.h>
#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/Core/FlexArray.h>
#include <PhxEngine/Core/StopWatch.h>
#include <json.hpp>

namespace PhxEngine::Assets
{
	/// Asynchronous loading state of a resource.
	enum class AsyncLoadState
	{
		/// No async operation in progress.
		Done = 0,
		/// Queued for asynchronous loading.
		Queued,
		/// In progress of calling BeginLoad() in a worker thread.
		Loading,
		/// BeginLoad() succeeded. EndLoad() can be called in the main thread.
		Success,
		/// BeginLoad() failed.
		Failed,
	};
	class Asset : public Core::Object
	{
		PHX_OBJECT(Asset, Object);

	public:
		Asset();

		const std::string& GetName() const { return m_name; }
		Core::StringHash GetNameHash() const { return m_nameHash; }
		int32_t GetMemoryUse() const { return m_memoryUsage; }
		Core::TimeStep GetUseTimer();

		/// Return the asynchronous loading state.
		AsyncLoadState GetAsyncLoadState() const { return m_asyncLoadState; }

	public:
		void SetAsyncLoadState(AsyncLoadState newState);
		void SetName(std::string const& name);
		void SetMemoryUsage(int32_t size);
		void ResetUseTimer();

	public:
		bool Load(std::ifstream& fileStream);
		virtual bool Save(std::ofstream& fileStream) const;

		virtual bool BeginLoad() = 0;
		virtual bool EndLoad() = 0;

		bool LoadFile(std::string const& fileName);
		virtual bool SaveFile(std::string const& filename) const;

	private:
		/// Name.
		std::string m_name;
		Core::StringHash m_nameHash;
		Core::StopWatch m_useTimer;
		int32_t m_memoryUsage;
		AsyncLoadState m_asyncLoadState;
	};


	inline const std::string& GetAssetName(Asset* asset)
	{
		return asset ? asset->GetName() : "";
	}

	inline Core::StringHash GetAssetType(Asset* asset, Core::StringHash defaultType)
	{
		return asset ? asset->GetType() : defaultType;
	}
#if false
	inline ResourceRef GetResourceRef(Resource* resource, StringHash defaultType)
	{
		return ResourceRef(GetResourceType(resource, defaultType), GetResourceName(resource));
	}
#endif
	template <class T> Core::FlexArray<std::string> GetResourceNames(Core::Span<std::shared_ptr<T>> assets)
	{
		Core::FlexArray<std::string> ret(assets.Size());
		for (size_t i = 0; i < assets.Size(); ++i)
			ret[i] = GetResourceName(assets[i]);

		return ret;
	}

#if false
	template <class T> ResourceRefList GetResourceRefList(Core::Span<std::shared_ptr<T>> assets)
	{
		return ResourceRefList(T::GetTypeStatic(), GetResourceNames(resources));
	}
#endif
}