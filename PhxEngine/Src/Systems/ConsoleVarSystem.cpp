#include "phxpch.h"
#include "PhxEngine/Systems/ConsoleVarSystem.h"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

using namespace PhxEngine;


enum class ConsoleVarType : uint8_t
{
    Integer,
    Float,
    String,
};

class ConsoleVarSystemImpl;

namespace PhxEngine
{
    struct ConsoleVarParameter
    {
        friend ConsoleVarSystemImpl;

        int32_t ArrayIndex;

        ConsoleVarType Type;
        ConsoleVarFlags Flags;
        std::string Name;
        std::string Description;
    };
}

template<typename T>
struct ConsoleVarStorage
{
    T Initial;
    T Current;
    ConsoleVarParameter* Parameter;
};

template<typename T>
struct ConsoleVarArray
{
    std::vector<ConsoleVarStorage<T>> ConsoleVars;
    int32_t LastConsoleVar{ 0 };

    ConsoleVarArray(size_t size)
    {
        ConsoleVars.resize(size);
    }

    T* GetCurrentPtr(int32_t index)
    {
        return &this->ConsoleVars[index].Current;
    };

    T GetCurrent(int32_t index)
    {
        return this->ConsoleVars[index].Current;
    };

    void SetCurrent(const T& val, int32_t index)
    {
        this->ConsoleVars[index].Current = val;
    }

    int Add(const T& value, ConsoleVarParameter* param)
    {
        return this->Add(value, value, param);
    }

    int Add(const T& initialValue, const T& currentValue, ConsoleVarParameter* param)
    {
        int index = LastConsoleVar;

        this->ConsoleVars[index].Current = currentValue;
        this->ConsoleVars[index].Initial = initialValue;
        this->ConsoleVars[index].Parameter = param;

        param->ArrayIndex = index;
        LastConsoleVar++;
        return index;
    }
};

class ConsoleVarSystemImpl : public ConsoleVarSystem
{
public:
    constexpr static int kMaxIntConsoleVars = 1000;
    ConsoleVarArray<int32_t> IntConsoleVars{ kMaxIntConsoleVars };

    constexpr static int kMaxFloatConsoleVars = 1000;
    ConsoleVarArray<double> FloatConsoleVars{ kMaxFloatConsoleVars };

    constexpr static int kMaxStringConsoleVars = 200;
    ConsoleVarArray<std::string> StringConsoleVars{ kMaxStringConsoleVars };

    template<typename T>
    ConsoleVarArray<T>* GetConsoleVarArray();

    template<>
    ConsoleVarArray<int32_t>* GetConsoleVarArray()
    {
        return &IntConsoleVars;
    }

    template<>
    ConsoleVarArray<double>* GetConsoleVarArray()
    {
        return &FloatConsoleVars;
    }
    template<>
    ConsoleVarArray<std::string>* GetConsoleVarArray()
    {
        return &StringConsoleVars;
    }

    double* GetFloatConsoleVar(Core::StringHash hash) override;
    void SetFloatConsoleVar(Core::StringHash hash, double value) override;

    int32_t* GetIntConsoleVar(Core::StringHash hash) override;
    void SetIntConsoleVar(Core::StringHash hash, int32_t value) override;

    const char* GetStringConsoleVar(Core::StringHash hash) override;
    void SetStringConsoleVar(Core::StringHash hash, const char* value) override;

    template<typename T>
    T* GetConsoleVarCurrent(uint32_t namehash)
    {
        ConsoleVarParameter* par = this->GetConsoleVar(namehash);
        if (!par)
        {
            return nullptr;
        }
        else
        {
            return this->GetConsoleVarArray<T>()->GetCurrentPtr(par->ArrayIndex);
        }
    }

    template<typename T>
    void SetConsoleVarCurrent(uint32_t namehash, const T& value)
    {
        ConsoleVarParameter* cvar = this->GetConsoleVar(namehash);
        if (cvar)
        {
            this->GetConsoleVarArray<T>()->SetCurrent(value, cvar->ArrayIndex);
        }
    }

