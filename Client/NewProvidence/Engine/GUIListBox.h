#pragma once

#include "GUIObjectNode.h"
#include "InputManager.h"
#include "TimeSlice.h"

#define LIST_BOX_SCROLL_DELAY 0.1f

class GUIListBox : public GUIObjectNode
{
public:
	static GUIListBox* CreateListBox(const char* imageFile, int x = 0, int y = 0, int w = 0, int h = 0, int entryHeight = 100, int spaceBetweenEntries = 0);
	static GUIListBox* CreateTemplatedListBox(const char* listboxTemplate, int x = 0, int y = 0, int w = 0, int h = 0, int dirButtonsX = 0, int contentY = 0, int upButtonW = 0, int upButtonH = 0, int downButtonW = 0, int downButtonH = 0, int barColumnW = 0, int entryHeight = 0, int spaceBetweenEntries = 0);

	explicit GUIListBox(bool templated);
	~GUIListBox();

	void SetHeight(int height) override { GUIObjectNode::SetHeight(height); ItemDisplayCount = height / (EntryHeight + SpaceBetweenEntries); }

	void SetItemClickCallback(const GUIFunctionCallback& callback) { m_ItemClickCallback = callback; }

	virtual void Input(int xOffset = 0, int yOffset = 0) override;
	virtual void Update(void) override;
	virtual void TrueRender(int x = 0, int y = 0) override;

	void SetToDestroy(std::stack<GUIObjectNode*>& destroyList) override;

	void AddItem(GUIObjectNode* item) { item->m_Created = true; m_ItemList.push_back(item); UpdateMover(m_FlowToBottom ? std::max<int>(int(m_ItemList.size()) - ItemDisplayCount, 0) : -1); }
	void RemoveItem(GUIObjectNode* item) { for (auto i = m_ItemList.begin(); i != m_ItemList.end(); ++i) if ((*i) == item) { m_ItemList.erase(i); return; } }
	void ClearItems() { for (auto iter = m_ItemList.begin(); iter != m_ItemList.end(); ++iter) { guiManager.DestroyNode((*iter)); } m_ItemList.clear(); SelectedIndex = -1; }
	GUIObjectNode* GetSelectedItem() { return (SelectedIndex == -1) ? nullptr : m_ItemList[SelectedIndex]; }
	int GetSelectedIndex() const { return SelectedIndex; }
	void SetSelectable(bool selectable) { m_Selectable = selectable; }
	void SetSelectedIndex(int index) { SelectedIndex = index; }
	void SetFlowToBottom(bool flowToBottom) { m_FlowToBottom = flowToBottom; }
	const std::vector<GUIObjectNode*> GetItemList() const { return m_ItemList; }
	unsigned int GetItemCount() const { return static_cast<unsigned int>(m_ItemList.size()); }
	unsigned int GetItemDisplayCount() const { return ItemDisplayCount; }
	inline int GetEntryHeight() const { return EntryHeight; }
	inline int GetSpaceBetweenEntries() const { return SpaceBetweenEntries; }
	inline void SetTemplate(const char* templateName) { if (strlen(templateName) == 0) { m_Templated = false; return; } m_Templated = true;  m_TemplateBox = GUITemplatedBox("ListBox", templateName, 1); SetListboxTemplateData(templateName); }

	inline GUIObjectNode* GetItemByName(std::string objectName);
	inline void SetTemplateData(int dirButtonsX, int contentY, int upButtonW, int upButtonH, int downButtonW, int downButtonH, int barColumnW);
	inline void SetListboxTemplateData(std::string templateName);

private:
	GUIFunctionCallback	m_ItemClickCallback;

	std::vector<GUIObjectNode*> m_ItemList;
	bool m_Selectable;
	int	SelectedIndex;
	int m_FlowToBottom;
	int MovementIndex;
	int MoverHeight;
	int MoverY;
	int MoverYDelta;
	bool m_Clicked;
	int ClickedY;
	float m_LastClickTime;

