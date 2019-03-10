#pragma once

#include "GUIObjectNode.h"
#include "InputManager.h"
#include "FontManager.h"

class GUIButton : public GUIObjectNode
{
public:
	static GUIButton* CreateButton(const char* imageFile, int x = 0, int y = 0, int w = 0, int h = 0);
	static GUIButton* CreateTemplatedButton(const char* templateName, int x = 0, int y = 0, int w = 0, int h = 0);

	explicit GUIButton(bool templated);
	~GUIButton();

	inline void SetLeftClickCallback(const GUIFunctionCallback& callback) { m_LeftClickCallback = callback; }
	inline void SetMiddleClickCallback(const GUIFunctionCallback& callback) { m_MiddleClickCallback = callback; }
	inline void SetRightClickCallback(const GUIFunctionCallback& callback) { m_RightClickCallback = callback; }
	inline void SetFont(const Font* font) { m_Font = font; }
	inline void SetFont(std::string fontName) { SetFont(fontManager.GetFont(fontName.c_str())); }
	inline void SetText(const std::string text) { m_Text = text; }
	inline void SetPressedSizeRatio(float ratio) { m_PressedSizeRatio = ratio; }
	inline void SetTemplate(const char* templateName) { if (strlen(templateName) == 0) { m_Templated = false; return; } m_Templated = true;  m_TemplateBox = GUITemplatedBox("Button", templateName, 2); }

	virtual void Input(int xOffset = 0, int yOffset = 0) override;
	virtual void TrueRender(int x = 0, int y = 0) override;

private:
	GUIFunctionCallback	m_LeftClickCallback;
	GUIFunctionCallback	m_MiddleClickCallback;
	GUIFunctionCallback	m_RightClickCallback;
	bool m_Pressed;
	const Font* m_Font;
	std::string m_Text;
	float m_PressedSizeRatio;

	bool m_Templated;
	GUITemplatedBox m_TemplateBox;
};

inline GUIButton* GUIButton::CreateButton(const char* imageFile, int x, int y, int w, int h)
{
	MANAGE_MEMORY_NEW("MenuUI_Button", sizeof(GUIButton));
	auto newButton = new GUIButton(false);
	newButton->SetTextureID(textureManager.LoadTextureGetID(imageFile));
	newButton->SetPosition(x, y);
	newButton->SetDimensions(w, h);
	return newButton;
}

inline GUIButton* GUIButton::CreateTemplatedButton(const char* templateName, int x, int y, int w, int h)
{
	MANAGE_MEMORY_NEW("MenuUI_Button", sizeof(GUIButton));
	auto newButton = new GUIButton(true);

	newButton->SetTemplate(templateName);
	newButton->SetTextureID(0);

	newButton->SetPosition(x, y);
	newButton->SetDimensions(w, h);

	newButton->SetClickX(w / 2);
	newButton->SetClickY(h / 2);

	return newButton;
}

inline GUIButton::GUIButton(bool templated) :
	m_LeftClickCallback(nullptr),
	m_MiddleClickCallback(nullptr),
	m_RightClickCallback(nullptr),
	m_Pressed(false),
	m_Font(nullptr),
	m_Text(""),
	m_PressedSizeRatio(0.95f),
	m_Templated(templated)
{
}


inline GUIButton::~GUIButton()
{
	MANAGE_MEMORY_DELETE("MenuUI_Button", sizeof(GUIButton));
}


inline void GUIButton::Input(int xOffset, int yOffset)
{
	if (m_SetToDestroy || !m_Visible) return;

	auto leftButtonState = inputManager.GetMouseButtonLeft();
	auto middleButtonState = inputManager.GetMouseButtonMiddle();
	auto rightButtonState = inputManager.GetMouseButtonRight();
	auto x = inputManager.GetMouseX();
	auto y = inputManager.GetMouseY();

	if (leftButtonState == MOUSE_BUTTON_UNPRESSED) m_Pressed = false;
	if ((x > xOffset + m_X) && (x < xOffset + m_X + m_Width) && (y > yOffset + m_Y) && (y < yOffset + m_Y + m_Height))
	{
		if (leftButtonState == MOUSE_BUTTON_PRESSED) m_Pressed = true;

		if (leftButtonState == MOUSE_BUTTON_PRESSED && m_LeftClickCallback != nullptr) { inputManager.TakeMouseButtonLeft(); m_LeftClickCallback(this); }
		if (middleButtonState == MOUSE_BUTTON_PRESSED && m_MiddleClickCallback != nullptr) { inputManager.TakeMouseButtonMiddle(); m_MiddleClickCallback(this); }
		if (rightButtonState == MOUSE_BUTTON_PRESSED && m_RightClickCallback != nullptr) { inputManager.TakeMouseButtonRight(); m_RightClickCallback(this); }
	}
	else m_Pressed = false;

	//  Take base node input
	GUIObjectNode::Input(xOffset, yOffset);
}

inline void GUIButton::TrueRender(int x, int y)
{
	if (((m_TextureID != 0) || m_Templated) && m_Width > 0 && m_Height > 0)
	{
		auto pressedSqueeze = m_PressedSizeRatio;

		if (m_Templated)
		{
			auto pressedIndex = (m_Pressed ? 1 : 0);
			pressedSqueeze = 1.0f;

			m_TemplateBox.Render(pressedIndex, x, y, m_Width, m_Height);
		}
		else
		{
			auto pressedWidthDelta = m_Pressed ? int(m_Width * (1.0f - pressedSqueeze)) : 0;
			auto pressedHeightDelta = m_Pressed ? int(m_Height * (1.0f - pressedSqueeze)) : 0;

			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, m_TextureID);

			glBegin(GL_QUADS);
				glTexCoord2f(0.0f, 0.0f); glVertex2i(x + pressedWidthDelta, y + pressedHeightDelta);
				glTexCoord2f(1.0f, 0.0f); glVertex2i(x + m_Width - pressedWidthDelta, y + pressedHeightDelta);
				glTexCoord2f(1.0f, 1.0f); glVertex2i(x + m_Width - pressedWidthDelta, y + m_Height - pressedHeightDelta);
				glTexCoord2f(0.0f, 1.0f); glVertex2i(x + pressedWidthDelta, y + m_Height - pressedHeightDelta);
			glEnd();
		}

		//  Render the font the same way regardless of templating
		if (m_Font != nullptr && !m_Text.empty())
		{
			m_Font->RenderText(m_Text.c_str(), x + m_Width / 2, y + m_Height / 2, true, true, m_Pressed ? pressedSqueeze : 1.0f, m_Pressed ? pressedSqueeze : 1.0f);
		}
	}
}