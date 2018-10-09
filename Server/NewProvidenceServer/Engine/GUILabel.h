#pragma once

#include "GUIObjectNode.h"
#include "FontManager.h"

class GUILabel : public GUIObjectNode
{
public:
	enum Justifications { JUSTIFY_LEFT = 0, JUSTIFY_RIGHT, JUSTIFY_CENTER, JUSTIFICATION_COUNT };

	static GUILabel* CreateLabel(const Font* font, const char* text, int x = 0, int y = 0, int w = 0, int h = 0);

	explicit GUILabel(const char* text = "");
	~GUILabel();

	inline std::string GetText() const { return m_Text; }

	void SetFont(const Font* font) { m_Font = font; }
	void SetText(const std::string text) { m_Text = text; }
	void SetJustification(int justify) { m_Justification = justify; }

	void Render(int xOffset = 0, int yOffset = 0) override;

private:
	const Font* m_Font;
	std::string m_Text;
	int m_Justification;
};

inline GUILabel* GUILabel::CreateLabel(const Font* font, const char* text, int x, int y, int w, int h)
{
	MANAGE_MEMORY_NEW("MenuUI_Label", sizeof(GUILabel));
	auto newLabel = new GUILabel(text);
	newLabel->SetFont(font);
	newLabel->SetX(x);
	newLabel->SetY(y);
	newLabel->SetWidth(w);
	newLabel->SetHeight(h);
	return newLabel;
}

inline GUILabel::GUILabel(const char* text) :
	m_Font(nullptr),
	m_Text(text),
	m_Justification(JUSTIFY_LEFT)
{

}


inline GUILabel::~GUILabel()
{
	MANAGE_MEMORY_DELETE("MenuUI_Label", sizeof(GUILabel));
}

inline void GUILabel::Render(int xOffset, int yOffset)
{
	glColor4f(m_Color.colorValues[0], m_Color.colorValues[1], m_Color.colorValues[2], m_Color.colorValues[3]);

	auto x = m_X + xOffset;
	auto y = m_Y + yOffset;

	//  Render the object if we're able
	if (!m_SetToDestroy && m_Visible && (m_Font != nullptr && !m_Text.empty()) && m_Width > 0 && m_Height > 0)
	{
		//  Render the font the same way regardless of templating
		if (m_Justification == JUSTIFY_CENTER)		m_Font->RenderText(m_Text.c_str(), x - m_Font->GetTextWidth(m_Text.c_str()) / 2, y);
		else if (m_Justification == JUSTIFY_LEFT)	m_Font->RenderText(m_Text.c_str(), x, y);
		else if (m_Justification == JUSTIFY_RIGHT)	m_Font->RenderText(m_Text.c_str(), x + m_Width - m_Font->GetTextWidth(m_Text.c_str()), y);
	}

	//  Pass the render call to all children
	for (auto iter = m_Children.begin(); iter != m_Children.end(); ++iter) (*iter)->Render(x, y);
}