    ConsoleVarParameter* GetConsoleVar(Core::StringHash hash);
    ConsoleVarParameter* CreateFloatConsoleVar(const char* name, const char* description, double defaultValue, double currentValue) override final;
    ConsoleVarParameter* CreateIntConsoleVar(const char* name, const char* description, int32_t defaultValue, int32_t currentValue) override final;
    ConsoleVarParameter* CreateStringConsoleVar(const char* name, const char* description, const char* defaultValue, const char* currentValue) override final;

    void DrawImguiEditor() override final;

    static ConsoleVarSystemImpl* GetInstance()
    {
        return static_cast<ConsoleVarSystemImpl*>(ConsoleVarSystem::GetInstance());
    }

private:
    ConsoleVarParameter* InitConsoleVar(const char* name, const char* description);
    void EditParameter(ConsoleVarParameter* p, float textWidth);

private:
    std::unordered_map<uint32_t, ConsoleVarParameter> m_savedConsoleVars;
    std::vector<ConsoleVarParameter*> m_cachedEditParameters;
};

double* ConsoleVarSystemImpl::GetFloatConsoleVar(Core::StringHash hash)
{
    return this->GetConsoleVarCurrent<double>(hash);
}

void ConsoleVarSystemImpl::SetFloatConsoleVar(Core::StringHash hash, double value)
{
    this->SetConsoleVarCurrent(hash, value);
}

int32_t* ConsoleVarSystemImpl::GetIntConsoleVar(Core::StringHash hash)
{
    return this->GetConsoleVarCurrent<int32_t>(hash);
}

void ConsoleVarSystemImpl::SetIntConsoleVar(Core::StringHash hash, int32_t value)
{
    this->SetConsoleVarCurrent(hash, value);
}

const char* ConsoleVarSystemImpl::GetStringConsoleVar(Core::StringHash hash)
{
    return this->GetConsoleVarCurrent<std::string>(hash)->c_str();
}

void ConsoleVarSystemImpl::SetStringConsoleVar(Core::StringHash hash, const char* value)
{
    this->SetConsoleVarCurrent<std::string>(hash, value);
}

ConsoleVarParameter* ConsoleVarSystemImpl::GetConsoleVar(Core::StringHash hash)
{
    auto it = this->m_savedConsoleVars.find(hash);

    if (it != this->m_savedConsoleVars.end())
    {
        return &(*it).second;
    }

    return nullptr;
}

ConsoleVarParameter* ConsoleVarSystemImpl::CreateFloatConsoleVar(const char* name, const char* description, double defaultValue, double currentValue)
{
    ConsoleVarParameter* param = this->InitConsoleVar(name, description);
    if (!param)
    {
        return nullptr;
    }

    param->Type = ConsoleVarType::Float;

    this->GetConsoleVarArray<double>()->Add(defaultValue, currentValue, param);

    return param;
}

ConsoleVarParameter* ConsoleVarSystemImpl::CreateIntConsoleVar(const char* name, const char* description, int32_t defaultValue, int32_t currentValue)
{
    ConsoleVarParameter* param = this->InitConsoleVar(name, description);
    if (!param)
    {
        return nullptr;
    }

    param->Type = ConsoleVarType::Integer;

    this->GetConsoleVarArray<int32_t>()->Add(defaultValue, currentValue, param);

    return param;
}

ConsoleVarParameter* ConsoleVarSystemImpl::CreateStringConsoleVar(const char* name, const char* description, const char* defaultValue, const char* currentValue)
{
    ConsoleVarParameter* param = this->InitConsoleVar(name, description);
    if (!param)
    {
        return nullptr;
    }

    param->Type = ConsoleVarType::String;

    this->GetConsoleVarArray<std::string>()->Add(defaultValue, currentValue, param);

    return param;
}

