#pragma once

#include "GUIObjectNode.h"
#include "InputManager.h"

#include <functional>

class GUIDropDown : public GUIObjectNode
{
public:
	typedef std::function<void(GUIObjectNode*)> GUIDropDownSelectCallback;

	static GUIDropDown* CreateDropDown(const char* imageFile, int x = 0, int y = 0, int w = 0, int h = 0);
	static GUIDropDown* CreateTemplatedDropDown(const char* dropdownTemplate, int x = 0, int y = 0, int w = 0, int h = 0, int dropDownX = 0, int dropDownY = 0, int dropDownW = 0, int dropDownH = 0);

	explicit GUIDropDown(bool templated);
	~GUIDropDown();

	void SetItemSelectCallback(const GUIDropDownSelectCallback& callback) { m_ItemSelectCallback = callback; }

	void Input(int xOffset = 0, int yOffset = 0) override;
	void Render(int xOffset = 0, int yOffset = 0) override;

	void SetToDestroy(std::stack<GUIObjectNode*>& destroyList) override;

	inline void AddItem(GUIObjectNode* item) { item->m_Created = true;  m_ItemList.push_back(item); if (m_Clicked) UpdateExpandedHeight(); }
	inline void ClearItems() { for (auto iter = m_ItemList.begin(); iter != m_ItemList.end(); ++iter) { guiManager.DestroyNode((*iter)); } m_ItemList.clear(); }
	inline void SelectItem(unsigned int index) { SelectedIndex = std::min<int>(index, static_cast<unsigned int>(m_ItemList.size() - 1)); }
	inline const GUIObjectNode* GetSelectedItem() const { return (SelectedIndex == -1) ? nullptr : m_ItemList[SelectedIndex]; }
	inline int GetSelectedIndex() const { return SelectedIndex; }

private:
	GUIDropDownSelectCallback	m_ItemSelectCallback;

	bool m_Clicked;
	unsigned int SelectedIndex;
	std::vector<GUIObjectNode*> m_ItemList;
	int DropDownX;
	int DropDownY;
	unsigned int DropDownW;
	unsigned int DropDownH;

	bool m_Templated;
	TextureManager::ManagedTexture* TextureTopLeftCorner;
	TextureManager::ManagedTexture* TextureTopRightCorner;
	TextureManager::ManagedTexture* TextureBottomLeftCorner;
	TextureManager::ManagedTexture* TextureBottomRightCorner;
	TextureManager::ManagedTexture* TextureLeftSide;
	TextureManager::ManagedTexture* TextureRightSide;
	TextureManager::ManagedTexture* TextureTopSide;
	TextureManager::ManagedTexture* TextureBottomSide;
	TextureManager::ManagedTexture* TextureMiddle;
	TextureManager::ManagedTexture* TextureDropDown;
	TextureManager::ManagedTexture* TextureSelector;

	unsigned int ExpandedHeight;
	inline void UpdateExpandedHeight() { ExpandedHeight = static_cast<unsigned int>(m_ItemList.size()) * m_Height + TextureTopSide->getHeight() + TextureBottomSide->getHeight(); }
};

inline GUIDropDown* GUIDropDown::CreateDropDown(const char* imageFile, int x, int y, int w, int h)
{
	MANAGE_MEMORY_NEW("MenuUI_Dropdown", sizeof(GUIDropDown));
	auto newDropDown = new GUIDropDown(false);
	newDropDown->SetTextureID(textureManager.LoadTextureGetID(imageFile));
	newDropDown->SetX(x);
	newDropDown->SetY(y);
	newDropDown->SetWidth(w);
	newDropDown->SetHeight(h);
	return newDropDown;
}

