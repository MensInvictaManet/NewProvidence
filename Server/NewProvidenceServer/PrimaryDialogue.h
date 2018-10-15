#pragma once

#include "Server.h"
#include "Engine/GUIObjectNode.h"
#include "Engine/GUIListBox.h"

GUIListBox* CurrentUserList = nullptr;

void UpdateCurrentUserList(std::unordered_map<UserConnection*, bool>& userConnectionList)
{
	//  Clear the current user list and rebuilt is using the most recent data
	CurrentUserList->ClearItems();
	for (auto iter = userConnectionList.begin(); iter != userConnectionList.end(); ++iter)
	{
		auto user = (*iter).first;
		auto newUserEntry = GUIObjectNode::CreateObjectNode("");
		
		//  Create the user identifier label
		auto userIDLabel = GUILabel::CreateLabel(fontManager.GetFont("Arial"), "", 10, 8, 200, 24);
		userIDLabel->SetText(user->UserIdentifier);
		newUserEntry->AddChild(userIDLabel);

		//  Create the user identifier label
		auto userNameLabel = GUILabel::CreateLabel(fontManager.GetFont("Arial"), "", 360, 8, 200, 24);
		userNameLabel->SetText(user->Username);
		newUserEntry->AddChild(userNameLabel);

		//  Create the user identifier label
		auto userStatusLabel = GUILabel::CreateLabel(fontManager.GetFont("Arial"), "", 620, 8, 200, 24);
		userStatusLabel->SetText(user->CurrentStatus);
		newUserEntry->AddChild(userStatusLabel);

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

	//  Load the current user id column label
	auto currentUserIDLabel = GUILabel::CreateLabel(fontManager.GetFont("Arial"), "USER ID:", 10, 4, currentUserListWidth, 24);
	currentUserListContainer->AddChild(currentUserIDLabel);

	//  Load the current user name column label
	auto currentUserNameLabel = GUILabel::CreateLabel(fontManager.GetFont("Arial"), "USERNAME:", 360, 4, currentUserListWidth, 24);
	currentUserListContainer->AddChild(currentUserNameLabel);

	//  Load the current user status column label
	auto currentUserStatusLabel = GUILabel::CreateLabel(fontManager.GetFont("Arial"), "STATUS:", 620, 4, currentUserListWidth, 24);
	currentUserListContainer->AddChild(currentUserStatusLabel);

	//  Create the current user listbox
	const int currentUserListButtonWidth = 16;
	const int currentUserListButtonHeight = 16;
	CurrentUserList = GUIListBox::CreateTemplatedListBox("Standard", 0, currentUserIDLabel->GetHeight(), currentUserListWidth, currentUserListHeight - currentUserIDLabel->GetHeight(), currentUserListWidth - currentUserListButtonWidth, currentUserIDLabel->GetHeight(), currentUserListButtonWidth, currentUserListButtonWidth, currentUserListButtonHeight, currentUserListButtonHeight, currentUserListButtonWidth, currentUserIDLabel->GetHeight(), 2);
	currentUserListContainer->AddChild(CurrentUserList);
}

void PrimaryDialogue::Update()
{
	GUIObjectNode::Update();

	ServerControl.MainProcess();
}