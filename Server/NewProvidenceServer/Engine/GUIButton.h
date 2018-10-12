#pragma once

#include "GUIObjectNode.h"
#include "InputManager.h"
#include "FontManager.h"

#include <functional>

class GUIButton : public GUIObjectNode
{
public:
	typedef std::function<void(GUIObjectNode*)> GUIButtonCallback;

	static GUIButton* CreateButton(const char* imageFile, int x = 0, int y = 0, int w = 0, int h = 0);
	static GUIButton* CreateTemplatedButton(const char* buttonTemplate, int x = 0, int y = 0, int w = 0, int h = 0);

	explicit GUIButton(bool templated);
	~GUIButton();

	void SetLeftClickCallback(const GUIButtonCallback& callback) { m_LeftClickCallback = callback; }
	void SetMiddleClickCallback(const GUIButtonCallback& callback) { m_MiddleClickCallback = callback; }
	void SetRightClickCallback(const GUIButtonCallback& callback) { m_RightClickCallback = callback; }
	void SetFont(const Font* font) { m_Font = font; }
	void SetFont(std::string fontName) { m_Font = fontManager.GetFont(fontName.c_str()); }
	void SetText(const std::string text) { m_Text = text; }
	void SetPressedSizeRatio(float ratio) { m_PressedSizeRatio = ratio; }

	void Input(int xOffset = 0, int yOffset = 0) override;
	void Render(int xOffset = 0, int yOffset = 0) override;

private:
	GUIButtonCallback	m_LeftClickCallback;
	GUIButtonCallback	m_MiddleClickCallback;
	GUIButtonCallback	m_RightClickCallback;
	bool m_Pressed;
	const Font* m_Font;
	std::string m_Text;
	float m_PressedSizeRatio;

	bool m_Templated;
	TextureManager::ManagedTexture* TextureTopLeftCorner[2];
	TextureManager::ManagedTexture* TextureTopRightCorner[2];
	TextureManager::ManagedTexture* TextureBottomLeftCorner[2];
	TextureManager::ManagedTexture* TextureBottomRightCorner[2];
	TextureManager::ManagedTexture* TextureLeftSide[2];
	TextureManager::ManagedTexture* TextureRightSide[2];
	TextureManager::ManagedTexture* TextureTopSide[2];
	TextureManager::ManagedTexture* TextureBottomSide[2];
	TextureManager::ManagedTexture* TextureMiddle[2];
};

inline GUIButton* GUIButton::CreateButton(const char* imageFile, int x, int y, int w, int h)
{
	MANAGE_MEMORY_NEW("MenuUI_Button", sizeof(GUIButton));
	auto newButton = new GUIButton(false);
	newButton->SetTextureID(textureManager.LoadTextureGetID(imageFile));
	newButton->SetX(x);
	newButton->SetY(y);
	newButton->SetWidth(w);
	newButton->SetHeight(h);
	return newButton;
}

