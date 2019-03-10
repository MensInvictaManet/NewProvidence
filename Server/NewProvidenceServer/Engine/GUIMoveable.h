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

	virtual void Input(int xOffset = 0, int yOffset = 0) override;
	virtual void TrueRender(int x = 0, int y = 0) override;

	inline void SetMoveable(bool moveable) { m_Moveable = moveable; if (!moveable) m_Grabbed = false; }
	inline void SetTemplate(const char* templateName) { m_Templated = true;  m_TemplateBox = GUITemplatedBox("Moveable", templateName, 1); }

private:
	bool m_Templated;
	GUITemplatedBox m_TemplateBox;

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
	newMoveable->SetPosition(x, y);
	newMoveable->SetDimensions(w, h);
	return newMoveable;
}

inline GUIMoveable* GUIMoveable::CreateTemplatedMoveable(const char* templateName, int x, int y, int w, int h, int grab_x, int grab_y, int grab_w, int grab_h)
{
	MANAGE_MEMORY_NEW("MenuUI_Moveable", sizeof(GUIMoveable));
	auto newMoveable = new GUIMoveable(true, grab_x, grab_y, grab_w, grab_h);

	newMoveable->SetTemplate(templateName);
	newMoveable->SetTextureID(0);

	newMoveable->SetPosition(x, y);
	newMoveable->SetDimensions(w, h);
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

inline void GUIMoveable::TrueRender(int x, int y)
{
	//  Render the object if we're able
	if (m_Templated && m_Width > 0 && m_Height > 0)  m_TemplateBox.Render(0, x, y, m_Width, m_Height);
}