#pragma once

#include <utility>      // For std::move, std::forward, std::in_place_t
#include <new>          // For placement new
#include <stdexcept>    // For std::logic_error (optional, for bad access)
#include <type_traits>  // For std::aligned_union, std::is_nothrow_constructible, etc.
#include <string>       // For example usage with std::string
#include <iostream>     // For example usage output
#include <cstdint>      // For uint64_t

namespace phx
{ // Namespace changed to phx

// --- Forward Declarations for Tag Types ---
    struct SuccessTag {};
    struct ErrorTag {};

    // Global instances of the tags for convenience
    constexpr SuccessTag success_tag{};
    constexpr ErrorTag error_tag{};

    // Example error enum (can be used if a specific error type other than uint64_t is needed)
    enum class OperationError
    {
        None, // Represents success if used as an error code, or a generic "no specific error"
        Failure,
        NotFound,
        AccessDenied,
        InvalidParameter,
        Timeout
        // Add more generic or specific error codes as needed
    };


    template <typename E>
    class Unexpected
    {
    public:
        explicit Unexpected(const E& error) : m_error(error) {}
        explicit Unexpected(E&& error) : m_error(std::move(error)) {}

        const E& error() const& { return m_error; }
        E& error()& { return m_error; }
        const E&& error() const&& { return std::move(m_error); }
        E&& error()&& { return std::move(m_error); }

    private:
        E m_error;
    };

    template <typename E>
    Unexpected<E> make_unexpected(E&& error)
    {
        return Unexpected<E>(std::forward<E>(error));
    }


    template <typename T, typename E = uint64_t> // uint64_t is now the default error type
    class Result
    {
    public:
        using ValueType = T;
        using ErrorType = E;
        using UnexpectedType = Unexpected<E>;

    private:
        union Storage
        {
            ValueType m_value;
            ErrorType m_error;

            Storage() {}
            ~Storage() {}
        };

        Storage m_storage;
        bool m_has_value;

        // --- Private Helper Methods ---
        template <typename... Args>
        void construct_value(Args&&... args)
        {
            new (&m_storage.m_value) ValueType(std::forward<Args>(args)...);
            m_has_value = true;
        }

        template <typename... Args>
        void construct_error(Args&&... args)
        {
            new (&m_storage.m_error) ErrorType(std::forward<Args>(args)...);
            m_has_value = false;
        }

        void destroy_value()
        {
            if (m_has_value)
            {
                m_storage.m_value.~ValueType();
            }
        }

        void destroy_error()
        {
            if (!m_has_value)
            {
                m_storage.m_error.~ErrorType();
            }
        }

        void destroy_current()
        {
            if (m_has_value)
            {
                destroy_value();
            }
            else
            {
                destroy_error();
            }
        }

    public:
        // --- Constructors ---

        // Default constructor: initializes to an error state with a default-constructed ErrorType (e.g., 0 for uint64_t)
        Result() noexcept(std::is_nothrow_default_constructible_v<ErrorType>)
        {
            construct_error(); // Constructs ErrorType()
        }

        // Construct with a value using SuccessTag (for explicit success construction or in-place)
        template <typename... Args,
            typename = std::enable_if_t<std::is_constructible_v<ValueType, Args...>>>
        Result(SuccessTag, Args&&... args) noexcept(std::is_nothrow_constructible_v<ValueType, Args...>)
        {
            construct_value(std::forward<Args>(args)...);
        }

        // Implicit conversion from T (if T is not Result itself or Unexpected)
        template <typename U = ValueType,
            typename = std::enable_if_t<
            std::is_convertible_v<U, ValueType> &&
            !std::is_same_v<std::decay_t<U>, Result<ValueType, ErrorType>> &&
            !std::is_same_v<std::decay_t<U>, UnexpectedType> &&
            !std::is_same_v<std::decay_t<U>, SuccessTag> &&
            !std::is_same_v<std::decay_t<U>, ErrorTag>
            >>
            Result(U&& value) noexcept(std::is_nothrow_constructible_v<ValueType, U>)
            : m_has_value(true)
        {
            new (&m_storage.m_value) ValueType(std::forward<U>(value));
        }


        // Construct with an error (via Unexpected<E>)
        Result(const UnexpectedType& unexpected) noexcept(std::is_nothrow_copy_constructible_v<ErrorType>)
            : m_has_value(false)
        {
            new (&m_storage.m_error) ErrorType(unexpected.error());
        }

