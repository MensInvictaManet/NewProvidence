#pragma once

#include "TextureManager.h"
#include "MemoryManager.h"
#include "TextureAnimation.h"
#include "Color.h"

#include <vector>
#include <assert.h>
#include <stack>

class GUIObjectNode
{
protected:
	std::vector<GUIObjectNode*> m_Children;
	std::vector<GUIObjectNode*> m_NewChildren;

public:
	static GUIObjectNode* CreateObjectNode(const char* imageFile);
	static GUIObjectNode* CreateObjectNode(TextureAnimation* anim);

	GUIObjectNode();
	virtual ~GUIObjectNode();

	virtual void Input(int xOffset = 0, int yOffset = 0);
	virtual void Update();
	virtual void Render(int xOffset = 0, int yOffset = 0);
	virtual void Render3D();
	virtual void SetToDestroy(std::stack<GUIObjectNode*>& destroyList);
	virtual void Destroy();
	virtual bool SortChild(GUIObjectNode* child);

	bool GetClickPosition(const std::string& objectName, int& xPos, int& yPos);
	int GetTrueX() const;
	int GetTrueY() const;

	inline void SetZOrder(int zOrder) { m_ZOrder = zOrder; if (m_Parent != nullptr) m_Parent->SortChild(this); }
	inline void SetX(int x) { m_X = x; }
	inline void SetY(int y) { m_Y = y; }
	inline void SetPosition(int x, int y) { SetX(x); SetY(y); }
	inline void SetWidth(int width) { m_Width = width; }
	virtual void SetHeight(int height) { m_Height = height; }
	inline void SetDimensions(int width, int height) { SetWidth(width); SetHeight(height); }
	inline void SetTextureID(int textureID) { m_TextureID = textureID; }
	inline void SetTextureAnimation(TextureAnimation* anim) { m_TextureAnimation = anim; }
	inline void SetVisible(bool visible) { m_Visible = visible; }
	inline void SetParent(GUIObjectNode* parent) { m_Parent = parent; }
	inline void SetColor(float r, float g, float b, float a) { m_Color.colorValues[0] = r; m_Color.colorValues[1] = g; m_Color.colorValues[2] = b; m_Color.colorValues[3] = a; }
	inline void SetObjectName(std::string objectName) { m_ObjectName = objectName; }
	inline void SetClickX(int clickX) { m_ClickX = clickX; }
	inline void SetClickY(int clickY) { m_ClickY = clickY; }

	int GetZOrder() const { return m_ZOrder; }
	int GetX() const { return m_X; }
	int GetY() const { return m_Y; }
	int GetWidth() const { return m_Width; }
	int GetHeight() const { return m_Height; }
	int GetTextureID() const { return m_TextureID; }
	TextureAnimation* GetTextureAnimation() const { return m_TextureAnimation; }
	bool GetVisible() const { return m_Visible; }
	inline const std::string& GetObjectName(void) const { return m_ObjectName; }
	GUIObjectNode* GetParent() { return m_Parent; }
	const GUIObjectNode* GetParent() const { return m_Parent; }
	float getColorR() const { return m_Color.colorValues[0]; }
	float getColorG() const { return m_Color.colorValues[1]; }
	float getColorB() const { return m_Color.colorValues[2]; }
	float getColorA() const { return m_Color.colorValues[3]; }

	void AddChild(GUIObjectNode* child);
	void AddChildSorted(GUIObjectNode* child);
	void RemoveChild(GUIObjectNode* child);
	GUIObjectNode* GetChildByName(std::string childName);

	int m_ZOrder;
	int m_X;
	int m_Y;
	int m_Width;
	int m_Height;
	GLuint m_TextureID;
	TextureAnimation* m_TextureAnimation;
	bool m_Visible;
	GUIObjectNode* m_Parent;
	bool m_Created;
	bool m_SetToDestroy;
	bool m_ExplicitObject;
	Color m_Color;

	std::string m_ObjectName;
	int m_ClickX;
	int m_ClickY;
};

inline GUIObjectNode* GUIObjectNode::CreateObjectNode(const char* imageFile)
{
	MANAGE_MEMORY_NEW("MenuUI_ObjectNode", sizeof(GUIObjectNode));
	auto newNode = new GUIObjectNode;
	newNode->m_ExplicitObject = true;
	newNode->SetTextureID(textureManager.LoadTextureGetID(imageFile));
	return newNode;
}

inline GUIObjectNode* GUIObjectNode::CreateObjectNode(TextureAnimation* anim)
{
	MANAGE_MEMORY_NEW("MenuUI_ObjectNode", sizeof(GUIObjectNode));
	auto newNode = new GUIObjectNode;
	newNode->m_ExplicitObject = true;
	newNode->SetTextureAnimation(anim);
	return newNode;
}

inline GUIObjectNode::GUIObjectNode() :
	m_ZOrder(0),
	m_X(0),
	m_Y(0),
	m_Width(0),
	m_Height(0),
	m_TextureID(0),
	m_TextureAnimation(nullptr),
	m_Visible(true),
	m_Parent(nullptr),
	m_Created(false),
	m_SetToDestroy(false),
	m_ExplicitObject(false),
	m_Color(1.0f, 1.0f, 1.0f, 1.0f),
	m_ObjectName(""),
	m_ClickX(0),
	m_ClickY(0)
{

}

