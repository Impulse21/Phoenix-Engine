#pragma once

#include "SparseSet.h"

#include <concepts>

namespace phx::ecs
{
    template<typename T>
    concept HasRequired = requires { typename T::Required; };

    template<typename T>
    concept HasStorageType = requires { typename T::StorageType; };

    // -- Default storage type ---
    template<typename T>
    struct StorageTypeOf
    {
        using type = SparseSet<T>;
    };

    template<typename T>
        requires HasStorageType<T>
    struct StorageTypeOf<T>
    {
        using type = typename T::StorageType;
    };

    template<typename T>
    using StorageTypeOf_t = typename StorageTypeOf<T>::type;


    template<typename T>
    concept SparseDriver = std::is_same_v<StorageTypeOf_t<T>, SparseSet<T>>;

}