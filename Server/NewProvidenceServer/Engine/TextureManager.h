#pragma once

#include <SDL.h>
#include <SDL_opengl.h>
#if USING_SDL_IMAGE
#include <SDL_image.h>
#endif
#include <unordered_map>

#include "WindowManager.h"

void Draw2DTexturedSquare(GLuint textureID, int x, int y, int w, int h)
{
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f); glVertex3i(x, y, 0);
		glTexCoord2f(1.0f, 0.0f); glVertex3i(x + w, y, 0);
		glTexCoord2f(1.0f, 1.0f); glVertex3i(x + w, y + h, 0);
		glTexCoord2f(0.0f, 1.0f); glVertex3i(x, y + h, 0);
	glEnd();
}

class TextureManager
{
public:
	struct ManagedTexture
	{
	public:
		ManagedTexture(SDL_Texture* texture, GLuint textureID, int width, int height, int listIndex) :
			m_Texture(texture),
			m_TextureID(textureID),
			m_Width(width),
			m_Height(height),
			m_ListIndex(listIndex)
		{}

		~ManagedTexture()
		{
			FreeTexture();
		}

		void RenderTexture(int x, int y, int width = -1, int height = -1) const
		{
			if (m_TextureID == 0) return;
			if (width < 0) width = m_Width;
			if (height < 0) height = m_Height;

			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, m_TextureID);

			glBegin(GL_QUADS);
				glTexCoord2f(0.0f, 0.0f); glVertex2i(x, y);
				glTexCoord2f(1.0f, 0.0f); glVertex2i(x + width, y);
				glTexCoord2f(1.0f, 1.0f); glVertex2i(x + width, y + height);
				glTexCoord2f(0.0f, 1.0f); glVertex2i(x, y + height);
			glEnd();
		}

		void RenderTexturePart(int x, int y, int sub_x, int sub_y, int sub_w, int sub_h) const
		{
			if (m_TextureID == 0) return;
			if (sub_w <= 0) sub_w = m_Width - sub_x;
			if (sub_h <= 0) sub_h = m_Height - sub_y;

			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, m_TextureID);

			glBegin(GL_QUADS);
				glTexCoord2f(float(sub_x) / (float(m_Width)), (float(sub_y) / float(m_Height)));
				glVertex3i(x, y, 0);
				glTexCoord2f((float(sub_x + sub_w) / float(m_Width)), (float(sub_y) / float(m_Height)));
				glVertex3i(x + sub_w, y, 0);
				glTexCoord2f((float(sub_x + sub_w) / float(m_Width)), (float(sub_y + sub_h) / float(m_Height)));
				glVertex3i(x + sub_w, y + sub_h, 0);
				glTexCoord2f((float(sub_x) / float(m_Width)), (float(sub_y + sub_h) / float(m_Height)));
				glVertex3i(x, y + sub_h, 0);
			glEnd();
		}

		void FreeTexture()
		{
			//  Free the texture if it exists
			if (m_Texture != nullptr)
			{
				SDL_DestroyTexture(m_Texture);
				m_Texture = nullptr;
				m_TextureID = 0;
				m_Width = 0;
				m_Height = 0;
			}
		}

		int getWidth() const { return m_Width; }
		int getHeight() const { return m_Height; }

		SDL_Texture* m_Texture;
		GLuint m_TextureID;
		int m_Width;
		int m_Height;
		int m_ListIndex;
	};

	static TextureManager& GetInstance() { static TextureManager INSTANCE; return INSTANCE; }

	GLuint LoadTextureGetID(const char* textureFile);
	ManagedTexture* LoadTexture(const char* textureFile);
	GLuint GetTextureID(const int index);
	ManagedTexture* GetManagedTexture(const int index);
	void Shutdown();

private:
	TextureManager()	{}
	~TextureManager()	{}

	int FirstFreeIndex();

	std::unordered_map<int, ManagedTexture*> m_TextureList;
	std::unordered_map<std::string, ManagedTexture*> m_TextureListByFile;
};

