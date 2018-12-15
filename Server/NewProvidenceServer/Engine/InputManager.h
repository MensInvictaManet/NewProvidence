#pragma once

#include <SDL.h>

#include "TimeSlice.h"
#include "WindowManager.h"

enum MouseButtonSimulation
{
	SIMULATED_MOUSE_BUTTON_UNPRESSED_PRESSED,		//  Button wants to simulate an unpress, but is pressed
	SIMULATED_MOUSE_BUTTON_UNPRESSED_HELD,			//  Button wants to simulate an unpress, but is held
	SIMULATED_MOUSE_BUTTON_PRESSED_UNPRESSED,		//  Button wants to simulate a press, but is unpressed
	SIMULATED_MOUSE_BUTTON_PRESSED_PRESSED,			//  Button wants to simulate a press, and is pressed
	SIMULATED_MOUSE_BUTTON_PRESSED_HELD,			//  Button wants to simulate a press, but is held
	SIMULATED_MOUSE_BUTTON_HELD_UNPRESSED,			//  Button wants to simulate a hold, but is unpressed
	SIMULATED_MOUSE_BUTTON_HELD_PRESSED,			//  Button wants to simulate a hold, but is pressed
	SIMULATED_MOUSE_BUTTON_HELD_HELD,				//  Button wants to simulate a hold, and is held (must continue)
	SIMULATED_MOUSE_UNSIMULATED						//  No simulation, don't use the simulation override at all
};

enum MouseButtonState
{
	MOUSE_BUTTON_UNPRESSED,
	MOUSE_BUTTON_PRESSED,
	MOUSE_BUTTON_PRESSED_TAKEN,
	MOUSE_BUTTON_HELD
};

class InputManager
{
public:
	enum KeyStates { KEY_STATE_UNPRESSED, KEY_STATE_NEW_PRESS, KEY_STATE_HELD };
	static InputManager& GetInstance() { static InputManager INSTANCE; return INSTANCE; }

	void GetInputForFrame();
	void Update();

	inline int GetMouseX() const { return m_WindowMouseX; }
	inline int GetMouseY() const { return m_WindowMouseY; }
	inline MouseButtonState GetMouseButtonLeft() const { return m_MouseButtonLeft; }
	inline MouseButtonState GetMouseButtonMiddle() const { return m_MouseButtonMiddle; }
	inline MouseButtonState GetMouseButtonRight() const { return m_MouseButtonRight; }
	inline void TakeMouseButtonLeft() { if (m_MouseButtonLeft == MOUSE_BUTTON_PRESSED) m_MouseButtonLeft = MOUSE_BUTTON_PRESSED_TAKEN; }
	inline void TakeMouseButtonMiddle() { if (m_MouseButtonMiddle == MOUSE_BUTTON_PRESSED) m_MouseButtonMiddle = MOUSE_BUTTON_PRESSED_TAKEN; }
	inline void TakeMouseButtonRight() { if (m_MouseButtonRight == MOUSE_BUTTON_PRESSED) m_MouseButtonRight = MOUSE_BUTTON_PRESSED_TAKEN; }
	inline bool GetKeyDown(int scanCode) const { return (m_KeyStates[scanCode] == KEY_STATE_NEW_PRESS); }
	inline bool GetKeyPressed(int scanCode) const { return (m_KeyStates[scanCode] != KEY_STATE_UNPRESSED); }
	inline char GetKeyState(int scanCode) const { return m_KeyStates[scanCode]; }
	inline char GetTab() const { return (m_KeyStates[SDL_SCANCODE_TAB] == KEY_STATE_NEW_PRESS); }
	inline bool GetBackspace() const { return (m_KeyStates[SDL_SCANCODE_BACKSPACE] != KEY_STATE_UNPRESSED); }
	inline bool GetEnter() const { return (m_KeyStates[SDL_SCANCODE_RETURN] == KEY_STATE_NEW_PRESS); }
	inline const std::string& GetKeyboardString() const { return m_KeyboardString; }
	inline bool GetIsMouseAutoMoving() const { return (m_MouseTargetPositionSpeedX != 0.0f || m_MouseTargetPositionSpeedY != 0.0f); }

