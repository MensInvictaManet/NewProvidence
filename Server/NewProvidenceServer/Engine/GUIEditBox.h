#pragma once

#include "GUIObjectNode.h"
#include "InputManager.h"
#include "FontManager.h"
#include "TimeSlice.h"

constexpr Uint32 ticksPerCursorSwitch = 500;

class GUIEditBox : public GUIObjectNode
{
public:
	enum TextAlignment { ALIGN_LEFT, ALIGN_CENTER };

	static GUIEditBox* CreateEditBox(const char* imageFile, int x = 0, int y = 0, int w = 0, int h = 0);
	static GUIEditBox* CreateTemplatedEditBox(const char* editboxTemplate, int x = 0, int y = 0, int w = 0, int h = 0);

	explicit GUIEditBox(bool templated);
	~GUIEditBox();

	void SetEnterKeyCallback(const GUIFunctionCallback& callback) { m_EnterKeyCallback = callback; }
	void SetTabKeyCallback(const GUIFunctionCallback& callback) { m_TabKeyCallback = callback; }

	inline void SetFont(const char* font) { m_Font = fontManager.GetFont(font); }
	inline void SetFont(const Font* font) { m_Font = font; }
	inline void SetText(const std::string text) { m_Text = text; ResetHiddenText(); }
	inline std::string GetText() const { return m_Text; }
	inline void SetTextAlignment(TextAlignment alignment) { m_TextAlignment = alignment; }
	inline void SetTemplate(const char* templateName) { if (strlen(templateName) == 0) { m_Templated = false; return; } m_Templated = true;  m_TemplateBox = GUITemplatedBox("EditBox", templateName, 2); }

	inline void SetEmptyText(std::string emptyText) { m_EmptyText = emptyText; }
	inline void SetEmptyTextColor(Color emptyTextColor) { m_EmptyTextColor = emptyTextColor; }
	inline void SetMaxStringLength(int maxStringLength) { m_MaxStringLength = maxStringLength; }
	inline void SetTextHidden(bool hidden) { m_TextHidden = hidden; }
	inline void SetHiddenCharacter(char hiddenChar) { m_HiddenChar = hiddenChar; ResetHiddenText(); }

	inline void ResetHiddenText() { m_HiddenText.clear(); for (auto i = 0; i < int(m_Text.size()); ++i) m_HiddenText += m_HiddenChar; }

	inline bool GetSelected() const { return m_Selected; }
	inline void SetSelected(bool selected) { m_Selected = selected; }

	virtual void Input(int xOffset = 0, int yOffset = 0) override;
	virtual void TrueRender(int x = 0, int y = 0) override;

private:
	GUIFunctionCallback m_EnterKeyCallback;
	GUIFunctionCallback m_TabKeyCallback;
	bool m_Selected;
	const Font* m_Font;
	std::string m_Text;

	std::string m_EmptyText;
	Color m_EmptyTextColor;
	int m_MaxStringLength = 128;
	bool m_TextHidden = false;
	char m_HiddenChar = '*';
	std::string m_HiddenText;

	TextAlignment m_TextAlignment;

	bool m_Templated;
	GUITemplatedBox m_TemplateBox;

	const float TIME_BETWEEN_BACKSPACES = 0.1f;
	float m_LastBackspaceTime;
};

inline GUIEditBox* GUIEditBox::CreateEditBox(const char* imageFile, int x, int y, int w, int h)
{
	MANAGE_MEMORY_NEW("MenuUI_Editbox", sizeof(GUIEditBox));
	auto newEditbox = new GUIEditBox(false);
	newEditbox->SetTextureID(textureManager.LoadTextureGetID(imageFile));
	newEditbox->SetPosition(x, y);
	newEditbox->SetDimensions(w, h);
	return newEditbox;
}

inline GUIEditBox* GUIEditBox::CreateTemplatedEditBox(const char* templateName, int x, int y, int w, int h)
{
	MANAGE_MEMORY_NEW("MenuUI_Editbox", sizeof(GUIEditBox));
	auto newEditbox = new GUIEditBox(true);

	auto templateFolder("Assets/UITemplates/EditBox/" + std::string(templateName) + "/");
	newEditbox->SetTemplate(templateName);
	newEditbox->SetTextureID(0);

	newEditbox->SetPosition(x, y);
	newEditbox->SetDimensions(w, h);

	newEditbox->SetClickX(w / 2);
	newEditbox->SetClickY(h / 2);

	return newEditbox;
}

