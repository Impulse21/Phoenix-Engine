#include <PhxEngine/EngineTuner.h>

#include <assert.h>
#include <string>

using namespace PhxEngine;

namespace PhxEngine
{
    class VariableGroup : public EngineVar
    {
    public:
        VariableGroup() = default;

        EngineVar* FindChild(const std::string& name)
        {
            auto iter = this->m_children.find(name);
            return iter == this->m_children.end() ? nullptr : iter->second;
        }

        void AddChild(const std::string& name, EngineVar& child)
        {
            this->m_children[name] = &child;
            child.m_groupPtr = this;
        }

        EngineVar* NextVariable(EngineVar* currentVariable);
        EngineVar* PrevVariable(EngineVar* currentVariable);
        EngineVar* FirstVariable();
        EngineVar* LastVariable();

        bool IsExpanded() const { return this->m_isExpanded; }

        virtual void Increment() override { this->m_isExpanded = true; }
        virtual void Decrement() override { this->m_isExpanded = false; }
        virtual void Toggle() override { this->m_isExpanded = !this->m_isExpanded; }

    private:
        bool m_isExpanded = false;
        std::unordered_map<std::string, EngineVar*> m_children;
    };
}


namespace
{
    PhxEngine::VariableGroup m_rootGroup;
	constexpr size_t kMaxUnregisteredTweaks = 1024;
	int m_unregisteredVariablesCount = 0; 

	char m_unregisteredPath[kMaxUnregisteredTweaks][128];
	EngineVar* m_unregisteredVariable[kMaxUnregisteredTweaks] = { nullptr };

	void AddToVariableGraph(std::string const& path, EngineVar& var)
	{
        std::vector<std::string> separatedPath;
        std::string leafName;
        size_t start = 0;
        size_t end = 0;

        while (1)
        {
            end = path.find('/', start);
            if (end == std::string::npos)
            {
                leafName = path.substr(start);
                break;
            }
            else
            {
                separatedPath.push_back(path.substr(start, end - start));
                start = end + 1;
            }
        }

        VariableGroup* group = &m_rootGroup;

        for (auto iter = separatedPath.begin(); iter != separatedPath.end(); ++iter)
        {
            VariableGroup* nextGroup;
            EngineVar* node = group->FindChild(*iter);
            if (node == nullptr)
            {
                nextGroup = new VariableGroup();
                group->AddChild(*iter, *nextGroup);
                group = nextGroup;
            }
            else
            {
                nextGroup = dynamic_cast<VariableGroup*>(node);
                assert(nextGroup != nullptr && "Attempted to trash the tweak graph");
                group = nextGroup;
            }
        }

        group->AddChild(leafName, var);
	}

	void RegisterVariable(const std::string& path, EngineVar& var)
	{
		// Prepare unregistered variables from static initialization
		if (m_unregisteredVariablesCount >= 0)
		{
			int32_t idx = m_unregisteredVariablesCount++;
			strcpy_s(m_unregisteredPath[idx], path.c_str());
			m_unregisteredVariable[idx] = &var;
		}
		else
		{
			AddToVariableGraph(path, var);
		}
	}
}

namespace PhxEngine
{

    EngineVar::EngineVar(const std::string& path, ActionCallback pfnCallback)
        : m_groupPtr(nullptr)
        , m_actionCallback(pfnCallback)
    {
        RegisterVariable(path, *this);
    }

    void EngineTunerService::Startup()
    {
        for (int32_t i = 0; i < m_unregisteredVariablesCount; ++i)
        {
            assert(strlen(m_unregisteredPath[i]) > 0 && "Register = %d\n", i);
            assert(m_unregisteredVariable[i] != nullptr);
            AddToVariableGraph(m_unregisteredPath[i], *m_unregisteredVariable[i]);
        }

        m_unregisteredVariablesCount = -1;
    }

    void EngineTunerService::SetValues(nlohmann::Json const& data)
    {
        assert(false);
    }

    void EngineTunerService::Shutdown()
    {
    }

    BoolVar::BoolVar(const std::string& path, bool val, ActionCallback pfnCallback)
        : EngineVar(path, pfnCallback)
        , m_flag(val)
    {
    }

    FloatVar::FloatVar(std::string const& path, float val, float minValue, float maxValue, float stepSize, ActionCallback pfnCallback)
        : EngineVar(path, pfnCallback)
    {
        assert(minValue <= maxValue);
        this->m_minValue = minValue;
        this->m_maxValue = maxValue;
        this->m_value = this->Clamp(val);
        this->m_stepSize = stepSize;
    }

    IntVar::IntVar(const std::string& path, int32_t val, int32_t minValue, int32_t maxValue, int32_t stepSize, ActionCallback pfnCallback)
        : EngineVar(path, pfnCallback)
    {
        assert(minValue <= maxValue);
        this->m_minValue = minValue;
        this->m_maxValue = maxValue;
        this->m_value = this->Clamp(val);
        this->m_stepSize = stepSize;
    }

    EngineVar* VariableGroup::NextVariable(EngineVar* currentVariable)
    {
        auto iter = this->m_children.begin();
        for (; iter != this->m_children.end(); ++iter)
        {
            if (currentVariable == iter->second)
                break;
        }

        assert(iter != this->m_children.end(), "Did not find engine variable in its designated group");

        auto nextIter = iter;
        ++nextIter;

        if (nextIter == this->m_children.end())
            return this->m_groupPtr ? this->m_groupPtr->NextVariable(this) : nullptr;
        else
            return nextIter->second;
    }

    EngineVar* VariableGroup::PrevVariable(EngineVar* currentVariable)
    {
        auto iter = this->m_children.begin();
        for (; iter != this->m_children.end(); ++iter)
        {
            if (currentVariable == iter->second)
                break;
        }

        assert(iter != this->m_children.end(), "Did not find engine variable in its designated group");

        if (iter == this->m_children.begin())
            return this;

        auto prevIter = iter;
        --prevIter;

        VariableGroup* isGroup = dynamic_cast<VariableGroup*>(prevIter->second);
        if (isGroup && isGroup->IsExpanded())
            return isGroup->LastVariable();

        return prevIter->second;
    }

    EngineVar* VariableGroup::FirstVariable()
    {
        return this->m_children.size() == 0 ? nullptr : this->m_children.begin()->second;
    }

    EngineVar* VariableGroup::LastVariable()
    {
        if (this->m_children.size() == 0)
            return this;

        auto lastVariable = this->m_children.end();
        --lastVariable;

        VariableGroup* isGroup = dynamic_cast<VariableGroup*>(lastVariable->second);
        if (isGroup && isGroup->IsExpanded())
            return isGroup->LastVariable();

        return lastVariable->second;
    }
}