inline GUIDropDown* GUIDropDown::CreateTemplatedDropDown(const char* dropdownTemplate, int x, int y, int w, int h, int dropDownX, int dropDownY, int dropDownW, int dropDownH)
{
	MANAGE_MEMORY_NEW("MenuUI_Dropdown", sizeof(GUIDropDown));
	auto newDropDown = new GUIDropDown(true);

	auto templateFolder("Assets/UITemplates/DropDown/" + std::string(dropdownTemplate) + "/");

	newDropDown->DropDownX = dropDownX;
	newDropDown->DropDownY = dropDownY;
	newDropDown->DropDownW = dropDownW;
	newDropDown->DropDownH = dropDownH;

	newDropDown->TextureTopLeftCorner = textureManager.LoadTexture(std::string(templateFolder + "TopLeftCorner.png").c_str());
	newDropDown->TextureTopRightCorner = textureManager.LoadTexture(std::string(templateFolder + "TopRightCorner.png").c_str());
	newDropDown->TextureBottomLeftCorner = textureManager.LoadTexture(std::string(templateFolder + "BottomLeftCorner.png").c_str());
	newDropDown->TextureBottomRightCorner = textureManager.LoadTexture(std::string(templateFolder + "BottomRightCorner.png").c_str());
	newDropDown->TextureLeftSide = textureManager.LoadTexture(std::string(templateFolder + "LeftSide.png").c_str());
	newDropDown->TextureRightSide = textureManager.LoadTexture(std::string(templateFolder + "RightSide.png").c_str());
	newDropDown->TextureTopSide = textureManager.LoadTexture(std::string(templateFolder + "TopSide.png").c_str());
	newDropDown->TextureBottomSide = textureManager.LoadTexture(std::string(templateFolder + "BottomSide.png").c_str());
	newDropDown->TextureMiddle = textureManager.LoadTexture(std::string(templateFolder + "Middle.png").c_str());
	newDropDown->TextureDropDown = textureManager.LoadTexture(std::string(templateFolder + "DropDown.png").c_str());
	newDropDown->TextureSelector = textureManager.LoadTexture(std::string(templateFolder + "Selector.png").c_str());
	newDropDown->SetTextureID(0);

	newDropDown->SetX(x);
	newDropDown->SetY(y);
	newDropDown->SetWidth(w);
	newDropDown->SetHeight(h);

	newDropDown->SetClickX(dropDownX + dropDownW / 2);
	newDropDown->SetClickY(dropDownY + dropDownH / 2);

	return newDropDown;
}

inline GUIDropDown::GUIDropDown(bool templated) :
	m_ItemSelectCallback(nullptr),
	m_Clicked(false),
	SelectedIndex(0),
	DropDownX(0),
	DropDownY(0),
	DropDownW(0),
	DropDownH(0),
	m_Templated(templated),
	TextureTopLeftCorner(nullptr),
	TextureTopRightCorner(nullptr),
	TextureBottomLeftCorner(nullptr),
	TextureBottomRightCorner(nullptr),
	TextureLeftSide(nullptr),
	TextureRightSide(nullptr),
	TextureTopSide(nullptr),
	TextureBottomSide(nullptr),
	TextureMiddle(nullptr),
	TextureDropDown(nullptr),
	TextureSelector(nullptr),
	ExpandedHeight(0)
{
}


inline GUIDropDown::~GUIDropDown()
{
	MANAGE_MEMORY_DELETE("MenuUI_Dropdown", sizeof(GUIDropDown));
}


inline void GUIDropDown::Input(int xOffset, int yOffset)
{
	if (m_SetToDestroy || !m_Visible) return;

	auto leftButtonState = inputManager.GetMouseButtonLeft();
	auto x = m_X + xOffset;
	auto y = m_Y + yOffset;
	auto mx = inputManager.GetMouseX();
	auto my = inputManager.GetMouseY();

	if (!m_Clicked)
	{
		//  If we click the drop-down button, drop it down and update the expanded height for rendering
		if ((leftButtonState == MOUSE_BUTTON_PRESSED) && (mx > x + DropDownX) && (mx < x + DropDownX + int(DropDownW)) && (my > y + DropDownY) && (my < y + DropDownY + int(DropDownH)))
		{
			inputManager.TakeMouseButtonLeft();
			m_Clicked = true;
			UpdateExpandedHeight();
			return;
		}
	}
	else
	{
		//  If we are inside of the drop-down box, select an index based on height
		if ((mx > x) && (mx < x + m_Width) && (my > y + m_Height) && (my < y + int(ExpandedHeight)))
		{
			SelectedIndex = static_cast<unsigned int>((my - y - m_Height) / m_Height);
		}

		//  If we click anywhere, close the drop-down box
		if (leftButtonState == MOUSE_BUTTON_PRESSED)
		{
			ExpandedHeight = 0;
			m_Clicked = false;
			return;
		}
	}

	//  Take base node input
	GUIObjectNode::Input(xOffset, yOffset);
}

