#include "PhxData/PhxData_pch.h"
#include "DataSerializer.h"

#include <PhxCore/VFS.h>

void phx::data::Save(IFileSystem* fs, const char* filename, data::AnyPtr object)
{
    if (object.type_id == Neat::get_id<int>())
    {
        return *static_cast<int*>(object.value_ptr);
    }
    else if (object.type_id == Neat::get_id<float>())
    {
        return *static_cast<float*>(object.value_ptr);
    }
    else if (object.type_id == Neat::get_id<double>())
    {
        return *static_cast<double*>(object.value_ptr);
    }
    else if (object.type_id == Neat::get_id<std::string>())
    {
        return *static_cast<std::string*>(object.value_ptr);
    }
    else
    {
        return json{};
    }
}

void phx::data::Load(IFileSystem* fs, const char* filename, data::AnyPtr object)
{
    if (data.is_null())
    {
        return;
    }

    Neat::Any value{};

    if (field.type == Neat::get_id<int>())
    {
        value = data.get<int>();
    }
    else if (field.type == Neat::get_id<bool>())
    {
        value = data.get<bool>();
    }
    else if (field.type == Neat::get_id<float>())
    {
        value = data.get<float>();
    }
    else if (field.type == Neat::get_id<double>())
    {
        value = data.get<double>();
    }
    else if (field.type == Neat::get_id<std::string>())
    {
        value = data.get<std::string>();
    }
    //else if (data.is_array()) {
    //    std::vector<std::any> vec;
    //    for (const auto& element : data) {
    //        vec.push_back(jsonToAny(element));
    //    }
    //    return vec;
    //}
    //else if (data.is_object()) {
    //    std::map<std::string, std::any> obj;
    //    for (auto it = data.begin(); it != data.end(); ++it) {
    //        obj[it.key()] = jsonToAny(it.value());
    //    }
    //    return obj;
    //}

    field.set_value(object, value);
}
