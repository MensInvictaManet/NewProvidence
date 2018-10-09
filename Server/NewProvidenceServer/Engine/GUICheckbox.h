#pragma once

#include "GUIObjectNode.h"
#include "InputManager.h"

#include <functional>

class GUICheckbox : public GUIObjectNode
{
public:
	typedef std::function<void(GUIObjectNode*)> GUICheckboxCallback;

	static GUICheckbox* CreateCheckbox(const char* imageFile, const char* checkFile, int x = 0, int y = 0, int w = 0, int h = 0);
	static GUICheckbox* CreateTemplatedCheckbox(const char* checkboxTemplate, int x = 0, int y = 0, int w = 0, int h = 0);

	explicit GUICheckbox(bool templated);
	~GUICheckbox();

	bool GetChecked() const { return m_Checked; }

	void SetChecked(bool checked);
	void SetCheckTextureID(int textureID) { m_CheckTextureID = textureID; }
	void SetCheckCallback(const GUICheckboxCallback& callback) { m_CheckCallback = callback; }

	void Input(int xOffset = 0, int yOffset = 0) override;
	void Render(int xOffset = 0, int yOffset = 0) override;

private:
	void ToggleCheck();
	void Check();
	void Uncheck();

	GLuint m_CheckTextureID;
	GUICheckboxCallback	m_CheckCallback;
	bool m_Checked;

