#pragma once

#include "Client.h"
#include "Engine/GUIObjectNode.h"
#include "Engine/GUIEditBox.h"
#include "Engine/GUILabel.h"
#include "Engine/GUIListBox.h"
#include "Engine/GUIProgressBar.h"
#include "Engine/FontManager.h"
#include "Engine/TimeSlice.h"
#include "Engine/StringTools.h"
#include "HostedFileData.h"
#include "FileUploadDialogue.h"
#include "FileTransfersDialogue.h"
#include <math.h>

const auto MainMenuBarHeight = 40;
const auto HostedFileListWidth = 620;
const auto HostedFileListHeight = 580;
const auto SearchFilterBoxWidth = 480;
const auto SearchFilterBoxHeight = 230;

int CurrentLatestUploadsStartingIndex = 0;

//  Global UI objects
GUIObjectNode* StatusBarBG = nullptr;
GUILabel* StatusBarTextLabel = nullptr;
GUIObjectNode* StatusBarTransferFill = nullptr;

GUIObjectNode* LoginMenuNode = nullptr;
GUIEditBox* UsernameEditBox = nullptr;
GUIEditBox* PasswordEditBox = nullptr;

GUIObjectNode* MainMenuBarUINode = nullptr;
GUIObjectNode* HostedFileListUINode = nullptr;
GUIObjectNode* SearchFilterUINode = nullptr;

GUIButton* UserSideTabButton = nullptr;
GUIObjectNode* UserIconNode = nullptr;
GUIButton* InboxSideTabButton = nullptr;
GUIObjectNode* InboxIconNode = nullptr;
GUIButton* NotificationsSideTabButton = nullptr;
GUIObjectNode* NotificationsIconNode = nullptr;
GUIButton* UploadSideTabButton = nullptr;
GUIObjectNode* UploadIconNode = nullptr;

GUILabel* LatestUploadsTitleLabel = nullptr;
GUIListBox* LatestUploadsListBox = nullptr;

GUIEditBox* FilterByUserEditBox = nullptr;
GUIDropDown* FilterByTypeDropDown = nullptr;
GUIDropDown* FilterBySubtypeDropDown = nullptr;

GUIObjectNode* CurrentTransferContainer = nullptr;
//GUIProgressBar* ProgressBarTest = nullptr;

Client& ClientControl = Client::GetInstance();


void SetStatusBarMessage(std::string statusBarMessage, bool error = false)
{
	if (StatusBarTextLabel == nullptr) return;

	StatusBarTextLabel->SetText(statusBarMessage);

	if (error) StatusBarBG->SetColor(0.8f, 0.1f, 0.1f, 1.0f);
	else StatusBarBG->SetColor(0.1f, 0.4f, 0.7f, 1.0f);
}

