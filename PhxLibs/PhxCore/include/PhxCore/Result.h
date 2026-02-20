#pragma once

#include <PhxCore/Assert.h>

#include <utility>
#include <new>
#include <type_traits>
#include <cstdint>

// NOTE: This class is AI Generated. Was to lazy to try and write my own.
namespace phx
{

    // -----------------------------------------------------------------------
    // Error codes - extend as needed
    // -----------------------------------------------------------------------
    enum class ResultError : uint32_t
    {
        None = 0,
        Failure,
        NotFound,
        AccessDenied,
        InvalidParameter,
        Timeout,
    };

    // -----------------------------------------------------------------------
    // Unexpected<E> - wraps an error value for unambiguous construction
    //
    // Usage:
    //   return phx::Unexpected(ResultError::NotFound);
    // -----------------------------------------------------------------------
    template <typename E>
    class Unexpected
    {
    public:
        explicit Unexpected(E error) noexcept(std::is_nothrow_move_constructible_v<E>)
            : m_error(std::move(error)) {}

        const E& Error() const& noexcept  { return m_error; }
        E&       Error() &      noexcept  { return m_error; }
        E&&      Error() &&     noexcept  { return std::move(m_error); }

    private:
        E m_error;
    };

    // Deduction guide so you can write Unexpected(ResultError::NotFound) without specifying E
    template <typename E>
    Unexpected(E) -> Unexpected<E>;


    // -----------------------------------------------------------------------
    // Result<T, E>
    //
    // Holds either a value (T) or an error (E). E defaults to ResultError.
    //
    // Common usage:
    //
    //   phx::Result<Window> CreateWindow(...)
    //   {
    //       if (failed) return phx::Unexpected(ResultError::Failure);
    //       return window;   // implicit success
    //   }
    //
    //   auto result = CreateWindow(...);
    //   if (!result)
    //       LOG_ERROR(result.GetError());
    //   Window w = result.GetValue();
    //
    // -----------------------------------------------------------------------
    template <typename T, typename E = ResultError>
    class Result
    {
    public:
        using ValueType     = T;
        using ErrorType     = E;
        using UnexpectedType = Unexpected<E>;

        // --- Success: implicit from T ---
        template <typename U = T,
            typename = std::enable_if_t<
                std::is_constructible_v<T, U&&> &&
                !std::is_same_v<std::decay_t<U>, Result> &&
                !std::is_same_v<std::decay_t<U>, Unexpected<E>>>>
        Result(U&& value) noexcept(std::is_nothrow_constructible_v<T, U>)
            : m_has_value(true)
        {
            new (&m_storage.value) T(std::forward<U>(value));
        }

        // --- Error: from Unexpected<E> ---
        Result(const UnexpectedType& u) noexcept(std::is_nothrow_copy_constructible_v<E>)
            : m_has_value(false)
        {
            new (&m_storage.error) E(u.Error());
        }

        Result(UnexpectedType&& u) noexcept(std::is_nothrow_move_constructible_v<E>)
            : m_has_value(false)
        {
            new (&m_storage.error) E(std::move(u).Error());
        }

        // --- Copy / Move ---
        Result(const Result& other) noexcept(
            std::is_nothrow_copy_constructible_v<T> &&
            std::is_nothrow_copy_constructible_v<E>)
            : m_has_value(other.m_has_value)
        {
            if (m_has_value)
                new (&m_storage.value) T(other.m_storage.value);
            else
                new (&m_storage.error) E(other.m_storage.error);
        }

        Result(Result&& other) noexcept(
            std::is_nothrow_move_constructible_v<T> &&
            std::is_nothrow_move_constructible_v<E>)
            : m_has_value(other.m_has_value)
        {
            if (m_has_value)
                new (&m_storage.value) T(std::move(other.m_storage.value));
            else
                new (&m_storage.error) E(std::move(other.m_storage.error));
        }

        Result& operator=(const Result& other)
        {
            if (this == &other) return *this;
            Destroy();
            m_has_value = other.m_has_value;
            if (m_has_value)
                new (&m_storage.value) T(other.m_storage.value);
            else
                new (&m_storage.error) E(other.m_storage.error);
            return *this;
        }

        Result& operator=(Result&& other) noexcept
        {
            if (this == &other) return *this;
            Destroy();
            m_has_value = other.m_has_value;
            if (m_has_value)
                new (&m_storage.value) T(std::move(other.m_storage.value));
            else
                new (&m_storage.error) E(std::move(other.m_storage.error));
            return *this;
        }

        ~Result() { Destroy(); }

        // --- Observers ---
        bool HasValue()  const noexcept { return m_has_value; }
        bool HasError()  const noexcept { return !m_has_value; }
        explicit operator bool() const noexcept { return m_has_value; }

        // --- Value access (asserts on bad access, no exceptions) ---
        const T& GetValue() const& noexcept
        {
            PHX_ASSERT(m_has_value, "Result: GetValue called on error state");
            return m_storage.value;
        }
        T& GetValue() & noexcept
        {
            PHX_ASSERT(m_has_value, "Result: GetValue called on error state");
            return m_storage.value;
        }
        T&& GetValue() && noexcept
        {
            PHX_ASSERT(m_has_value, "Result: GetValue called on error state");
            return std::move(m_storage.value);
        }

