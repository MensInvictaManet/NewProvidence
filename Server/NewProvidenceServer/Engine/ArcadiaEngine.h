#pragma once

#define AUDIO_ENABLED		true
#define CONSOLE_DISABLED	true

#define USING_SDL			true
#define USING_SDL_IMAGE		true
#define USING_SDL_MIXER		true
#define USING_GLEW			true
#define	USING_OPENGL		true
#define	USING_GLU			true

#if USING_SDL
#include <SDL.h>
#include <SDL_syswm.h>
#undef main
#pragma comment(lib, "SDL2.lib")
#endif

#if USING_GLEW
#include <GL/glew.h>
#pragma comment(lib, "glew32.lib")
#endif

#if USING_OPENGL
#include <SDL_opengl.h>
#pragma comment(lib, "opengl32.lib")
#endif

#if USING_GLU
#include <GL/GLU.h>
#pragma comment(lib, "glu32.lib")
#endif

#if USING_SDL_IMAGE
#include <SDL_image.h>
#pragma comment(lib, "SDL2_image.lib")
#endif

#if USING_SDL_MIXER
#include <SDL_mixer.h>
#pragma comment(lib, "SDL2_mixer.lib")
#endif

#if CONSOLE_DISABLED
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

#include <iostream>
#include <filesystem>

#include "WindowManager.h"
#include "TextureManager.h"
#include "GUIManager.h"
#include "InputManager.h"
#include "FontManager.h"
#include "TimeSlice.h"
#include "WinsockWrapper.h"
#include "MemoryManager.h"
#include "DebugConsole.h"
#include "AutoPlayManager.h"
#include "EventManager.h"

#if AUDIO_ENABLED
#include "SoundWrapper.h"
#endif

//  Screen dimension constants (Change using SetScreenDimensions prior to calling InitializeEngine)
static float ScreenWidth = 400.0f;
static float ScreenHeight = 400.0f;

static float Background_R = 0.5f;
static float Background_G = 0.5f;
static float Background_B = 0.5f;

static bool EscapeToQuit = true;

inline void ClearBackground() { glClearColor(Background_R, Background_G, Background_B, 1.f); }
inline void SetBackgroundColor(float r, float g, float b) { Background_R = r; Background_G = g; Background_B = b; ClearBackground(); }

inline void SetEscapeToQuit(bool escape) { EscapeToQuit = escape; }

inline void AddDebugConsoleCommands()
{
	//  MOVE_MOUSE_OVER: Automatically moves the mouse to a UI object click position
	debugConsole->AddDebugCommand("MOVE_MOUSE_OVER", [=](std::string commandString) -> bool
	{
		std::string args[3];
		auto argCount = 0;

		for (auto i = 0; i < 3; ++i)
		{
			if (commandString.empty()) break;
			size_t firstSpace = commandString.find_first_of(' ');
			if (firstSpace == -1) firstSpace = commandString.length();
			args[i] = commandString.substr(0, firstSpace);
			argCount = i + 1;
			commandString = (firstSpace == commandString.length()) ? "" : commandString.substr(firstSpace + 1, commandString.length());
		}

		auto mouseX = 0;
		auto mouseY = 0;
		auto objectFound = guiManager.GetClickPosition(args[0], mouseX, mouseY);
		switch (argCount)
		{
		case 1:
			InputManager::SetMousePosition(mouseX, mouseY);
			debugConsole->AddDebugConsoleLine("Mouse snapped to new position");
			break;

		case 2: //  A float for time was provided
		{
			auto time = float(atof(args[1].c_str()));
			inputManager.SetMousePositionTarget(mouseX, mouseY, time);
			debugConsole->AddDebugConsoleLine("Mouse moving to new position");
			break;
		}
		break;

		case 3: //  Two ints for speed were provided
		{
			auto speedX = float(atof(args[1].c_str()));
			auto speedY = float(atof(args[2].c_str()));
			inputManager.SetMousePositionTarget(mouseX, mouseY, speedX, speedY);
			debugConsole->AddDebugConsoleLine("Mouse moving to new position");
			break;
		}
		break;

		default: break;
		}

		return objectFound;
	});

	//  CLICK_MOUSE_LEFT: Simulates a left click
	debugConsole->AddDebugCommand("CLICK_MOUSE_LEFT", [=](std::string commandString) -> bool
	{
		debugConsole->AddDebugConsoleLine("Left mouse click simulated");
		inputManager.SetSimulatedMouseButtonLeft(MOUSE_BUTTON_PRESSED);
		return true;
	});

	//  CLICK_MOUSE_MIDDLE: Simulates a middle click
	debugConsole->AddDebugCommand("CLICK_MOUSE_MIDDLE", [=](std::string commandString) -> bool
	{
		debugConsole->AddDebugConsoleLine("Middle mouse click simulated");
		inputManager.SetSimulatedMouseButtonMiddle(MOUSE_BUTTON_PRESSED);
		return true;
	});

	//  CLICK_MOUSE_LEFT: Simulates a Right click
	debugConsole->AddDebugCommand("CLICK_MOUSE_RIGHT", [=](std::string commandString) -> bool
	{
		debugConsole->AddDebugConsoleLine("Right mouse click simulated");
		inputManager.SetSimulatedMouseButtonRight(MOUSE_BUTTON_PRESSED);
		return true;
	});

	//  ENTER_TEXT: Simulates a text input
	debugConsole->AddDebugCommand("ENTER_TEXT", [=](std::string commandString) -> bool
	{
		debugConsole->AddDebugConsoleLine("Text input simulated");
		inputManager.AddTextInput(commandString);
		return true;
	});
}

