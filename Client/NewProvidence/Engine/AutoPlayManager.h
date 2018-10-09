#pragma once

#include "DebugConsole.h"
#include "InputManager.h"

struct AutoPlayAction
{
	virtual ~AutoPlayAction() {}
	virtual std::string GetDependency() const = 0;
	virtual bool RunAction() = 0;
};

struct AutoPlayDebugCommandAction : public AutoPlayAction
{
	explicit AutoPlayDebugCommandAction(std::string debugCommand, std::string dependency)
	{
		m_DebugCommandString = debugCommand;
		m_Dependency = dependency;
	}

	std::string GetDependency() const override { return m_Dependency; }

	bool RunAction() override
	{
		debugConsole->EnterCommand(m_DebugCommandString);
		return true;
	}

private:
	std::string m_DebugCommandString;
	std::string m_Dependency;
};

class AutoPlayManager
{
public:
	static AutoPlayManager& GetInstance() { static AutoPlayManager INSTANCE; return INSTANCE; }

	void Update();
	void Shutdown();

	bool LoadAutoplayScript(const char* xmlFilename);
	static bool CheckDependency(std::string dependencyString);

private:
	AutoPlayManager();
	~AutoPlayManager();

	std::vector<AutoPlayAction*> m_AutoPlayActionList;
	int m_ActionListIndex;
};

inline void AutoPlayManager::Update()
{
	if (m_AutoPlayActionList.empty()) return;
	if (m_ActionListIndex >= int(m_AutoPlayActionList.size())) return;

	if (!CheckDependency(m_AutoPlayActionList[m_ActionListIndex]->GetDependency())) return;
	if (m_AutoPlayActionList[m_ActionListIndex]->RunAction()) m_ActionListIndex++;
}

inline void AutoPlayManager::Shutdown()
{
	while (!m_AutoPlayActionList.empty())
	{
		delete (*(--m_AutoPlayActionList.end()));
		m_AutoPlayActionList.pop_back();
	}
	m_ActionListIndex = 0;
}

inline bool AutoPlayManager::LoadAutoplayScript(const char* xmlFilename)
{
	auto animXML = xmlWrapper.LoadXMLFile(xmlFilename);
	if (animXML == nullptr) return false;

	const RapidXML_Node* baseNode = animXML->first_node();
	if (baseNode == nullptr || strcmp(baseNode->name(), "AutoplayScript") != 0) return false;

	for (auto iter = baseNode->first_node(); iter != nullptr; iter = iter->next_sibling())
	{
		AutoPlayAction* newAction = nullptr;

		//  Load in a Debug Console Command
		if (strcmp(iter->name(), "DebugCommand") == 0)
		{
			auto dataAttribute = iter->first_attribute("text");
			auto textCommand = std::string(dataAttribute->value());
			std::string dependency = "";
			dataAttribute = dataAttribute->next_attribute("dependency");
			if (dataAttribute != nullptr) dependency = std::string(dataAttribute->value());

			newAction = new AutoPlayDebugCommandAction(textCommand, dependency);
		}

		if (newAction != nullptr) m_AutoPlayActionList.push_back(newAction);
	}

	return true;
}

inline bool AutoPlayManager::CheckDependency(std::string dependencyString)
{
	if (dependencyString.empty()) return true;

	auto soleCommand = false;
	auto firstSpace = dependencyString.find_first_of(' ');
	if (firstSpace == -1) { soleCommand = true; firstSpace = dependencyString.length(); }
	auto dependencyType = dependencyString.substr(0, firstSpace);

	dependencyString = soleCommand ? "" : dependencyString.substr(firstSpace + 1, dependencyString.length());

	if (dependencyType.compare("MouseAutoMove") == 0)
	{
		auto active = (dependencyString.compare("True") == 0);
		return (inputManager.GetIsMouseAutoMoving() == active);
	}
	else if (dependencyType.compare("UIObjectExists") == 0)
	{
		auto mouseX = 0;
		auto mouseY = 0;
		auto exists = guiManager.GetClickPosition(dependencyString, mouseX, mouseY);
		return exists;
	}

	return false;
}

inline AutoPlayManager::AutoPlayManager() : 
	m_ActionListIndex(0)
{
}

inline AutoPlayManager::~AutoPlayManager()
{
	Shutdown();
}

//  Instance to be utilized by anyone including this header
AutoPlayManager& autoplayManager = AutoPlayManager::GetInstance();