	//  Template data
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

inline GUICheckbox* GUICheckbox::CreateCheckbox(const char* imageFile, const char* checkFile, int x, int y, int w, int h)
{
	MANAGE_MEMORY_NEW("MenuUI_Checkbox", sizeof(GUICheckbox));
	auto newCheckbox = new GUICheckbox(false);

	newCheckbox->SetTextureID(textureManager.LoadTextureGetID(imageFile));
	newCheckbox->SetCheckTextureID(textureManager.LoadTextureGetID(checkFile));

	newCheckbox->SetX(x);
	newCheckbox->SetY(y);
	newCheckbox->SetWidth(w);
	newCheckbox->SetHeight(h);
	return newCheckbox;
}

inline GUICheckbox* GUICheckbox::CreateTemplatedCheckbox(const char* checkboxTemplate, int x, int y, int w, int h)
{
	MANAGE_MEMORY_NEW("MenuUI_Checkbox", sizeof(GUICheckbox));
	auto newCheckbox = new GUICheckbox(true);

	auto templateFolder("Assets/UITemplates/Checkbox/" + std::string(checkboxTemplate) + "/");
	newCheckbox->TextureTopLeftCorner[0] = textureManager.LoadTexture(std::string(templateFolder + "U_TopLeftCorner.png").c_str());
	newCheckbox->TextureTopRightCorner[0] = textureManager.LoadTexture(std::string(templateFolder + "U_TopRightCorner.png").c_str());
	newCheckbox->TextureBottomLeftCorner[0] = textureManager.LoadTexture(std::string(templateFolder + "U_BottomLeftCorner.png").c_str());
	newCheckbox->TextureBottomRightCorner[0] = textureManager.LoadTexture(std::string(templateFolder + "U_BottomRightCorner.png").c_str());
	newCheckbox->TextureLeftSide[0] = textureManager.LoadTexture(std::string(templateFolder + "U_LeftSide.png").c_str());
	newCheckbox->TextureRightSide[0] = textureManager.LoadTexture(std::string(templateFolder + "U_RightSide.png").c_str());
	newCheckbox->TextureTopSide[0] = textureManager.LoadTexture(std::string(templateFolder + "U_TopSide.png").c_str());
	newCheckbox->TextureBottomSide[0] = textureManager.LoadTexture(std::string(templateFolder + "U_BottomSide.png").c_str());
	newCheckbox->TextureMiddle[0] = textureManager.LoadTexture(std::string(templateFolder + "U_Middle.png").c_str());
	newCheckbox->TextureTopLeftCorner[1] = textureManager.LoadTexture(std::string(templateFolder + "C_TopLeftCorner.png").c_str());
	newCheckbox->TextureTopRightCorner[1] = textureManager.LoadTexture(std::string(templateFolder + "C_TopRightCorner.png").c_str());
	newCheckbox->TextureBottomLeftCorner[1] = textureManager.LoadTexture(std::string(templateFolder + "C_BottomLeftCorner.png").c_str());
	newCheckbox->TextureBottomRightCorner[1] = textureManager.LoadTexture(std::string(templateFolder + "C_BottomRightCorner.png").c_str());
	newCheckbox->TextureLeftSide[1] = textureManager.LoadTexture(std::string(templateFolder + "C_LeftSide.png").c_str());
	newCheckbox->TextureRightSide[1] = textureManager.LoadTexture(std::string(templateFolder + "C_RightSide.png").c_str());
	newCheckbox->TextureTopSide[1] = textureManager.LoadTexture(std::string(templateFolder + "C_TopSide.png").c_str());
	newCheckbox->TextureBottomSide[1] = textureManager.LoadTexture(std::string(templateFolder + "C_BottomSide.png").c_str());
	newCheckbox->TextureMiddle[1] = textureManager.LoadTexture(std::string(templateFolder + "C_Middle.png").c_str());
	newCheckbox->SetTextureID(0);

	newCheckbox->SetX(x);
	newCheckbox->SetY(y);
	newCheckbox->SetWidth(w);
	newCheckbox->SetHeight(h);
	return newCheckbox;
}

inline GUICheckbox::GUICheckbox(bool templated) :
	m_CheckTextureID(0),
	m_CheckCallback(nullptr),
	m_Checked(false),
	m_Templated(templated)
{
	TextureTopLeftCorner[0]		= TextureTopLeftCorner[1]		= nullptr;
	TextureTopRightCorner[0]	= TextureTopRightCorner[1]		= nullptr;
	TextureBottomLeftCorner[0]	= TextureBottomLeftCorner[1]	= nullptr;
	TextureBottomRightCorner[0]	= TextureBottomRightCorner[1]	= nullptr;
	TextureLeftSide[0]			= TextureLeftSide[1]			= nullptr;
	TextureRightSide[0]			= TextureRightSide[1]			= nullptr;
	TextureTopSide[0]			= TextureTopSide[1]				= nullptr;
	TextureBottomSide[0]		= TextureBottomSide[1]			= nullptr;
	TextureMiddle[0]			= TextureMiddle[1]				= nullptr;
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

			TextureTopLeftCorner[checkedIndex]->RenderTexture(x, y, TextureTopLeftCorner[checkedIndex]->getWidth(), TextureTopLeftCorner[checkedIndex]->getHeight());
			TextureTopRightCorner[checkedIndex]->RenderTexture(x + m_Width - TextureTopRightCorner[checkedIndex]->getWidth(), y, TextureTopRightCorner[checkedIndex]->getWidth(), TextureTopRightCorner[checkedIndex]->getHeight());
			TextureBottomLeftCorner[checkedIndex]->RenderTexture(x, y + m_Height - TextureBottomLeftCorner[checkedIndex]->getHeight(), TextureBottomLeftCorner[checkedIndex]->getWidth(), TextureBottomLeftCorner[checkedIndex]->getHeight());
			TextureBottomRightCorner[checkedIndex]->RenderTexture(x + m_Width - TextureBottomRightCorner[checkedIndex]->getWidth(), y + m_Height - TextureBottomRightCorner[checkedIndex]->getHeight(), TextureBottomRightCorner[checkedIndex]->getWidth(), TextureBottomLeftCorner[checkedIndex]->getHeight());
			TextureLeftSide[checkedIndex]->RenderTexture(x, y + TextureTopLeftCorner[checkedIndex]->getHeight(), TextureLeftSide[checkedIndex]->getWidth(), m_Height - TextureTopLeftCorner[checkedIndex]->getHeight() - TextureBottomLeftCorner[checkedIndex]->getHeight());
			TextureRightSide[checkedIndex]->RenderTexture(x + m_Width - TextureRightSide[checkedIndex]->getWidth(), y + TextureTopRightCorner[checkedIndex]->getHeight(), TextureRightSide[checkedIndex]->getWidth(), m_Height - TextureTopRightCorner[checkedIndex]->getHeight() - TextureBottomRightCorner[checkedIndex]->getHeight());
			TextureTopSide[checkedIndex]->RenderTexture(x + TextureTopLeftCorner[checkedIndex]->getWidth(), y, m_Width - TextureBottomLeftCorner[checkedIndex]->getWidth() - TextureBottomRightCorner[checkedIndex]->getWidth(), TextureTopSide[checkedIndex]->getHeight());
			TextureBottomSide[checkedIndex]->RenderTexture(x + TextureBottomLeftCorner[checkedIndex]->getWidth(), y + m_Height - TextureBottomSide[checkedIndex]->getHeight(), m_Width - TextureBottomLeftCorner[checkedIndex]->getWidth() - TextureBottomRightCorner[checkedIndex]->getWidth(), TextureBottomSide[checkedIndex]->getHeight());
			TextureMiddle[checkedIndex]->RenderTexture(x + TextureLeftSide[checkedIndex]->getWidth(), y + TextureTopSide[checkedIndex]->getHeight(), m_Width - TextureLeftSide[checkedIndex]->getWidth() - TextureRightSide[checkedIndex]->getWidth(), m_Height - TextureTopSide[checkedIndex]->getHeight() - TextureBottomSide[checkedIndex]->getHeight());
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