void RequestLatestUploadFile(GUIObjectNode* node)
{
	auto entry = LatestUploadsListBox->GetSelectedItem();
	auto fileTypeIndex = HostedFileType(entry->GetChildByName("FileTypeImage")->GetZOrder());
	auto fileSubTypeIndex = HostedFileSubtype(entry->GetChildByName("FileSubTypeImage")->GetZOrder());

	FileTransfersDialogue::GetInstance()->AddDownloadToQueue(entry->GetObjectName(), fileTypeIndex, fileSubTypeIndex);
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
	fileTypeImage->SetObjectName("FileTypeImage");
	fileTypeImage->SetDimensions(20, 20);
	fileTypeImage->SetPosition(6, 8);
	fileTypeImage->SetZOrder(fileEntryData.FileType);
	entry->AddChild(fileTypeImage);

	auto fileSubTypeImage = GetFileSubTypeIconFromID(fileEntryData.FileSubType);
	fileSubTypeImage->SetObjectName("FileSubTypeImage");
	fileSubTypeImage->SetDimensions(20, 20);
	fileSubTypeImage->SetPosition(26, 8);
	fileSubTypeImage->SetZOrder(fileEntryData.FileSubType);
	entry->AddChild(fileSubTypeImage);

	auto titleLabel = GUILabel::CreateLabel(fontManager.GetFont("Arial"), fileEntryData.FileTitle.c_str(), 56, 9, 100, 20);
	entry->AddChild(titleLabel);

	auto uploaderLabel = GUILabel::CreateLabel(fontManager.GetFont("Arial"), fileEntryData.FileUploader.c_str(), 456, 9, 100, 20);
	entry->AddChild(uploaderLabel);

	auto button = GUIButton::CreateTemplatedButton("Standard", HostedFileListWidth - 100, 6, 80, 20);
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


void RequestHostedFileList(int searchIndex = 0)
{
	CurrentLatestUploadsStartingIndex = searchIndex;

	if (FilterByUserEditBox == nullptr) return;
	if (FilterByTypeDropDown == nullptr) return;
	if (FilterBySubtypeDropDown == nullptr) return;
	if (FilterByTypeDropDown->GetSelectedItem() == nullptr) return;
	if (FilterBySubtypeDropDown->GetSelectedItem() == nullptr) return;

	auto userName = FilterByUserEditBox->GetText();
	auto encryptedUser = (userName.length() == 0) ? EncryptedData() : Groundfish::Encrypt(userName.c_str(), int(FilterByUserEditBox->GetText().length()));
	auto filterType = HostedFileType(GetFileTypeIDFromName(FilterByTypeDropDown->GetSelectedItem()->GetObjectName()));
	auto filterSubtype = HostedFileSubtype(GetFileSubTypeIDFromName(FilterBySubtypeDropDown->GetSelectedItem()->GetObjectName()));

	SendMessage_RequestHostedFileList(CurrentLatestUploadsStartingIndex, ClientControl.GetServerSocket(), encryptedUser, filterType, filterSubtype);
}


void ApplySearchFilter(GUIObjectNode* object)
{
	RequestHostedFileList();
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


void UpdateUploadsRangeString(void)
{
	auto rangeString = std::to_string(CurrentLatestUploadsStartingIndex) + " to " + std::to_string(CurrentLatestUploadsStartingIndex + 20);
	LatestUploadsTitleLabel->SetText("Latest Uploaded Files (" + rangeString + ")");
}


void SetHomeMenuOpen(GUIObjectNode* button)
{
	//  TODO: Set up the "HOME" menu (inbox, notifications, hot files, etc)

	//  Make the latest uploads UI visible
	HostedFileListUINode->SetVisible(true);
	SearchFilterUINode->SetVisible(true);

	//  Make the sure base UI is set for logged in users (login screen off, main menu on)
	LoginMenuNode->SetVisible(false);
	MainMenuBarUINode->SetVisible(true);

	//  Make the upload and transfers UI invisible
	FileUploadDialogue::GetInstance()->SetUIHidden();
	FileTransfersDialogue::GetInstance()->SetUIHidden();

	//  Request a new hosted file list
	CurrentLatestUploadsStartingIndex = 0;
	UpdateUploadsRangeString();
	RequestHostedFileList(0);

	//  Set the status bar back to default
	SetStatusBarMessage("", false);
}


void SetBrowseMenuOpen(GUIObjectNode* button)
{
	//  Make the latest uploads UI visible
	HostedFileListUINode->SetVisible(true);
	SearchFilterUINode->SetVisible(true);

	//  Make the sure base UI is set for logged in users (login screen off, main menu on)
	LoginMenuNode->SetVisible(false);
	MainMenuBarUINode->SetVisible(true);

	//  Make the upload and transfers UI invisible
	FileUploadDialogue::GetInstance()->SetUIHidden();
	FileTransfersDialogue::GetInstance()->SetUIHidden();

	//  Request a new hosted file list
	CurrentLatestUploadsStartingIndex = 0;
	UpdateUploadsRangeString();
	RequestHostedFileList(0);

	//  Set the status bar back to default
	SetStatusBarMessage("", false);
}


void SetUploadMenuOpen(GUIObjectNode* button)
{
	//  Make the upload UI visible and the transfers UI invisible
	FileUploadDialogue::GetInstance()->OpenUploadMenu();
	FileTransfersDialogue::GetInstance()->SetUIHidden();

	//  Make the sure base UI is set for logged in users (login screen off, main menu on)
	LoginMenuNode->SetVisible(false);
	MainMenuBarUINode->SetVisible(true);

	//  Make the latest uploads UI invisible
	HostedFileListUINode->SetVisible(false);
	SearchFilterUINode->SetVisible(false);

	//  Set the status bar back to default
	SetStatusBarMessage("", false);
}


void SetTransfersMenuOpen(GUIObjectNode* button)
{
	//  Make the upload UI invisible and the transfers UI visible
	FileUploadDialogue::GetInstance()->SetUIHidden();
	FileTransfersDialogue::GetInstance()->OpenTransfersMenu();

	//  Make the sure base UI is set for logged in users (login screen off, main menu on)
	LoginMenuNode->SetVisible(false);
	MainMenuBarUINode->SetVisible(true);

	//  Make the latest uploads UI invisible
	HostedFileListUINode->SetVisible(false);
	SearchFilterUINode->SetVisible(false);

	//  Set the status bar back to default
	SetStatusBarMessage("", false);
}


void ShiftLatestUploadsLeft(GUIObjectNode* object)
{
	if (CurrentLatestUploadsStartingIndex < 20) return;
	auto usernameSize = ClientControl.GetUsername().size();
	RequestHostedFileList(CurrentLatestUploadsStartingIndex - 20);
	UpdateUploadsRangeString();
}


void ShiftLatestUploadsRight(GUIObjectNode* object)
{
	if (CurrentLatestUploadsStartingIndex > 50000) return;
	auto usernameSize = ClientControl.GetUsername().size();
	RequestHostedFileList(CurrentLatestUploadsStartingIndex + 20);
	UpdateUploadsRangeString();
}


void LoginRequestResponseCallback(int response, int inboxCount, int notificationCount)
{
	bool success = (response == LOGIN_RESPONSE_SUCCESS);
	LoginMenuNode->SetVisible(!success);
	MainMenuBarUINode->SetVisible(success);
	HostedFileListUINode->SetVisible(success);
	SearchFilterUINode->SetVisible(success);
	FileUploadDialogue::GetInstance()->SetUIHidden();

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

void FileSendFailureCallback(std::string failureReason)
{
	//  Generate the full string and set the status bar message
	auto fullString = "File upload initialization failed: " + failureReason;
	SetStatusBarMessage(fullString, true);

	ClientControl.CancelFileSend();
}

void InboxAndNotificationsCountCallback(int inboxCount, int notificationCount)
{
	SetInboxMessageCount(inboxCount);
	SetNotificationCount(notificationCount);
}

void LoginButtonLeftClickCallback(GUIObjectNode* button)
{
	//  Encrypt the username and password
	EncryptedData encryptedUsernameVector = Groundfish::Encrypt(UsernameEditBox->GetText().c_str(), int(UsernameEditBox->GetText().length()), rand() % 256);
	EncryptedData encryptedPasswordVector = Groundfish::Encrypt(PasswordEditBox->GetText().c_str(), int(PasswordEditBox->GetText().length()), rand() % 256);

	ClientControl.SetUsername(encryptedUsernameVector);

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
	void LoadHostedFileListUI();
	void LoadSearchFilterUI();

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
	StatusBarTextLabel->SetJustification(UI_JUSTIFY_CENTER);
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
	UsernameEditBox->SetEmptyText(" Username");
	UsernameEditBox->SetEmptyTextColor(Color(1.0f, 1.0f, 1.0f, 0.15f));
	UsernameEditBox->SetMaxStringLength(18);
	LoginMenuNode->AddChild(UsernameEditBox);

	//  Load the login screen password edit box
	PasswordEditBox = GUIEditBox::CreateEditBox("", passwordIcon->GetX() + passwordIcon->GetWidth() + 10, passwordIcon->GetY(), passwordUnderline->GetWidth() - passwordIcon->GetWidth() - 20, passwordIcon->GetHeight());
	PasswordEditBox->SetFont(fontManager.GetFont("Arial-12-White"));
	PasswordEditBox->SetColor(0.8f, 0.8f, 0.8f, 1.0f);
	PasswordEditBox->SetTextAlignment(GUIEditBox::ALIGN_LEFT);
	PasswordEditBox->SetEmptyText(" Password");
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

	//ProgressBarTest = GUIProgressBar::CreateTemplatedProgressBar("Standard", 100, 160, 300, 40);
	//ProgressBarTest->SetBarColor(COLOR_RED);
	//ProgressBarTest->SetFont("Arial");
	//LoginMenuNode->AddChild(ProgressBarTest);
}


void PrimaryDialogue::LoadMainMenuBarUI()
{
	//  Load the File Upload Dialogue and the File Transfers
	AddChild(FileUploadDialogue::GetInstance());
	AddChild(FileTransfersDialogue::GetInstance());

	//  Create the individual UI pieces
	LoadHostedFileListUI();
	LoadSearchFilterUI();
	LoadSideBarUI();

	//  Set the Client control callbacks
	ClientControl.SetLoginResponseCallback(LoginRequestResponseCallback);
	ClientControl.SetInboxAndNotificationCountCallback(InboxAndNotificationsCountCallback);
	ClientControl.SetLatestUploadsCallback(SetLatestUploads);
	ClientControl.SetFileRequestFailureCallback(FileRequestFailureCallback);
	ClientControl.SetFileSendFailureCallback(FileSendFailureCallback);
	ClientControl.SetFileRequestSuccessCallback(FileRequestSucceeded);
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
#ifdef _DEBUG //  Show the button area in debug mode
	homeButton->SetColorBytes(0, 0, 0, 50);
#endif

	//  Load the main menu "Home" button text
	auto homeButtonText = GUILabel::CreateLabel("Arial-12-White", "HOME", 60, 14, 100, 20, UI_JUSTIFY_CENTER);
	MainMenuBarUINode->AddChild(homeButtonText);

	//  Load the main menu divider text between HOME and BROWSE
	auto dividerLabel_1 = GUILabel::CreateLabel("Arial-12-White", "||", 104, 12, 100, 20, UI_JUSTIFY_CENTER);
	MainMenuBarUINode->AddChild(dividerLabel_1);

	//  Load the main menu "Browse" button
	auto browseButton = GUIButton::CreateButton("./Assets/Textures/Pixel_White.png");
	browseButton->SetColorBytes(0, 0, 0, 1);
	browseButton->SetPosition(119, 5);
	browseButton->SetDimensions(80, 28);
	browseButton->SetLeftClickCallback(SetBrowseMenuOpen);
	MainMenuBarUINode->AddChild(browseButton);
#ifdef _DEBUG //  Show the button area in debug mode
	browseButton->SetColorBytes(0, 0, 0, 50);
#endif

	//  Load the main menu "Browse" button text
	auto browseButtonText = GUILabel::CreateLabel("Arial-12-White", "BROWSE", 159, 14, 100, 20, UI_JUSTIFY_CENTER);
	MainMenuBarUINode->AddChild(browseButtonText);

	//  Load the main menu divider text between BROWSE and UPLOAD
	auto dividerLabel_2 = GUILabel::CreateLabel("Arial-12-White", "||", 213, 12, 100, 20, UI_JUSTIFY_CENTER);
	MainMenuBarUINode->AddChild(dividerLabel_2);

	//  Load the main menu "Upload" button
	auto uploadButton = GUIButton::CreateButton("./Assets/Textures/Pixel_White.png");
	uploadButton->SetColorBytes(0, 0, 0, 1);
	uploadButton->SetPosition(225, 5);
	uploadButton->SetDimensions(80, 28);
	uploadButton->SetLeftClickCallback(SetUploadMenuOpen);
	MainMenuBarUINode->AddChild(uploadButton);
#ifdef _DEBUG //  Show the button area in debug mode
	uploadButton->SetColorBytes(0, 0, 0, 50);
#endif

	//  Load the main menu "Upload" button text
	auto uploadButtonText = GUILabel::CreateLabel("Arial-12-White", "UPLOAD", 265, 14, 100, 20, UI_JUSTIFY_CENTER);
	MainMenuBarUINode->AddChild(uploadButtonText);

	//  Load the main menu divider text between BROWSE and UPLOAD
	auto dividerLabel_3 = GUILabel::CreateLabel("Arial-12-White", "||", 318, 12, 100, 20, UI_JUSTIFY_CENTER);
	MainMenuBarUINode->AddChild(dividerLabel_3);

	//  Load the main menu "Upload" button
	auto transfersButton = GUIButton::CreateButton("./Assets/Textures/Pixel_White.png");
	transfersButton->SetColorBytes(0, 0, 0, 1);
	transfersButton->SetPosition(332, 5);
	transfersButton->SetDimensions(110, 28);
	transfersButton->SetLeftClickCallback(SetTransfersMenuOpen);
	MainMenuBarUINode->AddChild(transfersButton);
#ifdef _DEBUG //  Show the button area in debug mode
	transfersButton->SetColorBytes(0, 0, 0, 50);
#endif

	//  Load the main menu "Transfers" button text
	auto transfersButtonText = GUILabel::CreateLabel("Arial-12-White", "TRANSFERS", 387, 14, 100, 20, UI_JUSTIFY_CENTER);
	MainMenuBarUINode->AddChild(transfersButtonText);
}


void PrimaryDialogue::LoadHostedFileListUI()
{
	if (HostedFileListUINode != nullptr) return;
	HostedFileListUINode = GUIObjectNode::CreateObjectNode("");
	HostedFileListUINode->SetVisible(false);
	AddChild(HostedFileListUINode);

	//  Load the background strip behind the hosted file list
	auto hostedFileListContainer = GUIObjectNode::CreateObjectNode("./Assets/Textures/Pixel_White.png");
	hostedFileListContainer->SetColor(0.4f, 0.4f, 0.7f, 1.0f);
	hostedFileListContainer->SetDimensions(HostedFileListWidth, HostedFileListHeight);
	hostedFileListContainer->SetPosition(80, 70);
	HostedFileListUINode->AddChild(hostedFileListContainer);

	auto fileTitleLabel = GUILabel::CreateLabel("Arial", "Hosted File Title:", 62, 10, 200, 20, UI_JUSTIFY_LEFT);
	hostedFileListContainer->AddChild(fileTitleLabel);

	auto fileUploaderLabel = GUILabel::CreateLabel("Arial", "File Uploader:", 462, 10, 200, 20, UI_JUSTIFY_LEFT);
	hostedFileListContainer->AddChild(fileUploaderLabel);

	auto rangeString = std::to_string(CurrentLatestUploadsStartingIndex) + " to " + std::to_string(CurrentLatestUploadsStartingIndex + 20);
	auto latestUploadedFilesLabelString = "Latest Uploaded Files (" + rangeString + ")";
	LatestUploadsTitleLabel = GUILabel::CreateLabel("Arial", latestUploadedFilesLabelString.c_str(), hostedFileListContainer->GetWidth() / 2, hostedFileListContainer->GetHeight() - 20, HostedFileListWidth - 20, 30, UI_JUSTIFY_CENTER);
	LatestUploadsTitleLabel->SetColor(0.2f, 0.2f, 0.2f, 1.0f);
	hostedFileListContainer->AddChild(LatestUploadsTitleLabel);

	LatestUploadsListBox = GUIListBox::CreateTemplatedListBox("Standard", 5, 30, HostedFileListWidth - 10, HostedFileListHeight - 54, HostedFileListWidth - 26, 2, 12, 12, 12, 12, 12, 24, 2);
	LatestUploadsListBox->SetItemClickCallback(UpdateLatestUploadsListBoxDownloadButtons);
	hostedFileListContainer->AddChild(LatestUploadsListBox);

	auto listBoxLeftArrowButton = GUIButton::CreateButton("./Assets/Textures/MainProgramUI/LeftArrowIcon.png");
	listBoxLeftArrowButton->SetColorBytes(64, 64, 64, 255);
	listBoxLeftArrowButton->SetDimensions(28, 28);
	listBoxLeftArrowButton->SetPosition(4, HostedFileListHeight - 26);
	listBoxLeftArrowButton->SetLeftClickCallback(ShiftLatestUploadsLeft);
	hostedFileListContainer->AddChild(listBoxLeftArrowButton);

	auto listBoxRightArrowButton = GUIButton::CreateButton("./Assets/Textures/MainProgramUI/RightArrowIcon.png");
	listBoxRightArrowButton->SetColorBytes(64, 64, 64, 255);
	listBoxRightArrowButton->SetDimensions(28, 28);
	auto rightArrowWidth = listBoxRightArrowButton->GetWidth();
	listBoxRightArrowButton->SetPosition(HostedFileListWidth - rightArrowWidth - 4, HostedFileListHeight - 26);
	listBoxRightArrowButton->SetLeftClickCallback(ShiftLatestUploadsRight);
	hostedFileListContainer->AddChild(listBoxRightArrowButton);
}


void PrimaryDialogue::LoadSearchFilterUI()
{
	if (SearchFilterUINode != nullptr) return;
	SearchFilterUINode = GUIObjectNode::CreateObjectNode("");
	SearchFilterUINode->SetVisible(false);
	AddChild(SearchFilterUINode);

	//  Load the background strip behind the search filter
	auto searchFilterContainer = GUIObjectNode::CreateObjectNode("./Assets/Textures/Pixel_White.png");
	searchFilterContainer->SetColor(0.4f, 0.4f, 0.7f, 1.0f);
	searchFilterContainer->SetDimensions(SearchFilterBoxWidth, SearchFilterBoxHeight);
	searchFilterContainer->SetPosition(80 + HostedFileListWidth + 20, 70);
	SearchFilterUINode->AddChild(searchFilterContainer);

	auto searchFiltersLabel = GUILabel::CreateLabel("Arial", "Search Filters:", 10, 10, 200, 20, UI_JUSTIFY_LEFT);
	searchFilterContainer->AddChild(searchFiltersLabel);

	auto filterByUserLabel = GUILabel::CreateLabel("Arial-12-White", "Filter By User:", 10, 60, 220, 20, UI_JUSTIFY_LEFT);
	searchFilterContainer->AddChild(filterByUserLabel);

	FilterByUserEditBox = GUIEditBox::CreateTemplatedEditBox("Standard", 160, 57, 240, 20);
	FilterByUserEditBox->SetEmptyText("Leave blank to not filter");
	FilterByUserEditBox->SetFont("Arial");
	FilterByUserEditBox->SetEmptyTextColor(COLOR_WHITE_FADED);
	searchFilterContainer->AddChild(FilterByUserEditBox);

	auto filterByTypeLabel = GUILabel::CreateLabel("Arial-12-White", "Filter By Type:", 10, 100, 200, 20, UI_JUSTIFY_LEFT);
	searchFilterContainer->AddChild(filterByTypeLabel);

	FilterByTypeDropDown = GUIDropDown::CreateTemplatedDropDown("Standard", 160, 96, 240, 24, 222, 5, 14, 14);
	auto defaultTypeFilter = GUILabel::CreateLabel("Arial", "NO FILTER", 111, 5, 100, 24, UI_JUSTIFY_CENTER);
	defaultTypeFilter->SetObjectName("NO TYPE");
	FilterByTypeDropDown->AddItem(defaultTypeFilter);
	auto fileTypeList = GetListOfFileTypes();
	for (auto iter = fileTypeList.begin(); iter != fileTypeList.end(); ++iter)
	{
		auto typeFilter = GUILabel::CreateLabel("Arial", (*iter).c_str(), 111, 5, 100, 24, UI_JUSTIFY_CENTER);
		typeFilter->SetObjectName((*iter));
		FilterByTypeDropDown->AddItem(typeFilter);
	}
	searchFilterContainer->AddChild(FilterByTypeDropDown);

	auto filterBySubtypeLabel = GUILabel::CreateLabel("Arial-12-White", "Filter By Subtype:", 10, 140, 200, 20, UI_JUSTIFY_LEFT);
	searchFilterContainer->AddChild(filterBySubtypeLabel);

	FilterBySubtypeDropDown = GUIDropDown::CreateTemplatedDropDown("Standard", 160, 136, 240, 24, 222, 5, 14, 14);
	auto defaultSubtypeFilter = GUILabel::CreateLabel("Arial", "NO FILTER", 111, 5, 100, 24, UI_JUSTIFY_CENTER);
	defaultSubtypeFilter->SetObjectName("NO SUBTYPE");
	FilterBySubtypeDropDown->AddItem(defaultSubtypeFilter);
	auto fileSubtypeList = GetListOfFileSubTypes();
	for (auto iter = fileSubtypeList.begin(); iter != fileSubtypeList.end(); ++iter)
	{
		auto subtypeFilter = GUILabel::CreateLabel("Arial", (*iter).c_str(), 111, 5, 100, 24, UI_JUSTIFY_CENTER);
		subtypeFilter->SetObjectName((*iter));
		FilterBySubtypeDropDown->AddItem(subtypeFilter);
	}
	searchFilterContainer->AddChild(FilterBySubtypeDropDown);

	auto applySearchFilterButton = GUIButton::CreateTemplatedButton("Standard", 10, 170, 200, 26);
	applySearchFilterButton->SetFont("Arial");
	applySearchFilterButton->SetText("Apply Search Filter");
	applySearchFilterButton->SetLeftClickCallback(ApplySearchFilter);
	searchFilterContainer->AddChild(applySearchFilterButton);
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
		StatusBarTextLabel->SetX(int(ScreenWidth / 2.0f));
	}
}