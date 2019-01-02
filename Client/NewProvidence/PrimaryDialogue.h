#pragma once

#include "Client.h"
#include "Engine/GUIObjectNode.h"
#include "Engine/GUIEditBox.h"
#include "Engine/GUILabel.h"
#include "Engine/GUIListBox.h"
#include "Engine/FontManager.h"
#include "Engine/TimeSlice.h"
#include "HostedFileData.h"
#include <math.h>

const auto MainMenuBarHeight = 40;
const auto LatestUploadsWidth = 600;
const auto LatestUploadsHeight = 580;

int CurrentLatestUploadsStartingIndex = 0;

//  Global UI objects
GUIObjectNode* StatusBarBG = nullptr;
GUILabel* StatusBarTextLabel = nullptr;
GUIObjectNode* StatusBarTransferFill = nullptr;

GUIObjectNode* LoginMenuNode = nullptr;
GUIEditBox* UsernameEditBox = nullptr;
GUIEditBox* PasswordEditBox = nullptr;

GUIObjectNode* MainMenuBarUINode = nullptr;
GUIObjectNode* LatestUploadsUINode = nullptr;
GUIObjectNode* UploadMenuUINode = nullptr;

GUIButton* UserSideTabButton = nullptr;
GUIObjectNode* UserIconNode = nullptr;
GUIButton* InboxSideTabButton = nullptr;
GUIObjectNode* InboxIconNode = nullptr;
GUIButton* NotificationsSideTabButton = nullptr;
GUIObjectNode* NotificationsIconNode = nullptr;
GUIButton* UploadSideTabButton = nullptr;
GUIObjectNode* UploadIconNode = nullptr;

GUIListBox* UploadFolderItemsListBox = nullptr;
GUILabel* UploadFileNameLabel = nullptr;
GUIEditBox* UploadFileTitleEditBox = nullptr;
GUIDropDown* UploadFileTypeDropDown = nullptr;
GUIDropDown* UploadFileSubTypeDropDown = nullptr;
std::string SelectedUploadFileName = "";

GUILabel* LatestUploadsTitleLabel = nullptr;
GUIListBox* LatestUploadsListBox = nullptr;
GUIObjectNode* CurrentTransferContainer = nullptr;

Client ClientControl;

void UpdateUploadFolderList(void)
{
	if (UploadFolderItemsListBox == nullptr) return;
	UploadFolderItemsListBox->ClearItems();

	std::string uploadFolder("_FilesToUpload");
	std::vector<std::string> fileList;
	ClientControl.DetectFilesInUploadFolder(uploadFolder, fileList);

	auto entryX = UploadFolderItemsListBox->GetWidth() / 2;
	auto entryY = 12;
	auto entryH = UploadFolderItemsListBox->GetEntryHeight();
	auto entryW = UploadFolderItemsListBox->GetWidth();

	for (auto iter = fileList.begin(); iter != fileList.end(); ++iter)
	{
		auto shortName = (*iter).substr(uploadFolder.length() + 1, (*iter).length() - uploadFolder.length() - 1);
		auto itemLabel = GUILabel::CreateLabel("Arial", shortName.c_str(), entryX, entryY, entryW, entryH, GUILabel::JUSTIFY_CENTER);
		itemLabel->SetObjectName(shortName);
		UploadFolderItemsListBox->AddItem(itemLabel);
	}
}

void SelectUploadItem(GUIObjectNode* object)
{
	if (object == nullptr) return;
	auto listBox = (GUIListBox*)(object);
	auto selectedItem = listBox->GetSelectedItem();
	SelectedUploadFileName = selectedItem->GetObjectName();
	UploadFileNameLabel->SetText("File Name:       " + SelectedUploadFileName);
	UploadFileTitleEditBox->SetText("");
}


void SetStatusBarMessage(std::string statusBarMessage, bool error = false)
{
	if (StatusBarTextLabel == nullptr) return;

	StatusBarTextLabel->SetText(statusBarMessage);

	if (error) StatusBarBG->SetColor(0.8f, 0.1f, 0.1f, 1.0f);
	else StatusBarBG->SetColor(0.1f, 0.4f, 0.7f, 1.0f);
}

void UploadFileToServer(GUIObjectNode* object)
{
	//  If there is no currently selected file to upload, return out
	if (SelectedUploadFileName.length() == 0) return;
	
	//  If the currently selected file does not exist or is not readable, return out
	auto fileToUpload = "_FilesToUpload/" + SelectedUploadFileName;
	std::ifstream fileCheck(fileToUpload);
	auto fileGood = fileCheck.good() && !fileCheck.bad();
	fileCheck.close();
	if (!fileGood)
	{
		SetStatusBarMessage("Upload Attempt Failed! File selected does not exist. Did you recently delete it?", true);
		return;
	}

	//  If there is no access to the given file title, or the title is blank, return out
	if (UploadFileTitleEditBox == nullptr) return;
	auto fileToUploadTitle = UploadFileTitleEditBox->GetText();
	if (fileToUploadTitle.length() == 0) return;
	if (fileToUploadTitle.length() > UPLOAD_TITLE_MAX_LENGTH)
	{
		SetStatusBarMessage("Upload Attempt Failed! File title can not be longer than " + std::to_string(UPLOAD_TITLE_MAX_LENGTH) + " characters. Try again...", true);
		return;
	}

	//  Get the file type and sub-type, ensuring they match
	auto fileTypeString = ((GUILabel*)(UploadFileTypeDropDown->GetSelectedItem()))->GetText();
	auto fileSubTypeString = ((GUILabel*)(UploadFileSubTypeDropDown->GetSelectedItem()))->GetText();
	int fileTypeID = GetFileTypeIDFromName(fileTypeString);
	int fileSubTypeID = GetFileSubTypeIDFromName(fileSubTypeString);

	//  Attempt to start an upload of the file to the server
	ClientControl.SendFileToServer(SelectedUploadFileName, fileToUpload, fileToUploadTitle, fileTypeID, fileSubTypeID);
}