        Result(UnexpectedType&& unexpected) noexcept(std::is_nothrow_move_constructible_v<ErrorType>)
            : m_has_value(false)
        {
            new (&m_storage.m_error) ErrorType(std::move(unexpected.error()));
        }

        // Construct with an error using ErrorTag (for explicit error construction or in-place)
        template <typename... Args,
            typename = std::enable_if_t<std::is_constructible_v<ErrorType, Args...>>>
        Result(ErrorTag, Args&&... args) noexcept(std::is_nothrow_constructible_v<ErrorType, Args...>)
        {
            construct_error(std::forward<Args>(args)...);
        }


        // --- Copy and Move Semantics ---
        Result(const Result& other) noexcept(
            std::is_nothrow_copy_constructible_v<ValueType>&&
            std::is_nothrow_copy_constructible_v<ErrorType>)
            : m_has_value(other.m_has_value)
        {
            if (m_has_value)
            {
                new (&m_storage.m_value) ValueType(other.m_storage.m_value);
            }
            else
            {
                new (&m_storage.m_error) ErrorType(other.m_storage.m_error);
            }
        }

        Result(Result&& other) noexcept(
            std::is_nothrow_move_constructible_v<ValueType>&&
            std::is_nothrow_move_constructible_v<ErrorType>)
            : m_has_value(other.m_has_value)
        {
            if (m_has_value)
            {
                new (&m_storage.m_value) ValueType(std::move(other.m_storage.m_value));
            }
            else
            {
                new (&m_storage.m_error) ErrorType(std::move(other.m_storage.m_error));
            }
        }

        Result& operator=(const Result& other) noexcept(
            std::is_nothrow_copy_constructible_v<ValueType>&& std::is_nothrow_copy_assignable_v<ValueType>&&
            std::is_nothrow_copy_constructible_v<ErrorType>&& std::is_nothrow_copy_assignable_v<ErrorType>)
        {
            if (this == &other) return *this;
            destroy_current();
            m_has_value = other.m_has_value;
            if (m_has_value)
            {
                new (&m_storage.m_value) ValueType(other.m_storage.m_value);
            }
            else
            {
                new (&m_storage.m_error) ErrorType(other.m_storage.m_error);
            }
            return *this;
        }

        Result& operator=(Result&& other) noexcept(
            std::is_nothrow_move_constructible_v<ValueType>&& std::is_nothrow_move_assignable_v<ValueType>&&
            std::is_nothrow_move_constructible_v<ErrorType>&& std::is_nothrow_move_assignable_v<ErrorType>)
        {
            if (this == &other) return *this;
            destroy_current();
            m_has_value = other.m_has_value;
            if (m_has_value)
            {
                new (&m_storage.m_value) ValueType(std::move(other.m_storage.m_value));
            }
            else
            {
                new (&m_storage.m_error) ErrorType(std::move(other.m_storage.m_error));
            }
            return *this;
        }

        // --- Destructor ---
        ~Result()
        {
            destroy_current();
        }

        // --- Observers ---
        bool HasValue() const noexcept { return m_has_value; }
        explicit operator bool() const noexcept { return m_has_value; }
        bool HasError() const noexcept { return !m_has_value; }

        // --- Value Accessors ---
        const ValueType& GetValue() const&
        {
            if (!m_has_value) throw std::logic_error("Result does not contain a value (GetValue const&)");
            return m_storage.m_value;
        }
        ValueType& GetValue()&
        {
            if (!m_has_value) throw std::logic_error("Result does not contain a value (GetValue &)");
            return m_storage.m_value;
        }
        const ValueType&& GetValue() const&&
        {
            if (!m_has_value) throw std::logic_error("Result does not contain a value (GetValue const&&)");
            return std::move(m_storage.m_value);
        }
        ValueType&& GetValue()&&
        {
            if (!m_has_value) throw std::logic_error("Result does not contain a value (GetValue &&)");
            return std::move(m_storage.m_value);
        }

        const ValueType* operator->() const
        {
            if (!m_has_value) throw std::logic_error("Result does not contain a value (operator-> const)");
            return &m_storage.m_value;
        }
        ValueType* operator->()
        {
            if (!m_has_value) throw std::logic_error("Result does not contain a value (operator->)");
            return &m_storage.m_value;
        }