	inline void SetMouseButtonLeft(bool setting) { if (setting != (m_MouseButtonLeft != MOUSE_BUTTON_UNPRESSED)) m_MouseButtonLeft = (setting ? MOUSE_BUTTON_PRESSED : MOUSE_BUTTON_UNPRESSED); }
	inline void SetMouseButtonMiddle(bool setting) { if (setting != (m_MouseButtonMiddle != MOUSE_BUTTON_UNPRESSED)) m_MouseButtonMiddle = (setting ? MOUSE_BUTTON_PRESSED : MOUSE_BUTTON_UNPRESSED); }
	inline void SetMouseButtonRight(bool setting) { if (setting != (m_MouseButtonRight != MOUSE_BUTTON_UNPRESSED)) m_MouseButtonRight = (setting ? MOUSE_BUTTON_PRESSED : MOUSE_BUTTON_UNPRESSED); }
	inline void ResetTab(KeyStates newState) { m_KeyStates[SDL_SCANCODE_TAB] = newState; }
	inline void ResetEnter(KeyStates newState) { m_KeyStates[SDL_SCANCODE_RETURN] = newState; }


	void AddKeyToString(int key);
	void SetMousePositionTarget(int xPos, int yPos, float time);
	void SetMousePositionTarget(int xPos, int yPos, float xSpeed, float ySpeed);
	void SetSimulatedMouseButtonLeft(MouseButtonState simulatedState);
	void SetSimulatedMouseButtonMiddle(MouseButtonState simulatedState);
	void SetSimulatedMouseButtonRight(MouseButtonState simulatedState);

	static void GetMousePosition(int& xPos, int& yPos, bool inWindow = true);
	static void SetMousePosition(int xPos = 0, int yPos = 0, bool inWindow = true);
	void AddTextInput(std::string& textInput);

private:
	InputManager();
	~InputManager();

	void DetermineMouseClickStates();

	int m_WindowMouseX;
	int m_WindowMouseY;
	float m_MouseX;
	float m_MouseY;
	MouseButtonState m_MouseButtonLeft;
	MouseButtonState m_MouseButtonMiddle;
	MouseButtonState m_MouseButtonRight;
	char m_KeyStates[SDL_NUM_SCANCODES];
	std::string m_KeyboardString;

	//  Automated mouse movement
	int m_MouseTargetPositionX;
	int m_MouseTargetPositionY;
	float m_MouseTargetPositionSpeedX;
	float m_MouseTargetPositionSpeedY;

	//  Simulated mouse control
	MouseButtonSimulation m_SimulatedMouseButtonLeft;
	MouseButtonSimulation m_SimulatedMouseButtonMiddle;
	MouseButtonSimulation m_SimulatedMouseButtonRight;
};

inline void InputManager::GetInputForFrame()
{
	m_KeyboardString = "";

	SDL_GetMouseState(&m_WindowMouseX, &m_WindowMouseY);

	DetermineMouseClickStates();

	auto keyStates = SDL_GetKeyboardState(nullptr);
	for (auto i = 0; i < SDL_NUM_SCANCODES; ++i)
	{
		m_KeyStates[i] = (keyStates[i] == 0) ? 0 : (m_KeyStates[i] == 0 ? 1 : 2);
	}
}

inline void InputManager::Update()
{
	auto mouseX = 0;
	auto mouseY = 0;
	GetMousePosition(mouseX, mouseY, false);
	if (mouseX != int(m_MouseX)) m_MouseX = float(mouseX);
	if (mouseY != int(m_MouseY)) m_MouseY = float(mouseY);

	if (m_MouseTargetPositionSpeedX == 0.0f && m_MouseTargetPositionSpeedY == 0.0f) return;

	auto left = 0;
	auto top = 0;
	windowManager.GetWindowTopLeft(left, top, -1);

	if (mouseX == left + m_MouseTargetPositionX)	m_MouseTargetPositionSpeedX = 0.0f;
	if (mouseY == top + m_MouseTargetPositionY)		m_MouseTargetPositionSpeedY = 0.0f;

	if (m_MouseTargetPositionSpeedX != 0)
	{
		if (abs(left + m_MouseTargetPositionX - mouseX) <= (m_MouseTargetPositionSpeedX * frameSecondsF)) m_MouseX = float(left + m_MouseTargetPositionX);
		else m_MouseX += ((left + m_MouseTargetPositionX - mouseX) > 0) ? (m_MouseTargetPositionSpeedX * frameSecondsF) : (-m_MouseTargetPositionSpeedX * frameSecondsF);
	}

	if (m_MouseTargetPositionSpeedY != 0)
	{
		if (abs(top + m_MouseTargetPositionY - mouseY) <= (m_MouseTargetPositionSpeedY * frameSecondsF)) m_MouseY = float(top + m_MouseTargetPositionY);
		else m_MouseY += ((top + m_MouseTargetPositionY - mouseY) > 0) ? (m_MouseTargetPositionSpeedY * frameSecondsF) : (-m_MouseTargetPositionSpeedY * frameSecondsF);
	}

	//  Update the mouse position
	mouseX = int(m_MouseX);
	mouseY = int(m_MouseY);
	SetMousePosition(mouseX, mouseY, false);
}