void RequestLatestUploadFile(GUIObjectNode* node)
{
	auto entry = LatestUploadsListBox->GetSelectedItem();
	SendMessage_FileRequest(entry->GetObjectName(), ClientControl.GetServerSocket(), NEW_PROVIDENCE_IP);
}


void UpdateLatestUploadsListBoxDownloadButtons(GUIObjectNode* object)
{
	if (object == nullptr) return;
	auto listBox = (GUIListBox*)(object);
	auto listBoxItems = listBox->GetItemList();
	auto selectedItem = listBox->GetSelectedItem();

	for (auto iter = listBoxItems.begin(); iter != listBoxItems.end(); ++iter)
	{
		auto downloadButton = (*iter)->GetChildByName("Download Button");
		if (downloadButton == nullptr) continue;
		auto visible = ((*iter) == selectedItem);
		downloadButton->SetVisible(visible);
	}
}


void AddLatestUploadEntry(HostedFileEntry fileEntryData)
{
	auto entry = GUIObjectNode::CreateObjectNode("");
	entry->SetObjectName(fileEntryData.FileTitle);

	auto fileTypeImage = GetFileTypeIconFromID(fileEntryData.FileType);
	fileTypeImage->SetDimensions(20, 20);
	fileTypeImage->SetColorBytes(64, 64, 128, 255);
	fileTypeImage->SetPosition(6, 8);
	entry->AddChild(fileTypeImage);

	auto fileSubTypeImage = GetFileSubTypeIconFromID(fileEntryData.FileSubType);
	fileSubTypeImage->SetDimensions(20, 20);
	fileSubTypeImage->SetColorBytes(255, 255, 255, 255);
	fileSubTypeImage->SetPosition(26, 8);
	entry->AddChild(fileSubTypeImage);

	auto label = GUILabel::CreateLabel(fontManager.GetFont("Arial"), fileEntryData.FileTitle.c_str(), 56, 9, 100, 20);
	entry->AddChild(label);

	auto button = GUIButton::CreateTemplatedButton("Standard", LatestUploadsWidth - 100, 6, 80, 20);
	button->SetFont("Arial");
	button->SetText("Download");
	button->SetObjectName("Download Button");
	button->SetLeftClickCallback(RequestLatestUploadFile);
	entry->AddChild(button);

	LatestUploadsListBox->AddItem(entry);
}


void SetLatestUploads(std::vector<HostedFileEntry> latestUploadsList)
{
	if (LatestUploadsListBox == nullptr) return;
	LatestUploadsListBox->ClearItems();

	for (auto iter = latestUploadsList.begin(); iter != latestUploadsList.end(); ++iter)
		AddLatestUploadEntry((*iter));

	UpdateLatestUploadsListBoxDownloadButtons(LatestUploadsListBox);
}


void SetUserMenuOpen(GUIObjectNode* node)
{
	//SideBarOpen = !SideBarOpen;

	//SideBarBox->SetVisible(SideBarOpen);
	// TODO 
	//UserSideTabButton->SetX(SideBarOpen ? SideBarWidth : 0);

	//InboxSideTabButton->SetVisible(!SideBarOpen);
	//NotificationsSideTabButton->SetVisible(!SideBarOpen);

	//  TODO: Show User Menu in SideBar
}


void SetInboxOpen(GUIObjectNode* button)
{
	//SideBarOpen = !SideBarOpen;

	//SideBarBox->SetVisible(SideBarOpen);
	//  TODO
	//InboxSideTabButton->SetX(SideBarOpen ? SideBarWidth : 0);

	//UserSideTabButton->SetVisible(!SideBarOpen);
	//NotificationsSideTabButton->SetVisible(!SideBarOpen);

	//  TODO: Show Inbox Menu in SideBar
}


void SetInboxMessageCount(int messageCount)
{
	if (InboxIconNode == nullptr) return;

	auto empty = (messageCount <= 0);
	InboxIconNode->SetTextureID(textureManager.LoadTextureGetID(empty ? "./Assets/Textures/MainProgramUI/InboxEmpty.png" : "./Assets/Textures/MainProgramUI/InboxMailReceived.png"));
	if (empty)		InboxIconNode->SetColor(0.6f, 0.6f, 0.6f, 1.0f);
	else			InboxIconNode->SetColor(1.0f, 0.2f, 0.2f, 1.0f);
}


void SetNotificationsOpen(GUIObjectNode* button)
{
	//SideBarOpen = !SideBarOpen;

	//SideBarBox->SetVisible(SideBarOpen);
	//  TODO
	//NotificationsSideTabButton->SetX(SideBarOpen ? SideBarWidth : 0);

	//UserSideTabButton->SetVisible(!SideBarOpen);
	//InboxSideTabButton->SetVisible(!SideBarOpen);

	//  TODO: Show Notifications Menu in SideBar
}