        const ValueType& operator*() const& { return GetValue(); }
        ValueType& operator*()& { return GetValue(); }
        const ValueType&& operator*() const&& { return std::move(GetValue()); }
        ValueType&& operator*()&& { return std::move(GetValue()); }

        // --- Error Accessors ---
        const ErrorType& GetError() const&
        {
            if (m_has_value) throw std::logic_error("Result does not contain an error (GetError const&)");
            return m_storage.m_error;
        }
        ErrorType& GetError()&
        {
            if (m_has_value) throw std::logic_error("Result does not contain an error (GetError &)");
            return m_storage.m_error;
        }
        const ErrorType&& GetError() const&&
        {
            if (m_has_value) throw std::logic_error("Result does not contain an error (GetError const&&)");
            return std::move(m_storage.m_error);
        }
        ErrorType&& GetError()&&
        {
            if (m_has_value) throw std::logic_error("Result does not contain an error (GetError &&)");
            return std::move(m_storage.m_error);
        }

        // --- Value Or Default ---
        template <typename U>
        ValueType ValueOr(U&& default_value) const&
        {
            return m_has_value ? m_storage.m_value : static_cast<ValueType>(std::forward<U>(default_value));
        }
        template <typename U>
        ValueType ValueOr(U&& default_value)&&
        {
            return m_has_value ? std::move(m_storage.m_value) : static_cast<ValueType>(std::forward<U>(default_value));
        }

        // --- Emplace ---
        template <typename... Args>
        ValueType& EmplaceValue(Args&&... args) noexcept(std::is_nothrow_constructible_v<ValueType, Args...>)
        {
            destroy_current();
            construct_value(std::forward<Args>(args)...);
            return m_storage.m_value;
        }

        template <typename... Args>
        ErrorType& EmplaceError(Args&&... args) noexcept(std::is_nothrow_constructible_v<ErrorType, Args...>)
        {
            destroy_current();
            construct_error(std::forward<Args>(args)...);
            return m_storage.m_error;
        }
    };


    // Helper functions to create Result objects
    // Now also defaults E to uint64_t if not specified
    template <typename T, typename... Args, typename E = uint64_t>
    Result<T, E> make_success(Args&&... args)
    {
        return Result<T, E>(success_tag, std::forward<Args>(args)...);
    }

    template <typename T, typename... Args, typename E = uint64_t>
    Result<T, E> make_error(Args&&... args)
    {
        return Result<T, E>(error_tag, std::forward<Args>(args)...);
    }


    // Specialization for Result<void, E> (operations that succeed without a value)
    // E will also default to uint64_t if not specified, due to the primary template's default
    template <typename E>
    class Result<void, E>
    {
    public:
        using ValueType = void;
        using ErrorType = E;
        using UnexpectedType = Unexpected<E>;

    private:
        union Storage
        {
            ErrorType m_error;
            Storage() {}
            ~Storage() {}
        };
        Storage m_storage;
        bool m_has_value;

        template <typename... Args>
        void construct_error(Args&&... args)
        {
            new (&m_storage.m_error) ErrorType(std::forward<Args>(args)...);
            m_has_value = false;
        }
        void destroy_error()
        {
            if (!m_has_value)
            {
                m_storage.m_error.~ErrorType();
            }
        }

    public:
        // Default constructor: initializes to an error state with a default-constructed ErrorType (e.g. 0 for uint64_t)
        Result() noexcept(std::is_nothrow_default_constructible_v<ErrorType>)
        {
            construct_error(); // Constructs ErrorType()
        }

        // Construct for success (void) using SuccessTag
        Result(SuccessTag) noexcept : m_has_value(true) {}

        // Construct with an error (via Unexpected<E>)
        Result(const UnexpectedType& unexpected) noexcept(std::is_nothrow_copy_constructible_v<ErrorType>)
            : m_has_value(false)
        {
            new (&m_storage.m_error) ErrorType(unexpected.error());
        }
        Result(UnexpectedType&& unexpected) noexcept(std::is_nothrow_move_constructible_v<ErrorType>)
            : m_has_value(false)
        {
            new (&m_storage.m_error) ErrorType(std::move(unexpected.error()));
        }
        // Construct with an error (directly, using ErrorTag for disambiguation)
        template <typename... Args,
            typename = std::enable_if_t<std::is_constructible_v<ErrorType, Args...>>>
        Result(ErrorTag, Args&&... args) noexcept(std::is_nothrow_constructible_v<ErrorType, Args...>)
        {
            construct_error(std::forward<Args>(args)...);
        }