inline void GUIDropDown::Render(int xOffset, int yOffset)
{
	glColor4f(m_Color.colorValues[0], m_Color.colorValues[1], m_Color.colorValues[2], m_Color.colorValues[3]);

	//  Render the object if we're able
	if (!m_SetToDestroy && m_Visible && ((m_TextureID != 0) || m_Templated) && m_Width > 0 && m_Height > 0)
	{
		auto x = m_X + xOffset;
		auto y = m_Y + yOffset;

		if (m_Templated)
		{
			TextureTopLeftCorner->RenderTexture(x, y, TextureTopLeftCorner->getWidth(), TextureTopLeftCorner->getHeight());
			TextureTopRightCorner->RenderTexture(x + m_Width - TextureRightSide->getWidth(), y, TextureTopRightCorner->getWidth(), TextureTopRightCorner->getHeight());
			TextureBottomLeftCorner->RenderTexture(x, y + m_Height - TextureBottomSide->getHeight() + ExpandedHeight, TextureBottomLeftCorner->getWidth(), TextureBottomLeftCorner->getHeight());
			TextureBottomRightCorner->RenderTexture(x + m_Width - TextureRightSide->getWidth(), y + m_Height - TextureBottomSide->getHeight() + ExpandedHeight, TextureBottomRightCorner->getWidth(), TextureBottomRightCorner->getHeight());
			TextureLeftSide->RenderTexture(x, y + TextureTopSide->getHeight(), TextureLeftSide->getWidth(), m_Height - TextureTopSide->getHeight() - TextureBottomSide->getHeight() + ExpandedHeight);
			TextureRightSide->RenderTexture(x + m_Width - TextureRightSide->getWidth(), y + TextureTopSide->getHeight(), TextureRightSide->getWidth(), m_Height - TextureTopSide->getHeight() - TextureBottomSide->getHeight() + ExpandedHeight);
			TextureTopSide->RenderTexture(x + TextureLeftSide->getWidth(), y, m_Width - TextureLeftSide->getWidth() - TextureRightSide->getWidth(), TextureTopSide->getHeight());
			TextureBottomSide->RenderTexture(x + TextureLeftSide->getWidth(), y + m_Height - TextureBottomSide->getHeight() + ExpandedHeight, m_Width - TextureLeftSide->getWidth() - TextureRightSide->getWidth(), TextureBottomSide->getHeight());
			TextureMiddle->RenderTexture(x + TextureLeftSide->getWidth(), y + TextureTopSide->getHeight(), m_Width - TextureLeftSide->getWidth() - TextureRightSide->getWidth(), m_Height - TextureTopSide->getHeight() - TextureBottomSide->getHeight() + ExpandedHeight);
			TextureDropDown->RenderTexture(x + DropDownX, y + DropDownY, DropDownW, DropDownH);

			if (!m_ItemList.empty())
			{
				m_ItemList[SelectedIndex]->Render(x + 10, y);

				if (m_Clicked)
				{
					auto i = 1;
					for (auto iter = m_ItemList.begin(); iter != m_ItemList.end(); ++iter, ++i)
					{
						(*iter)->Render(x + 10, y + m_Height * i);
					}

					TextureSelector->RenderTexture(x + TextureLeftSide->getWidth(), y + TextureTopSide->getHeight() + m_Height * (SelectedIndex + 1), m_Width - TextureLeftSide->getWidth() - TextureRightSide->getWidth(), m_Height - TextureTopSide->getHeight() - TextureBottomSide->getHeight());
				}
			}
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, m_TextureID);

			glBegin(GL_QUADS);
				glTexCoord2f(0.0f, 0.0f); glVertex2i(x + m_Width, y + m_Height);
				glTexCoord2f(1.0f, 0.0f); glVertex2i(x + m_Width - m_Width, y + m_Height);
				glTexCoord2f(1.0f, 1.0f); glVertex2i(x + m_Width - m_Width, y + m_Height - m_Height);
				glTexCoord2f(0.0f, 1.0f); glVertex2i(x + m_Width, y + m_Height - m_Height);
			glEnd();
		}
	}

	//  Pass the render call to all children
	for (auto iter = m_Children.begin(); iter != m_Children.end(); ++iter) (*iter)->Render(xOffset + m_X, yOffset + m_Y);
}

inline void GUIDropDown::SetToDestroy(std::stack<GUIObjectNode*>& destroyList)
{
	//  Pass the 'set to destroy' call to all items in item list
	for (auto iter = m_ItemList.begin(); iter != m_ItemList.end(); ++iter) (*iter)->SetToDestroy(destroyList);

	GUIObjectNode::SetToDestroy(destroyList);
}