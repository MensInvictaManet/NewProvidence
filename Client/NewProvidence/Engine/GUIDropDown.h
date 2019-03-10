#pragma once

#include "GUIObjectNode.h"
#include "InputManager.h"

class GUIDropDown : public GUIObjectNode
{
public:
	static GUIDropDown* CreateTemplatedDropDown(const char* dropdownTemplate, int x = 0, int y = 0, int w = 0, int h = 0, int dropDownX = 0, int dropDownY = 0, int dropDownW = 0, int dropDownH = 0);

	explicit GUIDropDown(bool templated);
	~GUIDropDown();

	void SetItemSelectCallback(const GUIFunctionCallback& callback) { m_ItemSelectCallback = callback; }

	virtual void Input(int xOffset = 0, int yOffset = 0) override;
	virtual void TrueRender(int x = 0, int y = 0) override;

	void SetToDestroy(std::stack<GUIObjectNode*>& destroyList) override;

	inline void AddItem(GUIObjectNode* item) { item->m_Created = true;  m_ItemList.push_back(item); if (m_Clicked) UpdateExpandedHeight(); }
	inline void ClearItems() { for (auto iter = m_ItemList.begin(); iter != m_ItemList.end(); ++iter) { guiManager.DestroyNode((*iter)); } m_ItemList.clear(); SelectedIndex = 0; }
	inline const GUIObjectNode* GetSelectedItem() const { return (SelectedIndex == -1) ? nullptr : m_ItemList[SelectedIndex]; }
	inline int GetSelectedIndex() const { return SelectedIndex; }
	inline void SetTemplate(const char* templateName) { if (strlen(templateName) == 0) { m_Templated = false; return; } m_Templated = true;  m_TemplateBox = GUITemplatedBox("DropDown", templateName, 1); }

private:
	GUIFunctionCallback	m_ItemSelectCallback;

	bool m_Clicked;
	unsigned int SelectedIndex;
	std::vector<GUIObjectNode*> m_ItemList;
	int DropDownX;
	int DropDownY;
	unsigned int DropDownW;
	unsigned int DropDownH;

	bool m_Templated;
	GUITemplatedBox m_TemplateBox;
	TextureManager::ManagedTexture* TextureDropDown;
	TextureManager::ManagedTexture* TextureSelector;

	unsigned int ExpandedHeight;
	inline void UpdateExpandedHeight() { ExpandedHeight = static_cast<unsigned int>(m_ItemList.size()) * m_Height + (m_Templated ? (m_TemplateBox.TopSide(0)->getHeight() + m_TemplateBox.BottomSide(0)->getHeight()) : 0); }
};

GUIDropDown* GUIDropDown::CreateTemplatedDropDown(const char* templateName, int x, int y, int w, int h, int dropDownX, int dropDownY, int dropDownW, int dropDownH)
{
	MANAGE_MEMORY_NEW("MenuUI_Dropdown", sizeof(GUIDropDown));
	auto newDropDown = new GUIDropDown(true);

	newDropDown->DropDownX = dropDownX;
	newDropDown->DropDownY = dropDownY;
	newDropDown->DropDownW = dropDownW;
	newDropDown->DropDownH = dropDownH;

	auto templateFolder("Assets/UITemplates/DropDown/" + std::string(templateName) + "/");
	newDropDown->SetTemplate(templateName);
	newDropDown->TextureDropDown = textureManager.LoadTexture(std::string(templateFolder + "DropDown.png").c_str());
	newDropDown->TextureSelector = textureManager.LoadTexture(std::string(templateFolder + "Selector.png").c_str());
	newDropDown->SetTextureID(0);

	newDropDown->SetPosition(x, y);
	newDropDown->SetDimensions(w, h);

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
	TextureDropDown(nullptr),
	TextureSelector(nullptr),
	ExpandedHeight(0)
{
}


GUIDropDown::~GUIDropDown()
{
	MANAGE_MEMORY_DELETE("MenuUI_Dropdown", sizeof(GUIDropDown));
}


void GUIDropDown::Input(int xOffset, int yOffset)
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
			SetRenderLast(true);
			UpdateExpandedHeight();
			return;
		}
	}
	else
	{
		//  If we are inside of the drop-down box, select an index based on height
		if ((mx > x) && (mx < x + m_Width) && (my > y + m_Height) && (my < y + int(ExpandedHeight)))
		{
			auto oldSelection = SelectedIndex;
			SelectedIndex = static_cast<unsigned int>((my - y - m_Height) / m_Height);
			if ((oldSelection != SelectedIndex) && (m_ItemSelectCallback != nullptr)) m_ItemSelectCallback(this);
		}

		//  If we click anywhere, close the drop-down box
		if (leftButtonState == MOUSE_BUTTON_PRESSED)
		{
			inputManager.TakeMouseButtonLeft();
			ExpandedHeight = 0;
			m_Clicked = false;
			SetRenderLast(false);
			return;
		}
	}

	//  Take base node input
	GUIObjectNode::Input(xOffset, yOffset);
}

void GUIDropDown::TrueRender(int x, int y)
{
	if (((m_TextureID != 0) || m_Templated) && m_Width > 0 && m_Height > 0)
	{
		if (m_Templated)
		{
			m_TemplateBox.Render(0, x, y, m_Width, m_Height + ExpandedHeight);
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

					TextureSelector->RenderTexture(x + m_TemplateBox.LeftSide(0)->getWidth(), y + m_TemplateBox.TopSide(0)->getHeight() + m_Height * (SelectedIndex + 1), m_Width - m_TemplateBox.LeftSide(0)->getWidth() - m_TemplateBox.RightSide(0)->getWidth(), m_Height - m_TemplateBox.TopSide(0)->getHeight() - m_TemplateBox.BottomSide(0)->getHeight());
				}
			}
		}
	}
}

void GUIDropDown::SetToDestroy(std::stack<GUIObjectNode*>& destroyList)
{
	//  Pass the 'set to destroy' call to all items in item list
	for (auto iter = m_ItemList.begin(); iter != m_ItemList.end(); ++iter) (*iter)->SetToDestroy(destroyList);

	GUIObjectNode::SetToDestroy(destroyList);
}