        const T& operator*()  const& noexcept { return GetValue(); }
        T&       operator*()  &      noexcept { return GetValue(); }
        T&&      operator*()  &&     noexcept { return std::move(GetValue()); }
        const T* operator->() const  noexcept { return &GetValue(); }
        T*       operator->()        noexcept { return &GetValue(); }

        // --- Error access ---
        const E& GetError() const& noexcept
        {
            PHX_ASSERT(!m_has_value, "Result: GetError called on success state");
            return m_storage.error;
        }
        E& GetError() & noexcept
        {
            PHX_ASSERT(!m_has_value, "Result: GetError called on success state");
            return m_storage.error;
        }

        // --- Value or default ---
        template <typename U>
        T ValueOr(U&& fallback) const&
        {
            return m_has_value ? m_storage.value : static_cast<T>(std::forward<U>(fallback));
        }
        template <typename U>
        T ValueOr(U&& fallback) &&
        {
            return m_has_value ? std::move(m_storage.value) : static_cast<T>(std::forward<U>(fallback));
        }

    private:
        union Storage
        {
            T value;
            E error;
            Storage() {}
            ~Storage() {}
        };

        Storage m_storage;
        bool    m_has_value;

        void Destroy() noexcept
        {
            if (m_has_value)
                m_storage.value.~T();
            else
                m_storage.error.~E();
        }
    };


    // -----------------------------------------------------------------------
    // Result<void, E>  - for operations that succeed without returning a value
    //
    // Usage:
    //   phx::Result<void> MountFilesystem(...)
    //   {
    //       if (failed) return phx::Unexpected(ResultError::Failure);
    //       return phx::Ok();   // explicit void success
    //   }
    //
    //   if (!MountFilesystem(...))
    //       LOG_ERROR("mount failed");
    // -----------------------------------------------------------------------

    // Tag for explicit void success
    struct OkTag {};
    inline constexpr OkTag Ok() { return {}; }

    template <typename E>
    class Result<void, E>
    {
    public:
        using ValueType      = void;
        using ErrorType      = E;
        using UnexpectedType = Unexpected<E>;

        // Success
        Result(OkTag) noexcept : m_has_value(true) {}

        // Error
        Result(const UnexpectedType& u) noexcept(std::is_nothrow_copy_constructible_v<E>)
            : m_has_value(false)
        {
            new (&m_storage.error) E(u.Error());
        }
        Result(UnexpectedType&& u) noexcept(std::is_nothrow_move_constructible_v<E>)
            : m_has_value(false)
        {
            new (&m_storage.error) E(std::move(u).Error());
        }

        Result(const Result& other) noexcept(std::is_nothrow_copy_constructible_v<E>)
            : m_has_value(other.m_has_value)
        {
            if (!m_has_value)
                new (&m_storage.error) E(other.m_storage.error);
        }
        Result(Result&& other) noexcept(std::is_nothrow_move_constructible_v<E>)
            : m_has_value(other.m_has_value)
        {
            if (!m_has_value)
                new (&m_storage.error) E(std::move(other.m_storage.error));
        }

        ~Result()
        {
            if (!m_has_value)
                m_storage.error.~E();
        }

        bool HasValue()  const noexcept { return m_has_value; }
        bool HasError()  const noexcept { return !m_has_value; }
        explicit operator bool() const noexcept { return m_has_value; }

        const E& GetError() const& noexcept
        {
            PHX_ASSERT(!m_has_value, "Result<void>: GetError called on success state");
            return m_storage.error;
        }
        E& GetError() & noexcept
        {
            PHX_ASSERT(!m_has_value, "Result<void>: GetError called on success state");
            return m_storage.error;
        }

    private:
        union Storage
        {
            E error;
            Storage() {}
            ~Storage() {}
        };

        Storage m_storage;
        bool    m_has_value;
    };

} // namespace phx


/*
===========================================================================
  USAGE EXAMPLES
===========================================================================

// -- Non-void success / error --
phx::Result<Window> CreateWindow(const WindowDesc& desc)
{
    GLFWwindow* w = glfwCreateWindow(...);
    if (!w)
        return phx::Unexpected(phx::ResultError::Failure);
    return MakeWindow(w);  // implicit success
}

auto result = CreateWindow(desc);
if (!result)
{
    PHX_CORE_ERROR("Window creation failed: {}", (uint32_t)result.GetError());
    return;
}
Window window = result.GetValue();  // or *result

// -- Void success / error --
phx::Result<void> Mount(const char* path)
{
    if (!valid)
        return phx::Unexpected(phx::ResultError::NotFound);
    return phx::Ok();
}

if (!Mount("res://"))
    PHX_CORE_ERROR("Mount failed");

// -- Custom error type --
phx::Result<int, MyError> Foo();

===========================================================================
*/
