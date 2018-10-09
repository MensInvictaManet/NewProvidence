#pragma once

#include <SDL.h>
#undef main

#include "MemoryManager.h"

#include <unordered_map>
#include <sstream>

class WindowManager
{
private:
	struct WindowLink
	{
		SDL_Window*		m_Window;
		SDL_GLContext	m_Context;
		SDL_Renderer*	m_Renderer;
		int				m_WindowID;
		bool			m_Shown;
		int				m_Width;
		int				m_Height;
		bool			m_MouseFocus;
		bool			m_KeyboardFocus;
		bool			m_Minimized;

		WindowLink(SDL_Window* window, SDL_GLContext context, SDL_Renderer* renderer, int windowID, bool shown, int width, int height) :
			m_Window(window),
			m_Context(context),
			m_Renderer(renderer),
			m_WindowID(windowID),
			m_Shown(shown),
			m_Width(width),
			m_Height(height),
			m_MouseFocus(false),
			m_KeyboardFocus(false),
			m_Minimized(false)
		{}

		void HandleEvent(SDL_Event& e);
	};

public:
	static WindowManager& GetInstance() { static WindowManager INSTANCE; return INSTANCE; }

	bool GetWindowTopLeft(int& xPos, int& yPos, const int index = -1);

	int CreateNewWindow(const char* title = "", int x = SDL_WINDOWPOS_UNDEFINED, int y = SDL_WINDOWPOS_UNDEFINED, int w = 100, int h = 100, bool shown = true, bool current = false, bool vsync = true);
	bool DestroyWindow(const int index);
	void HandleEvent(SDL_Event& e);
	void Render();
	void Shutdown();

	SDL_Window* GetWindow(const int index = -1);
	SDL_GLContext GetContext(const int index = -1);
	SDL_Renderer* GetRenderer(const int index = -1);
	int GetWindowID(const int index = -1);
	bool GetWindowShown(const int index = -1);
	int GetWindowWidth(const int index = -1);
	int GetWindowHeight(const int index = -1);
	bool SetCurrentWindow(const int index = -1);
	bool SetWindowCaption(const int index = -1, const char* newCaption = "");

private:
	WindowManager();
	~WindowManager();

	int FirstFreeIndex();
	int FirstUsedIndex();
	static void DestroyWindow(WindowLink* windowLink);

	std::unordered_map<int, WindowLink*> m_WindowList;
	int m_CurrentWindow;
};

inline void WindowManager::WindowLink::HandleEvent(SDL_Event& e)
{
	//If an event was detected for this window
	if (e.type == SDL_WINDOWEVENT && e.window.windowID == m_WindowID)
	{
		switch (e.window.event)
		{
			//Window appeared
		case SDL_WINDOWEVENT_SHOWN:
			m_Shown = true;
			break;

			//Window disappeared
		case SDL_WINDOWEVENT_HIDDEN:
			m_Shown = false;
			break;

			//Get new dimensions and repaint
		case SDL_WINDOWEVENT_SIZE_CHANGED:
			m_Width = e.window.data1;
			m_Height = e.window.data2;
			SDL_RenderPresent(m_Renderer);
			break;

			//Repaint on expose
		case SDL_WINDOWEVENT_EXPOSED:
			SDL_RenderPresent(m_Renderer);
			break;

			//Mouse enter
		case SDL_WINDOWEVENT_ENTER:
			m_MouseFocus = true;
			break;

			//Mouse exit
		case SDL_WINDOWEVENT_LEAVE:
			m_MouseFocus = false;
			break;

			//Keyboard focus gained
		case SDL_WINDOWEVENT_FOCUS_GAINED:
			m_KeyboardFocus = true;
			break;

			//Keyboard focus lost
		case SDL_WINDOWEVENT_FOCUS_LOST:
			m_KeyboardFocus = false;
			break;

			//Window minimized
		case SDL_WINDOWEVENT_MINIMIZED:
			m_Minimized = true;
			break;

			//Window maxized
		case SDL_WINDOWEVENT_MAXIMIZED:
			m_Minimized = false;
			break;

			//Window restored
		case SDL_WINDOWEVENT_RESTORED:
			m_Minimized = false;
			break;

			//Hide on close
		case SDL_WINDOWEVENT_CLOSE:
			SDL_HideWindow(m_Window);
			break;

		case SDL_WINDOWEVENT_MOVED:
		default:
			break;
		}
	}
}

