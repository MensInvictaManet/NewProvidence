#pragma once

#include "GUIObjectNode.h"
#include "InputManager.h"
#include "FontManager.h"
#include "GUIListBox.h"
#include "GUILabel.h"

#include <unordered_map>
#include <functional>

class DebugConsole : public GUIObjectNode
{
private:
	DebugConsole();

public:
	typedef std::function<bool(std::string)> DebugConsoleCallback;

	static DebugConsole* GetInstance() { static DebugConsole* INSTANCE = new DebugConsole; return INSTANCE; }

	void SetWindowDimensions(int width, int height);
	void SetFont(const Font* font) { m_Font = font; }
	void AddDebugCommand(std::string command, DebugConsoleCallback callback) { m_DebugConsoleCommands[command] = callback; }

	void EnterCommand(std::string& commandString);
	void AddDebugConsoleLine(std::string newLine) const;

	virtual void Input(int xOffset = 0, int yOffset = 0) override;
	virtual void TrueRender(int x = 0, int y = 0) override;

	inline GUIListBox* GetListbox() { return m_DebugConsoleListBox; }

private:
	int m_WindowWidth;
	int m_WindowHeight;
	const Font* m_Font;
	std::string m_Text;
	float m_LastBackspaceTime;
	GUIListBox* m_DebugConsoleListBox;

	const float TIME_BETWEEN_BACKSPACES = 0.1f;
	std::unordered_map<std::string, DebugConsoleCallback> m_DebugConsoleCommands;
};

DebugConsole::DebugConsole() :
	m_WindowWidth(1),
	m_WindowHeight(1),
	m_Font(nullptr),
	m_Text(""),
	m_LastBackspaceTime(0),
	m_DebugConsoleListBox(nullptr)
{
	SetZOrder(-9999);
	SetVisible(false);

	m_DebugConsoleListBox = GUIListBox::CreateListBox("DebugConsole", 4, 8, m_WindowWidth, m_WindowHeight, 22, 1);
	m_DebugConsoleListBox->SetSelectable(false);
	m_DebugConsoleListBox->SetFlowToBottom(true);
	AddChild(m_DebugConsoleListBox);
}

inline void DebugConsole::SetWindowDimensions(int width, int height)
{
	m_WindowWidth = width;
	m_WindowHeight = height;

	auto debugConsoleEntrySpacing = m_DebugConsoleListBox->GetEntryHeight() + m_DebugConsoleListBox->GetSpaceBetweenEntries();
	auto debugConsoleHeightSubtract = 0;
	auto debugConsoleHeight = ((height / 2) - debugConsoleEntrySpacing - debugConsoleHeightSubtract);
	if ((debugConsoleHeight % debugConsoleEntrySpacing) != 0)
		debugConsoleHeight = (debugConsoleHeight / debugConsoleEntrySpacing * debugConsoleEntrySpacing);

	m_DebugConsoleListBox->SetDimensions(width, debugConsoleHeight);
}

inline void DebugConsole::EnterCommand(std::string& commandString)
{
	auto soleCommand = false;
	auto firstSpace = commandString.find_first_of(' ');
	if (firstSpace == -1) { soleCommand = true; firstSpace = commandString.length(); }
	auto command = commandString.substr(0, firstSpace);

	auto debugCommand = m_DebugConsoleCommands.find(command);
	if (debugCommand == m_DebugConsoleCommands.end())
	{
		AddDebugConsoleLine("\"" + commandString + "\" is not a known command. Try again.");
		return;
	}

	commandString = soleCommand ? "" : commandString.substr(firstSpace + 1, commandString.length());
	/*auto returnVal = debugCommand->second(commandString);*/
	debugCommand->second(commandString);
}

inline void DebugConsole::AddDebugConsoleLine(std::string newLine) const
{
	auto newLabel = GUILabel::CreateLabel(fontManager.GetFont("Arial-12-White"), newLine.c_str(), 6, 0, m_WindowWidth - 16, 22);
	m_DebugConsoleListBox->AddItem(newLabel);
}



inline void DebugConsole::Input(int xOffset, int yOffset)
{
	if (inputManager.GetKeyState(SDL_SCANCODE_GRAVE) == 1)
	{
		SetVisible(!GetVisible());
		m_Text.clear();
		return;
	}

	if (m_SetToDestroy || !m_Visible) return;

	if (inputManager.GetBackspace() && !m_Text.empty() && (gameSecondsF - m_LastBackspaceTime >= TIME_BETWEEN_BACKSPACES))
	{
		m_Text.erase(--m_Text.end());
		m_LastBackspaceTime = gameSecondsF;
	}
	else if (inputManager.GetEnter() && !m_Text.empty())
	{
		EnterCommand(m_Text);
		m_Text.clear();
	}
	else m_Text += inputManager.GetKeyboardString();

	//  Take base node input
	GUIObjectNode::Input(xOffset, yOffset);
}

void DebugConsole::TrueRender(int x, int y)
{
	//  Render the untextured gray box
	glDisable(GL_TEXTURE_2D);
	glColor4f(0.1f, 0.1f, 0.1f, 0.9f);
	glBegin(GL_QUADS);
		glVertex2i(0, 0);
		glVertex2i(m_WindowWidth, 0);
		glVertex2i(m_WindowWidth, m_WindowHeight / 2);
		glVertex2i(0, m_WindowHeight / 2);
	glEnd();

	//  Render the basic line perimiter of the console
	glColor3i(0, 0, 0);
	glLineWidth(3);
	glBegin(GL_LINE_STRIP);
		glVertex2i(1, 1);
		glVertex2i(m_WindowWidth - 1, 1);
		glVertex2i(m_WindowWidth - 1, m_WindowHeight / 2 - 1);
		glVertex2i(1, m_WindowHeight / 2 - 1);
		glVertex2i(1, 1);
		glVertex2i(1, m_WindowHeight / 2 - 26 - 1);
		glVertex2i(m_WindowWidth - 1, m_WindowHeight / 2 - 26 - 1);
	glEnd();

	//  Render the text in the lower box
	if (m_Font != nullptr && !m_Text.empty())
	{
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		m_Font->RenderText(m_Text.c_str(), 10, m_WindowHeight / 2 - 21);
	}
}

//  Instance to be utilized by anyone including this header
auto debugConsole = DebugConsole::GetInstance();