void SetNotificationCount(int notificationCount)
{
	if (NotificationsSideTabButton == nullptr) return;

	auto empty = (notificationCount <= 0);
	NotificationsIconNode->SetTextureID(textureManager.LoadTextureGetID(empty ? "./Assets/Textures/MainProgramUI/NotificationsEmpty.png" : "./Assets/Textures/MainProgramUI/NotificationsReceived.png"));
	if (empty)		NotificationsIconNode->SetColor(0.6f, 0.6f, 0.6f, 1.0f);
	else			NotificationsIconNode->SetColor(1.0f, 0.2f, 0.2f, 1.0f);
}


void SetHomeMenuOpen(GUIObjectNode* button)
{

	//  Make the latest uploads UI visible
	LatestUploadsUINode->SetVisible(true);

	//  Make the sure base UI is set for logged in users (login screen off, main menu on)
	LoginMenuNode->SetVisible(false);
	MainMenuBarUINode->SetVisible(true);

	//  Make the upload UI invisible
	UploadMenuUINode->SetVisible(false);
}


void SetBrowseMenuOpen(GUIObjectNode* button)
{
	//  TODO: Add the browse menu UI and trigger it here
}


void SetUploadMenuOpen(GUIObjectNode* button)
{
	//  Make the upload UI visible
	UploadMenuUINode->SetVisible(true);

	//  Make the sure base UI is set for logged in users (login screen off, main menu on)
	LoginMenuNode->SetVisible(false);
	MainMenuBarUINode->SetVisible(true);

	//  Make the latest uploads UI invisible
	LatestUploadsUINode->SetVisible(false);

	//  Run a new detection of items in the uploads folder
	UpdateUploadFolderList();
}


void ShiftLatestUploadsLeft(GUIObjectNode* object)
{
	if (CurrentLatestUploadsStartingIndex < 20) return;
	CurrentLatestUploadsStartingIndex -= 20;
	SendMessage_RequestLatestUploads(CurrentLatestUploadsStartingIndex, ClientControl.GetServerSocket());

	auto rangeString = std::to_string(CurrentLatestUploadsStartingIndex) + " to " + std::to_string(CurrentLatestUploadsStartingIndex + 20);
	LatestUploadsTitleLabel->SetText("Latest Uploaded Files (" + rangeString + "):");
}


void ShiftLatestUploadsRight(GUIObjectNode* object)
{
	if (CurrentLatestUploadsStartingIndex > 50000) return;
	CurrentLatestUploadsStartingIndex += 20;
	SendMessage_RequestLatestUploads(CurrentLatestUploadsStartingIndex, ClientControl.GetServerSocket());

	auto rangeString = std::to_string(CurrentLatestUploadsStartingIndex) + " to " + std::to_string(CurrentLatestUploadsStartingIndex + 20);
	LatestUploadsTitleLabel->SetText("Latest Uploaded Files (" + rangeString + "):");
}


void SetTransferPercentage(double percent, double time, uint64_t fileSize, uint64_t timeRemaining, bool download)
{
	int averageKBs = int((double(fileSize) / 1024.0) / time);

	std::string transferType = (download ? "Download" : "Upload");

	if (percent >= 1.0f)
	{
		StatusBarTransferFill->SetVisible(false);
		SetStatusBarMessage(transferType + " completed in " + std::to_string(int(time)) + " seconds (avg " + std::to_string(averageKBs) + " KB/s)", false);
	}
	else
	{
		auto minutesRemaining = timeRemaining / 60;
		auto secondsRemaining = timeRemaining % 60;
		auto timeString = (minutesRemaining != 0) ? (std::to_string(minutesRemaining) + " minutes") : std::to_string(secondsRemaining) + " seconds";

		StatusBarTransferFill->SetVisible(true);
		StatusBarTransferFill->SetWidth(int(float(percent) * ScreenWidth));
		SetStatusBarMessage(transferType + " " + std::to_string(int(float(percent) * 100.0f)) + "% complete (est. " + timeString + " remaining)", false);
	}
}


void LoginRequestResponseCallback(int response, int inboxCount, int notificationCount)
{
	bool success = (response == LOGIN_RESPONSE_SUCCESS);
	LoginMenuNode->SetVisible(!success);
	MainMenuBarUINode->SetVisible(success);
	LatestUploadsUINode->SetVisible(success);
	UploadMenuUINode->SetVisible(false);

	UpdateUploadFolderList();

	PasswordEditBox->SetText("");
	SetStatusBarMessage(LoginResponses[response], !success);
	SetInboxMessageCount(inboxCount);
	SetNotificationCount(notificationCount);
}

void FileRequestSucceeded(std::string fileID)
{
	//  Shorten any null values at the end of the string (which can come from decryption)
	while (fileID.size() > 0 && fileID[fileID.size() - 1] == 0) fileID = fileID.substr(0, fileID.size() - 1);
	
	//  Generate the full string and set the status bar message
	auto fullString = "File download request success [" + fileID + "]:  Downloading now...";
	SetStatusBarMessage(fullString, false);
}

