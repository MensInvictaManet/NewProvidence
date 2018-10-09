#pragma once

#include "Server.h"

class PrimaryDialogue : public GUIObjectNode
{
public:
	PrimaryDialogue();
	~PrimaryDialogue() {}

private:
	void InitializeServer();

public:
	virtual void Update();

	Server ServerControl;
};

inline PrimaryDialogue::PrimaryDialogue()
{
	InitializeServer();
}

void PrimaryDialogue::InitializeServer()
{
	assert(ServerControl.Initialize());
}

void PrimaryDialogue::Update()
{
	ServerControl.MainProcess();
}