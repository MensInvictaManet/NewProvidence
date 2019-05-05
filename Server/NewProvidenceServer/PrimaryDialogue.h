#pragma once

#include "Server.h"
#include "Engine/GUIObjectNode.h"
#include "Engine/GUIListBox.h"

Server ServerControl;
GUIListBox* HostedFileListBox = nullptr;
GUIListBox* CurrentUserList = nullptr;

void AddDebugCommand_AddUserData(void)
{
	//  AddUserData: ["AddUserData USER PASS"] adds a user to the server if they don't already exist 
	debugConsole->AddDebugCommand("AddUserData", [=](std::string commandString) -> bool
	{
		auto firstSpace = commandString.find_first_of(' ');
		auto lastSpace = commandString.find_last_of(' ');
		auto fullCommand = (lastSpace != (commandString.length() - 1));

		if ((firstSpace == -1) || (firstSpace != lastSpace) || !fullCommand)
		{
			debugConsole->AddDebugConsoleLine("Proper use of AddUserData command: \"AddUserData USER PASS\"");
			return false;
		}

		auto username = commandString.substr(0, firstSpace);
		auto password = commandString.substr(firstSpace + 1, commandString.length() - 1 - firstSpace);
		debugConsole->AddDebugConsoleLine("Adding user data: (" + username + ")[" + password + "]");

		ServerControl.AddUserLoginDetails(username, password);

		return true;
	});
}

void DeleteHostedFile(GUIObjectNode* fileDeleteButton)
{
	auto fileChecksum = ((GUIButton*)fileDeleteButton)->GetObjectName();
	ServerControl.DeleteHostedFile(fileChecksum);
}

