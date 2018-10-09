#pragma once

#include "GUIObjectNode.h"
#include "InputManager.h"

class GUIMoveable : public GUIObjectNode
{
public:
	static GUIMoveable* CreateMoveable(const char* imageFile, int x = 0, int y = 0, int w = 0, int h = 0, int grab_x = 0, int grab_y = 0, int grab_w = 0, int grab_h = 0);
	static GUIMoveable* CreateTemplatedMoveable(const char* moveableTemplate, int x = 0, int y = 0, int w = 0, int h = 0, int grab_x = 0, int grab_y = 0, int grab_w = 0, int grab_h = 0);

	GUIMoveable(bool templated, int grab_x, int grab_y, int grab_w, int grab_h);
	~GUIMoveable();

	void Input(int xOffset = 0, int yOffset = 0) override;
	void Render(int xOffset = 0, int yOffset = 0) override;

	inline void SetMoveable(bool moveable) { m_Moveable = moveable; if (!moveable) m_Grabbed = false; }

private:
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

	bool m_Moveable;
	bool m_Grabbed;
	int m_GrabX;
	int m_GrabY;
	int m_GrabW;
	int m_GrabH;
	int m_GrabLastX;
	int m_GrabLastY;
};

inline GUIMoveable* GUIMoveable::CreateMoveable(const char* imageFile, int x, int y, int w, int h, int grab_x, int grab_y, int grab_w, int grab_h)
{
	MANAGE_MEMORY_NEW("MenuUI_Moveable", sizeof(GUIMoveable));
	auto newMoveable = new GUIMoveable(false, grab_x, grab_y, grab_w, grab_h);
	newMoveable->SetTextureID(textureManager.LoadTextureGetID(imageFile));
	newMoveable->SetX(x);
	newMoveable->SetY(y);
	newMoveable->SetWidth(w);
	newMoveable->SetHeight(h);
	return newMoveable;
}

inline GUIMoveable* GUIMoveable::CreateTemplatedMoveable(const char* moveableTemplate, int x, int y, int w, int h, int grab_x, int grab_y, int grab_w, int grab_h)
{
	MANAGE_MEMORY_NEW("MenuUI_Moveable", sizeof(GUIMoveable));
	auto newMoveable = new GUIMoveable(true, grab_x, grab_y, grab_w, grab_h);

	auto templateFolder("Assets/UITemplates/Moveable/" + std::string(moveableTemplate) + "/");
	newMoveable->TextureTopLeftCorner = textureManager.LoadTexture(std::string(templateFolder + "TopLeftCorner.png").c_str());
	newMoveable->TextureTopRightCorner = textureManager.LoadTexture(std::string(templateFolder + "TopRightCorner.png").c_str());
	newMoveable->TextureBottomLeftCorner = textureManager.LoadTexture(std::string(templateFolder + "BottomLeftCorner.png").c_str());
	newMoveable->TextureBottomRightCorner = textureManager.LoadTexture(std::string(templateFolder + "BottomRightCorner.png").c_str());
	newMoveable->TextureLeftSide = textureManager.LoadTexture(std::string(templateFolder + "LeftSide.png").c_str());
	newMoveable->TextureRightSide = textureManager.LoadTexture(std::string(templateFolder + "RightSide.png").c_str());
	newMoveable->TextureTopSide = textureManager.LoadTexture(std::string(templateFolder + "TopSide.png").c_str());
	newMoveable->TextureBottomSide = textureManager.LoadTexture(std::string(templateFolder + "BottomSide.png").c_str());
	newMoveable->TextureMiddle = textureManager.LoadTexture(std::string(templateFolder + "Middle.png").c_str());
	newMoveable->SetTextureID(0);

	newMoveable->SetX(x);
	newMoveable->SetY(y);
	newMoveable->SetWidth(w);
	newMoveable->SetHeight(h);
	return newMoveable;
}

