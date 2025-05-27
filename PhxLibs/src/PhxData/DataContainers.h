#pragma once

#include <cstring>
#include <iostream>
#include <cstddef>

namespace phx::data
{
    struct String
    {
        char* Data;
        size_t Length;

        String()
            : Data(nullptr)
            , Length(0) 
        {
        }

        String(const char* cstr)
        {
            if (cstr)
            {
                Length = std::strlen(cstr);
                Data = new char[Length + 1];
                std::memcpy(Data, cstr, Length + 1);
            }
            else
            {
                Data = nullptr;
                Length = 0;
            }
        }

        String(const String& other)
            : Length(other.Length)
        {
            Data = new char[Length + 1];
            std::memcpy(Data, other.Data, Length + 1);
        }

        String(String&& other) noexcept
            : Data(other.Data), Length(other.Length)
        {
            other.Data = nullptr;
            other.Length = 0;
        }

        ~String()
        {
            if (Data)
                delete[] Data;
        }

        String& operator=(const String& other)
        {
            if (this != &other)
            {
                if (Data)
                    delete[] Data;

                Length = other.Length;
                Data = new char[Length + 1];
                std::memcpy(Data, other.Data, Length + 1);
            }

            return *this;
        }

        String& operator=(String&& other) noexcept
        {
            if (this != &other)
            {
                if (Data)
                    delete[] Data;
                Data = other.Data;
                Length = other.Length;
                other.Data = nullptr;
                other.Length = 0;
            }
            return *this;
        }

        bool operator==(const String& other) const
        {
            if (Length != other.Length)
                return false;
            return std::memcmp(Data, other.Data, Length) == 0;
        }

        bool operator!=(const String& other) const
        {
            return !(*this == other);
        }

        friend std::ostream& operator<<(std::ostream& os, const String& str)
        {
            return os << (str.Data ? str.Data : "");
        }
    };

    template<typename T>
    struct FlexArray
    {
        T* Data;
        size_t Size;
        size_t Capacity;

        FlexArray()
            : Data(nullptr)
            , Size(0)
            , Capacity(0) 
        {
        }

        // Constructor with initial Capacity
        FlexArray(size_t initialCapacity)
            : Size(0)
            , Capacity(initialCapacity)
        {
            Data = new T[Capacity];
        }

        // Copy constructor
        FlexArray(const FlexArray& other)
            : Size(other.Size)
            , Capacity(other.Capacity)
        {
            Data = new T[Capacity];
            std::memcpy(Data, other.Data, sizeof(T) * Size);
        }

        // Move constructor
        FlexArray(FlexArray&& other) noexcept
            : Data(other.Data)
            , Size(other.Size)
            , Capacity(other.Capacity)
        {
            other.Data = nullptr;
            other.Size = 0;
            other.Capacity = 0;
        }

        // Destructor
        ~FlexArray()
        {
            if (Data)
                delete[] Data;
        }

        // Copy assignment
        FlexArray& operator=(const FlexArray& other)
        {
            if (this != &other)
            {
                if (Data)
                    delete[] Data;

                Size = other.Size;
                Capacity = other.Capacity;
                Data = new T[Capacity];
                std::memcpy(Data, other.Data, sizeof(T) * Size);
            }
            return *this;
        }

        // Move assignment
        FlexArray& operator=(FlexArray&& other) noexcept
        {
            if (this != &other)
            {
                if (Data)
                    delete[] Data;

                Data = other.Data;
                Size = other.Size;
                Capacity = other.Capacity;

                other.Data = nullptr;
                other.Size = 0;
                other.Capacity = 0;
            }
            return *this;
        }

        // Indexing
        T& operator[](size_t index) { return Data[index]; }
        const T& operator[](size_t index) const { return Data[index]; }

        // Equality comparison
        bool operator==(const FlexArray& other) const
        {
            if (Size != other.Size)
                return false;

            for (size_t i = 0; i < Size; ++i)
            {
                if (!(Data[i] == other.Data[i]))
                    return false;
            }

            return true;
        }

        bool operator!=(const FlexArray& other) const
        {
            return !(*this == other);
        }

        // Append element (grows if needed)
        void push_back(const T& value)
        {
            if (Size == Capacity)
            {
                resize(Capacity > 0 ? Capacity * 2 : 4);
            }
            Data[Size++] = value;
        }

        void resize(size_t newCapacity)
        {
            T* newData = new T[newCapacity];
            if (Data)
            {
                std::memcpy(newData, Data, sizeof(T) * Size);
                delete[] Data;
            }

            Data = newData;
            Capacity = newCapacity;
        }

        // Print contents (requires T to be streamable)
        friend std::ostream& operator<<(std::ostream& os, const FlexArray& arr)
        {
            os << "[";
            for (size_t i = 0; i < arr.Size; ++i)
            {
                os << arr.Data[i];
                if (i + 1 < arr.Size)
                    os << ", ";
            }
            os << "]";
            return os;
        }
    };
}