void ConsoleVarSystemImpl::DrawImguiEditor()
{
    static std::string searchText = "";

    ImGui::InputText("Filter", &searchText);
    static bool bShowAdvanced = false;
    ImGui::Checkbox("Advanced", &bShowAdvanced);
    ImGui::Separator();
    this->m_cachedEditParameters.clear();

    auto addToEditList = [&](ConsoleVarParameter* parameter)
    {
        bool bHidden = ((uint32_t)parameter->Flags & (uint32_t)ConsoleVarFlags::NoEdit);
        bool bIsAdvanced = ((uint32_t)parameter->Flags & (uint32_t)ConsoleVarFlags::Advanced);

        if (!bHidden)
        {
            if (!(!bShowAdvanced && bIsAdvanced) && parameter->Name.find(searchText) != std::string::npos)
            {
                this->m_cachedEditParameters.push_back(parameter);
            };
        }
    };

    for (int i = 0; i < this->GetConsoleVarArray<int32_t>()->LastConsoleVar; i++)
    {
        addToEditList(this->GetConsoleVarArray<int32_t>()->ConsoleVars[i].Parameter);
    }
    for (int i = 0; i < this->GetConsoleVarArray<double>()->LastConsoleVar; i++)
    {
        addToEditList(this->GetConsoleVarArray<double>()->ConsoleVars[i].Parameter);
    }
    for (int i = 0; i < this->GetConsoleVarArray<std::string>()->LastConsoleVar; i++)
    {
        addToEditList(this->GetConsoleVarArray<std::string>()->ConsoleVars[i].Parameter);
    }

    if (this->m_cachedEditParameters.size() > 10)
    {
        std::unordered_map<std::string, std::vector<ConsoleVarParameter*>> categorizedParams;

        //insert all the edit parameters into the hashmap by category
        for (auto p : this->m_cachedEditParameters)
        {
            int dotPos = -1;
            //find where the first dot is to categorize
            for (int i = 0; i < p->Name.length(); i++)
            {
                if (p->Name[i] == '.')
                {
                    dotPos = i;
                    break;
                }
            }
            std::string category = "";
            if (dotPos != -1)
            {
                category = p->Name.substr(0, dotPos);
            }

            auto it = categorizedParams.find(category);
            if (it == categorizedParams.end())
            {
                categorizedParams[category] = std::vector<ConsoleVarParameter*>();
                it = categorizedParams.find(category);
            }
            it->second.push_back(p);
        }

        for (auto [category, parameters] : categorizedParams)
        {
            //alphabetical sort
            std::sort(parameters.begin(), parameters.end(), [](ConsoleVarParameter* A, ConsoleVarParameter* B)
                {
                    return A->Name < B->Name;
                });

            if (ImGui::BeginMenu(category.c_str()))
            {
                float maxTextWidth = 0;

                for (auto p : parameters)
                {
                    maxTextWidth = std::max(maxTextWidth, ImGui::CalcTextSize(p->Name.c_str()).x);
                }
                for (auto p : parameters)
                {
                    this->EditParameter(p, maxTextWidth);
                }

                ImGui::EndMenu();
            }
        }
    }
    else
    {
        //alphabetical sort
        std::sort(this->m_cachedEditParameters.begin(), this->m_cachedEditParameters.end(), [](ConsoleVarParameter* A, ConsoleVarParameter* B)
            {
                return A->Name < B->Name;
            });

        float maxTextWidth = 0;
        for (auto p : this->m_cachedEditParameters)
        {
            maxTextWidth = std::max(maxTextWidth, ImGui::CalcTextSize(p->Name.c_str()).x);
        }
        for (auto p : this->m_cachedEditParameters)
        {
            this->EditParameter(p, maxTextWidth);
        }
    }
}