	bool m_Templated;
	GUITemplatedBox m_TemplateBox;

	TextureManager::ManagedTexture* TextureUpButton;
	TextureManager::ManagedTexture* TextureDownButton;
	TextureManager::ManagedTexture* TextureBarColumn;
	TextureManager::ManagedTexture* TextureMoverTop;
	TextureManager::ManagedTexture* TextureMoverMiddle;
	TextureManager::ManagedTexture* TextureMoverBottom;
	TextureManager::ManagedTexture* TextureSelector;
	int DirectionalButtonsX;
	int ContentY;
	int UpButtonW;
	int UpButtonH;
	int DownButtonW;
	int DownButtonH;
	int BarColumnW;
	int EntryHeight;
	int SpaceBetweenEntries;
	int ItemDisplayCount;

	void UpdateMover(int index = -1);
};

inline GUIListBox* GUIListBox::CreateListBox(const char* imageFile, int x, int y, int w, int h, int entryHeight, int spaceBetweenEntries)
{
	MANAGE_MEMORY_NEW("MenuUI_Listbox", sizeof(GUIListBox));
	auto newListbox = new GUIListBox(false);
	newListbox->SetTextureID(textureManager.LoadTextureGetID(imageFile));
	newListbox->SetPosition(x, y);
	newListbox->SetDimensions(w, h);

	newListbox->EntryHeight = entryHeight;
	newListbox->SpaceBetweenEntries = spaceBetweenEntries;
	newListbox->ItemDisplayCount = h / (entryHeight + spaceBetweenEntries);
	return newListbox;
}

inline GUIListBox* GUIListBox::CreateTemplatedListBox(const char* templateName, int x, int y, int w, int h, int dirButtonsX, int contentY, int upButtonW, int upButtonH, int downButtonW, int downButtonH, int barColumnW, int entryHeight, int spaceBetweenEntries)
{
	MANAGE_MEMORY_NEW("MenuUI_Listbox", sizeof(GUIListBox));
	auto newListbox = new GUIListBox(true);

	newListbox->SetTemplate(templateName);
	newListbox->SetTextureID(0);

	newListbox->DirectionalButtonsX = dirButtonsX;
	newListbox->ContentY = contentY;
	newListbox->UpButtonW = upButtonW;
	newListbox->UpButtonH = upButtonH;
	newListbox->DownButtonW = downButtonW;
	newListbox->DownButtonH = downButtonH;
	newListbox->BarColumnW = barColumnW;

	newListbox->EntryHeight = entryHeight;
	newListbox->SpaceBetweenEntries = spaceBetweenEntries;

	newListbox->SetPosition(x, y);
	newListbox->SetDimensions(w, h);

	newListbox->ItemDisplayCount = h / (entryHeight + spaceBetweenEntries);

	return newListbox;
}

inline GUIListBox::GUIListBox(bool templated) :
	m_ItemClickCallback(nullptr),
	m_Selectable(true),
	SelectedIndex(-1),
	m_FlowToBottom(false),
	MovementIndex(0),
	MoverHeight(-1),
	MoverY(-1),
	MoverYDelta(-1),
	m_Clicked(false),
	ClickedY(-1),
	m_LastClickTime(0.0f),
	m_Templated(templated)
{
	TextureUpButton = nullptr;
	TextureDownButton = nullptr;
	TextureBarColumn = nullptr;
	TextureMoverTop = nullptr;
	TextureMoverMiddle = nullptr;
	TextureMoverBottom = nullptr;
	TextureSelector = nullptr;
	DirectionalButtonsX = 0;
	ContentY = 0;
	UpButtonW = -1;
	UpButtonH = -1;
	DownButtonW = -1;
	DownButtonH = -1;
	BarColumnW = -1;
	EntryHeight = -1;
	SpaceBetweenEntries = -1;
	ItemDisplayCount = -1;
}