inline void InputManager::AddKeyToString(int key)
{
	if (key < 32) return;
	if ((key > 126) && ((key < 256) || (key > 265))) return;

	//  If the symbol is a letter and shift is held down, capitalize it
	if ((key >= 97) && (key <= 122) && (GetKeyDown(Uint8(303)) || GetKeyDown(Uint8(304))))
		key -= 32;

	if ((GetKeyDown(Uint8(303)) || GetKeyDown(Uint8(304))))
	{
		switch (key)
		{
		case 39:	key = 34;		break;
		case 44:	key = 60;		break;
		case 45:	key = 95;		break;
		case 46:	key = 62;		break;
		case 47:	key = 63;		break;
		case 48:	key = 41;		break;
		case 49:	key = 33;		break;
		case 50:	key = 64;		break;
		case 51:	key = 35;		break;
		case 52:	key = 36;		break;
		case 53:	key = 37;		break;
		case 54:	key = 94;		break;
		case 55:	key = 38;		break;
		case 56:	key = 42;		break;
		case 57:	key = 40;		break;
		case 59:	key = 58;		break;
		case 61:	key = 43;		break;
		case 91:	key = 123;		break;
		case 92:	key = 124;		break;
		case 93:	key = 125;		break;
		case 96:	key = 126;		break;
		default:break;
		}
	}

	switch (key)
	{
	case 256:
	case 257:
	case 258:
	case 259:
	case 260:
	case 261:
	case 262:
	case 263:
	case 264:
	case 265:
		key -= 208;
		break;
	default:break;
	}


	m_KeyboardString += UCHAR(key);
}

inline void InputManager::SetMousePositionTarget(int xPos, int yPos, float time)
{
	int mouseX;
	int mouseY;
	GetMousePosition(mouseX, mouseY);

	auto speedX = abs((xPos - mouseX) / time);
	auto speedY = abs((yPos - mouseY) / time);

	SetMousePositionTarget(xPos, yPos, speedX, speedY);
}

inline void InputManager::SetMousePositionTarget(int xPos, int yPos, float xSpeed, float ySpeed)
{
	m_MouseTargetPositionX = xPos;
	m_MouseTargetPositionY = yPos;
	m_MouseTargetPositionSpeedX = xSpeed;
	m_MouseTargetPositionSpeedY = ySpeed;
}

inline void InputManager::SetSimulatedMouseButtonLeft(MouseButtonState simulatedState)
{
	switch (simulatedState)
	{
	default:
	case MOUSE_BUTTON_UNPRESSED:
		m_SimulatedMouseButtonLeft = (m_MouseButtonLeft == MOUSE_BUTTON_UNPRESSED) ? SIMULATED_MOUSE_UNSIMULATED : ((m_MouseButtonLeft == MOUSE_BUTTON_PRESSED) ? SIMULATED_MOUSE_BUTTON_UNPRESSED_PRESSED : SIMULATED_MOUSE_BUTTON_UNPRESSED_HELD);
		break;
	case MOUSE_BUTTON_PRESSED:
	case MOUSE_BUTTON_PRESSED_TAKEN:
		m_SimulatedMouseButtonLeft = (m_MouseButtonLeft == MOUSE_BUTTON_UNPRESSED) ? SIMULATED_MOUSE_BUTTON_PRESSED_UNPRESSED : ((m_MouseButtonLeft == MOUSE_BUTTON_PRESSED) ? SIMULATED_MOUSE_BUTTON_PRESSED_PRESSED : SIMULATED_MOUSE_BUTTON_PRESSED_HELD);
		break;
	case MOUSE_BUTTON_HELD:
		m_SimulatedMouseButtonLeft = (m_MouseButtonLeft == MOUSE_BUTTON_UNPRESSED) ? SIMULATED_MOUSE_BUTTON_HELD_UNPRESSED : ((m_MouseButtonLeft == MOUSE_BUTTON_PRESSED) ? SIMULATED_MOUSE_BUTTON_HELD_PRESSED : SIMULATED_MOUSE_BUTTON_HELD_HELD);
		break;
	}
}