        Result(const Result& other) noexcept(std::is_nothrow_copy_constructible_v<ErrorType>)
            : m_has_value(other.m_has_value)
        {
            if (!m_has_value)
            {
                new (&m_storage.m_error) ErrorType(other.m_storage.m_error);
            }
        }
        Result(Result&& other) noexcept(std::is_nothrow_move_constructible_v<ErrorType>)
            : m_has_value(other.m_has_value)
        {
            if (!m_has_value)
            {
                new (&m_storage.m_error) ErrorType(std::move(other.m_storage.m_error));
            }
        }
        Result& operator=(const Result& other) noexcept(
            std::is_nothrow_copy_constructible_v<ErrorType>&& std::is_nothrow_copy_assignable_v<ErrorType>)
        {
            if (this == &other) return *this;
            if (!m_has_value) destroy_error();
            m_has_value = other.m_has_value;
            if (!m_has_value)
            {
                new (&m_storage.m_error) ErrorType(other.m_storage.m_error);
            }
            return *this;
        }
        Result& operator=(Result&& other) noexcept(
            std::is_nothrow_move_constructible_v<ErrorType>&& std::is_nothrow_move_assignable_v<ErrorType>)
        {
            if (this == &other) return *this;
            if (!m_has_value) destroy_error();
            m_has_value = other.m_has_value;
            if (!m_has_value)
            {
                new (&m_storage.m_error) ErrorType(std::move(other.m_storage.m_error));
            }
            return *this;
        }

        ~Result()
        {
            if (!m_has_value)
            {
                destroy_error();
            }
        }

        bool HasValue() const noexcept { return m_has_value; }
        explicit operator bool() const noexcept { return m_has_value; }
        bool HasError() const noexcept { return !m_has_value; }

        void GetValue() const
        {
            if (!m_has_value) throw std::logic_error("Result<void, E> does not contain a value (it's an error)");
        }
        void operator*() const { GetValue(); }


        const ErrorType& GetError() const&
        {
            if (m_has_value) throw std::logic_error("Result<void, E> does not contain an error (it's a success)");
            return m_storage.m_error;
        }
        ErrorType& GetError()&
        {
            if (m_has_value) throw std::logic_error("Result<void, E> does not contain an error (it's a success)");
            return m_storage.m_error;
        }
        const ErrorType&& GetError() const&&
        {
            if (m_has_value) throw std::logic_error("Result<void, E> does not contain an error (it's a success)");
            return std::move(m_storage.m_error);
        }
        ErrorType&& GetError()&&
        {
            if (m_has_value) throw std::logic_error("Result<void, E> does not contain an error (it's a success)");
            return std::move(m_storage.m_error);
        }

        template <typename... Args>
        void EmplaceValue() noexcept
        { // For void, just set to success
            if (!m_has_value) destroy_error();
            m_has_value = true;
        }

        template <typename... Args>
        ErrorType& EmplaceError(Args&&... args) noexcept(std::is_nothrow_constructible_v<ErrorType, Args...>)
        {
            if (!m_has_value) destroy_error();
            construct_error(std::forward<Args>(args)...);
            return m_storage.m_error;
        }
    };

    // --- Example Usage ---