void FileRequestFailureCallback(std::string fileID, std::string failureReason)
{
	//  Shorten any null values at the end of the string (which can come from decryption)
	while (fileID.size() > 0 && fileID[fileID.size() - 1] == 0) fileID = fileID.substr(0, fileID.size() - 1);

	//  Generate the full string and set the status bar message
	auto fullString = "File download request failed [" + fileID + "]: " + failureReason;
	SetStatusBarMessage(fullString, true);
}

void InboxAndNotificationsCountCallback(int inboxCount, int notificationCount)
{
	SetInboxMessageCount(inboxCount);
	SetNotificationCount(notificationCount);
}

void LoginButtonLeftClickCallback(GUIObjectNode* button)
{
	//  Encrypt the username and password
	std::vector<unsigned char> encryptedUsernameVector = Groundfish::Encrypt(UsernameEditBox->GetText().c_str(), int(UsernameEditBox->GetText().length()), rand() % 256);
	std::vector<unsigned char> encryptedPasswordVector = Groundfish::Encrypt(PasswordEditBox->GetText().c_str(), int(PasswordEditBox->GetText().length()), rand() % 256);

	SendMessage_UserLoginRequest(encryptedUsernameVector, encryptedPasswordVector, ClientControl.GetServerSocket());
}

void TabFromUsernameBoxCallback(GUIObjectNode* node)
{
	if (UsernameEditBox->GetSelected() == false) return;
	UsernameEditBox->SetSelected(false);
	PasswordEditBox->SetSelected(true);
}

void TabFromPasswordBoxCallback(GUIObjectNode* node)
{
	if (PasswordEditBox->GetSelected() == false) return;
	PasswordEditBox->SetSelected(false);
	UsernameEditBox->SetSelected(true);
}

void EnterFromUsernameOrPasswordBoxCallback(GUIObjectNode* node)
{
	if (UsernameEditBox->GetText().length() == 0) return;
	if (PasswordEditBox->GetText().length() == 0) return;

	LoginButtonLeftClickCallback(nullptr);
}


class PrimaryDialogue : public GUIObjectNode
{
public:
	PrimaryDialogue();
	~PrimaryDialogue();

	virtual void Update();
	void Shutdown();

private:
	void InitializeClient();
	void LoadStatusBar();
	void LoadLoginMenu();
	void LoadMainMenuBarUI();
	void LoadSideBarUI();
	void LoadLatestUploadsUI();
	void LoadUploadMenuUI();

	void UpdateUI();

	//  Status Bar objects
	GUIObjectNode* StatusBarNode	= nullptr;

	//  Login Menu objects
	GUIButton* LoginButton			= nullptr;
	GUIObjectNode* ConnectedIcon	= nullptr;

	enum ConnectionStatus { CONNECTION_STATUS_NO_CONNECTION, CONNECTION_STATUS_CANNOT_CONNECT, CONNECTION_STATUS_CONNECTED };
	ConnectionStatus ClientConnected = CONNECTION_STATUS_NO_CONNECTION;
};


PrimaryDialogue::PrimaryDialogue()
{
	LoadStatusBar();
	LoadLoginMenu();
	LoadMainMenuBarUI();
	InitializeClient();
}


PrimaryDialogue::~PrimaryDialogue()
{
	Shutdown();
}


inline void PrimaryDialogue::Update()
{
	GUIObjectNode::Update();

	if (ClientConnected == CONNECTION_STATUS_CONNECTED) ClientControl.MainProcess();

	UpdateUI();
}


inline void PrimaryDialogue::Shutdown()
{
	//  Shut down the client
	ClientControl.Shutdown();

	//  Shut down the winsock wrapper
	winsockWrapper.WinsockShutdown();
}


void PrimaryDialogue::InitializeClient()
{
	//  Initialize the client controller, then start a task to connect (asynchronous, so we can wait for the connection to say it's valid)
	ClientControl.Initialize();
	ClientConnected = (ClientControl.Connect() ? CONNECTION_STATUS_CONNECTED : CONNECTION_STATUS_CANNOT_CONNECT);

	//  Show a status bar message to tell the user of their connection status
	auto connectionError = (ClientConnected != CONNECTION_STATUS_CONNECTED);
	SetStatusBarMessage(connectionError ? "Failed to connect to server. Contact administrator." : "Successfully connected to server!", connectionError);
	ConnectedIcon->SetVisible(!connectionError);
}


void PrimaryDialogue::LoadStatusBar()
{
	if (StatusBarNode != nullptr) return;
	StatusBarNode = GUIObjectNode::CreateObjectNode("");
	AddChild(StatusBarNode);

	//  Load the background strip behind the status bar
	auto backgroundStripHeight = 24;
	auto backgroundStripY = (int(ScreenHeight) - backgroundStripHeight);
	StatusBarBG = GUIObjectNode::CreateObjectNode("./Assets/Textures/Pixel_TransparentGray.png");
	StatusBarBG->SetColor(0.1f, 0.4f, 0.7f, 1.0f);
	StatusBarBG->SetDimensions(int(ScreenWidth), backgroundStripHeight);
	StatusBarBG->SetPosition(0, backgroundStripY);
	StatusBarNode->AddChild(StatusBarBG);

	//  Load the status bar transfer fill, for showing download progress
	StatusBarTransferFill = GUIObjectNode::CreateObjectNode("./Assets/Textures/Pixel_TransparentDarkRed.png");
	StatusBarTransferFill->SetObjectName("StatusBarTransferFill");
	StatusBarTransferFill->SetDimensions(0, backgroundStripHeight);
	StatusBarTransferFill->SetPosition(0, backgroundStripY);
	StatusBarNode->AddChild(StatusBarTransferFill);

	//  Load the status bar text label
	StatusBarTextLabel = GUILabel::CreateLabel(fontManager.GetFont("Arial-12-White"), "Test", -100, 0, int(ScreenWidth), backgroundStripHeight);
	StatusBarTextLabel->SetY(backgroundStripY + 5);
	StatusBarTextLabel->SetJustification(GUILabel::JUSTIFY_CENTER);
	StatusBarNode->AddChild(StatusBarTextLabel);
}