inline void InputManager::SetSimulatedMouseButtonMiddle(MouseButtonState simulatedState)
{
	switch (simulatedState)
	{
	default:
	case MOUSE_BUTTON_UNPRESSED:
		m_SimulatedMouseButtonMiddle = (m_MouseButtonMiddle == MOUSE_BUTTON_UNPRESSED) ? SIMULATED_MOUSE_UNSIMULATED : ((m_MouseButtonMiddle == MOUSE_BUTTON_PRESSED) ? SIMULATED_MOUSE_BUTTON_UNPRESSED_PRESSED : SIMULATED_MOUSE_BUTTON_UNPRESSED_HELD);
		break;
	case MOUSE_BUTTON_PRESSED:
	case MOUSE_BUTTON_PRESSED_TAKEN:
		m_SimulatedMouseButtonMiddle = (m_MouseButtonMiddle == MOUSE_BUTTON_UNPRESSED) ? SIMULATED_MOUSE_BUTTON_PRESSED_UNPRESSED : ((m_MouseButtonMiddle == MOUSE_BUTTON_PRESSED) ? SIMULATED_MOUSE_BUTTON_PRESSED_PRESSED : SIMULATED_MOUSE_BUTTON_PRESSED_HELD);
		break;
	case MOUSE_BUTTON_HELD:
		m_SimulatedMouseButtonMiddle = (m_MouseButtonMiddle == MOUSE_BUTTON_UNPRESSED) ? SIMULATED_MOUSE_BUTTON_HELD_UNPRESSED : ((m_MouseButtonMiddle == MOUSE_BUTTON_PRESSED) ? SIMULATED_MOUSE_BUTTON_HELD_PRESSED : SIMULATED_MOUSE_BUTTON_HELD_HELD);
		break;
	}
}

inline void InputManager::SetSimulatedMouseButtonRight(MouseButtonState simulatedState)
{
	switch (simulatedState)
	{
	default:
	case MOUSE_BUTTON_UNPRESSED:
		m_SimulatedMouseButtonRight = (m_MouseButtonRight == MOUSE_BUTTON_UNPRESSED) ? SIMULATED_MOUSE_UNSIMULATED : ((m_MouseButtonRight == MOUSE_BUTTON_PRESSED) ? SIMULATED_MOUSE_BUTTON_UNPRESSED_PRESSED : SIMULATED_MOUSE_BUTTON_UNPRESSED_HELD);
		break;
	case MOUSE_BUTTON_PRESSED:
	case MOUSE_BUTTON_PRESSED_TAKEN:
		m_SimulatedMouseButtonRight = (m_MouseButtonRight == MOUSE_BUTTON_UNPRESSED) ? SIMULATED_MOUSE_BUTTON_PRESSED_UNPRESSED : ((m_MouseButtonRight == MOUSE_BUTTON_PRESSED) ? SIMULATED_MOUSE_BUTTON_PRESSED_PRESSED : SIMULATED_MOUSE_BUTTON_PRESSED_HELD);
		break;
	case MOUSE_BUTTON_HELD:
		m_SimulatedMouseButtonRight = (m_MouseButtonRight == MOUSE_BUTTON_UNPRESSED) ? SIMULATED_MOUSE_BUTTON_HELD_UNPRESSED : ((m_MouseButtonRight == MOUSE_BUTTON_PRESSED) ? SIMULATED_MOUSE_BUTTON_HELD_PRESSED : SIMULATED_MOUSE_BUTTON_HELD_HELD);
		break;
	}
}

inline void InputManager::GetMousePosition(int& xPos, int& yPos, bool inWindow)
{
	RECT rect = { 0 };

	if (inWindow)
	{
		auto window = windowManager.GetWindow(-1);
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

		rect.left += side_border_width;
		rect.top += top_border_full_height;

		SetForegroundWindow(hwnd);
		SetActiveWindow(hwnd);
		SetFocus(hwnd);
		GetWindowRect(hwnd, &rect);
	}

	POINT mousePos;
	GetCursorPos(&mousePos);
	xPos = mousePos.x - rect.left;
	yPos = mousePos.y - rect.top;
}

