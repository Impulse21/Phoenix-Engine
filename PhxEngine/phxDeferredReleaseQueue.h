#pragma once

#include <functional>

#include "phxDisplay.h"

namespace phx
{


	struct DeleteItem
	{
		uint64_t Frame;
		std::function<void()> DeleteFn;
	};

	//////////////////////////////////////////////////////////////////////////
	// DeferredReleasePtr
	// Mostly a copy of Microsoft::WRL::ComPtr<T>
	//////////////////////////////////////////////////////////////////////////


	namespace DeferredDeleteQueue
	{
		void Enqueue(DeleteItem&& deleteItem);
		void ReleaseItems(uint64_t completedFrame = std::numeric_limits<uint64_t>::max());
	}

	template <typename T>
	class DeferredReleasePtr
	{
	public:
		typedef T InterfaceType;

		template <bool b, typename U = void>
		struct EnableIf
		{
		};

		template <typename U>
		struct EnableIf<true, U>
		{
			typedef U type;
		};

	protected:
		InterfaceType* ptr_;
		template<class U> friend class DeferredReleasePtr;

		void InternalAddRef() const noexcept
		{
			if (ptr_ != nullptr)
			{
				ptr_->AddRef();
			}
		}

		void InternalRelease() noexcept
		{
			T* temp = ptr_;

			if (temp != nullptr)
			{
				ptr_ = nullptr;
				DeferredDeleteQueue::Enqueue(
					{
						.Frame = gfx::GetFrameCount(),
						.DeleteFn = [temp]()
						{
								temp->Release();
						}
					});
			}
		}

	public:

		DeferredReleasePtr() noexcept : ptr_(nullptr)
		{
		}

		DeferredReleasePtr(std::nullptr_t) noexcept : ptr_(nullptr)
		{
		}

		template<class U>
		DeferredReleasePtr(U* other) noexcept : ptr_(other)
		{
			InternalAddRef();
		}

		DeferredReleasePtr(const DeferredReleasePtr& other) noexcept : ptr_(other.ptr_)
		{
			InternalAddRef();
		}

		// copy ctor that allows to instanatiate class when U* is convertible to T*
		template<class U>
		DeferredReleasePtr(const DeferredReleasePtr<U>& other, typename std::enable_if<std::is_convertible<U*, T*>::value, void*>::type* = nullptr) noexcept :
			ptr_(other.ptr_)

		{
			InternalAddRef();
		}

		DeferredReleasePtr(DeferredReleasePtr&& other) noexcept : ptr_(nullptr)
		{
			if (this != reinterpret_cast<DeferredReleasePtr*>(&reinterpret_cast<unsigned char&>(other)))
			{
				Swap(other);
			}
		}

		// Move ctor that allows instantiation of a class when U* is convertible to T*
		template<class U>
		DeferredReleasePtr(DeferredReleasePtr<U>&& other, typename std::enable_if<std::is_convertible<U*, T*>::value, void*>::type* = nullptr) noexcept :
			ptr_(other.ptr_)
		{
			other.ptr_ = nullptr;
		}

		~DeferredReleasePtr() noexcept
		{
			InternalRelease();
		}

		DeferredReleasePtr& operator=(std::nullptr_t) noexcept
		{
			InternalRelease();
			return *this;
		}

		DeferredReleasePtr& operator=(T* other) noexcept
		{
			if (ptr_ != other)
			{
				DeferredReleasePtr(other).Swap(*this);
			}
			return *this;
		}

		template <typename U>
		DeferredReleasePtr& operator=(U* other) noexcept
		{
			DeferredReleasePtr(other).Swap(*this);
			return *this;
		}

		DeferredReleasePtr& operator=(const DeferredReleasePtr& other) noexcept  // NOLINT(bugprone-unhandled-self-assignment)
		{
			if (ptr_ != other.ptr_)
			{
				DeferredReleasePtr(other).Swap(*this);
			}
			return *this;
		}

		template<class U>
		DeferredReleasePtr& operator=(const DeferredReleasePtr<U>& other) noexcept
		{
			DeferredReleasePtr(other).Swap(*this);
			return *this;
		}

		DeferredReleasePtr& operator=(DeferredReleasePtr&& other) noexcept
		{
			DeferredReleasePtr(static_cast<DeferredReleasePtr&&>(other)).Swap(*this);
			return *this;
		}

		template<class U>
		DeferredReleasePtr& operator=(DeferredReleasePtr<U>&& other) noexcept
		{
			DeferredReleasePtr(static_cast<DeferredReleasePtr<U>&&>(other)).Swap(*this);
			return *this;
		}

		void Swap(DeferredReleasePtr&& r) noexcept
		{
			T* tmp = ptr_;
			ptr_ = r.ptr_;
			r.ptr_ = tmp;
		}

		void Swap(DeferredReleasePtr& r) noexcept
		{
			T* tmp = ptr_;
			ptr_ = r.ptr_;
			r.ptr_ = tmp;
		}

		[[nodiscard]] T* Get() const noexcept
		{
			return ptr_;
		}

		operator T* () const
		{
			return ptr_;
		}

		InterfaceType* operator->() const noexcept
		{
			return ptr_;
		}

		T** operator&()   // NOLINT(google-runtime-operator)
		{
			return &ptr_;
		}

		[[nodiscard]] T* const* GetAddressOf() const noexcept
		{
			return &ptr_;
		}

		[[nodiscard]] T** GetAddressOf() noexcept
		{
			return &ptr_;
		}

		[[nodiscard]] T** ReleaseAndGetAddressOf() noexcept
		{
			InternalRelease();
			return &ptr_;
		}

		T* Detach() noexcept
		{
			T* ptr = ptr_;
			ptr_ = nullptr;
			return ptr;
		}

		// Set the pointer while keeping the object's reference count unchanged
		void Attach(InterfaceType* other)
		{
			if (ptr_ != nullptr)
			{
				auto ref = ptr_->Release();
				(void)ref;

				// Attaching to the same object only works if duplicate references are being coalesced. Otherwise
				// re-attaching will cause the pointer to be released and may cause a crash on a subsequent dereference.
				assert(ref != 0 || ptr_ != other);
			}

			ptr_ = other;
		}

		// Create a wrapper around a raw object while keeping the object's reference count unchanged
		static DeferredReleasePtr<T> Create(T* other)
		{
			DeferredReleasePtr<T> ptr;
			ptr.Attach(other);
			return ptr;
		}

		void Reset()
		{
			InternalRelease();
		}
	};    // DeferredReleasePtr
}