void PrimaryDialogue::LoadLoginMenu()
{
	if (LoginMenuNode != nullptr) return;
	LoginMenuNode = GUIObjectNode::CreateObjectNode("");
	AddChild(LoginMenuNode);

	//  Load the background strip behind the login screen
	auto backgroundStripHeight = 200;
	auto backgroundStrip = GUIObjectNode::CreateObjectNode("./Assets/Textures/Pixel_TransparentGray.png");
	backgroundStrip->SetDimensions(int(ScreenWidth), backgroundStripHeight);
	backgroundStrip->SetPosition(0, (int(ScreenHeight) / 2) - (backgroundStrip->GetHeight() / 2));
	LoginMenuNode->AddChild(backgroundStrip);

	//  Load the login screen primary logo
	auto mainLogo = GUIObjectNode::CreateObjectNode("./Assets/Textures/LoginMenu/PrimaryLogo.png");
	mainLogo->SetDimensions(148, 148);
	mainLogo->SetPosition(220, (int(ScreenHeight) / 2) - (mainLogo->GetHeight() / 2));
	LoginMenuNode->AddChild(mainLogo);

	//  Load the login screen primary logo title
	auto mainLogoTitle = GUIObjectNode::CreateObjectNode("./Assets/Textures/LoginMenu/PrimaryLogoTitle.png");
	mainLogoTitle->SetDimensions(512, 64);
	mainLogoTitle->SetPosition((int(ScreenWidth) / 2) - (mainLogoTitle->GetWidth() / 2), (int(ScreenHeight) / 2) - (mainLogoTitle->GetHeight() / 2) - 60);
	LoginMenuNode->AddChild(mainLogoTitle);

	//  Load the login screen username icon
	auto usernameIcon = GUIObjectNode::CreateObjectNode("./Assets/Textures/LoginMenu/UsernameIcon.png");
	usernameIcon->SetDimensions(32, 32);
	usernameIcon->SetPosition(480, 339);
	LoginMenuNode->AddChild(usernameIcon);

	//  Load the login screen password icon
	auto passwordIcon = GUIObjectNode::CreateObjectNode("./Assets/Textures/LoginMenu/PasswordIcon.png");
	passwordIcon->SetDimensions(32, 32);
	passwordIcon->SetPosition(480, 389);
	LoginMenuNode->AddChild(passwordIcon);

	//  Load the login screen username input underline
	auto usernameUnderline = GUIObjectNode::CreateObjectNode("./Assets/Textures/LoginMenu/Pixel_DarkGray.png");
	usernameUnderline->SetDimensions(320, 1);
	usernameUnderline->SetPosition(usernameIcon->GetX(), usernameIcon->GetY() + usernameIcon->GetHeight() - usernameUnderline->GetHeight());
	LoginMenuNode->AddChild(usernameUnderline);

	//  Load the login screen password input underline
	auto passwordUnderline = GUIObjectNode::CreateObjectNode("./Assets/Textures/LoginMenu/Pixel_DarkGray.png");
	passwordUnderline->SetDimensions(320, 1);
	passwordUnderline->SetPosition(passwordIcon->GetX(), passwordIcon->GetY() + passwordIcon->GetHeight() - passwordUnderline->GetHeight());
	LoginMenuNode->AddChild(passwordUnderline);

	//  Load the login screen username edit box
	UsernameEditBox = GUIEditBox::CreateEditBox("", usernameIcon->GetX() + usernameIcon->GetWidth() + 10, usernameIcon->GetY(), usernameUnderline->GetWidth() - usernameIcon->GetWidth() - 20, usernameIcon->GetHeight());
	UsernameEditBox->SetFont(fontManager.GetFont("Arial-12-White"));
	UsernameEditBox->SetColor(0.8f, 0.8f, 0.8f, 1.0f);
	UsernameEditBox->SetTextAlignment(GUIEditBox::ALIGN_LEFT);
	UsernameEditBox->SetEmptyText("Username");
	UsernameEditBox->SetEmptyTextColor(Color(1.0f, 1.0f, 1.0f, 0.15f));
	UsernameEditBox->SetMaxStringLength(18);
	LoginMenuNode->AddChild(UsernameEditBox);

	//  Load the login screen password edit box
	PasswordEditBox = GUIEditBox::CreateEditBox("", passwordIcon->GetX() + passwordIcon->GetWidth() + 10, passwordIcon->GetY(), passwordUnderline->GetWidth() - passwordIcon->GetWidth() - 20, passwordIcon->GetHeight());
	PasswordEditBox->SetFont(fontManager.GetFont("Arial-12-White"));
	PasswordEditBox->SetColor(0.8f, 0.8f, 0.8f, 1.0f);
	PasswordEditBox->SetTextAlignment(GUIEditBox::ALIGN_LEFT);
	PasswordEditBox->SetEmptyText("Password");
	PasswordEditBox->SetEmptyTextColor(Color(1.0f, 1.0f, 1.0f, 0.15f));
	PasswordEditBox->SetMaxStringLength(18);
	PasswordEditBox->SetTextHidden(true);
	PasswordEditBox->SetHiddenCharacter('*');
	LoginMenuNode->AddChild(PasswordEditBox);

	//  Load the login screen login icon
	LoginButton = GUIButton::CreateButton("./Assets/Textures/LoginMenu/LoginButton.png", 906, (int(ScreenHeight) / 2) - (mainLogo->GetHeight() / 2) + 14, 128, 128);
	LoginButton->SetLeftClickCallback(LoginButtonLeftClickCallback);
	LoginMenuNode->AddChild(LoginButton);

	//  Load the "CONNECTED" icon and hide it until connection is valid
	ConnectedIcon = GUIObjectNode::CreateObjectNode("./Assets/Textures/LoginMenu/Connected.png");
	ConnectedIcon->SetDimensions(128, 32);
	ConnectedIcon->SetPosition(230, (int(ScreenHeight) / 2) + (mainLogo->GetHeight() / 2) - 3);
	ConnectedIcon->SetVisible(false);
	LoginMenuNode->AddChild(ConnectedIcon);

	//  Set enter and tab callbacks on the username and password edit boxes, and default to having the username box selected
	UsernameEditBox->SetTabKeyCallback(TabFromUsernameBoxCallback);
	PasswordEditBox->SetTabKeyCallback(TabFromPasswordBoxCallback);
	UsernameEditBox->SetEnterKeyCallback(EnterFromUsernameOrPasswordBoxCallback);
	PasswordEditBox->SetEnterKeyCallback(EnterFromUsernameOrPasswordBoxCallback);
	UsernameEditBox->SetSelected(true);
}


