#pragma once

#include <PhxCore/Memory.h>

namespace phx
{
    // ArrayAligned ///////////////////////////////////////////////////////
    template <typename T>
    struct Array 
    {
    public:
        Array();
        ~Array();

        void Intialize(phx::IAllocator* Allocator, uint32_t initial_capacity, uint32_t initial_Size = 0);
        void Finalize();

        void Push(const T& element);
        void Push(T&& value);

        template<typename... Args>
        T& Emplace(Args&&... args);

        T& PushUse(); // Grow the Size and return T to be filled.

        void Pop();
        void DeleteSwap(uint32_t index);

        T& operator[](uint32_t index);
        const T& operator[](uint32_t index) const;

        void Clear();
        void SetSize(uint32_t new_size);
        void SetCapacity(uint32_t new_capacity);
        void Grow(uint32_t new_capacity);

        T& Back();
        const T& Back() const;

        T& Front();
        const T& Front() const;

        uint32_t SizeInBytes() const;
        uint32_t CapacityInBytes() const;


        T* begin() { return Data; }
        T* end() { return Data + Size; }
        const T* begin() const { return Data; }
        const T* end()   const { return Data + Size; }

        bool IsEmpty() const { return Size == 0; }
        bool IsNotEmpty() const { return !IsEmpty(); }

        T* Data;
        uint32_t Size;       // Occupied Size
        uint32_t Capacity;   // Allocated capacity
        IAllocator* Allocator;

    }; // struct Array


    template <typename T, uint32_t N>
    struct FixedArray
    {
        T Data[N];

        T* begin() { return Data; }
        T* end() { return Data + N; }
        const T* begin() const { return Data; }
        const T* end()   const { return Data + N; }

        T& operator[](uint32_t index) { return Data[index]; }
        const T& operator[](uint32_t index) const { return Data[index]; }
    };

    // Implementation /////////////////////////////////////////////////////

    // ArrayAligned ///////////////////////////////////////////////////////
    template<typename T>
    inline Array<T>::Array() {
        //PHX_CORE_ASSERT( true );
    }

    template<typename T>
    inline Array<T>::~Array() {
        //PHX_CORE_ASSERT( data == nullptr );
    }

    template<typename T>
    inline void Array<T>::Intialize(phx::IAllocator* allocator, uint32_t initial_capacity, uint32_t initial_Size)
    {
        Data = nullptr;
        Size = initial_Size;
        Capacity = 0;
        Allocator = allocator;

        if (initial_capacity > 0)
        {
            Grow(initial_capacity);
        }
    }

    template<typename T>
    inline void Array<T>::Finalize()
    {
        if (Capacity > 0)
        {
            Allocator->Deallocate(Data);
        }

        Data = nullptr;
        Size = Capacity = 0;
    }

    template<typename T>
    inline void Array<T>::Push(const T& element) 
    {
        if (Size >= Capacity)
        {
            Grow(Capacity ? Capacity * 2 : 4);
        }

        new (&Data[Size++]) T(element);
    }

    template<typename T>
    inline void Array<T>::Push(T&& value)
    {
        if (Size == Capacity)
            Grow(Capacity ? Capacity * 2 : 4);

        new (&Data[Size++]) T(std::move(value));
    }

    template<typename T>
    inline T& Array<T>::PushUse()
    {
        if (Size >= Capacity) 
        {
            Grow(Capacity ? Capacity * 2 : 4);
        }
        ++Size;

        return Back();
    }

    template<typename T>
    inline void Array<T>::Pop() 
    {
        PHX_CORE_ASSERTm_(Size > 0);
        --Size;
    }

    template<typename T>
    inline void Array<T>::DeleteSwap(uint32_t index) 
    {
        PHX_CORE_ASSERT(Size > 0 && index < Size);
        Data[index] = Data[--Size];
    }

    template<typename T>
    inline T& Array<T>::operator [](uint32_t index) 
    {
        PHX_CORE_ASSERT(index < Size);
        return Data[index];
    }

    template<typename T>
    inline const T& Array<T>::operator [](uint32_t index) const
    {
        PHX_CORE_ASSERT(index < Size);
        return Data[index];
    }

    template<typename T>
    inline void Array<T>::Clear() 
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            for (size_t i = 0; i < Size; ++i)
                Data[i].~T();
        }
        Size = 0;
    }

    template<typename T>
    inline void Array<T>::SetSize(uint32_t new_size) 
    {
        if (new_size > Capacity)
        {
            grow(new_size);
        }
        Size = new_size;
    }

    template<typename T>
    inline void Array<T>::SetCapacity(uint32_t new_capacity) 
    {
        if (new_capacity > Capacity)
        {
            Grow(new_capacity);
        }
    }

    template<typename T>
    inline void Array<T>::Grow(uint32_t new_capacity) 
    {
        if (new_capacity < Capacity * 2)
        {
            new_capacity = Capacity * 2;
        }
        else if (new_capacity < 4) 
        {
            new_capacity = 4;
        }

        T* new_data = (T*)Allocator->Allocate(new_capacity * sizeof(T), alignof(T));
        if (Capacity)
        {
            std::memcpy(new_data, Data, Capacity * sizeof(T));

            Allocator->Deallocate(Data);
        }

        Data = new_data;
        Capacity = new_capacity;
    }

    template<typename T>
    inline T& Array<T>::Back() 
    {
        PHX_CORE_ASSERT(Size);
        return Data[Size - 1];
    }

    template<typename T>
    inline const T& Array<T>::Back() const 
    {
        PHX_CORE_ASSERT(Size);
        return Data[Size - 1];
    }

    template<typename T>
    inline T& Array<T>::Front() 
    {
        PHX_CORE_ASSERT(Size);
        return Data[0];
    }

    template<typename T>
    inline const T& Array<T>::Front() const 
    {
        PHX_CORE_ASSERT(Size);
        return Data[0];
    }

    template<typename T>
    inline uint32_t Array<T>::SizeInBytes() const 
    {
        return Size * sizeof(T);
    }

    template<typename T>
    inline uint32_t Array<T>::CapacityInBytes() const 
    {
        return Capacity * sizeof(T);
    }

    template<typename T>
    template<typename ...Args>
    inline T& Array<T>::Emplace(Args && ...args)
    {
        if (Size == Capacity)
            Grow(Capacity ? Capacity * 2 : 4);

        T* elem = new (&Data[Size]) T(std::forward<Args>(args)...);
        ++Size;
        return *elem;
    }
}