ConsoleVarParameter* ConsoleVarSystemImpl::InitConsoleVar(const char* name, const char* description)
{
    if (this->GetConsoleVar(name))
    {
        return nullptr; //return null if the cvar already exists
    }

    uint32_t namehash = Core::StringHash{ name };
    this->m_savedConsoleVars[namehash] = ConsoleVarParameter{};

    ConsoleVarParameter& newParam = this->m_savedConsoleVars[namehash];

    newParam.Name = name;
    newParam.Description = description;

    return &newParam;
}

void Label(const char* label, float textWidth)
{
    constexpr float Slack = 50;
    constexpr float EditorWidth = 100;

    const ImVec2 lineStart = ImGui::GetCursorScreenPos();
    const ImGuiStyle& style = ImGui::GetStyle();
    float fullWidth = textWidth + Slack;

    ImVec2 textSize = ImGui::CalcTextSize(label);

    ImVec2 startPos = ImGui::GetCursorScreenPos();

    ImGui::Text(label);

    ImVec2 finalPos = { startPos.x + fullWidth, startPos.y };

    ImGui::SameLine();
    ImGui::SetCursorScreenPos(finalPos);

    ImGui::SetNextItemWidth(EditorWidth);
}

void ConsoleVarSystemImpl::EditParameter(ConsoleVarParameter* p, float textWidth)
{
    const bool readonlyFlag = ((uint32_t)p->Flags & (uint32_t)ConsoleVarFlags::EditReadOnly);
    const bool checkboxFlag = ((uint32_t)p->Flags & (uint32_t)ConsoleVarFlags::EditCheckbox);
    const bool dragFlag = ((uint32_t)p->Flags & (uint32_t)ConsoleVarFlags::EditFloatDrag);


    switch (p->Type)
    {
    case ConsoleVarType::Integer:

        if (readonlyFlag)
        {
            std::string displayFormat = p->Name + "= %i";
            ImGui::Text(displayFormat.c_str(), this->GetConsoleVarArray<int32_t>()->GetCurrent(p->ArrayIndex));
        }
        else
        {
            if (checkboxFlag)
            {
                bool bCheckbox = this->GetConsoleVarArray<int32_t>()->GetCurrent(p->ArrayIndex) != 0;
                Label(p->Name.c_str(), textWidth);

                ImGui::PushID(p->Name.c_str());

                if (ImGui::Checkbox("", &bCheckbox))
                {
                    this->GetConsoleVarArray<int32_t>()->SetCurrent(bCheckbox ? 1 : 0, p->ArrayIndex);
                }
                ImGui::PopID();
            }
            else
            {
                Label(p->Name.c_str(), textWidth);
                ImGui::PushID(p->Name.c_str());
                ImGui::InputInt("", this->GetConsoleVarArray<int32_t>()->GetCurrentPtr(p->ArrayIndex));
                ImGui::PopID();
            }
        }
        break;

    case ConsoleVarType::Float:

        if (readonlyFlag)
        {
            std::string displayFormat = p->Name + "= %f";
            ImGui::Text(displayFormat.c_str(), this->GetConsoleVarArray<double>()->GetCurrent(p->ArrayIndex));
        }
        else
        {
            Label(p->Name.c_str(), textWidth);
            ImGui::PushID(p->Name.c_str());
            if (dragFlag)
            {
                ImGui::InputDouble("", this->GetConsoleVarArray<double>()->GetCurrentPtr(p->ArrayIndex), 0, 0, "%.3f");
            }
            else
            {
                ImGui::InputDouble("", this->GetConsoleVarArray<double>()->GetCurrentPtr(p->ArrayIndex), 0, 0, "%.3f");
            }
            ImGui::PopID();
        }
        break;

    case ConsoleVarType::String:

        if (readonlyFlag)
        {
            std::string displayFormat = p->Name + "= %s";
            ImGui::PushID(p->Name.c_str());
            ImGui::Text(displayFormat.c_str(), this->GetConsoleVarArray<std::string>()->GetCurrent(p->ArrayIndex));

            ImGui::PopID();
        }
        else
        {
            Label(p->Name.c_str(), textWidth);
            ImGui::InputText("", this->GetConsoleVarArray<std::string>()->GetCurrentPtr(p->ArrayIndex));

            ImGui::PopID();
        }
        break;

    default:
        break;
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip(p->Description.c_str());
    }
}