inline GUIListBox::~GUIListBox()
{
	MANAGE_MEMORY_DELETE("MenuUI_Listbox", sizeof(GUIListBox));
}


inline void GUIListBox::Input(int xOffset, int yOffset)
{
	if (m_SetToDestroy || !m_Visible) return;

	auto leftButtonState = inputManager.GetMouseButtonLeft();
	auto x = m_X + xOffset;
	auto y = m_Y + yOffset;
	auto mx = inputManager.GetMouseX();
	auto my = inputManager.GetMouseY();

	if (m_Clicked)
	{
		if (leftButtonState == MOUSE_BUTTON_UNPRESSED)
		{
			m_Clicked = false;
			return;
		}

		if (std::abs(my - ClickedY) > MoverYDelta)
		{
			//  The farthest we can shift is to the top or bottom of the list
			auto indexChange = std::min<int>(std::max<int>((my - ClickedY) / MoverYDelta, -MovementIndex), (int(m_ItemList.size()) - ItemDisplayCount - MovementIndex));
			ClickedY += indexChange * MoverYDelta;
			MovementIndex += indexChange;
			UpdateMover();
		}
	}

	if (leftButtonState != MOUSE_BUTTON_PRESSED)
	{
		for (auto i = 0; i != int(m_ItemList.size()); ++i) m_ItemList[i]->Input(x, y + ((EntryHeight + SpaceBetweenEntries) * i));
		return;
	}
	if (mx < x || mx > x + m_Width || my < y || my > y + m_Height) return;

	//  If we're left of the directional buttons, assume we're clicking an entry in the list and find out which one
	if (m_Selectable && mx < x + DirectionalButtonsX)
	{
		auto newSelectedIndex = (int(my) - y - m_TemplateBox.TopSide(0)->getHeight()) / (EntryHeight + SpaceBetweenEntries) + MovementIndex;
		if (newSelectedIndex >= MovementIndex && newSelectedIndex < int(m_ItemList.size()) && newSelectedIndex < MovementIndex + int(ItemDisplayCount))
		{
			//  If this index is already selected, check for input inside of the entry node first
			auto selected_y_offset = (EntryHeight + SpaceBetweenEntries) * (SelectedIndex - MovementIndex);
			if (newSelectedIndex == SelectedIndex) m_ItemList[SelectedIndex]->Input(x, y + selected_y_offset);
			if (inputManager.GetMouseButtonLeft() != MOUSE_BUTTON_PRESSED) return;

			inputManager.TakeMouseButtonLeft();
			bool newSelection = (SelectedIndex != newSelectedIndex);
			SelectedIndex = newSelectedIndex;
			if (newSelection && m_ItemClickCallback != nullptr) m_ItemClickCallback(this);
			return;
		}
	}

	//  If we're clicking to the right of where the directional buttons start horizontally...
	if ((gameSecondsF - m_LastClickTime > LIST_BOX_SCROLL_DELAY) && mx > x + DirectionalButtonsX)
	{
		//  If we're clicking the up button, move the movement index up one if possible
		if ((mx < x + DirectionalButtonsX + UpButtonW) && (my > y + ContentY) && (my < y + ContentY + UpButtonH))
		{
			inputManager.TakeMouseButtonLeft();
			if (MovementIndex > 0) MovementIndex -= 1;
			UpdateMover();
			m_LastClickTime = gameSecondsF;
			return;
		}

		//  If we're clicking the down button, move the movement index down one if possible
		if ((mx < x + DirectionalButtonsX + DownButtonW) && (my > y + m_Height - ContentY - DownButtonH) && (my < y + m_Height - ContentY))
		{
			inputManager.TakeMouseButtonLeft();
			if (MovementIndex < int(m_ItemList.size() - ItemDisplayCount)) MovementIndex += 1;
			UpdateMover();
			m_LastClickTime = gameSecondsF;
			return;
		}
	}

	//  If we're clicking inside of the mover, keep track of our click so we can drag it
	if ((leftButtonState == MOUSE_BUTTON_PRESSED) && (mx > x + DirectionalButtonsX) && (mx < x + DirectionalButtonsX + BarColumnW) && (my > y + ContentY + UpButtonH + MoverY) && (my < y + ContentY + UpButtonH + MoverY + MoverHeight))
	{
		inputManager.TakeMouseButtonLeft();
		m_Clicked = true;
		ClickedY = my;
	}

	//  Take base node input
	GUIObjectNode::Input(xOffset, yOffset);
}


