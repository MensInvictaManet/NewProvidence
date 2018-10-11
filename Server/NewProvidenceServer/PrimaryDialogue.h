#pragma once

#include "Server.h"
#include "Engine/GUIObjectNode.h"

class PrimaryDialogue : public GUIObjectNode
{
public:
	PrimaryDialogue();
	~PrimaryDialogue() {}

private:
	void InitializeServer();
	void LoadMainProgramUI();

	GUIObjectNode* MainProgramUINode = nullptr;

public:
	virtual void Update();

	Server ServerControl;
};

inline PrimaryDialogue::PrimaryDialogue()
{
	LoadMainProgramUI();
	InitializeServer();
}

void PrimaryDialogue::InitializeServer()
{
	assert(ServerControl.Initialize());
}

void PrimaryDialogue::LoadMainProgramUI()
{
	if (MainProgramUINode != nullptr) return;
	MainProgramUINode = GUIObjectNode::CreateObjectNode("");
	AddChild(MainProgramUINode);

	//  TODO: Add connected user list (with IP and User Identifier (no name))
}

void PrimaryDialogue::Update()
{
	ServerControl.MainProcess();
}