#pragma once

#include "GUIObjectNode.h"
#include <stack>

class GUIManager
{
public:
	static GUIManager& GetInstance() { static GUIManager INSTANCE; return INSTANCE; }

	GUIObjectNode* GetBaseNode() const { return m_BaseNode; }
	std::stack<GUIObjectNode*>& GetDestroyList() { return m_NodesToDestroy; }
	void AddChild(GUIObjectNode* node) const { if (m_BaseNode != nullptr) m_BaseNode->AddChild(node); }

	void DestroyNode(GUIObjectNode* nodeToDestroy);
	void Input() const;
	void Update() const;
	void Render() const;
	void Render3D() const;
	void EndStep();
	void Shutdown();

	bool GetClickPosition(std::string& objectName, int& xPos, int& yPos) const;

private:
	GUIManager();
	~GUIManager();

	GUIObjectNode* m_BaseNode;
	std::stack<GUIObjectNode*> m_NodesToDestroy;
};

inline void GUIManager::DestroyNode(GUIObjectNode* nodeToDestroy)
{
	if (nodeToDestroy->m_SetToDestroy == true) return;

	nodeToDestroy->SetToDestroy(m_NodesToDestroy);
}

inline void GUIManager::Input() const
{
	m_BaseNode->Input();
}

inline void GUIManager::Update() const
{
	m_BaseNode->Update();
}

inline void GUIManager::Render() const
{
	m_BaseNode->Render();
}

inline void GUIManager::Render3D() const
{
	m_BaseNode->Render3D();
}

inline GUIManager::GUIManager()
{
	m_BaseNode = GUIObjectNode::CreateObjectNode("");
	m_BaseNode->m_Created = true;
}

inline GUIManager::~GUIManager()
{

}

inline void GUIManager::EndStep()
{
	while (!m_NodesToDestroy.empty())
	{
		auto node = m_NodesToDestroy.top();
		node->Destroy();
		delete node;
		m_NodesToDestroy.pop();
	}
}

inline void GUIManager::Shutdown()
{
	m_BaseNode->SetToDestroy(m_NodesToDestroy);
	EndStep();
}

inline bool GUIManager::GetClickPosition(std::string& objectName, int& xPos, int& yPos) const
{
	return m_BaseNode->GetClickPosition(objectName, xPos, yPos);
}

//  Instance to be utilized by anyone including this header
GUIManager& guiManager = GUIManager::GetInstance();