void UpdateHostedFileList()
{
	std::list<HostedFileData> dataList;
	NPSQL::GetHostedFileList(dataList, 0, 100);
	dataList.sort(CompareUploadsByTimeAdded);

	//  Clear the hosted file list and rebuild it using the most recent data
	HostedFileListBox->ClearItems();
	for (HostedFileData fileData : dataList)
	{
		auto newFileDataEntry = GUIObjectNode::CreateObjectNode("");

		//  Create the file identifier label
		auto fileIDLabel = GUILabel::CreateLabel("Arial", fileData.FileTitleChecksum.c_str(), 10, 8, 200, 24);
		newFileDataEntry->AddChild(fileIDLabel);

		//  Create the file size label
		uint64_t gigabytes = fileData.FileSize / (1024 * 1024 * 1024);
		uint64_t megabytes = fileData.FileSize / (1024 * 1024);
		uint64_t kilobytes = fileData.FileSize / (1024);
		auto sizeString = (gigabytes > 9) ? (std::to_string(gigabytes) + "GB") : ((megabytes > 0) ? (std::to_string(megabytes) + " MB") : ((kilobytes > 0) ? (std::to_string(kilobytes) + " KB") : (std::to_string(fileData.FileSize) + " Bytes")));
		auto fileSizeLabel = GUILabel::CreateLabel("Arial", sizeString.c_str(), 330, 8, 200, 24);
		newFileDataEntry->AddChild(fileSizeLabel);

		auto fileType = fileData.FileType;
		auto fileTypeImage = GetFileTypeIconFromID(fileType);
		fileTypeImage->SetDimensions(20, 20);
		fileTypeImage->SetPosition(450, 8);
		newFileDataEntry->AddChild(fileTypeImage);

		auto fileSubType = fileData.FileSubType;
		auto fileSubTypeImage = GetFileSubTypeIconFromID(fileSubType);
		fileSubTypeImage->SetDimensions(20, 20);
		fileSubTypeImage->SetPosition(470, 8);
		newFileDataEntry->AddChild(fileSubTypeImage);

		//  Decrypt the file title and set the file title label
		auto fileTitle = Groundfish::DecryptToString(fileData.EncryptedFileTitle.data());
		auto fileTitleLimitedString = fileTitle.substr(0, std::min<int>(40, fileTitle.length()));
		auto fileTitleLabel = GUILabel::CreateLabel("Arial", fileTitleLimitedString.c_str(), 520, 8, 200, 24);
		newFileDataEntry->AddChild(fileTitleLabel);

		//  Create the delete button
		auto deleteButton = GUIButton::CreateTemplatedButton("Standard", 940, 6, 60, 20);
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

void UpdateCurrentUserList(std::unordered_map<UserConnection*, bool> userConnectionList)
{
	//  Clear out any users no longer connected, and update existing users from the new data
	auto currentUserList = CurrentUserList->GetItemList();
	for (auto iter = currentUserList.begin(); iter != currentUserList.end();)
	{
		auto user = ServerControl.GetUserConnectionByIP((*iter)->GetObjectName());

		//  If the user no longer exists in the current connected user list, delete the iterator and continue
		if (user == nullptr)
		{
			CurrentUserList->RemoveItem((*iter));
			iter = currentUserList.erase(iter);
			continue;
		}

		//  Create the user identifier label
		auto userIDLabel = static_cast<GUILabel*>((*iter)->GetChildByName("UserIDLabel"));
		assert(userIDLabel != nullptr);
		userIDLabel->SetText(user->UserIdentifier);

		//  Create the user identifier label
		auto userStatusLabel = static_cast<GUILabel*>((*iter)->GetChildByName("UserStatusLabel"));
		assert(userStatusLabel != nullptr);
		userStatusLabel->SetText(user->StatusString);

		auto userIter = userConnectionList.find(user);
		assert(userIter != userConnectionList.end());
		userConnectionList.erase(userIter);

		iter++;
	}

	//  If there are any new user connections, they should still be in the passed list. Add entries for each new user
	for (auto iter = userConnectionList.begin(); iter != userConnectionList.end(); ++iter)
	{
		auto user = (*iter).first;
		auto newUserEntry = GUIObjectNode::CreateObjectNode("");
		newUserEntry->SetObjectName((*iter).first->IPAddress);
		
		//  Create the user identifier label
		auto userIDLabel = GUILabel::CreateLabel(fontManager.GetFont("Arial"), "", 10, 8, 200, 24);
		userIDLabel->SetObjectName("UserIDLabel");
		userIDLabel->SetText(user->UserIdentifier);
		newUserEntry->AddChild(userIDLabel);

		//  Create the user identifier label
		auto userStatusLabel = GUILabel::CreateLabel(fontManager.GetFont("Arial"), "", 360, 8, 200, 24);
		userStatusLabel->SetObjectName("UserStatusLabel");
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

	AddDebugCommand_AddUserData();
}


void PrimaryDialogue::InitializeServer()
{
	assert(ServerControl.Initialize());

	UpdateHostedFileList();
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
	const int listWidth = 1020;
	const int listHeight = 500;
	auto hostedFileListContainer = GUIObjectNode::CreateObjectNode("./Assets/Textures/Pixel_White.png");
	hostedFileListContainer->SetColor(0.5f, 0.25f, 0.25f, 1.0f);
	hostedFileListContainer->SetDimensions(listWidth, listHeight);
	hostedFileListContainer->SetPosition(260, 5);
	MainProgramUINode->AddChild(hostedFileListContainer);

	//  Load the file ID column label
	auto fileIDLabel = GUILabel::CreateLabel(fontManager.GetFont("Arial"), "FILE ID:", 10, 4, listWidth, 24);
	hostedFileListContainer->AddChild(fileIDLabel);

	//  Load the file size column label
	auto fileSizeLabel = GUILabel::CreateLabel(fontManager.GetFont("Arial"), "FILE SIZE:", 330, 4, listWidth, 24);
	hostedFileListContainer->AddChild(fileSizeLabel);

	//  Load the file type column label
	auto fileTypeLabel = GUILabel::CreateLabel(fontManager.GetFont("Arial"), "TYPE:", 450, 4, listWidth, 24);
	hostedFileListContainer->AddChild(fileTypeLabel);

	//  Load the file title column label
	auto fileTitleLabel = GUILabel::CreateLabel(fontManager.GetFont("Arial"), "TITLE:", 520, 4, listWidth, 24);
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