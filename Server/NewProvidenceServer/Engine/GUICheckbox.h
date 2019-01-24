#pragma once

#include "GUIObjectNode.h"
#include "InputManager.h"

#include <functional>

class GUICheckbox : public GUIObjectNode
{
public:
	static GUICheckbox* CreateCheckbox(const char* imageFile, const char* checkFile, int x = 0, int y = 0, int w = 0, int h = 0);
	static GUICheckbox* CreateTemplatedCheckbox(const char* templateName, int x = 0, int y = 0, int w = 0, int h = 0);

	explicit GUICheckbox(bool templated);
	~GUICheckbox();

	inline bool GetChecked() const { return m_Checked; }

	inline void SetChecked(bool checked);
	inline void SetCheckTextureID(int textureID) { m_CheckTextureID = textureID; }
	inline void SetCheckCallback(const GUIFunctionCallback& callback) { m_CheckCallback = callback; }
	inline void SetTemplate(const char* templateName) { if (strlen(templateName) == 0) { m_Templated = false; return; } m_Templated = true;  m_TemplateBox = GUITemplatedBox("Checkbox", templateName, 2); }

	void Input(int xOffset = 0, int yOffset = 0) override;
	void Render(int xOffset = 0, int yOffset = 0) override;

private:
	void ToggleCheck();
	void Check();
	void Uncheck();

	GLuint m_CheckTextureID;
	GUIFunctionCallback	m_CheckCallback;
	bool m_Checked;

	//  Template data
	bool m_Templated;
	GUITemplatedBox m_TemplateBox;
};

inline GUICheckbox* GUICheckbox::CreateCheckbox(const char* imageFile, const char* checkFile, int x, int y, int w, int h)
{
	MANAGE_MEMORY_NEW("MenuUI_Checkbox", sizeof(GUICheckbox));
	auto newCheckbox = new GUICheckbox(false);

	newCheckbox->SetTextureID(textureManager.LoadTextureGetID(imageFile));
	newCheckbox->SetCheckTextureID(textureManager.LoadTextureGetID(checkFile));

	newCheckbox->SetPosition(x, y);
	newCheckbox->SetDimensions(w, h);
	return newCheckbox;
}

inline GUICheckbox* GUICheckbox::CreateTemplatedCheckbox(const char* templateName, int x, int y, int w, int h)
{
	MANAGE_MEMORY_NEW("MenuUI_Checkbox", sizeof(GUICheckbox));
	auto newCheckbox = new GUICheckbox(true);

	newCheckbox->SetTemplate(templateName);
	newCheckbox->SetTextureID(0);

	newCheckbox->SetPosition(x, y);
	newCheckbox->SetDimensions(w, h);
	return newCheckbox;
}

inline GUICheckbox::GUICheckbox(bool templated) :
	m_CheckTextureID(0),
	m_CheckCallback(nullptr),
	m_Checked(false),
	m_Templated(templated)
{
}


inline GUICheckbox::~GUICheckbox()
{
	MANAGE_MEMORY_DELETE("MenuUI_Checkbox", sizeof(GUICheckbox));
}


inline void GUICheckbox::SetChecked(bool checked)
{
	if (checked == m_Checked) return;
	else (checked ? Check() : Uncheck());
}


inline void GUICheckbox::Input(int xOffset, int yOffset)
{
	if (m_SetToDestroy || !m_Visible) return;

	auto leftButtonState = inputManager.GetMouseButtonLeft();
	auto x = inputManager.GetMouseX();
	auto y = inputManager.GetMouseY();

	if ((x > xOffset + m_X) && (x < xOffset + m_X + m_Width) && (y > yOffset + m_Y) && (y < yOffset + m_Y + m_Height))
	{
		if (leftButtonState == MOUSE_BUTTON_PRESSED)
		{
			inputManager.TakeMouseButtonLeft();
			ToggleCheck();
		}
	}

	//  Take base node input
	GUIObjectNode::Input(xOffset, yOffset);
}

inline void GUICheckbox::Render(int xOffset, int yOffset)
{
	glColor4f(m_Color.colorValues[0], m_Color.colorValues[1], m_Color.colorValues[2], m_Color.colorValues[3]);

	//  Render the object if we're able
	if (!m_SetToDestroy && m_Visible && ((m_TextureID != 0) || m_Templated) && m_Width > 0 && m_Height > 0)
	{
		auto x = m_X + xOffset;
		auto y = m_Y + yOffset;


		if (m_Templated)
		{
			auto checkedIndex = (m_Checked ? 1 : 0);

			m_TemplateBox.Render(checkedIndex, x, y, m_Width, m_Height);
		}
		else
		{
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, m_TextureID);

			glBegin(GL_QUADS);
				glTexCoord2f(0.0f, 0.0f); glVertex3i(x, y, 0);
				glTexCoord2f(1.0f, 0.0f); glVertex3i(x + m_Width, y, 0);
				glTexCoord2f(1.0f, 1.0f); glVertex3i(x + m_Width, y + m_Height, 0);
				glTexCoord2f(0.0f, 1.0f); glVertex3i(x, y + m_Height, 0);
			glEnd();

			if (m_Checked)
			{
				glBindTexture(GL_TEXTURE_2D, m_CheckTextureID);

				glBegin(GL_QUADS);
					glTexCoord2f(0.0f, 0.0f); glVertex3i(xOffset + m_X, yOffset + m_Y, 0);
					glTexCoord2f(1.0f, 0.0f); glVertex3i(xOffset + m_X + m_Width, yOffset + m_Y, 0);
					glTexCoord2f(1.0f, 1.0f); glVertex3i(xOffset + m_X + m_Width, yOffset + m_Y + m_Height, 0);
					glTexCoord2f(0.0f, 1.0f); glVertex3i(xOffset + m_X, yOffset + m_Y + m_Height, 0);
				glEnd();
			}
		}
	}

	//  Pass the render call to all children
	for (auto iter = m_Children.begin(); iter != m_Children.end(); ++iter) (*iter)->Render(xOffset + m_X, yOffset + m_Y);
}

inline void GUICheckbox::ToggleCheck()
{
	m_Checked ? Uncheck() : Check();
}

inline void GUICheckbox::Check()
{
	m_Checked = true;
	if (m_CheckCallback != nullptr) m_CheckCallback(this);
}

inline void GUICheckbox::Uncheck()
{
	m_Checked = false;
	if (m_CheckCallback != nullptr) m_CheckCallback(this);
}