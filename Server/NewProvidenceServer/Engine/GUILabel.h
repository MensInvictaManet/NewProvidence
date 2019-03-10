#pragma once

#include "GUIObjectNode.h"
#include "FontManager.h"

class GUILabel : public GUIObjectNode
{
public:
	static GUILabel* CreateLabel(const char* font, const char* text, int x = 0, int y = 0, int w = 0, int h = 0, int justification = UI_JUSTIFY_LEFT);
	static GUILabel* CreateLabel(const Font* font, const char* text, int x = 0, int y = 0, int w = 0, int h = 0, int justification = UI_JUSTIFY_LEFT);

	explicit GUILabel(const char* text = "");
	~GUILabel();

	inline std::string GetText() const { return m_Text; }

	void SetFont(const Font* font) { m_Font = font; }
	void SetFont(std::string fontName) { m_Font = fontManager.GetFont(fontName.c_str()); }
	void SetText(const std::string text) { m_Text = text; }
	void SetJustification(int justify) { m_Justification = justify; }

	void TrueRender(int x = 0, int y = 0) override;

private:
	const Font* m_Font;
	std::string m_Text;
	int m_Justification;
};


inline GUILabel* GUILabel::CreateLabel(const char* font, const char* text, int x, int y, int w, int h, int justification)
{
	return GUILabel::CreateLabel(fontManager.GetFont(font), text, x, y, w, h, justification);
}


inline GUILabel* GUILabel::CreateLabel(const Font* font, const char* text, int x, int y, int w, int h, int justification)
{
	MANAGE_MEMORY_NEW("MenuUI_Label", sizeof(GUILabel));
	auto newLabel = new GUILabel(text);
	newLabel->SetFont(font);
	newLabel->SetPosition(x, y);
	newLabel->SetDimensions(w, h);
	newLabel->SetJustification(justification);
	return newLabel;
}

inline GUILabel::GUILabel(const char* text) :
	m_Font(nullptr),
	m_Text(text),
	m_Justification(UI_JUSTIFY_LEFT)
{

}


inline GUILabel::~GUILabel()
{
	MANAGE_MEMORY_DELETE("MenuUI_Label", sizeof(GUILabel));
}

inline void GUILabel::TrueRender(int x, int y)
{
	//  Render the object if we're able
	if (!m_SetToDestroy && m_Visible && (m_Font != nullptr && !m_Text.empty()) && m_Width > 0 && m_Height > 0)
	{
		//  Render the font the same way regardless of templating
		if (m_Justification == UI_JUSTIFY_CENTER)		m_Font->RenderText(m_Text.c_str(), x - m_Font->GetTextWidth(m_Text.c_str()) / 2, y);
		else if (m_Justification == UI_JUSTIFY_LEFT)	m_Font->RenderText(m_Text.c_str(), x, y);
		else if (m_Justification == UI_JUSTIFY_RIGHT)	m_Font->RenderText(m_Text.c_str(), x + m_Width - m_Font->GetTextWidth(m_Text.c_str()), y);
	}
}