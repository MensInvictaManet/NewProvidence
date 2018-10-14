#pragma once

#include "Server.h"
#include "Engine/GUIObjectNode.h"
#include "Engine/GUIListBox.h"

GUIListBox* CurrentUserList = nullptr;

void UpdateCurrentUserList(std::unordered_map<UserConnection*, bool>& userConnectionList)
{
	CurrentUserList->ClearItems();
	for (auto iter = userConnectionList.begin(); iter != userConnectionList.end(); ++iter)
	{
		auto user = (*iter).first;
		auto newUserEntry = GUIObjectNode::CreateObjectNode("");

		auto newUserNameLabel = GUILabel::CreateLabel(fontManager.GetFont("Arial"), "", 8, 8, 200, 24);
		newUserNameLabel->SetText(user->UserIdentifier);
		newUserEntry->AddChild(newUserNameLabel);

		//  Add the entry into the Current User List
		CurrentUserList->AddItem(newUserEntry);
	}
}

class PrimaryDialogue : public GUIObjectNode
{
public:
	PrimaryDialogue();
	~PrimaryDialogue() {}

private:
	void InitializeServer();
	void LoadMainProgramUI();

	void LoadCurrentUserListUI();

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

	//  Create the current user list
	LoadCurrentUserListUI();
	
	//  Set the Server control callbacks
	ServerControl.SetUserConnectionListChangedCallback(UpdateCurrentUserList);
}

void PrimaryDialogue::LoadCurrentUserListUI()
{
	//  Load the container
	const int currentUserListWidth = int(ScreenWidth) - 20;
	const int currentUserListHeight = int(ScreenHeight) - 20;
	auto currentUserListContainer = GUIObjectNode::CreateObjectNode("./Assets/Textures/Pixel_White.png");
	currentUserListContainer->SetColor(0.4f, 0.4f, 0.7f, 1.0f);
	currentUserListContainer->SetDimensions(currentUserListWidth, currentUserListHeight);
	currentUserListContainer->SetPosition(((int(ScreenWidth) - currentUserListWidth) / 2), ((int(ScreenHeight) - currentUserListHeight) / 2));
	MainProgramUINode->AddChild(currentUserListContainer);

	//  Load the current user list label
	auto currentUserListLabel = GUILabel::CreateLabel(fontManager.GetFont("Arial"), "Current user list:", 10, 4, currentUserListWidth, 24);
	currentUserListContainer->AddChild(currentUserListLabel);

	//  Create the current user listbox
	const int currentUserListButtonWidth = 16;
	const int currentUserListButtonHeight = 16;
	CurrentUserList = GUIListBox::CreateTemplatedListBox("Standard", 0, currentUserListLabel->GetHeight(), currentUserListWidth, currentUserListHeight - currentUserListLabel->GetHeight(), currentUserListWidth - currentUserListButtonWidth, currentUserListLabel->GetHeight(), currentUserListButtonWidth, currentUserListButtonWidth, currentUserListButtonHeight, currentUserListButtonHeight, currentUserListButtonWidth, currentUserListLabel->GetHeight(), 2);
	currentUserListContainer->AddChild(CurrentUserList);
}

void PrimaryDialogue::Update()
{
	GUIObjectNode::Update();

	ServerControl.MainProcess();
}