void PrimaryDialogue::LoadMainMenuBarUI()
{
	//  Create the individual UI pieces
	LoadUploadMenuUI();
	LoadLatestUploadsUI();
	LoadSideBarUI();

	//  Set the Client control callbacks
	ClientControl.SetLoginResponseCallback(LoginRequestResponseCallback);
	ClientControl.SetInboxAndNotificationCountCallback(InboxAndNotificationsCountCallback);
	ClientControl.SetLatestUploadsCallback(SetLatestUploads);
	ClientControl.SetFileRequestFailureCallback(FileRequestFailureCallback);
	ClientControl.SetFileRequestSuccessCallback(FileRequestSucceeded);
	ClientControl.SetTransferPercentCompleteCallback(SetTransferPercentage);
}

void PrimaryDialogue::LoadSideBarUI()
{
	if (MainMenuBarUINode != nullptr) return;
	MainMenuBarUINode = GUIObjectNode::CreateObjectNode("");
	MainMenuBarUINode->SetVisible(false);
	AddChild(MainMenuBarUINode);

	//  Load the background strip behind the status bar
	auto mainMenuBackgroundBox = GUIObjectNode::CreateObjectNode("./Assets/Textures/Pixel_White.png");
	mainMenuBackgroundBox->SetColor(0.25f, 0.25f, 0.25f, 1.0f);
	mainMenuBackgroundBox->SetDimensions(int(ScreenWidth), MainMenuBarHeight);
	mainMenuBackgroundBox->SetPosition(0, 0);
	MainMenuBarUINode->AddChild(mainMenuBackgroundBox);

	//  Load the main menu "Home" button
	auto homeButton = GUIButton::CreateButton("./Assets/Textures/Pixel_White.png");
	homeButton->SetColorBytes(0, 0, 0, 1);
	homeButton->SetPosition(31, 5);
	homeButton->SetDimensions(60, 28);
	homeButton->SetLeftClickCallback(SetHomeMenuOpen);
	MainMenuBarUINode->AddChild(homeButton);

	//  Load the main menu "Home" button text
	auto homeButtonText = GUILabel::CreateLabel("Arial-12-White", "HOME", 60, 14, 100, 20, GUILabel::JUSTIFY_CENTER);
	MainMenuBarUINode->AddChild(homeButtonText);

	//  Load the main menu divider text between HOME and BROWSE
	auto dividerLabel_1 = GUILabel::CreateLabel("Arial-12-White", "||", 110, 12, 100, 20, GUILabel::JUSTIFY_CENTER);
	MainMenuBarUINode->AddChild(dividerLabel_1);

	//  Load the main menu "Browse" button
	auto browseButton = GUIButton::CreateButton("./Assets/Textures/Pixel_White.png");
	browseButton->SetColorBytes(0, 0, 0, 1);
	browseButton->SetPosition(125, 5);
	browseButton->SetDimensions(80, 28);
	browseButton->SetLeftClickCallback(SetBrowseMenuOpen);
	MainMenuBarUINode->AddChild(browseButton);

	//  Load the main menu "Browse" button text
	auto browseButtonText = GUILabel::CreateLabel("Arial-12-White", "BROWSE", 165, 14, 100, 20, GUILabel::JUSTIFY_CENTER);
	MainMenuBarUINode->AddChild(browseButtonText);

	//  Load the main menu divider text between BROWSE and UPLOAD
	auto dividerLabel_2 = GUILabel::CreateLabel("Arial-12-White", "||", 217, 12, 100, 20, GUILabel::JUSTIFY_CENTER);
	MainMenuBarUINode->AddChild(dividerLabel_2);

	//  Load the main menu "Upload" button
	auto uploadButton = GUIButton::CreateButton("./Assets/Textures/Pixel_White.png");
	uploadButton->SetColorBytes(0, 0, 0, 1);
	uploadButton->SetPosition(232, 5);
	uploadButton->SetDimensions(80, 28);
	uploadButton->SetLeftClickCallback(SetUploadMenuOpen);
	MainMenuBarUINode->AddChild(uploadButton);

	//  Load the main menu "Upload" button text
	auto uploadButtonText = GUILabel::CreateLabel("Arial-12-White", "UPLOAD", 272, 14, 100, 20, GUILabel::JUSTIFY_CENTER);
	MainMenuBarUINode->AddChild(uploadButtonText);
}