inline void ResizeWindow(void)
{
	glViewport(0, 0, GLsizei(ScreenWidth), GLsizei(ScreenHeight));

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0f, GLdouble(ScreenWidth) / GLdouble(ScreenHeight), 0.0f, 100.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

inline void SetScreenDimensions(float screenWidth, float screenHeight)
{
	ScreenWidth = screenWidth;
	ScreenHeight = screenHeight;
	ResizeWindow();
	debugConsole->SetWindowDimensions(int(ScreenWidth), int(ScreenHeight));
}

inline bool InitializeSDL()
{
	//  Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
	{
		printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
		return false;
	}

	//  Set SDL to use OpenGL 3.1
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_GL_SetSwapInterval(0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	//  Set the OpenGL attributes for multisampling
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

	//  Set the texture filtering to linear
	if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"))
	{
		printf("Warning: Linear texture filtering not enabled!");
	}

#if USING_SDL_IMAGE
	//  Initialize PNG loading
	int imgFlags = IMG_INIT_PNG;
	if (!(IMG_Init(imgFlags) & imgFlags))
	{
		printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
		return false;
	}
#endif

	//  Enable text input
	SDL_StartTextInput();

	return true;
}

inline bool InitializeOpenGL()
{
	auto success = true;
	GLenum error;

	//  Initialize the Projection Matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	//  Check for an error
	error = glGetError();
	if (error != GL_NO_ERROR)
	{
		printf("Error initializing OpenGL! %p\n", gluErrorString(error));
		success = false;
	}

	//  Initialize Modelview Matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	//  Check for an error
	error = glGetError();
	if (error != GL_NO_ERROR)
	{
		printf("Error initializing OpenGL! %p\n", gluErrorString(error));
		success = false;
	}

	//  Initialize clear color
	ClearBackground();

	//  Check for an error
	error = glGetError();
	if (error != GL_NO_ERROR)
	{
		printf("Error initializing OpenGL! %p\n", gluErrorString(error));
		success = false;
	}

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	return success;
}

inline bool InitializeEngine(const char* programTitle, float screenWidth, float screenHeght)
{
	SetScreenDimensions(screenWidth, screenHeght);

	//  Initialize SDL
	if (!InitializeSDL())
	{
		printf("Unable to initialize SDL!\n");
		return false;
	}

	auto windowIndex = windowManager.CreateNewWindow(programTitle, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, int(ScreenWidth), int(ScreenHeight), true, false, false);
	if (windowIndex == -1) return false;

	SDL_SetRenderDrawColor(windowManager.GetRenderer(windowIndex), 0xFF, 0xFF, 0xFF, 0xFF);

	//  Initialize Winsock
	winsockWrapper.WinsockInitialize();

	//  Add the Debug Console to the GUI Manager
	debugConsole->SetWindowDimensions(int(ScreenWidth), int(ScreenHeight));
	AddDebugConsoleCommands();
	guiManager.AddChild(debugConsole);

	//  Initialize OpenGL
	if (!InitializeOpenGL())
	{
		printf("Unable to initialize OpenGL!\n");
		return false;
	}

	//  Set the active texture setting
	glActiveTexture(GL_TEXTURE0);

	// If there are any OpenGL errors, throw an exception now
	auto error = glGetError();
	while (error != GL_NO_ERROR)
	{
		std::cout << "Error initializing OpenGL! " << gluErrorString(error) << "\n";
		throw std::runtime_error("OpenGL error");
		error = glGetError();
	}

#if AUDIO_ENABLED
#if USING_SDL_MIXER
	//Initialize SDL_mixer 
	if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 4096) == -1)
	{
		printf("Unable to initialize SDL_mixer!\n");
		return false;
	}
#endif
#endif

	// print out some info about the graphics drivers
	std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
	std::cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
	std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
	std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

	// make sure OpenGL version 3.2 API is available
	if (!GL_VERSION_3_2)
		throw std::runtime_error("OpenGL 3.2 API is not available.");

	return true;
}