inline GUIButton* GUIButton::CreateTemplatedButton(const char* buttonTemplate, int x, int y, int w, int h)
{
	MANAGE_MEMORY_NEW("MenuUI_Button", sizeof(GUIButton));
	auto newButton = new GUIButton(true);

	auto templateFolder("Assets/UITemplates/Button/" + std::string(buttonTemplate) + "/");
	newButton->TextureTopLeftCorner[0] = textureManager.LoadTexture(std::string(templateFolder + "U_TopLeftCorner.png").c_str());
	newButton->TextureTopRightCorner[0] = textureManager.LoadTexture(std::string(templateFolder + "U_TopRightCorner.png").c_str());
	newButton->TextureBottomLeftCorner[0] = textureManager.LoadTexture(std::string(templateFolder + "U_BottomLeftCorner.png").c_str());
	newButton->TextureBottomRightCorner[0] = textureManager.LoadTexture(std::string(templateFolder + "U_BottomRightCorner.png").c_str());
	newButton->TextureLeftSide[0] = textureManager.LoadTexture(std::string(templateFolder + "U_LeftSide.png").c_str());
	newButton->TextureRightSide[0] = textureManager.LoadTexture(std::string(templateFolder + "U_RightSide.png").c_str());
	newButton->TextureTopSide[0] = textureManager.LoadTexture(std::string(templateFolder + "U_TopSide.png").c_str());
	newButton->TextureBottomSide[0] = textureManager.LoadTexture(std::string(templateFolder + "U_BottomSide.png").c_str());
	newButton->TextureMiddle[0] = textureManager.LoadTexture(std::string(templateFolder + "U_Middle.png").c_str());
	newButton->TextureTopLeftCorner[1] = textureManager.LoadTexture(std::string(templateFolder + "C_TopLeftCorner.png").c_str());
	newButton->TextureTopRightCorner[1] = textureManager.LoadTexture(std::string(templateFolder + "C_TopRightCorner.png").c_str());
	newButton->TextureBottomLeftCorner[1] = textureManager.LoadTexture(std::string(templateFolder + "C_BottomLeftCorner.png").c_str());
	newButton->TextureBottomRightCorner[1] = textureManager.LoadTexture(std::string(templateFolder + "C_BottomRightCorner.png").c_str());
	newButton->TextureLeftSide[1] = textureManager.LoadTexture(std::string(templateFolder + "C_LeftSide.png").c_str());
	newButton->TextureRightSide[1] = textureManager.LoadTexture(std::string(templateFolder + "C_RightSide.png").c_str());
	newButton->TextureTopSide[1] = textureManager.LoadTexture(std::string(templateFolder + "C_TopSide.png").c_str());
	newButton->TextureBottomSide[1] = textureManager.LoadTexture(std::string(templateFolder + "C_BottomSide.png").c_str());
	newButton->TextureMiddle[1] = textureManager.LoadTexture(std::string(templateFolder + "C_Middle.png").c_str());
	newButton->SetTextureID(0);

	newButton->SetX(x);
	newButton->SetY(y);
	newButton->SetWidth(w);
	newButton->SetHeight(h);

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
	TextureTopLeftCorner[0] = TextureTopLeftCorner[1] = nullptr;
	TextureTopRightCorner[0] = TextureTopRightCorner[1] = nullptr;
	TextureBottomLeftCorner[0] = TextureBottomLeftCorner[1] = nullptr;
	TextureBottomRightCorner[0] = TextureBottomRightCorner[1] = nullptr;
	TextureLeftSide[0] = TextureLeftSide[1] = nullptr;
	TextureRightSide[0] = TextureRightSide[1] = nullptr;
	TextureTopSide[0] = TextureTopSide[1] = nullptr;
	TextureBottomSide[0] = TextureBottomSide[1] = nullptr;
	TextureMiddle[0] = TextureMiddle[1] = nullptr;
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

inline void GUIButton::Render(int xOffset, int yOffset)
{
	glColor4f(m_Color.colorValues[0], m_Color.colorValues[1], m_Color.colorValues[2], m_Color.colorValues[3]);

	//  Render the object if we're able
	if (!m_SetToDestroy && m_Visible)
	{
		if (((m_TextureID != 0) || m_Templated) && m_Width > 0 && m_Height > 0)
		{
			auto x = m_X + xOffset;
			auto y = m_Y + yOffset;
			auto pressedSqueeze = m_PressedSizeRatio;

			if (m_Templated)
			{
				auto pressedIndex = (m_Pressed ? 1 : 0);
				pressedSqueeze = 1.0f;

				TextureTopLeftCorner[pressedIndex]->RenderTexture(x, y, TextureTopLeftCorner[pressedIndex]->getWidth(), TextureTopLeftCorner[pressedIndex]->getHeight());
				TextureTopRightCorner[pressedIndex]->RenderTexture(x + m_Width - TextureTopRightCorner[pressedIndex]->getWidth(), y, TextureTopRightCorner[pressedIndex]->getWidth(), TextureTopRightCorner[pressedIndex]->getHeight());
				TextureBottomLeftCorner[pressedIndex]->RenderTexture(x, y + m_Height - TextureBottomLeftCorner[pressedIndex]->getHeight(), TextureBottomLeftCorner[pressedIndex]->getWidth(), TextureBottomLeftCorner[pressedIndex]->getHeight());
				TextureBottomRightCorner[pressedIndex]->RenderTexture(x + m_Width - TextureBottomRightCorner[pressedIndex]->getWidth(), y + m_Height - TextureBottomRightCorner[pressedIndex]->getHeight(), TextureBottomRightCorner[pressedIndex]->getWidth(), TextureBottomLeftCorner[pressedIndex]->getHeight());
				TextureLeftSide[pressedIndex]->RenderTexture(x, y + TextureTopLeftCorner[pressedIndex]->getHeight(), TextureLeftSide[pressedIndex]->getWidth(), m_Height - TextureTopLeftCorner[pressedIndex]->getHeight() - TextureBottomLeftCorner[pressedIndex]->getHeight());
				TextureRightSide[pressedIndex]->RenderTexture(x + m_Width - TextureRightSide[pressedIndex]->getWidth(), y + TextureTopRightCorner[pressedIndex]->getHeight(), TextureRightSide[pressedIndex]->getWidth(), m_Height - TextureTopRightCorner[pressedIndex]->getHeight() - TextureBottomRightCorner[pressedIndex]->getHeight());
				TextureTopSide[pressedIndex]->RenderTexture(x + TextureTopLeftCorner[pressedIndex]->getWidth(), y, m_Width - TextureBottomLeftCorner[pressedIndex]->getWidth() - TextureBottomRightCorner[pressedIndex]->getWidth(), TextureTopSide[pressedIndex]->getHeight());
				TextureBottomSide[pressedIndex]->RenderTexture(x + TextureBottomLeftCorner[pressedIndex]->getWidth(), y + m_Height - TextureBottomSide[pressedIndex]->getHeight(), m_Width - TextureBottomLeftCorner[pressedIndex]->getWidth() - TextureBottomRightCorner[pressedIndex]->getWidth(), TextureBottomSide[pressedIndex]->getHeight());
				TextureMiddle[pressedIndex]->RenderTexture(x + TextureLeftSide[pressedIndex]->getWidth(), y + TextureTopSide[pressedIndex]->getHeight(), m_Width - TextureLeftSide[pressedIndex]->getWidth() - TextureRightSide[pressedIndex]->getWidth(), m_Height - TextureTopSide[pressedIndex]->getHeight() - TextureBottomSide[pressedIndex]->getHeight());
			}
			else
			{
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, m_TextureID);

				auto pressedWidthDelta = m_Pressed ? int(m_Width * (1.0f - pressedSqueeze)) : 0;
				auto pressedHeightDelta = m_Pressed ? int(m_Height * (1.0f - pressedSqueeze)) : 0;

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

		//  Pass the render call to all children
		for (auto iter = m_Children.begin(); iter != m_Children.end(); ++iter) (*iter)->Render(xOffset + m_X, yOffset + m_Y);
	}
}