void PrimaryDialogue::LoadLatestUploadsUI()
{
	if (LatestUploadsUINode != nullptr) return;
	LatestUploadsUINode = GUIObjectNode::CreateObjectNode("");
	LatestUploadsUINode->SetVisible(false);
	AddChild(LatestUploadsUINode);

	//  Load the background strip behind the status bar
	auto latestUploadsContainer = GUIObjectNode::CreateObjectNode("./Assets/Textures/Pixel_White.png");
	latestUploadsContainer->SetColor(0.4f, 0.4f, 0.7f, 1.0f);
	latestUploadsContainer->SetDimensions(LatestUploadsWidth, LatestUploadsHeight);
	latestUploadsContainer->SetPosition(80, 70);
	LatestUploadsUINode->AddChild(latestUploadsContainer);

	auto rangeString = std::to_string(CurrentLatestUploadsStartingIndex) + " to " + std::to_string(CurrentLatestUploadsStartingIndex + 20);
	auto latestUploadedFilesLabelString = "Latest Uploaded Files (" + rangeString + "):";
	LatestUploadsTitleLabel = GUILabel::CreateLabel(fontManager.GetFont("Arial"), latestUploadedFilesLabelString.c_str(), 10, 10, LatestUploadsWidth - 20, 30);
	LatestUploadsTitleLabel->SetColor(0.2f, 0.2f, 0.2f, 1.0f);
	latestUploadsContainer->AddChild(LatestUploadsTitleLabel);

	LatestUploadsListBox = GUIListBox::CreateTemplatedListBox("Standard", 5, 30, LatestUploadsWidth - 10, LatestUploadsHeight - 54, LatestUploadsWidth - 26, 2, 12, 12, 12, 12, 12, 24, 2);
	LatestUploadsListBox->SetItemClickCallback(UpdateLatestUploadsListBoxDownloadButtons);
	latestUploadsContainer->AddChild(LatestUploadsListBox);

	auto listBoxLeftArrowButton = GUIButton::CreateButton("./Assets/Textures/MainProgramUI/LeftArrowIcon.png");
	listBoxLeftArrowButton->SetColorBytes(64, 64, 64, 255);
	listBoxLeftArrowButton->SetDimensions(28, 28);
	listBoxLeftArrowButton->SetPosition(4, LatestUploadsHeight - 26);
	listBoxLeftArrowButton->SetLeftClickCallback(ShiftLatestUploadsLeft);
	latestUploadsContainer->AddChild(listBoxLeftArrowButton);

	auto listBoxRightArrowButton = GUIButton::CreateButton("./Assets/Textures/MainProgramUI/RightArrowIcon.png");
	listBoxRightArrowButton->SetColorBytes(64, 64, 64, 255);
	listBoxRightArrowButton->SetDimensions(28, 28);
	auto rightArrowWidth = listBoxRightArrowButton->GetWidth();
	listBoxRightArrowButton->SetPosition(LatestUploadsWidth - rightArrowWidth - 4, LatestUploadsHeight - 26);
	listBoxRightArrowButton->SetLeftClickCallback(ShiftLatestUploadsRight);
	latestUploadsContainer->AddChild(listBoxRightArrowButton);

}