inline void GUIListBox::Update(void)
{
	//  Update the items contained within
	for (auto iter = m_ItemList.begin(); iter != m_ItemList.end(); ++iter)
		(*iter)->Update();

	GUIObjectNode::Update();
}

inline void GUIListBox::TrueRender(int x, int y)
{
	//  Render the object if we're able
	if (m_Width > 0 && m_Height > 0)
	{
		if ((m_TextureID != 0) || m_Templated)
		{
			//  Render the background object, templated or single-textured
			if (m_Templated)
			{
				m_TemplateBox.Render(0, x, y, m_Width, m_Height);
			}
			else
			{
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, m_TextureID);

				glBegin(GL_QUADS);
				glTexCoord2f(0.0f, 0.0f); glVertex2i(x, y);
				glTexCoord2f(1.0f, 0.0f); glVertex2i(x + m_Width, y);
				glTexCoord2f(1.0f, 1.0f); glVertex2i(x + m_Width, y + m_Height);
				glTexCoord2f(0.0f, 1.0f); glVertex2i(x, y + m_Height);
				glEnd();
			}
		}

		//  Render the scroll buttons if needed
		auto renderScrollButtons = (m_Templated && int(m_ItemList.size()) > ItemDisplayCount);
		if (renderScrollButtons)
		{
			TextureUpButton->RenderTexture(x + DirectionalButtonsX, y + ContentY, UpButtonW, UpButtonH);
			TextureBarColumn->RenderTexture(x + DirectionalButtonsX, y + ContentY + UpButtonH, BarColumnW, (m_Height - ContentY - DownButtonH) - (ContentY + UpButtonH));
			TextureDownButton->RenderTexture(x + DirectionalButtonsX, y + m_Height - ContentY - DownButtonH, DownButtonW, DownButtonH);
			TextureMoverTop->RenderTexture(x + DirectionalButtonsX + 1, y + ContentY + UpButtonH + MoverY, BarColumnW - 2, TextureMoverTop->getHeight());
			TextureMoverMiddle->RenderTexture(x + DirectionalButtonsX + 1, y + ContentY + UpButtonH + MoverY + TextureMoverTop->getHeight(), BarColumnW - 2, MoverHeight - TextureMoverTop->getHeight() - TextureMoverBottom->getHeight());
			TextureMoverBottom->RenderTexture(x + DirectionalButtonsX + 1, y + ContentY + UpButtonH + MoverY + MoverHeight - TextureMoverBottom->getHeight(), BarColumnW - 2, TextureMoverBottom->getHeight());
		}

		//  Render the items contained within
		for (auto i = MovementIndex; i < int(m_ItemList.size()) && i < MovementIndex + ItemDisplayCount; ++i)
		{
			m_ItemList[i]->Render(x, y + ((EntryHeight + SpaceBetweenEntries) * (i - MovementIndex)));
		}

		//  Render the selector if an item is selected
		if (m_Selectable && SelectedIndex != -1 && (SelectedIndex >= MovementIndex) && (SelectedIndex < MovementIndex + int(ItemDisplayCount)))
		{
			auto width = (renderScrollButtons ? (DirectionalButtonsX - m_TemplateBox.LeftSide(0)->getWidth()) : m_Width - m_TemplateBox.LeftSide(0)->getWidth() - m_TemplateBox.RightSide(0)->getWidth()) - 2;
			TextureSelector->RenderTexture(x + m_TemplateBox.LeftSide(0)->getWidth(), y + m_TemplateBox.TopSide(0)->getHeight() + (EntryHeight + SpaceBetweenEntries) * (SelectedIndex - MovementIndex), width, EntryHeight);
		}
	}
}