inline void ShutdownEngine()
{
	//  Shut down the manager classes that need it
	windowManager.Shutdown();
	guiManager.Shutdown();

#if USING_SDL
	//  Disable text input
	SDL_StopTextInput();

	SDL_Quit();
#endif
}

inline void RenderScreen()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	//  Initiate the 3D Rendering Context
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, GLdouble(ScreenWidth) / GLdouble(ScreenHeight), 1.0, 2000.0);

	glDisable(GL_TEXTURE_2D);

	guiManager.Render3D();

	//  Initiate the 2D Rendering Context
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, GLdouble(ScreenWidth), GLdouble(ScreenHeight), 0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);

	//  Render the 2D GUI through the GUIManager
	guiManager.Render();
}

inline void PrimaryLoop()
{
	//  While application is running
	auto quit = false;
	while (!quit)
	{
		DetermineTimeSlice();

		//  Get the current state of mouse and keyboard input
		inputManager.GetInputForFrame();
		if (EscapeToQuit && inputManager.GetKeyPressed(SDL_SCANCODE_ESCAPE)) quit = true;

		//  Handle events on queue
		SDL_Event e;
		while (SDL_PollEvent(&e) != 0)
		{
			switch (e.type)
			{
			case SDL_QUIT:
				quit = true;
				break;

			case SDL_TEXTINPUT:
				inputManager.AddKeyToString(e.text.text[0]);
				break;

			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				switch (e.button.button)
				{
				case 1: inputManager.SetMouseButtonLeft((e.type == SDL_MOUSEBUTTONDOWN)); break;
				case 2: inputManager.SetMouseButtonMiddle((e.type == SDL_MOUSEBUTTONDOWN)); break;
				case 3: inputManager.SetMouseButtonRight((e.type == SDL_MOUSEBUTTONDOWN)); break;
				default: break;
				}
				break;

			case SDL_DROPFILE:
			{
				auto dropFilePath = std::string(e.drop.file);
				if (std::filesystem::exists(dropFilePath))
				{
					auto isFolder = std::filesystem::is_directory(dropFilePath);
					auto fileDrop = FileDropEventData(dropFilePath, isFolder, "ArcadiaEngine");
					eventManager.BroadcastEvent(&fileDrop);
				}
			}
			break;

			default:
				windowManager.HandleEvent(e);
				break;
			}
		}

		//  Pre-Update
		autoplayManager.Update();

		//  Input
		guiManager.Input();

		//  Update
		guiManager.Update();
		inputManager.Update();

		//  Render
		RenderScreen();
		windowManager.Render();

		//  End Step
		guiManager.EndStep();
	}
}