#pragma once

#include "TextureManager.h"
#include "XMLWrapper.h"

#include <unordered_map>

class FontManager;

struct Font
{
	friend class FontManager;

public:
	Font();
	~Font();

	void Shutdown();
	void RenderText(const char* text, int x, int y, bool x_centered = false, bool y_centered = false, float x_scale = 1.0f, float y_scale = 1.0f, Color& fontColor = COLOR_WHITE) const;
	unsigned int GetTextWidth(const char* text) const;
	unsigned int GetCharacterCountBeforePassingWidth(const char* text, unsigned int width, bool cut_at_spaces = true) const;

	inline unsigned int GetFontHeight(void)	const { return m_MaxHeight; }

protected:
	TextureManager::ManagedTexture* m_Texture;
	unsigned int m_X[95];
	unsigned int m_Y[95];
	unsigned int m_Width[95];
	unsigned int m_Height[95];
	unsigned int m_MaxHeight;
};

class FontManager
{
public:
	static FontManager& GetInstance() { static FontManager INSTANCE; return INSTANCE; }

	void SetFontFolder(const char* folder_name);
	bool LoadFont(const char* font_name);
	Font* GetFont(const char* font_name);

	void Shutdown(void);

private:
	FontManager();
	~FontManager();

	typedef std::unordered_map<std::string, Font> FontListType;
	FontListType m_FontList;
	std::string m_FontFolder;
};

inline Font::Font() :
	m_Texture(nullptr),
	m_MaxHeight(0)
{
	memset(m_X, 0, sizeof(unsigned int) * 95);
	memset(m_Y, 0, sizeof(unsigned int) * 95);
	memset(m_Width, 0, sizeof(unsigned int) * 95);
	memset(m_Height, 0, sizeof(unsigned int) * 95);
}


inline Font::~Font()
{
}


inline void Font::Shutdown(void)
{
}


inline void Font::RenderText(const char* text, int x, int y, bool x_centered, bool y_centered, float x_scale, float y_scale, Color& color) const
{
	if (m_Texture == nullptr) return;

	//  Determine the X offset
	UINT x_offset = x;
	if (x_centered) x_offset -= UINT((GetTextWidth(text) / 2.0f) * x_scale);

	//  Determine the Y offset
	UINT y_offset = y;
	if (y_centered) y_offset -= UINT(((m_MaxHeight / 2.0f)) * y_scale);

	//  Render each character in a line while updating the X offset
	glColor4f(color.R, color.G, color.B, color.A);
	for (size_t i = 0; i < strlen(text); ++i)
	{
		m_Texture->RenderTexturePart(x_offset, y_offset, m_X[text[i] - 32], m_Y[text[i] - 32], int(m_Width[text[i] - 32] * x_scale), int(m_Height[text[i] - 32] * y_scale));
		x_offset += int(m_Width[text[i] - 32] * x_scale);
	}
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}


inline unsigned int Font::GetTextWidth(const char* text) const
{
	unsigned int width = 0;
	for (unsigned int i = 0; i < strlen(text); ++i) width += m_Width[text[i] - 32];
	return width;
}


inline unsigned int Font::GetCharacterCountBeforePassingWidth(const char* text, unsigned int width, bool cut_at_spaces) const
{
	UINT character_count;
	std::string new_string("");
	for (character_count = 0; character_count < strlen(text); ++character_count)
	{
		new_string += text[character_count];
		if (GetTextWidth(new_string.c_str()) > width) { break; }
	}

	if (character_count == strlen(text)) { return character_count; }

	if (cut_at_spaces)
	{
		auto new_return_value = new_string.find_last_of(" ");
		if ((new_return_value != std::string::npos) && (new_return_value + 1 < new_string.size()))
		{
			character_count = UINT(new_return_value + 1);
		}
	}
	return character_count;
}


inline void FontManager::SetFontFolder(const char* folder_name)
{
	m_FontFolder = std::string(folder_name);
}


inline bool FontManager::LoadFont(const char* font_name)
{
	std::string font_name_string(font_name);

	auto existingFont = m_FontList.find(font_name_string);
	if (existingFont != m_FontList.end()) return false;

	std::string full_file_path(m_FontFolder);
	full_file_path.append(font_name);
	full_file_path.append("/");
	full_file_path.append(font_name);
	full_file_path.append(".xml");

	auto fontXML = xmlWrapper.LoadXMLFile(full_file_path.c_str());
	if (fontXML == nullptr) return false;

	const RapidXML_Node* baseNode = fontXML->first_node();
	if (baseNode == nullptr || strcmp(baseNode->name(), "BitmapFont") != 0) return false;

	auto font_texture(m_FontFolder);
	font_texture.append(font_name);
	font_texture.append("/");
	font_texture.append(font_name);
	font_texture.append(".png");

	auto fontTexture = textureManager.LoadTexture(font_texture.c_str());
	if (fontTexture == nullptr) { return false; }

	Font new_font;
	new_font.m_Texture = fontTexture;

	for (auto iter = baseNode->first_node(); iter != nullptr; iter = iter->next_sibling())
	{
		auto dataNode = iter->first_node("ID");
		unsigned int index = atoi(dataNode->value()) - 32;
		dataNode = dataNode->next_sibling("X");
		new_font.m_X[index] = atoi(dataNode->value());
		dataNode = dataNode->next_sibling("Y");
		new_font.m_Y[index] = atoi(dataNode->value());
		dataNode = dataNode->next_sibling("W");
		new_font.m_Width[index] = atoi(dataNode->value());
		dataNode = dataNode->next_sibling("H");
		new_font.m_Height[index] = atoi(dataNode->value());
		if (new_font.m_MaxHeight < new_font.m_Height[index]) new_font.m_MaxHeight = new_font.m_Height[index];
	}

	xmlWrapper.UnloadXMLDoc(fontXML);

	m_FontList[font_name_string] = new_font;

	return true;
}


inline Font* FontManager::GetFont(const char* font_name)
{
	std::string font_name_string(font_name);
	auto iter = m_FontList.find(font_name_string);
	if (iter == m_FontList.end()) { return nullptr; }

	return &((*iter).second);
}


inline void FontManager::Shutdown(void)
{
	for (auto iter = m_FontList.begin(); iter != m_FontList.end(); ++iter)
	{
		(*iter).second.Shutdown();
	}
	m_FontList.clear();
}


inline FontManager::FontManager()
{
}


inline FontManager::~FontManager()
{
	if (!m_FontList.empty())
	{
		Shutdown();
	}
}

//  Instance to be utilized by anyone including this header
FontManager& fontManager = FontManager::GetInstance();