void PrimaryDialogue::LoadUploadMenuUI()
{
	if (UploadMenuUINode != nullptr) return;
	UploadMenuUINode = GUIObjectNode::CreateObjectNode("");
	UploadMenuUINode->SetVisible(false);
	AddChild(UploadMenuUINode);

	auto filesInUploadFolderLabel = GUILabel::CreateLabel("Arial-12-White", "Files In The Upload Folder:", 34, 92, 200, 20);
	filesInUploadFolderLabel->SetColorBytes(160, 160, 160, 255);
	UploadMenuUINode->AddChild(filesInUploadFolderLabel);

	UploadFolderItemsListBox = GUIListBox::CreateTemplatedListBox("Standard", 32, 108, 432, 568, 414, 0, 18, 18, 18, 18, 18, 32, 2);
	UploadFolderItemsListBox->SetColorBytes(87, 28, 87, 255);
	UploadFolderItemsListBox->SetItemClickCallback(SelectUploadItem);
	UploadMenuUINode->AddChild(UploadFolderItemsListBox);

	UploadFileNameLabel = GUILabel::CreateLabel("Arial-12-White", "File Name:       NO FILE SELECTED", 530, 126, 200, 20);
	UploadFileNameLabel->SetColorBytes(160, 160, 160, 255);
	UploadMenuUINode->AddChild(UploadFileNameLabel);

	auto fileTitleLabel = GUILabel::CreateLabel("Arial-12-White", "File Title:", 530, 160, 200, 20);
	fileTitleLabel->SetColorBytes(160, 160, 160, 255);
	UploadMenuUINode->AddChild(fileTitleLabel);

	UploadFileTitleEditBox = GUIEditBox::CreateTemplatedEditBox("Standard", 640, 150, 600, 32);
	UploadFileTitleEditBox->SetFont("Arial");
	UploadFileTitleEditBox->SetTextAlignment(GUIEditBox::ALIGN_LEFT);
	UploadFileTitleEditBox->SetColorBytes(230, 230, 230, 255);
	UploadMenuUINode->AddChild(UploadFileTitleEditBox);

	auto fileDescriptionLabel = GUILabel::CreateLabel("Arial-12-White", "File Description:", 530, 196, 200, 20);
	fileDescriptionLabel->SetColorBytes(160, 160, 160, 255);
	UploadMenuUINode->AddChild(fileDescriptionLabel);

	auto fileDescriptionEditBox = GUIEditBox::CreateTemplatedEditBox("Standard", 528, 218, 712, 110);
	fileDescriptionEditBox->SetFont("Arial");
	fileDescriptionEditBox->SetTextAlignment(GUIEditBox::ALIGN_LEFT);
	fileDescriptionEditBox->SetColorBytes(230, 230, 230, 255);
	UploadMenuUINode->AddChild(fileDescriptionEditBox);

	auto fileTypeLabel = GUILabel::CreateLabel("Arial-12-White", "File Type:", 530, 344, 200, 20);
	fileTypeLabel->SetColorBytes(160, 160, 160, 255);
	UploadMenuUINode->AddChild(fileTypeLabel);

	//  Note: Create the upload button before the drop-downs, as they overlap it and the click will otherwise go through
	auto fileUploadButton = GUIButton::CreateTemplatedButton("Standard", 824, 470, 420, 50);
	fileUploadButton->SetColorBytes(120, 215, 120, 255);
	fileUploadButton->SetFont("Arial");
	fileUploadButton->SetText("UPLOAD FILE");
	fileUploadButton->SetLeftClickCallback(UploadFileToServer);
	UploadMenuUINode->AddChild(fileUploadButton);

	UploadFileTypeDropDown = GUIDropDown::CreateTemplatedDropDown("Standard", 824, 338, 420, 40, 386, 10, 16, 16);
	auto fileTypeX = UploadFileTypeDropDown->GetWidth() / 2;
	auto fileTypeY = UploadFileTypeDropDown->GetHeight() / 2 - 6;
	auto fileTypeList = GetListOfFileTypes();
	for (auto iter = fileTypeList.begin(); iter != fileTypeList.end(); ++iter)
		UploadFileTypeDropDown->AddItem(GUILabel::CreateLabel("Arial", (*iter).c_str(), fileTypeX, fileTypeY, 200, 24, GUILabel::JUSTIFY_CENTER));
	UploadMenuUINode->AddChild(UploadFileTypeDropDown);

	auto fileSubTypeLabel = GUILabel::CreateLabel("Arial-12-White", "File Type:", 530, 386, 200, 20);
	fileSubTypeLabel->SetColorBytes(160, 160, 160, 255);
	UploadMenuUINode->AddChild(fileSubTypeLabel);

	UploadFileSubTypeDropDown = GUIDropDown::CreateTemplatedDropDown("Standard", 824, 378, 420, 40, 386, 10, 16, 16);
	auto fileSubTypeX = UploadFileSubTypeDropDown->GetWidth() / 2;
	auto fileSubTypeY = UploadFileSubTypeDropDown->GetHeight() / 2 - 6;
	auto fileSubTypeList = GetListOfFileSubTypes();
	for (auto iter = fileSubTypeList.begin(); iter != fileSubTypeList.end(); ++iter)
		UploadFileSubTypeDropDown->AddItem(GUILabel::CreateLabel("Arial", (*iter).c_str(), fileSubTypeX, fileSubTypeY, 200, 20, GUILabel::JUSTIFY_CENTER));
	UploadMenuUINode->AddChild(UploadFileSubTypeDropDown);
}


void PrimaryDialogue::UpdateUI()
{
	auto loginButtonVisible = ((UsernameEditBox->GetText().length() > 0) && (PasswordEditBox->GetText().length() > 0));
	LoginButton->SetVisible((ClientConnected == CONNECTION_STATUS_CONNECTED) && loginButtonVisible);

	auto statusMessageExists = (StatusBarTextLabel->GetText().length() != 0);
	StatusBarNode->SetVisible(statusMessageExists);
	StatusBarTextLabel->SetVisible(statusMessageExists);
	if (statusMessageExists)
	{
		auto statusBarTextRangeExtension = 200.0;
		auto statusBarTextRange = (statusBarTextRangeExtension * 2.0 + ScreenWidth);
		auto statusBarPassTime = 20.0;
		auto statusBarPassRatio = fmod(gameSeconds, statusBarPassTime) / statusBarPassTime;
		auto statusBarX = (ScreenWidth + statusBarTextRangeExtension) - (statusBarTextRange * statusBarPassRatio);
		StatusBarTextLabel->SetX(int(ScreenWidth / 2.0f));// int(statusBarX));
	}
}