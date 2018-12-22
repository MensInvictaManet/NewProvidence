#pragma once

#include "Server.h"
#include "Engine/GUIObjectNode.h"
#include "Engine/GUIListBox.h"

Server ServerControl;
GUIListBox* HostedFileListBox = nullptr;
GUIListBox* CurrentUserList = nullptr;

void DeleteHostedFile(GUIObjectNode* fileDeleteButton)
{
	auto fileChecksum = ((GUIButton*)fileDeleteButton)->GetObjectName();
	ServerControl.DeleteHostedFile(fileChecksum);
}

void UpdateHostedFileList(const std::list<HostedFileData>& hostedFileDataList)
{
	//  Clear the hosted file list and rebuild it using the most recent data
	HostedFileListBox->ClearItems();
	for (auto iter = hostedFileDataList.begin(); iter != hostedFileDataList.end(); ++iter)
	{
		auto fileData = (*iter);
		auto newFileDataEntry = GUIObjectNode::CreateObjectNode("");

		//  Create the file identifier label
		auto fileIDLabel = GUILabel::CreateLabel("Arial", fileData.FileTitleChecksum.c_str(), 10, 8, 200, 24);
		newFileDataEntry->AddChild(fileIDLabel);

		//  Create the file size label
		auto megabytes = fileData.FileSize / 1000000;
		auto kilobytes = fileData.FileSize / 1000;
		auto sizeString = (megabytes > 0) ? (std::to_string(megabytes) + " MB") : ((kilobytes > 0) ? (std::to_string(kilobytes) + " KB") : (std::to_string(fileData.FileSize) + " Bytes"));
		auto fileSizeLabel = GUILabel::CreateLabel("Arial", sizeString.c_str(), 330, 8, 200, 24);
		newFileDataEntry->AddChild(fileSizeLabel);

		//  Decrypt the file title and set the file title label
		auto decryptedFileTitleVector = Groundfish::Decrypt(fileData.EncryptedFileTitle.data());
		auto fileTitle = std::string((char*)decryptedFileTitleVector.data(), decryptedFileTitleVector.size());
		auto fileTitleLimitedString = fileTitle.substr(0, std::min<int>(40, fileTitle.length()));
		auto fileTitleLabel = GUILabel::CreateLabel("Arial", fileTitleLimitedString.c_str(), 450, 8, 200, 24);
		newFileDataEntry->AddChild(fileTitleLabel);

		//  Create the delete button
		auto deleteButton = GUIButton::CreateTemplatedButton("Standard", 860, 6, 60, 20);
		deleteButton->SetColor(1.0f, 0.4f, 0.4f, 1.0f);
		deleteButton->SetFont("Arial");
		deleteButton->SetText("delete");
		deleteButton->SetObjectName(fileData.FileTitleChecksum);
		deleteButton->SetLeftClickCallback(DeleteHostedFile);
		newFileDataEntry->AddChild(deleteButton);

		//  Add the entry into the Current User List
		HostedFileListBox->AddItem(newFileDataEntry);
	}
}

void UpdateCurrentUserList(const std::unordered_map<UserConnection*, bool>& userConnectionList)
{
	//  Clear the current user list and rebuild it using the most recent data
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
		auto userStatusLabel = GUILabel::CreateLabel(fontManager.GetFont("Arial"), "", 360, 8, 200, 24);
		userStatusLabel->SetText(user->StatusString);
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

	void LoadHostedFileListUI();
	void LoadCurrentUserListUI();

	GUIObjectNode* MainProgramUINode = nullptr;

public:
	virtual void Update();
};


inline PrimaryDialogue::PrimaryDialogue()
{
	LoadMainProgramUI();
	InitializeServer();
}


void PrimaryDialogue::InitializeServer()
{
	assert(ServerControl.Initialize());

	UpdateHostedFileList(ServerControl.GetHostedFileDataList());
}


