#pragma once

#include "GUIObjectNode.h"
#include "FontManager.h"
#include "StringTools.h"

class GUIProgressBar : public GUIObjectNode
{
public:
	enum TextAlignment { ALIGN_LEFT, ALIGN_CENTER };

	static GUIProgressBar* CreateTemplatedProgressBar(const char* templateName, int x = 0, int y = 0, int w = 0, int h = 0);

	explicit GUIProgressBar(bool templated);
	~GUIProgressBar();

	inline void SetFont(const Font* font) { m_Font = font; }
	inline void SetFont(std::string fontName) { SetFont(fontManager.GetFont(fontName.c_str())); }
	inline void SetTemplate(const char* templateName) { if (strlen(templateName) == 0) { m_Templated = false; return; } m_Templated = true;  m_TemplateBox = GUITemplatedBox("ProgressBar", templateName, 2); }
	inline void SetBarColor(Color color) { m_BarColor = color; }
	inline void SetTextColor(Color color) { m_TextColor = color; }
	inline void SetProgress(float progress) { m_Progress = progress; }

	void Render(int xOffset = 0, int yOffset = 0) override;

private:
	const Font* m_Font;
	bool m_Templated;
	GUITemplatedBox m_TemplateBox;
	Color m_BarColor;
	Color m_TextColor;
	float m_Progress;
};

inline GUIProgressBar* GUIProgressBar::CreateTemplatedProgressBar(const char* templateName, int x, int y, int w, int h)
{
	MANAGE_MEMORY_NEW("MenuUI_ProgressBar", sizeof(GUIProgressBar));
	auto newNode = new GUIProgressBar(true);

	newNode->SetPosition(x, y);
	newNode->SetDimensions(w, h);

	auto templateFolder("Assets/UITemplates/ProgressBar/" + std::string(templateName) + "/");
	newNode->SetTemplate(templateName);
	newNode->SetTextureID(0);

	return newNode;
}

inline GUIProgressBar::GUIProgressBar(bool templated) :
	m_Font(nullptr),
	m_Templated(templated),
	m_BarColor(COLOR_RED),
	m_TextColor(COLOR_BLACK),
	m_Progress(0.0f)
{
	SetColor(COLOR_GRAY);
}


inline GUIProgressBar::~GUIProgressBar()
{
	MANAGE_MEMORY_DELETE("MenuUI_ProgressBar", sizeof(GUIProgressBar));
}


inline void GUIProgressBar::Render(int xOffset, int yOffset)
{
	glColor4f(m_Color.colorValues[0], m_Color.colorValues[1], m_Color.colorValues[2], m_Color.colorValues[3]);

	//  Render the object if we're able
	if (!m_SetToDestroy && m_Visible && m_Width > 0 && m_Height > 0)
	{
		auto x = m_X + xOffset;
		auto y = m_Y + yOffset;

		if (m_Templated)
		{
			//  Render the main border and background
			m_TemplateBox.Render(0, x, y, m_Width, m_Height);

			//  Render the progress bar overlay
			glColor4f(m_BarColor.colorValues[0], m_BarColor.colorValues[1], m_BarColor.colorValues[2], m_BarColor.colorValues[3]);
			m_TemplateBox.Render(1, x, y, int(m_Width * m_Progress), m_Height, true, m_Width);

			if (m_Font != nullptr)
			{
				auto percentString = getDoubleStringRounded(m_Progress / 100.0, 2) + "%";
				m_Font->RenderText(percentString.c_str(), x + m_Width / 2, y + m_Height / 2, true, true, 1.0f, 1.0f, m_TextColor);
			}
		}
	}

	//  Pass the render call to all children
	for (auto iter = m_Children.begin(); iter != m_Children.end(); ++iter) (*iter)->Render(xOffset + m_X, yOffset + m_Y);
}