inline void GUIListBox::UpdateMover(int indexOverride)
{
	if (indexOverride != -1) MovementIndex = std::max<int>(std::min<int>(indexOverride, int(m_ItemList.size()) - ItemDisplayCount), 0);

	auto mover_size_percent = float(ItemDisplayCount) / float(m_ItemList.size());
	MoverHeight = static_cast<unsigned int>((float(m_Height - ContentY - DownButtonH) - (ContentY + UpButtonH)) * mover_size_percent);

	//  Determine how much we move up and down when mouse movement warrants a change
	//  NOTE: Even though MoverYDelta is confirmed, we do the calculations twice because rounding the MoverYDelta can lead to a rounding error on MoverY position determination
	auto mover_position_percent_delta = float(1) / float(m_ItemList.size());
	MoverYDelta = static_cast<unsigned int>((float(m_Height - ContentY - UpButtonH) - (ContentY + DownButtonH)) * mover_position_percent_delta);
	MoverY = static_cast<unsigned int>((float(m_Height - ContentY - UpButtonH) - (ContentY + DownButtonH)) * mover_position_percent_delta * float(MovementIndex));

	//  There is a small chance of a float precision error leading to us being 1 pixel away from the bottom when updating the mover at the bottom position. Check for that.
	auto MoverBarHeightTotal = (m_Height - ContentY - ContentY - DownButtonH - UpButtonH);
	if ((MovementIndex + ItemDisplayCount == m_ItemList.size()) && (MoverY + MoverHeight < MoverBarHeightTotal)) MoverY = MoverBarHeightTotal - MoverHeight;
}

inline void GUIListBox::SetToDestroy(std::stack<GUIObjectNode*>& destroyList)
{
	//  Pass the 'set to destroy' call to all items in item list
	for (auto iter = m_ItemList.begin(); iter != m_ItemList.end(); ++iter) (*iter)->SetToDestroy(destroyList);

	GUIObjectNode::SetToDestroy(destroyList);
}


inline GUIObjectNode* GUIListBox::GetItemByName(std::string objectName)
{
	for (auto iter = m_ItemList.begin(); iter != m_ItemList.end(); ++iter)
		if ((*iter)->GetObjectName() == objectName)
			return (*iter);

	return nullptr;
}

inline void GUIListBox::SetTemplateData(int dirButtonsX, int contentY, int upButtonW, int upButtonH, int downButtonW, int downButtonH, int barColumnW)
{
	DirectionalButtonsX = dirButtonsX;
	ContentY = contentY;
	UpButtonW = upButtonW;
	UpButtonH = upButtonH;
	DownButtonW = downButtonW;
	DownButtonH = downButtonH;
	BarColumnW = barColumnW;
}

inline void GUIListBox::SetListboxTemplateData(std::string templateName)
{
	auto templateFolder("Assets/UITemplates/ListBox/" + std::string(templateName) + "/");
	TextureUpButton = textureManager.LoadTexture(std::string(templateFolder + "UpButton.png").c_str());
	TextureDownButton = textureManager.LoadTexture(std::string(templateFolder + "DownButton.png").c_str());
	TextureBarColumn = textureManager.LoadTexture(std::string(templateFolder + "BarColumn.png").c_str());
	TextureMoverTop = textureManager.LoadTexture(std::string(templateFolder + "MoverTop.png").c_str());
	TextureMoverMiddle = textureManager.LoadTexture(std::string(templateFolder + "MoverMiddle.png").c_str());
	TextureMoverBottom = textureManager.LoadTexture(std::string(templateFolder + "MoverBottom.png").c_str());
	TextureSelector = textureManager.LoadTexture(std::string(templateFolder + "Selector.png").c_str());
}