inline void InputManager::SetMousePosition(int xPos, int yPos, bool inWindow)
{
	RECT rect = { 0 };

	if (inWindow)
	{
		auto window = windowManager.GetWindow(-1);
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

		SetForegroundWindow(hwnd);
		SetActiveWindow(hwnd);
		SetFocus(hwnd);
		GetWindowRect(hwnd, &rect);

		rect.left += side_border_width;
		rect.top += top_border_full_height;
	}

	SetCursorPos(rect.left + xPos, rect.top + yPos);
}

inline void InputManager::AddTextInput(std::string& textInput)
{
	m_KeyboardString += textInput;
}


inline InputManager::InputManager() :
	m_WindowMouseX(0),
	m_WindowMouseY(0),
	m_MouseX(0.0f),
	m_MouseY(0.0f),
	m_MouseButtonLeft(MOUSE_BUTTON_UNPRESSED),
	m_MouseButtonMiddle(MOUSE_BUTTON_UNPRESSED),
	m_MouseButtonRight(MOUSE_BUTTON_UNPRESSED),
	m_KeyboardString(""),
	m_MouseTargetPositionX(-1),
	m_MouseTargetPositionY(-1),
	m_MouseTargetPositionSpeedX(0),
	m_MouseTargetPositionSpeedY(0),
	m_SimulatedMouseButtonLeft(SIMULATED_MOUSE_UNSIMULATED),
	m_SimulatedMouseButtonMiddle(SIMULATED_MOUSE_UNSIMULATED),
	m_SimulatedMouseButtonRight(SIMULATED_MOUSE_UNSIMULATED)
{
	memset(m_KeyStates, 0, sizeof(m_KeyStates));
}

inline InputManager::~InputManager()
{
}