    /*
    // Example function using default error type (uint64_t)
    phx::Result<std::string> ParseData(const std::string& input)
    {
        if (input == "valid_data")
        {
            return std::string("Parsed: Some data"); // Implicitly uses uint64_t as E
        }
        else if (input == "empty_data")
        {
            return phx::make_unexpected(1001ULL); // Error code 1001
        }
        else if (input == "another_success")
        {
            return phx::MakeSuccess<std::string>("Another success string"); // E defaults to uint64_t
        }
        // Default constructor of Result<std::string> (which is Result<std::string, uint64_t>)
        // will create a default error (0 for uint64_t).
        // For clarity, explicitly return a specific error:
        return phx::make_unexpected(0ULL); // Or some other defined error code like a static const
    }

    // Example function using a custom error type (OperationError enum)
    phx::Result<int, OperationError> PerformOperation(int value)
    {
        if (value < 0)
        {
            return phx::make_unexpected(OperationError::InvalidParameter);
        }
        if (value == 0)
        {
            return phx::make_unexpected(OperationError::Failure);
        }
        // Success
        return value * 2;
    }


    phx::Result<void> PerformVoidOperation(bool should_succeed)
    {
        if (should_succeed)
        {
            std::cout << "PerformVoidOperation: Succeeded (simulated)." << std::endl;
            return phx::Result<void>(phx::success_tag); // Explicit success, E defaults to uint64_t
        }
        else
        {
            std::cout << "PerformVoidOperation: Failed (simulated)." << std::endl;
            return phx::make_unexpected(500ULL); // Error code 500
        }
    }

    // To run the example:
    int main()
    {
        std::cout << "--- Testing Result<std::string> (defaulting to uint64_t error) ---" << std::endl;

        phx::Result<std::string> default_constructed_res_val; // Defaults to error (0 for uint64_t)
        if (default_constructed_res_val.HasError())
        {
            std::cout << "default_constructed_res_val Error: "
                      << default_constructed_res_val.GetError()
                      << " (Expected default uint64_t error, typically 0)" << std::endl;
        }

        phx::Result<std::string> res1 = ParseData("valid_data");
        if (res1)
        {
            std::cout << "res1 (valid_data) Value: " << *res1 << std::endl;
        }

        phx::Result<std::string> res2 = ParseData("empty_data");
        if (!res2)
        {
            std::cout << "res2 (empty_data) Error Code: " << res2.GetError() << std::endl;
        }

        phx::Result<std::string> res3 = ParseData("unknown_data");
        std::cout << "res3 (unknown_data) ValueOrDefault: " << res3.ValueOr("Default For Unknown") << std::endl;
         if (res3.HasError())
         {
            std::cout << "res3 (unknown_data) Error Code: " << res3.GetError() << std::endl;
        }

        std::cout << "\n--- Testing Result<int, OperationError> (custom error type) ---" << std::endl;
        phx::Result<int, OperationError> op_res1 = PerformOperation(10);
        if (op_res1)
        {
            std::cout << "op_res1 (10) Value: " << *op_res1 << std::endl;
        }

        phx::Result<int, OperationError> op_res2 = PerformOperation(-5);
        if (!op_res2)
        {
            std::cout << "op_res2 (-5) Error: " << static_cast<int>(op_res2.GetError()) << std::endl;
        }

        phx::Result<int, OperationError> op_res3_default; // Defaults to OperationError()
         if (op_res3_default.HasError())
        {
            std::cout << "op_res3_default Error: "
                      << static_cast<int>(op_res3_default.GetError())
                      << " (Expected default OperationError)" << std::endl;
        }


        std::cout << "\n--- Testing Result<void> (defaulting to uint64_t error) ---" << std::endl;

        phx::Result<void> void_op1 = PerformVoidOperation(true);
        if (void_op1)
        {
            std::cout << "void_op1 (true) Succeeded!" << std::endl;
        }

        phx::Result<void> void_op2 = PerformVoidOperation(false);
        if (!void_op2)
        {
            std::cout << "void_op2 (false) Failed with error code: " << void_op2.GetError() << std::endl;
        }

        phx::Result<void> default_constructed_void_res; // Defaults to error (0 for uint64_t)
        if (default_constructed_void_res.HasError())
        {
            std::cout << "default_constructed_void_res Error: "
                      << default_constructed_void_res.GetError()
                      << " (Expected default uint64_t error, typically 0)" << std::endl;
        }


        std::cout << "\n--- Testing Emplace ---" << std::endl;
        res1.EmplaceError(9999ULL); // E is uint64_t
        if(res1.HasError())
        {
            std::cout << "res1 (emplaced error) Error Code: " << res1.GetError() << std::endl;
        }
        res1.EmplaceValue("New emplaced content after uint64_t error!");
        if(res1.HasValue())
        {
            std::cout << "res1 (emplaced value) Value: " << res1.GetValue() << std::endl;
        }

        op_res1.EmplaceError(OperationError::Timeout);
        if(op_res1.HasError())
        {
             std::cout << "op_res1 (emplaced error) Error: " << static_cast<int>(op_res1.GetError()) << std::endl;
        }


        return 0;
    }
    */

} // namespace phx