//set the cvar data purely by type and index

template<typename T>
T GetConsoleVarCurrentByIndex(int32_t index) 
{
    return ConsoleVarSystemImpl::GetInstance()->GetConsoleVarArray<T>()->GetCurrent(index);
}

template<typename T>
T* PtrGetConsoleVarCurrentByIndex(int32_t index) 
{
    return ConsoleVarSystemImpl::GetInstance()->GetConsoleVarArray<T>()->GetCurrentPtr(index);
}

template<typename T>
void SetConsoleVarCurrentByIndex(int32_t index, const T& data) 
{
    ConsoleVarSystemImpl::GetInstance()->GetConsoleVarArray<T>()->SetCurrent(data, index);
}

AutoConsoleVar_Float::AutoConsoleVar_Float(const char* name, const char* description, double defaultValue, ConsoleVarFlags flags)
{
	ConsoleVarParameter* cvar = ConsoleVarSystem::GetInstance()->CreateFloatConsoleVar(name, description, defaultValue, defaultValue);
	cvar->Flags = flags;
	index = cvar->ArrayIndex;
}

double AutoConsoleVar_Float::Get()
{
	return GetConsoleVarCurrentByIndex<ConsoleVarType>(index);
}

double* AutoConsoleVar_Float::GetPtr()
{
    return PtrGetConsoleVarCurrentByIndex<ConsoleVarType>(index);
}

float AutoConsoleVar_Float::GetFloat()
{
    return static_cast<float>(this->Get());
}

float* AutoConsoleVar_Float::GetFloatPtr()
{
    float* result = reinterpret_cast<float*>(GetPtr());
    return result;
}

void AutoConsoleVar_Float::Set(double f)
{
    SetConsoleVarCurrentByIndex<ConsoleVarType>(index, f);
}

AutoConsoleVar_Int::AutoConsoleVar_Int(const char* name, const char* description, int32_t defaultValue, ConsoleVarFlags flags)
{
    ConsoleVarParameter* cvar = ConsoleVarSystem::GetInstance()->CreateIntConsoleVar(name, description, defaultValue, defaultValue);
    cvar->Flags = flags;
    index = cvar->ArrayIndex;
}

int32_t AutoConsoleVar_Int::Get()
{
    return GetConsoleVarCurrentByIndex<ConsoleVarType>(index);
}

int32_t* AutoConsoleVar_Int::GetPtr()
{
    return PtrGetConsoleVarCurrentByIndex<ConsoleVarType>(index);
}

void AutoConsoleVar_Int::Set(int32_t val)
{
    SetConsoleVarCurrentByIndex<ConsoleVarType>(index, val);
}

void AutoConsoleVar_Int::Toggle()
{
    bool enabled = Get() != 0;

    Set(enabled ? 0 : 1);
}

AutoConsoleVar_String::AutoConsoleVar_String(const char* name, const char* description, const char* defaultValue, ConsoleVarFlags flags)
{
    ConsoleVarParameter* cvar = ConsoleVarSystem::GetInstance()->CreateStringConsoleVar(name, description, defaultValue, defaultValue);
    cvar->Flags = flags;
    index = cvar->ArrayIndex;
}

const char* AutoConsoleVar_String::Get()
{
    return GetConsoleVarCurrentByIndex<ConsoleVarType>(index).c_str();
};

void AutoConsoleVar_String::Set(std::string&& val)
{
    SetConsoleVarCurrentByIndex<ConsoleVarType>(index, val);
}

ConsoleVarSystem* PhxEngine::ConsoleVarSystem::GetInstance()
{
    static ConsoleVarSystemImpl singleton = {};
    return &singleton;
}