inline GLuint TextureManager::LoadTextureGetID(const char* textureFile)
{
	auto* texture = LoadTexture(textureFile);
	return (texture != nullptr) ? texture->m_TextureID : 0;
}


inline TextureManager::ManagedTexture* TextureManager::LoadTexture(const char* textureFile)
{
#if USING_SDL_IMAGE
	auto iter = m_TextureListByFile.find(textureFile);
	if (iter != m_TextureListByFile.end()) return (*iter).second;

	SDL_Texture* sdlTexture;

	//  Load the surface from the given file
	auto sdlSurface = IMG_Load(textureFile);
	if (sdlSurface == nullptr)
	{
		printf("Unable to load image %s! SDL_image Error: %s\n", textureFile, IMG_GetError());
		return nullptr;
	}

	//  Color key the image (Aqua for transparency)
	SDL_SetColorKey(sdlSurface, SDL_TRUE, SDL_MapRGB(sdlSurface->format, 0, 0xFF, 0xFF));

	//  Create texture from surface pixels
	auto renderer = WindowManager::GetInstance().GetRenderer(-1);
	sdlTexture = SDL_CreateTextureFromSurface(renderer, sdlSurface);
	if (sdlTexture == nullptr)
	{
		SDL_FreeSurface(sdlSurface);
		printf("Unable to create texture from %s! SDL Error: %s\n", textureFile, SDL_GetError());
		return nullptr;
	}

	//  Get the dimensions of the texture surface as well as the bpp value
	auto width = sdlSurface->w;
	auto height = sdlSurface->h;
	auto mode = (sdlSurface->format->BytesPerPixel == 4) ? GL_RGBA : GL_RGB;

	//  Create the OpenGL texture and push the surface data to it
	GLuint textureID = 0;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, mode, sdlSurface->w, sdlSurface->h, 0, mode, GL_UNSIGNED_BYTE, sdlSurface->pixels);

	//  Set the texture parameters for the loaded texture
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//  Get rid of the old loaded surface
	SDL_FreeSurface(sdlSurface);

	//  Create a ManagedTexture with our new data, and stick it in the TextureList
	auto index = FirstFreeIndex();
	MANAGE_MEMORY_NEW("TextureManager", sizeof(ManagedTexture));
	auto managedTexture = new ManagedTexture(sdlTexture, textureID, width, height, index);
	if (managedTexture == nullptr)
	{
		glDeleteTextures(1, &textureID);
		printf("Unable to create ManagedTexture with data\n");
		return nullptr;
	}
	m_TextureList[index] = managedTexture;
	m_TextureListByFile[std::string(textureFile)] = managedTexture;

	return managedTexture;
#else
	return nullptr;
#endif
}

inline GLuint TextureManager::GetTextureID(const int index)
{
	auto iter = m_TextureList.find(index);
	if (iter == m_TextureList.end()) return 0;

	return (*iter).second->m_TextureID;
}

inline TextureManager::ManagedTexture* TextureManager::GetManagedTexture(const int index)
{
	auto iter = m_TextureList.find(index);
	if (iter == m_TextureList.end()) return nullptr;

	return (*iter).second;
}

inline void TextureManager::Shutdown()
{
	while (!m_TextureList.empty())
	{
		m_TextureList.begin()->second->FreeTexture();
		MANAGE_MEMORY_DELETE("TextureManager", sizeof(ManagedTexture));
		delete m_TextureList.begin()->second;
		m_TextureList.erase(m_TextureList.begin());
	}
	m_TextureListByFile.clear();
}

inline int TextureManager::FirstFreeIndex()
{
	//  Find the first free index
	auto index = 0;
	for (; ; ++index)
	{
		auto iter = m_TextureList.find(index);
		if (iter == m_TextureList.end()) return index;
	}
}

//  Instance to be utilized by anyone including this header
TextureManager& textureManager = TextureManager::GetInstance();