inline GUIMoveable::GUIMoveable(bool templated, int grab_x, int grab_y, int grab_w, int grab_h) :
	m_Templated(templated),
	m_Moveable(true),
	m_Grabbed(false),
	m_GrabX(grab_x),
	m_GrabY(grab_y),
	m_GrabW(grab_w),
	m_GrabH(grab_h),
	m_GrabLastX(0),
	m_GrabLastY(0)
{
	TextureTopLeftCorner = nullptr;
	TextureTopRightCorner = nullptr;
	TextureBottomLeftCorner = nullptr;
	TextureBottomRightCorner = nullptr;
	TextureLeftSide = nullptr;
	TextureRightSide = nullptr;
	TextureTopSide = nullptr;
	TextureBottomSide = nullptr;
	TextureMiddle = nullptr;
}

inline GUIMoveable::~GUIMoveable()
{
	MANAGE_MEMORY_DELETE("MenuUI_Moveable", sizeof(GUIMoveable));
}

inline void GUIMoveable::Input(int xOffset, int yOffset)
{
	if (m_SetToDestroy || !m_Visible) return;

	auto leftButtonState = inputManager.GetMouseButtonLeft();
	auto x = inputManager.GetMouseX();
	auto y = inputManager.GetMouseY();

	if (m_Moveable)
	{
		if (leftButtonState == MOUSE_BUTTON_UNPRESSED) m_Grabbed = false;
		else if (m_Grabbed == false)
		{
			if ((x > xOffset + m_X + m_GrabX) && (x < xOffset + m_X + m_GrabX + m_GrabW) && (y > yOffset + m_Y + m_GrabY) && (y < yOffset + m_Y + m_GrabY + m_GrabH))
			{
				if (leftButtonState == MOUSE_BUTTON_PRESSED)
				{
					inputManager.TakeMouseButtonLeft();
					m_Grabbed = true;
					m_GrabLastX = x;
					m_GrabLastY = y;
				}
			}
		}
		else
		{
			m_X += (x - m_GrabLastX);
			m_Y += (y - m_GrabLastY);
			m_GrabLastX = x;
			m_GrabLastY = y;
		}
	}

	//  Take base node input
	GUIObjectNode::Input(xOffset, yOffset);
}

inline void GUIMoveable::Render(int xOffset, int yOffset)
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
			TextureTopRightCorner->RenderTexture(x + m_Width - TextureTopRightCorner->getWidth(), y, TextureTopRightCorner->getWidth(), TextureTopRightCorner->getHeight());
			TextureBottomLeftCorner->RenderTexture(x, y + m_Height - TextureBottomLeftCorner->getHeight(), TextureBottomLeftCorner->getWidth(), TextureBottomLeftCorner->getHeight());
			TextureBottomRightCorner->RenderTexture(x + m_Width - TextureBottomRightCorner->getWidth(), y + m_Height - TextureBottomRightCorner->getHeight(), TextureBottomRightCorner->getWidth(), TextureBottomLeftCorner->getHeight());
			TextureLeftSide->RenderTexture(x, y + TextureTopLeftCorner->getHeight(), TextureLeftSide->getWidth(), m_Height - TextureTopLeftCorner->getHeight() - TextureBottomLeftCorner->getHeight());
			TextureRightSide->RenderTexture(x + m_Width - TextureRightSide->getWidth(), y + TextureTopRightCorner->getHeight(), TextureRightSide->getWidth(), m_Height - TextureTopRightCorner->getHeight() - TextureBottomRightCorner->getHeight());
			TextureTopSide->RenderTexture(x + TextureTopLeftCorner->getWidth(), y, m_Width - TextureBottomLeftCorner->getWidth() - TextureBottomRightCorner->getWidth(), TextureTopSide->getHeight());
			TextureBottomSide->RenderTexture(x + TextureBottomLeftCorner->getWidth(), y + m_Height - TextureBottomSide->getHeight(), m_Width - TextureBottomLeftCorner->getWidth() - TextureBottomRightCorner->getWidth(), TextureBottomSide->getHeight());
			TextureMiddle->RenderTexture(x + TextureLeftSide->getWidth(), y + TextureTopSide->getHeight(), m_Width - TextureLeftSide->getWidth() - TextureRightSide->getWidth(), m_Height - TextureTopSide->getHeight() - TextureBottomSide->getHeight());
		}
	}

	//  Do the base node render
	GUIObjectNode::Render(xOffset, yOffset);
}