inline void InputManager::DetermineMouseClickStates()
{
	if (m_MouseButtonLeft == MOUSE_BUTTON_PRESSED) m_MouseButtonLeft = MOUSE_BUTTON_HELD;
	if (m_MouseButtonMiddle == MOUSE_BUTTON_PRESSED) m_MouseButtonMiddle = MOUSE_BUTTON_HELD;
	if (m_MouseButtonRight == MOUSE_BUTTON_PRESSED) m_MouseButtonRight = MOUSE_BUTTON_HELD;

	switch (m_SimulatedMouseButtonLeft)
	{
	case SIMULATED_MOUSE_BUTTON_UNPRESSED_PRESSED:
	case SIMULATED_MOUSE_BUTTON_UNPRESSED_HELD:
		m_MouseButtonLeft = MOUSE_BUTTON_UNPRESSED;
		m_SimulatedMouseButtonLeft = SIMULATED_MOUSE_UNSIMULATED;
		break;

	case SIMULATED_MOUSE_BUTTON_PRESSED_UNPRESSED:
		m_MouseButtonLeft = MOUSE_BUTTON_PRESSED;
		m_SimulatedMouseButtonLeft = SIMULATED_MOUSE_BUTTON_PRESSED_PRESSED;
		break;

	case SIMULATED_MOUSE_BUTTON_PRESSED_PRESSED:
		m_MouseButtonLeft = MOUSE_BUTTON_UNPRESSED;
		m_SimulatedMouseButtonLeft = SIMULATED_MOUSE_UNSIMULATED;
		break;

	case SIMULATED_MOUSE_BUTTON_PRESSED_HELD:
		m_MouseButtonLeft = MOUSE_BUTTON_PRESSED;
		m_SimulatedMouseButtonLeft = SIMULATED_MOUSE_UNSIMULATED;
		break;

	case SIMULATED_MOUSE_BUTTON_HELD_UNPRESSED:
		m_MouseButtonLeft = MOUSE_BUTTON_PRESSED;
		m_SimulatedMouseButtonLeft = SIMULATED_MOUSE_BUTTON_HELD_PRESSED;
		break;

	case SIMULATED_MOUSE_BUTTON_HELD_PRESSED:
		m_MouseButtonLeft = MOUSE_BUTTON_HELD;
		m_SimulatedMouseButtonLeft = SIMULATED_MOUSE_UNSIMULATED;
		break;

	case SIMULATED_MOUSE_BUTTON_HELD_HELD:
		m_MouseButtonLeft = MOUSE_BUTTON_HELD;
		break;

	case SIMULATED_MOUSE_UNSIMULATED:
	default:break;
	}

	switch (m_SimulatedMouseButtonMiddle)
	{
	case SIMULATED_MOUSE_BUTTON_UNPRESSED_PRESSED:
	case SIMULATED_MOUSE_BUTTON_UNPRESSED_HELD:
		m_MouseButtonMiddle = MOUSE_BUTTON_UNPRESSED;
		m_SimulatedMouseButtonMiddle = SIMULATED_MOUSE_UNSIMULATED;
		break;

	case SIMULATED_MOUSE_BUTTON_PRESSED_UNPRESSED:
		m_MouseButtonMiddle = MOUSE_BUTTON_PRESSED;
		m_SimulatedMouseButtonMiddle = SIMULATED_MOUSE_BUTTON_PRESSED_PRESSED;
		break;

	case SIMULATED_MOUSE_BUTTON_PRESSED_PRESSED:
		m_MouseButtonMiddle = MOUSE_BUTTON_UNPRESSED;
		m_SimulatedMouseButtonMiddle = SIMULATED_MOUSE_UNSIMULATED;
		break;

	case SIMULATED_MOUSE_BUTTON_PRESSED_HELD:
		m_MouseButtonMiddle = MOUSE_BUTTON_PRESSED;
		m_SimulatedMouseButtonMiddle = SIMULATED_MOUSE_UNSIMULATED;
		break;

	case SIMULATED_MOUSE_BUTTON_HELD_UNPRESSED:
		m_MouseButtonMiddle = MOUSE_BUTTON_PRESSED;
		m_SimulatedMouseButtonMiddle = SIMULATED_MOUSE_BUTTON_HELD_PRESSED;
		break;

	case SIMULATED_MOUSE_BUTTON_HELD_PRESSED:
		m_MouseButtonMiddle = MOUSE_BUTTON_HELD;
		m_SimulatedMouseButtonMiddle = SIMULATED_MOUSE_UNSIMULATED;
		break;

	case SIMULATED_MOUSE_BUTTON_HELD_HELD:
		m_MouseButtonMiddle = MOUSE_BUTTON_HELD;
		break;

	case SIMULATED_MOUSE_UNSIMULATED:
	default:break;
	}

	switch (m_SimulatedMouseButtonRight)
	{
	case SIMULATED_MOUSE_BUTTON_UNPRESSED_PRESSED:
	case SIMULATED_MOUSE_BUTTON_UNPRESSED_HELD:
		m_MouseButtonRight = MOUSE_BUTTON_UNPRESSED;
		m_SimulatedMouseButtonRight = SIMULATED_MOUSE_UNSIMULATED;
		break;

	case SIMULATED_MOUSE_BUTTON_PRESSED_UNPRESSED:
		m_MouseButtonRight = MOUSE_BUTTON_PRESSED;
		m_SimulatedMouseButtonRight = SIMULATED_MOUSE_BUTTON_PRESSED_PRESSED;
		break;

	case SIMULATED_MOUSE_BUTTON_PRESSED_PRESSED:
		m_MouseButtonRight = MOUSE_BUTTON_UNPRESSED;
		m_SimulatedMouseButtonRight = SIMULATED_MOUSE_UNSIMULATED;
		break;

	case SIMULATED_MOUSE_BUTTON_PRESSED_HELD:
		m_MouseButtonRight = MOUSE_BUTTON_PRESSED;
		m_SimulatedMouseButtonRight = SIMULATED_MOUSE_UNSIMULATED;
		break;

	case SIMULATED_MOUSE_BUTTON_HELD_UNPRESSED:
		m_MouseButtonRight = MOUSE_BUTTON_PRESSED;
		m_SimulatedMouseButtonRight = SIMULATED_MOUSE_BUTTON_HELD_PRESSED;
		break;

	case SIMULATED_MOUSE_BUTTON_HELD_PRESSED:
		m_MouseButtonRight = MOUSE_BUTTON_HELD;
		m_SimulatedMouseButtonRight = SIMULATED_MOUSE_UNSIMULATED;
		break;

	case SIMULATED_MOUSE_BUTTON_HELD_HELD:
		m_MouseButtonRight = MOUSE_BUTTON_HELD;
		break;

	case SIMULATED_MOUSE_UNSIMULATED:
	default:break;
	}
}

//  Instance to be utilized by anyone including this header
InputManager& inputManager = InputManager::GetInstance();

#define KEY_PRESSED(key) inputManager.GetKeyPressed(key)