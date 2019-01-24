#pragma once

#include "Engine/ArcadiaEngine.h"

#include "Engine/GLMCamera.h"
#include "Engine/Shader.h"
#include "Engine/Program.h"

#include "Engine/GUIMoveable.h"
#include "Engine/GUIDropDown.h"
#include "Engine/GUILabel.h"
#include "Engine/GUIButton.h"

//  Dialogues
#include "PrimaryDialogue.h"

GUIObjectNode* currentDialogue;

void CreateInitializationData()
{
	//  Load the default fonts
	fontManager.SetFontFolder("Assets/Fonts/");
	fontManager.LoadFont("Arial");
	fontManager.LoadFont("Arial-12-White");

	//  Set the font and template on the Debug Console
	debugConsole->SetFont(fontManager.GetFont("Arial-12-White"));
	debugConsole->GetListbox()->SetTemplate("DebugConsole");
	debugConsole->GetListbox()->SetTemplateData(int(ScreenWidth) - 20, 0, 12, 12, 12, 12, 12);

	//  Create the first test dialogue and add it to the scene
	currentDialogue = new PrimaryDialogue;
	guiManager.GetBaseNode()->AddChild(currentDialogue);

	SetEscapeToQuit(false);
}

void AppMain()
{
	SetBackgroundColor(0.0f, 0.04f, 0.08f);
	if (!InitializeEngine("New Providence", 1280.0f, 720.0f))
	{
		ShutdownEngine();
		throw std::runtime_error("InitializeEngine failed");
	}

	//  Create data for different systems to start the main program functionality
	CreateInitializationData();

	//  Load the Primary Loop
	PrimaryLoop();

	//  Free resources and close SDL before exiting
	ShutdownEngine();
}