inline GUIEditBox::GUIEditBox(bool templated) :
	m_Selected(false),
	m_Font(nullptr),
	m_Text(""),
	m_EmptyText(""),
	m_EmptyTextColor(Color(0.0f, 0.0f, 0.0f, 1.0f)),
	m_MaxStringLength(128),
	m_TextHidden(false),
	m_HiddenChar('*'),
	m_HiddenText(""),
	m_TextAlignment(ALIGN_CENTER),
	m_LastBackspaceTime(0),
	m_Templated(templated)
{
}


inline GUIEditBox::~GUIEditBox()
{
	MANAGE_MEMORY_DELETE("MenuUI_Editbox", sizeof(GUIEditBox));
}


inline void GUIEditBox::Input(int xOffset, int yOffset)
{
	if (m_SetToDestroy || !m_Visible) return;

	auto leftButtonState = inputManager.GetMouseButtonLeft();
	auto x = inputManager.GetMouseX();
	auto y = inputManager.GetMouseY();

	if (leftButtonState == MOUSE_BUTTON_PRESSED || leftButtonState == MOUSE_BUTTON_PRESSED_TAKEN)
	{
		if ((x > xOffset + m_X) && (x < xOffset + m_X + m_Width) && (y > yOffset + m_Y) && (y < yOffset + m_Y + m_Height))
		{
			if (leftButtonState == MOUSE_BUTTON_PRESSED)
			{
				inputManager.TakeMouseButtonLeft();
				m_Selected = true;
			}
		}
		else m_Selected = false;
	}

	//  If selected, take keyboard text input
	if (m_Selected)
	{
		if (inputManager.GetEnter() && m_EnterKeyCallback != nullptr)	{ m_EnterKeyCallback(this);		inputManager.ResetEnter(InputManager::KEY_STATE_HELD);		return; }
		if (inputManager.GetTab() && m_TabKeyCallback != nullptr)		{ m_TabKeyCallback(this);		inputManager.ResetTab(InputManager::KEY_STATE_HELD);		return; }

		if (inputManager.GetBackspace() && !m_Text.empty() && (gameSecondsF - m_LastBackspaceTime >= TIME_BETWEEN_BACKSPACES))
		{
			m_Text.erase(--m_Text.end());
			m_LastBackspaceTime = gameSecondsF;
			m_HiddenText.erase(--m_HiddenText.end());
		}
		else if (int(m_Text.length()) < m_MaxStringLength)
		{
			m_Text += inputManager.GetKeyboardString();
			ResetHiddenText();
		}
	}

	//  Take base node input
	GUIObjectNode::Input(xOffset, yOffset);
}

inline void GUIEditBox::TrueRender(int x, int y)
{
	if (m_Width > 0 && m_Height > 0)
	{
		if (m_Templated)
		{
			auto pressedIndex = (m_Selected ? 1 : 0);
			m_TemplateBox.Render(pressedIndex, x, y, m_Width, m_Height);
		}
		else if (m_TextureID != 0)
		{
			Draw2DTexturedSquare(m_TextureID, x, y, m_Width, m_Height);
		}

		//  Render the font the same way regardless of templating
		if (m_Font != nullptr)
		{
			auto textX = (m_TextAlignment == ALIGN_CENTER) ? (x + m_Width / 2) : x + 4;

			auto showingCursor = m_Selected && ((gameTicksUint % (ticksPerCursorSwitch * 2)) < ticksPerCursorSwitch);
			auto textString = (m_TextHidden ? m_HiddenText : m_Text);
			auto textDisplay = showingCursor ? (textString + "|") : textString;

			//  If our text is empty and we have an "empty" setting text, show it in the empty text color
			if (m_Text.empty() && !m_EmptyText.empty())
				m_Font->RenderText(m_EmptyText.c_str(), textX, y + m_Height / 2, (m_TextAlignment == ALIGN_CENTER), true, 1.0f, 1.0f, m_EmptyTextColor);

			//  We can have a text to display even if our text is empty (because of the cursor) so display it if it exists
			if (!textDisplay.empty())
				m_Font->RenderText(textDisplay.c_str(), textX, y + m_Height / 2, (m_TextAlignment == ALIGN_CENTER), true);
		}
	}
}