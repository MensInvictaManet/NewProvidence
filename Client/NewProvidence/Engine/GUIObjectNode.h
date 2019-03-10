#pragma once

#include "TextureManager.h"
#include "MemoryManager.h"
#include "TextureAnimation.h"
#include "Color.h"

#include <vector>
#include <assert.h>
#include <stack>
#include <functional>

enum UI_Justifications { UI_JUSTIFY_LEFT = 0, UI_JUSTIFY_RIGHT, UI_JUSTIFY_CENTER, UI_JUSTIFICATION_COUNT };

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
	void Render(int xOffset = 0, int yOffset = 0); //  Note: Non-virtual so no one overrides this... override TrueRender instead
	virtual void TrueRender(int xOffset = 0, int yOffset = 0);
	virtual void Render3D();
	virtual void SetToDestroy(std::stack<GUIObjectNode*>& destroyList);
	virtual void Destroy();
	virtual bool SortChild(GUIObjectNode* child);
	virtual bool IsMouseWithin(int mX, int mY);

	virtual void RenderChildren(int x, int y);
	inline void ApplyObjectColor() const { glColor4f(m_Color.colorValues[0], m_Color.colorValues[1], m_Color.colorValues[2], m_Color.colorValues[3]); }

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
	inline void SetColorBytes(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) { SetColor(float(r) / 255.0f, float(g) / 255.0f, float(b) / 255.0f, float(a) / 255.0f); }
	inline void SetColor(Color color) { m_Color = color; }
	inline void SetObjectName(std::string objectName) { m_ObjectName = objectName; }
	inline void SetClickX(int clickX) { m_ClickX = clickX; }
	inline void SetClickY(int clickY) { m_ClickY = clickY; }
	inline void SetRenderLast(bool last) { m_RenderLast = last; }

	inline int GetZOrder() const { return m_ZOrder; }
	inline int GetX() const { return m_X; }
	inline int GetY() const { return m_Y; }
	inline int GetWidth() const { return m_Width; }
	inline int GetHeight() const { return m_Height; }
	inline int GetTextureID() const { return m_TextureID; }
	inline TextureAnimation* GetTextureAnimation() const { return m_TextureAnimation; }
	inline bool GetVisible() const { return m_Visible; }
	inline const std::string& GetObjectName(void) const { return m_ObjectName; }
	inline GUIObjectNode* GetParent() { return m_Parent; }
	inline const GUIObjectNode* GetParent() const { return m_Parent; }
	inline float getColorR() const { return m_Color.colorValues[0]; }
	inline float getColorG() const { return m_Color.colorValues[1]; }
	inline float getColorB() const { return m_Color.colorValues[2]; }
	inline float getColorA() const { return m_Color.colorValues[3]; }
	inline bool GetRenderLast() const { return m_RenderLast; }


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
	bool m_RenderLast;

	std::string m_ObjectName;
	int m_ClickX;
	int m_ClickY;
};

typedef std::function<void(GUIObjectNode*)> GUIFunctionCallback;

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

void GUIObjectNode::Render(int xOffset, int yOffset)
{
	//  If we're not able to render the object, return out
	if (m_SetToDestroy || !m_Visible) return;

	ApplyObjectColor();

	auto x = m_X + xOffset;
	auto y = m_Y + yOffset;

	TrueRender(x, y);
	RenderChildren(x, y);
}


void GUIObjectNode::TrueRender(int x, int y)
{
	//  Render the object if we're able
	if (m_Width > 0 && m_Height > 0)
	{
		if (m_TextureID != 0) Draw2DTexturedSquare(m_TextureID, x, y, m_Width, m_Height);
		else if (m_TextureAnimation != nullptr) m_TextureAnimation->Render(x, y);
	}
}

void GUIObjectNode::RenderChildren(int x, int y)
{
	//  Pass the 2D render call to all children
	std::vector<GUIObjectNode*> lastRenders;
	for (auto iter = m_Children.begin(); iter != m_Children.end(); ++iter)
	{
		if ((*iter)->GetRenderLast())
		{
			lastRenders.push_back((*iter));
			continue;
		}
		(*iter)->Render(x, y);
	}
	for (auto iter = lastRenders.begin(); iter != lastRenders.end(); ++iter) (*iter)->Render(x, y);
}