void PrimaryDialogue::LoadMainProgramUI()
{
	if (MainProgramUINode != nullptr) return;
	MainProgramUINode = GUIObjectNode::CreateObjectNode("");
	AddChild(MainProgramUINode);

	//  Create the hosted file list and current user list
	LoadHostedFileListUI();
	LoadCurrentUserListUI();

	//  Set the Server control callbacks
	ServerControl.SetHostedFileListChangedCallback(UpdateHostedFileList);
	ServerControl.SetUserConnectionListChangedCallback(UpdateCurrentUserList);
}


void PrimaryDialogue::LoadHostedFileListUI()
{
	//  Load the container
	const int listWidth = 940;
	const int listHeight = 500;
	auto hostedFileListContainer = GUIObjectNode::CreateObjectNode("./Assets/Textures/Pixel_White.png");
	hostedFileListContainer->SetColor(0.5f, 0.25f, 0.25f, 1.0f);
	hostedFileListContainer->SetDimensions(listWidth, listHeight);
	hostedFileListContainer->SetPosition(340, 5);
	MainProgramUINode->AddChild(hostedFileListContainer);

	//  Load the file ID column label
	auto fileIDLabel = GUILabel::CreateLabel(fontManager.GetFont("Arial"), "FILE ID:", 10, 4, listWidth, 24);
	hostedFileListContainer->AddChild(fileIDLabel);

	//  Load the file size column label
	auto fileSizeLabel = GUILabel::CreateLabel(fontManager.GetFont("Arial"), "FILE SIZE:", 330, 4, listWidth, 24);
	hostedFileListContainer->AddChild(fileSizeLabel);

	//  Load the file size column label
	auto fileTitleLabel = GUILabel::CreateLabel(fontManager.GetFont("Arial"), "FILE TITLE:", 450, 4, listWidth, 24);
	hostedFileListContainer->AddChild(fileTitleLabel);

	//  Create the hosted file listbox
	const int listboxButtonWidth = 12;
	const int listboxButtonHeight = 12;
	HostedFileListBox = GUIListBox::CreateTemplatedListBox("Standard", 0, fileIDLabel->GetHeight(), listWidth, listHeight - fileIDLabel->GetHeight(), listWidth - listboxButtonWidth - 2, 2, listboxButtonWidth, listboxButtonWidth, listboxButtonWidth, listboxButtonHeight, listboxButtonHeight, fileIDLabel->GetHeight(), 2);
	hostedFileListContainer->AddChild(HostedFileListBox);
}


void PrimaryDialogue::LoadCurrentUserListUI()
{
	//  Load the container
	const int listWidth = 940;
	const int listHeight = 500;
	auto currentUserListContainer = GUIObjectNode::CreateObjectNode("./Assets/Textures/Pixel_White.png");
	currentUserListContainer->SetColor(0.4f, 0.4f, 0.7f, 1.0f);
	currentUserListContainer->SetDimensions(listWidth, listHeight);
	currentUserListContainer->SetPosition(340, 515);
	MainProgramUINode->AddChild(currentUserListContainer);

	//  Load the current user id column label
	auto currentUserIDLabel = GUILabel::CreateLabel(fontManager.GetFont("Arial"), "USER ID:", 10, 4, listWidth, 24);
	currentUserListContainer->AddChild(currentUserIDLabel);

	//  Load the current user status column label
	auto currentUserStatusLabel = GUILabel::CreateLabel(fontManager.GetFont("Arial"), "STATUS:", 360, 4, listWidth, 24);
	currentUserListContainer->AddChild(currentUserStatusLabel);

	//  Create the current user listbox
	const int listboxButtonWidth = 12;
	const int listboxButtonHeight = 12;
	CurrentUserList = GUIListBox::CreateTemplatedListBox("Standard", 0, currentUserIDLabel->GetHeight(), listWidth, listHeight - currentUserIDLabel->GetHeight(), listWidth - listboxButtonWidth - 2, 2, listboxButtonWidth, listboxButtonWidth, listboxButtonHeight, listboxButtonHeight, listboxButtonWidth, currentUserIDLabel->GetHeight(), 2);
	currentUserListContainer->AddChild(CurrentUserList);
}


void PrimaryDialogue::Update()
{
	GUIObjectNode::Update();

	ServerControl.MainProcess();
}