inline GUIObjectNode::~GUIObjectNode()
{
	if (m_ExplicitObject) MANAGE_MEMORY_DELETE("MenuUI_ObjectNode", sizeof(GUIObjectNode));
}


inline void GUIObjectNode::Input(int xOffset, int yOffset)
{
	if (m_SetToDestroy || !m_Visible) return;

	//  Pass the input call to all children (in reverse, so things rendered last are input checked first)
	if (m_Children.size() != 0)
		for (int i = int(m_Children.size()) - 1; i >= 0; --i)
			m_Children[i]->Input(xOffset + m_X, yOffset + m_Y);
}

inline void GUIObjectNode::Update()
{
	if (m_SetToDestroy || !m_Visible) return;

	for (auto iter = m_NewChildren.begin(); iter != m_NewChildren.end(); ++iter)
	{
		AddChildSorted((*iter));
		(*iter)->m_Created = true;
	}
	m_NewChildren.clear();

	//  Pass the update call to all children
	for (auto iter = m_Children.begin(); iter != m_Children.end(); ++iter) (*iter)->Update();
}

inline void GUIObjectNode::Render(int xOffset, int yOffset)
{
	glColor4f(m_Color.colorValues[0], m_Color.colorValues[1], m_Color.colorValues[2], m_Color.colorValues[3]);

	auto x = m_X + xOffset;
	auto y = m_Y + yOffset;

	//  Render the object if we're able
	if (!m_SetToDestroy && m_Visible)
	{
		if (m_Width > 0 && m_Height > 0)
		{
			if (m_TextureID != 0)
			{
				glBindTexture(GL_TEXTURE_2D, m_TextureID);

				glBegin(GL_QUADS);
					glTexCoord2f(0.0f, 0.0f); glVertex2i(x, y);
					glTexCoord2f(1.0f, 0.0f); glVertex2i(x + m_Width, y);
					glTexCoord2f(1.0f, 1.0f); glVertex2i(x + m_Width, y + m_Height);
					glTexCoord2f(0.0f, 1.0f); glVertex2i(x, y + m_Height);
				glEnd();
			}
			else if (m_TextureAnimation != nullptr)
			{
				m_TextureAnimation->Render(x, y);
			}
		}

		//  Pass the render call to all children
		for (auto iter = m_Children.begin(); iter != m_Children.end(); ++iter) (*iter)->Render(x, y);
	}
}

inline void GUIObjectNode::Render3D()
{
	//  Pass the render 3D call to all children
	for (auto iter = m_Children.begin(); iter != m_Children.end(); ++iter) (*iter)->Render3D();
}

inline void GUIObjectNode::SetToDestroy(std::stack<GUIObjectNode*>& destroyList)
{
	assert(m_Created);

	m_SetToDestroy = true;
	destroyList.push(this);

	for (auto iter = m_Children.begin(); iter != m_Children.end(); ++iter) (*iter)->SetToDestroy(destroyList);
}

inline void GUIObjectNode::Destroy()
{
	if (m_Parent != nullptr) m_Parent->RemoveChild(this);
	assert(m_Created);
}

inline bool GUIObjectNode::SortChild(GUIObjectNode* child)
{
	auto childIter = std::find(m_Children.begin(), m_Children.end(), child);
	if (childIter == m_Children.end()) return false;
	if (m_Children.size() <= 1) return true;

	m_Children.erase(childIter);
	AddChildSorted(child);
	return true;
}

inline bool GUIObjectNode::GetClickPosition(const std::string& objectName, int& xPos, int& yPos)
{
	if (m_ObjectName.compare(objectName) == 0)
	{
		xPos = GetTrueX() + m_ClickX;
		yPos = GetTrueY() + m_ClickY;
		return true;
	}

	for (auto iter = m_Children.begin(); iter != m_Children.end(); ++iter)
	{
		if ((*iter)->GetClickPosition(objectName, xPos, yPos)) return true;
	}

	return false;
}

inline int GUIObjectNode::GetTrueX() const
{
	return GetX() + (GetParent() ? GetParent()->GetTrueX() : 0);
}

inline int GUIObjectNode::GetTrueY() const
{
	return GetY() + (GetParent() ? GetParent()->GetTrueY() : 0);
}

inline void GUIObjectNode::AddChild(GUIObjectNode* child)
{
	assert(!m_SetToDestroy);

	m_NewChildren.push_back(child);
	child->m_Parent = this;
}

inline void GUIObjectNode::AddChildSorted(GUIObjectNode* child)
{
	assert(!m_SetToDestroy);

	for (size_t i = 0; i < m_Children.size(); ++i)
	{
		if (m_Children[i]->GetZOrder() >= child->GetZOrder()) continue;

		m_Children.insert(m_Children.begin() + i, child);
		return;
	}

	m_Children.push_back(child);
}

inline void GUIObjectNode::RemoveChild(GUIObjectNode* child)
{
	assert(child->m_Created);

	for (auto iter = m_Children.begin(); iter != m_Children.end(); ++iter)
	{
		if ((*iter) != child) continue;
		m_Children.erase(iter);
		return;
	}
}


inline GUIObjectNode* GUIObjectNode::GetChildByName(std::string childName)
{
	for (auto iter = m_Children.begin(); iter != m_Children.end(); ++iter)
		if ((*iter)->GetObjectName() == childName)
			return (*iter);

	return nullptr;
}