inline void GUIObjectNode::Render3D()
{
	//  Pass the 3D render call to all children
	std::vector<GUIObjectNode*> lastRenders;
	for (auto iter = m_Children.begin(); iter != m_Children.end(); ++iter)
	{
		if ((*iter)->GetRenderLast())
		{
			lastRenders.push_back((*iter));
			continue;
		}
		(*iter)->Render3D();
	}
	for (auto iter = lastRenders.begin(); iter != lastRenders.end(); ++iter) (*iter)->Render3D();
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

inline bool GUIObjectNode::IsMouseWithin(int mX, int mY)
{
	auto within = true;
	within &= (mX > m_X);
	within &= (mY > m_Y);
	within &= (mX < m_X + m_Width);
	within &= (mY < m_Y + m_Height);
	return within;
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

	for (auto iter = m_NewChildren.begin(); iter != m_NewChildren.end(); ++iter)
		if ((*iter)->GetObjectName() == childName)
			return (*iter);

	return nullptr;
}



class GUITemplatedBox
{
private:
	enum TemplatePortions { TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT, LEFT_SIDE, RIGHT_SIDE, TOP_SIDE, BOTTOM_SIDE, MIDDLE, TEMPLATE_PORTIONS_COUNT };
	std::unordered_map<int, TextureManager::ManagedTexture*> TextureMap;

public:
	inline TextureManager::ManagedTexture* TopLeft(int layerIndex) { return TextureMap[TEMPLATE_PORTIONS_COUNT * layerIndex + TOP_LEFT]; }
	inline TextureManager::ManagedTexture* TopRight(int layerIndex) { return TextureMap[TEMPLATE_PORTIONS_COUNT * layerIndex + TOP_RIGHT]; }
	inline TextureManager::ManagedTexture* BottomLeft(int layerIndex) { return TextureMap[TEMPLATE_PORTIONS_COUNT * layerIndex + BOTTOM_LEFT]; }
	inline TextureManager::ManagedTexture* BottomRight(int layerIndex) { return TextureMap[TEMPLATE_PORTIONS_COUNT * layerIndex + BOTTOM_RIGHT]; }
	inline TextureManager::ManagedTexture* TopSide(int layerIndex) { return TextureMap[TEMPLATE_PORTIONS_COUNT * layerIndex + TOP_SIDE]; }
	inline TextureManager::ManagedTexture* LeftSide(int layerIndex) { return TextureMap[TEMPLATE_PORTIONS_COUNT * layerIndex + LEFT_SIDE]; }
	inline TextureManager::ManagedTexture* RightSide(int layerIndex) { return TextureMap[TEMPLATE_PORTIONS_COUNT * layerIndex + RIGHT_SIDE]; }
	inline TextureManager::ManagedTexture* BottomSide(int layerIndex) { return TextureMap[TEMPLATE_PORTIONS_COUNT * layerIndex + BOTTOM_SIDE]; }
	inline TextureManager::ManagedTexture* Middle(int layerIndex) { return TextureMap[TEMPLATE_PORTIONS_COUNT * layerIndex + MIDDLE]; }

	GUITemplatedBox()
	{
	}

	GUITemplatedBox(const char* uiObjectType, const char* templateName, int boxCount)
	{
		TextureMap.clear();
		auto templateFolder("Assets/UITemplates/" + std::string(uiObjectType) + "/" + std::string(templateName) + "/");

		for (auto i = 0; i < boxCount; ++i)
		{
			auto layerIndex = TEMPLATE_PORTIONS_COUNT * i;
			TextureMap[layerIndex + TOP_LEFT] = textureManager.LoadTexture(std::string(templateFolder + std::to_string(i) + "_TopLeftCorner.png").c_str());
			TextureMap[layerIndex + TOP_RIGHT] = textureManager.LoadTexture(std::string(templateFolder + std::to_string(i) + "_TopRightCorner.png").c_str());
			TextureMap[layerIndex + BOTTOM_LEFT] = textureManager.LoadTexture(std::string(templateFolder + std::to_string(i) + "_BottomLeftCorner.png").c_str());
			TextureMap[layerIndex + BOTTOM_RIGHT] = textureManager.LoadTexture(std::string(templateFolder + std::to_string(i) + "_BottomRightCorner.png").c_str());
			TextureMap[layerIndex + LEFT_SIDE] = textureManager.LoadTexture(std::string(templateFolder + std::to_string(i) + "_LeftSide.png").c_str());
			TextureMap[layerIndex + RIGHT_SIDE] = textureManager.LoadTexture(std::string(templateFolder + std::to_string(i) + "_RightSide.png").c_str());
			TextureMap[layerIndex + TOP_SIDE] = textureManager.LoadTexture(std::string(templateFolder + std::to_string(i) + "_TopSide.png").c_str());
			TextureMap[layerIndex + BOTTOM_SIDE] = textureManager.LoadTexture(std::string(templateFolder + std::to_string(i) + "_BottomSide.png").c_str());
			TextureMap[layerIndex + MIDDLE] = textureManager.LoadTexture(std::string(templateFolder + std::to_string(i) + "_Middle.png").c_str());
		}
	}

	void Render(int textureIndex, int x, int y, int w, int h, bool cutoff = false, int fullWidth = -1)
	{
		assert(TextureMap.empty() == false);
		if (TextureMap.empty() == true) return;

		auto layerIndex = TEMPLATE_PORTIONS_COUNT * textureIndex;
		if (cutoff)
		{
			if (w == 0) return;
			auto leftWidth = std::min<int>(w, TopLeft(textureIndex)->getWidth());
			TextureMap[layerIndex + TOP_LEFT]->RenderTexturePart(x, y, 0, 0, leftWidth, TopLeft(textureIndex)->getHeight());
			TextureMap[layerIndex + LEFT_SIDE]->RenderTexturePart(x, y + TopLeft(textureIndex)->getHeight(), 0, 0, leftWidth, h - TopLeft(textureIndex)->getHeight() - BottomLeft(textureIndex)->getHeight());
			TextureMap[layerIndex + BOTTOM_LEFT]->RenderTexturePart(x, y + h - BottomLeft(textureIndex)->getHeight(), 0, 0, leftWidth, BottomLeft(textureIndex)->getHeight());

			auto middleWidth = std::min<int>(w - TopLeft(textureIndex)->getWidth(), fullWidth - TopLeft(textureIndex)->getWidth() - TopRight(textureIndex)->getWidth());
			if (middleWidth <= 0) return;
			TextureMap[layerIndex + TOP_SIDE]->RenderTexturePart(x + TopLeft(textureIndex)->getWidth(), y, 0, 0, middleWidth, TopSide(textureIndex)->getHeight());
			TextureMap[layerIndex + MIDDLE]->RenderTexturePart(x + LeftSide(textureIndex)->getWidth(), y + TopSide(textureIndex)->getHeight(), 0, 0, middleWidth, h - TopSide(textureIndex)->getHeight() - BottomSide(textureIndex)->getHeight());
			TextureMap[layerIndex + BOTTOM_SIDE]->RenderTexturePart(x + BottomLeft(textureIndex)->getWidth(), y + h - BottomSide(textureIndex)->getHeight(), 0, 0, middleWidth, BottomSide(textureIndex)->getHeight());

			auto rightWidth = w - leftWidth - middleWidth;
			if (rightWidth <= 0) return;
			TextureMap[layerIndex + TOP_RIGHT]->RenderTexturePart(x + fullWidth - TopRight(textureIndex)->getWidth(), y, 0, 0, rightWidth, TopRight(textureIndex)->getHeight());
			TextureMap[layerIndex + RIGHT_SIDE]->RenderTexturePart(x + fullWidth - RightSide(textureIndex)->getWidth(), y + TopRight(textureIndex)->getHeight(), 0, 0, rightWidth, h - TopRight(textureIndex)->getHeight() - BottomRight(textureIndex)->getHeight());
			TextureMap[layerIndex + BOTTOM_RIGHT]->RenderTexturePart(x + fullWidth - BottomRight(textureIndex)->getWidth(), y + h - BottomRight(textureIndex)->getHeight(), 0, 0, rightWidth, BottomLeft(textureIndex)->getHeight());
		}
		else
		{
			TextureMap[layerIndex + TOP_LEFT]->RenderTexture(x, y, TopLeft(textureIndex)->getWidth(), TopLeft(textureIndex)->getHeight());
			TextureMap[layerIndex + LEFT_SIDE]->RenderTexture(x, y + TopLeft(textureIndex)->getHeight(), LeftSide(textureIndex)->getWidth(), h - TopLeft(textureIndex)->getHeight() - BottomLeft(textureIndex)->getHeight());
			TextureMap[layerIndex + BOTTOM_LEFT]->RenderTexture(x, y + h - BottomLeft(textureIndex)->getHeight(), BottomLeft(textureIndex)->getWidth(), BottomLeft(textureIndex)->getHeight());

			TextureMap[layerIndex + TOP_SIDE]->RenderTexture(x + TopLeft(textureIndex)->getWidth(), y, w - BottomLeft(textureIndex)->getWidth() - BottomRight(textureIndex)->getWidth(), TopSide(textureIndex)->getHeight());
			TextureMap[layerIndex + MIDDLE]->RenderTexture(x + LeftSide(textureIndex)->getWidth(), y + TopSide(textureIndex)->getHeight(), w - LeftSide(textureIndex)->getWidth() - RightSide(textureIndex)->getWidth(), h - TopSide(textureIndex)->getHeight() - BottomSide(textureIndex)->getHeight());
			TextureMap[layerIndex + BOTTOM_SIDE]->RenderTexture(x + BottomLeft(textureIndex)->getWidth(), y + h - BottomSide(textureIndex)->getHeight(), w - BottomLeft(textureIndex)->getWidth() - BottomRight(textureIndex)->getWidth(), BottomSide(textureIndex)->getHeight());

			TextureMap[layerIndex + TOP_RIGHT]->RenderTexture(x + w - TopRight(textureIndex)->getWidth(), y, TopRight(textureIndex)->getWidth(), TopRight(textureIndex)->getHeight());
			TextureMap[layerIndex + RIGHT_SIDE]->RenderTexture(x + w - RightSide(textureIndex)->getWidth(), y + TopRight(textureIndex)->getHeight(), RightSide(textureIndex)->getWidth(), h - TopRight(textureIndex)->getHeight() - BottomRight(textureIndex)->getHeight());
			TextureMap[layerIndex + BOTTOM_RIGHT]->RenderTexture(x + w - BottomRight(textureIndex)->getWidth(), y + h - BottomRight(textureIndex)->getHeight(), BottomRight(textureIndex)->getWidth(), BottomLeft(textureIndex)->getHeight());
		}
	}
};