inline int WindowManager::CreateNewWindow(const char* title, int x, int y, int w, int h, bool shown, bool current, bool vsync)
{
	auto index = FirstFreeIndex();

	//  Create the Window
	auto newWindow = SDL_CreateWindow(title, x, y, w, h, SDL_WINDOW_OPENGL | (shown ? SDL_WINDOW_SHOWN : 0));
	if (newWindow == nullptr)
	{
		printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
		return -1;
	}

	//  Create the OpenGL context
	auto newContext = SDL_GL_CreateContext(newWindow);
	if (newContext == nullptr)
	{
		printf("OpenGL context could not be created! SDL Error: %s\n", SDL_GetError());
		SDL_DestroyWindow(newWindow);
		return -1;
	}

	GLenum glew = glewInit();
	if (glew != GLEW_OK)
	{
		char errorString[200];
		sprintf_s(errorString, "%s", glewGetErrorString(glew));
		fprintf(stderr, "Error: GLEW init failed");
		return false;
	}

	//  Create the Renderer
	auto newRenderer = SDL_CreateRenderer(newWindow, -1, SDL_RENDERER_ACCELERATED);
	if (newRenderer == nullptr)
	{
		printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
		SDL_GL_DeleteContext(newContext);
		SDL_DestroyWindow(newWindow);
		return -1;
	}

	//  Create the WindowLink and assign the new data
	MANAGE_MEMORY_NEW("WindowManager", sizeof(WindowLink));
	m_WindowList[index] = new WindowLink(newWindow, newContext, newRenderer, SDL_GetWindowID(newWindow), shown, w, h);
	if ((m_WindowList.find(index) == m_WindowList.end()) || m_WindowList[index] == nullptr)
	{
		printf("A new WindowLink could not be allocated!\n");
		SDL_GL_DeleteContext(newContext);
		SDL_DestroyWindow(newWindow);
		return -1;
	}

	//  (Temporarily) swap to the new window as current to set VSYNC
	SDL_GL_MakeCurrent(m_WindowList[index]->m_Window, m_WindowList[index]->m_Context);
	SDL_GL_SetSwapInterval(vsync ? 1 : 0);
	if (vsync && (SDL_GL_SetSwapInterval(1) < 0))
	{
		printf("Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError());
	}

	//  Set the current window based on whether we're setting this or not
	if (current | (m_CurrentWindow == -1)) m_CurrentWindow = index;
	SDL_GL_MakeCurrent(m_WindowList[m_CurrentWindow]->m_Window, m_WindowList[m_CurrentWindow]->m_Context);

	//  Return the new window's index
	return index;
}

inline bool WindowManager::DestroyWindow(const int index)
{
	auto iter = m_WindowList.find(index);
	if (iter == m_WindowList.end()) return false;

	DestroyWindow((*iter).second);
	m_WindowList.erase(iter);
	return true;
}

inline void WindowManager::HandleEvent(SDL_Event& e)
{
	for (auto iter = m_WindowList.begin(); iter != m_WindowList.end(); ++iter) (*iter).second->HandleEvent(e);
}

inline void WindowManager::Render()
{
	for (auto iter = m_WindowList.begin(); iter != m_WindowList.end(); ++iter)
	{
		if ((*iter).second->m_Minimized) continue;

		SDL_GL_SwapWindow((*iter).second->m_Window);
	}
}

inline void WindowManager::Shutdown()
{
	while (!m_WindowList.empty())
	{
		DestroyWindow(m_WindowList.begin()->second);
		MANAGE_MEMORY_DELETE("WindowManager", sizeof(WindowLink));
		delete m_WindowList.begin()->second;
		m_WindowList.erase(m_WindowList.begin());
	}
}


inline bool WindowManager::GetWindowTopLeft(int& xPos, int& yPos, const int index)
{
	SDL_Window* window;
	if (index == -1) window = m_WindowList[m_CurrentWindow]->m_Window;
	else
	{
		auto iter = m_WindowList.find(index);
		if (iter == m_WindowList.end()) return false;
		window = iter->second->m_Window;
	}

	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(window, &wmInfo);
	auto hwnd = wmInfo.info.win.window;

	RECT rcClient, rcWind;
	POINT ptDiff;
	GetClientRect(hwnd, &rcClient);
	GetWindowRect(hwnd, &rcWind);
	ptDiff.x = (rcWind.right - rcWind.left) - rcClient.right;
	ptDiff.y = (rcWind.bottom - rcWind.top) - rcClient.bottom;

	auto side_border_width = (ptDiff.x / 2);
	auto top_border_thickness = GetSystemMetrics(SM_CYCAPTION);
	auto top_border_full_height = top_border_thickness + ((ptDiff.y - top_border_thickness) / 2);

	xPos = rcWind.left + side_border_width;
	yPos = rcWind.top + top_border_full_height;
	return true;
}

inline SDL_Window* WindowManager::GetWindow(int index)
{
	if (index == -1) return m_WindowList[m_CurrentWindow]->m_Window;

	auto iter = m_WindowList.find(index);
	if (iter == m_WindowList.end()) return nullptr;

	return (*iter).second->m_Window;
}

inline SDL_GLContext WindowManager::GetContext(int index)
{
	if (index == -1) return m_WindowList[m_CurrentWindow]->m_Context;

	auto iter = m_WindowList.find(index);
	if (iter == m_WindowList.end()) return nullptr;

	return (*iter).second->m_Context;
}

inline SDL_Renderer* WindowManager::GetRenderer(int index)
{
	if (index == -1) return m_WindowList[m_CurrentWindow]->m_Renderer;

	auto iter = m_WindowList.find(index);
	if (iter == m_WindowList.end()) return nullptr;

	return (*iter).second->m_Renderer;
}

inline int WindowManager::GetWindowID(const int index)
{
	if (index == -1) return m_WindowList[m_CurrentWindow]->m_WindowID;

	auto iter = m_WindowList.find(index);
	if (iter == m_WindowList.end()) return -1;

	return (*iter).second->m_WindowID;
}

inline bool WindowManager::GetWindowShown(const int index)
{
	if (index == -1) return m_WindowList[m_CurrentWindow]->m_Shown;

	auto iter = m_WindowList.find(index);
	if (iter == m_WindowList.end()) return false;

	return (*iter).second->m_Shown;
}

inline int WindowManager::GetWindowWidth(const int index)
{
	if (index == -1) return m_WindowList[m_CurrentWindow]->m_Width;

	auto iter = m_WindowList.find(index);
	if (iter == m_WindowList.end()) return -1;

	return (*iter).second->m_Width;
}

inline int WindowManager::GetWindowHeight(const int index)
{
	if (index == -1) return m_WindowList[m_CurrentWindow]->m_Height;

	auto iter = m_WindowList.find(index);
	if (iter == m_WindowList.end()) return -1;

	return (*iter).second->m_Height;
}

inline bool WindowManager::SetCurrentWindow(const int index)
{
	if (index == -1)
	{
		SDL_GL_MakeCurrent(m_WindowList[m_CurrentWindow]->m_Window, m_WindowList[m_CurrentWindow]->m_Context);
		return true;
	}

	auto iter = m_WindowList.find(index);
	if (iter == m_WindowList.end()) return false;

	SDL_GL_MakeCurrent((*iter).second->m_Window, (*iter).second->m_Context);
	return true;
}

inline bool WindowManager::SetWindowCaption(const int index, const char* newCaption)
{
	if (index < -1 || index >= int(m_WindowList.size())) return false;
	WindowLink* window = m_WindowList[(index == -1) ? m_CurrentWindow : index];
	SDL_SetWindowTitle(window->m_Window, newCaption);
	return true;
}

inline WindowManager::WindowManager() :
	m_CurrentWindow(-1)
{

}

inline WindowManager::~WindowManager()
{
	Shutdown();
}

inline int WindowManager::FirstFreeIndex()
{
	//  Find the first free index
	auto index = 0;
	for (; ; ++index)
	{
		auto iter = m_WindowList.find(index);
		if (iter == m_WindowList.end()) return index;
	}
}

inline int WindowManager::FirstUsedIndex()
{
	if (m_WindowList.size() == 0) return -1;

	//  Find the first free index
	auto index = 0;
	for (; ; ++index)
	{
		auto iter = m_WindowList.find(index);
		if (iter != m_WindowList.end()) return index;
	}
}

inline void WindowManager::DestroyWindow(WindowLink* windowLink)
{
	SDL_GL_DeleteContext(windowLink->m_Context);

	SDL_DestroyWindow(windowLink->m_Window);

	windowLink->m_Renderer = nullptr;
}

//  Instance to be utilized by anyone including this header
WindowManager& windowManager = WindowManager::GetInstance();