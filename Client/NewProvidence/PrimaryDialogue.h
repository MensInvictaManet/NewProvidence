#pragma once

#include "Client.h"
#include "Engine/GUIObjectNode.h"
#include "Engine/GUIEditBox.h"
#include "Engine/GUILabel.h"
#include "Engine/GUIListBox.h"
#include "Engine/FontManager.h"
#include "Engine/TimeSlice.h"
#include <math.h>

const auto SideBarWidth = 300;
bool SideBarOpen = false;
const auto LatestUploadsWidth = 600;
const auto LatestUploadsHeight = 300;

//  Global UI objects
GUIObjectNode* StatusBarBG = nullptr;
GUILabel* StatusBarTextLabel = nullptr;
GUIObjectNode* StatusBarDownloadFill = nullptr;

GUIObjectNode* LoginMenuNode = nullptr;
GUIEditBox* UsernameEditBox = nullptr;
GUIEditBox* PasswordEditBox = nullptr;

GUIObjectNode* MainProgramUINode = nullptr;
GUIButton* UserSideTabButton = nullptr;
GUIObjectNode* UserIconNode = nullptr;
GUIButton* InboxSideTabButton = nullptr;
GUIObjectNode* InboxIconNode = nullptr;
GUIButton* NotificationsSideTabButton = nullptr;
GUIObjectNode* NotificationsIconNode = nullptr;
GUIObjectNode* SideBarBox = nullptr;

GUIListBox* LatestUploadsListBox = nullptr;
GUIObjectNode* CurrentDownloadContainer = nullptr;

Client ClientControl;

void RequestLatestUploadFile(GUIObjectNode* node)
{
	auto entry = LatestUploadsListBox->GetSelectedItem();
	SendMessage_FileRequest(entry->GetObjectName(), ClientControl.GetServerSocket(), NEW_PROVIDENCE_IP);
}


void AddLatestUploadEntry(std::string upload)
{
	auto entry = GUIObjectNode::CreateObjectNode("");
	entry->SetObjectName(upload);

	auto label = GUILabel::CreateLabel(fontManager.GetFont("Arial"), upload.c_str(), 6, 9, 100, 20);
	entry->AddChild(label);

	auto button = GUIButton::CreateTemplatedButton("Standard", LatestUploadsWidth - 100, 6, 80, 20);
	button->SetFont("Arial");
	button->SetText("Download");
	button->SetLeftClickCallback(RequestLatestUploadFile);
	entry->AddChild(button);

	LatestUploadsListBox->AddItem(entry);
}


void SetLatestUploads(std::vector<std::string> latestUploadsList)
{
	if (LatestUploadsListBox == nullptr) return;

	for (auto iter = latestUploadsList.begin(); iter != latestUploadsList.end(); ++iter)
	{
		AddLatestUploadEntry((*iter));
	}
}


void SetUserMenuOpen(GUIObjectNode*)
{
	SideBarOpen = !SideBarOpen;

	SideBarBox->SetVisible(SideBarOpen);
	UserSideTabButton->SetX(SideBarOpen ? SideBarWidth : 0);

	InboxSideTabButton->SetVisible(!SideBarOpen);
	NotificationsSideTabButton->SetVisible(!SideBarOpen);

	//  TODO: Show User Menu in SideBar
}


void SetInboxOpen(GUIObjectNode* button)
{
	SideBarOpen = !SideBarOpen;

	SideBarBox->SetVisible(SideBarOpen);
	InboxSideTabButton->SetX(SideBarOpen ? SideBarWidth : 0);

	UserSideTabButton->SetVisible(!SideBarOpen);
	NotificationsSideTabButton->SetVisible(!SideBarOpen);

	//  TODO: Show Inbox Menu in SideBar
}


void SetInboxMessageCount(int messageCount)
{
	auto empty = (messageCount <= 0);
	InboxIconNode->SetTextureID(textureManager.LoadTextureGetID(empty ? "./Assets/Textures/MainProgramUI/InboxEmpty.png" : "./Assets/Textures/MainProgramUI/InboxMailReceived.png"));
	if (empty)		InboxIconNode->SetColor(0.6f, 0.6f, 0.6f, 1.0f);
	else			InboxIconNode->SetColor(1.0f, 0.2f, 0.2f, 1.0f);
}


void SetNotificationsOpen(GUIObjectNode* button)
{
	SideBarOpen = !SideBarOpen;

	SideBarBox->SetVisible(SideBarOpen);
	NotificationsSideTabButton->SetX(SideBarOpen ? SideBarWidth : 0);

	UserSideTabButton->SetVisible(!SideBarOpen);
	InboxSideTabButton->SetVisible(!SideBarOpen);

	//  TODO: Show Notifications Menu in SideBar
}


void SetNotificationCount(int notificationCount)
{
	auto empty = (notificationCount <= 0);
	NotificationsIconNode->SetTextureID(textureManager.LoadTextureGetID(empty ? "./Assets/Textures/MainProgramUI/NotificationsEmpty.png" : "./Assets/Textures/MainProgramUI/NotificationsReceived.png"));
	if (empty)		NotificationsIconNode->SetColor(0.6f, 0.6f, 0.6f, 1.0f);
	else			NotificationsIconNode->SetColor(1.0f, 0.2f, 0.2f, 1.0f);
}


void SetStatusBarMessage(std::string statusBarMessage, bool error = false)
{
	if (StatusBarTextLabel == nullptr) return;

	StatusBarTextLabel->SetText(statusBarMessage);

	if (error) StatusBarBG->SetColor(0.8f, 0.1f, 0.1f, 1.0f);
	else StatusBarBG->SetColor(0.1f, 0.4f, 0.7f, 1.0f);
}


void SetDownloadPercentage(float percent, double time, int fileSize)
{
	int averageKBs = int((float(fileSize) / 1024.0f) / time);

	if (percent >= 1.0f)
	{
		StatusBarDownloadFill->SetVisible(false);
		SetStatusBarMessage("Download completed in " + std::to_string(int(time)) + " seconds (avg " + std::to_string(averageKBs) + " KB/s)", false);
	}
	else
	{
		StatusBarDownloadFill->SetVisible(true);
		StatusBarDownloadFill->SetWidth(int(percent * ScreenWidth));
		SetStatusBarMessage("Download " + std::to_string(int(percent * 100.0f)) + "% complete...", false);
	}
}


void LoginRequestResponseCallback(bool success, int inboxCount, int notificationCount)
{
	LoginMenuNode->SetVisible(!success);
	MainProgramUINode->SetVisible(success);

	PasswordEditBox->SetText("");
	SetStatusBarMessage(success ? "Successfully logged in to server!" : "Failed to log in to server. Try again.", !success);
	SetInboxMessageCount(inboxCount);
	SetNotificationCount(notificationCount);
}

void FileRequestSucceeded(std::string fileID)
{
	SetStatusBarMessage("File download request success: [" + fileID + "] downloading now...", false);
}

void FileRequestFailureCallback(std::string fileID, std::string failureReason)
{
	SetStatusBarMessage("File download request failed: [" + fileID + "] " + failureReason, true);
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
	void LoadMainProgramUI();
	void LoadSideBarUI();
	void LoadLatestUploadsUI();

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
	LoadMainProgramUI();
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

	//  Load the status bar download fill, for showing download progress
	StatusBarDownloadFill = GUIObjectNode::CreateObjectNode("./Assets/Textures/Pixel_TransparentDarkRed.png");
	StatusBarDownloadFill->SetObjectName("StatusBarDownloadFill");
	StatusBarDownloadFill->SetDimensions(0, backgroundStripHeight);
	StatusBarDownloadFill->SetPosition(0, backgroundStripY);
	StatusBarNode->AddChild(StatusBarDownloadFill);

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
}


void PrimaryDialogue::LoadMainProgramUI()
{
	if (MainProgramUINode != nullptr) return;
	MainProgramUINode = GUIObjectNode::CreateObjectNode("");
	MainProgramUINode->SetVisible(false);
	AddChild(MainProgramUINode);

	//  Create the individual UI pieces
	LoadLatestUploadsUI();
	LoadSideBarUI();

	//  Set the Client control callbacks
	ClientControl.SetLoginResponseCallback(LoginRequestResponseCallback);
	ClientControl.SetInboxAndNotificationCountCallback(InboxAndNotificationsCountCallback);
	ClientControl.SetLatestUploadsCallback(SetLatestUploads);
	ClientControl.SetFileRequestFailureCallback(FileRequestFailureCallback);
	ClientControl.SetFileRequestSuccessCallback(FileRequestSucceeded);
	ClientControl.SetDownloadPercentCompleteCallback(SetDownloadPercentage);
}

void PrimaryDialogue::LoadSideBarUI()
{
	//  Load the background strip behind the status bar
	SideBarBox = GUIObjectNode::CreateObjectNode("./Assets/Textures/Pixel_White.png");
	SideBarBox->SetColor(0.25f, 0.25f, 0.25f, 1.0f);
	SideBarBox->SetDimensions(SideBarWidth, int(ScreenHeight) - StatusBarBG->GetHeight());
	SideBarBox->SetPosition(0, 0);
	SideBarBox->SetVisible(false);
	MainProgramUINode->AddChild(SideBarBox);

	//  Load the side-bar user tab
	UserSideTabButton = GUIButton::CreateButton("./Assets/Textures/MainProgramUI/SideBarTab.png", 0, 60, 40, 40);
	UserSideTabButton->SetPressedSizeRatio(1.0f);
	UserSideTabButton->SetLeftClickCallback(SetUserMenuOpen);
	MainProgramUINode->AddChild(UserSideTabButton);

	//  Load the side-bar user icon
	UserIconNode = GUIObjectNode::CreateObjectNode("./Assets/Textures/MainProgramUI/UserIcon.png");
	UserIconNode->SetDimensions(24, 24);
	UserIconNode->SetColor(0.6f, 0.6f, 0.6f, 1.0f);
	UserIconNode->SetPosition((UserSideTabButton->GetWidth() / 2) - (UserIconNode->GetWidth() / 2), (UserSideTabButton->GetHeight() / 2) - (UserIconNode->GetHeight() / 2));
	UserSideTabButton->AddChild(UserIconNode);

	//  Load the side-bar inbox tab
	InboxSideTabButton = GUIButton::CreateButton("./Assets/Textures/MainProgramUI/SideBarTab.png", 0, 110, 40, 40);
	InboxSideTabButton->SetPressedSizeRatio(1.0f);
	InboxSideTabButton->SetLeftClickCallback(SetInboxOpen);
	MainProgramUINode->AddChild(InboxSideTabButton);

	//  Load the side-bar inbox icon
	InboxIconNode = GUIObjectNode::CreateObjectNode("");
	InboxIconNode->SetDimensions(24, 24);
	InboxIconNode->SetPosition((InboxSideTabButton->GetWidth() / 2) - (InboxIconNode->GetWidth() / 2), (InboxSideTabButton->GetHeight() / 2) - (InboxIconNode->GetHeight() / 2));
	InboxSideTabButton->AddChild(InboxIconNode);
	SetInboxMessageCount(0);

	//  Load the side-bar notifications tab
	NotificationsSideTabButton = GUIButton::CreateButton("./Assets/Textures/MainProgramUI/SideBarTab.png", 0, 160, 40, 40);
	NotificationsSideTabButton->SetPressedSizeRatio(1.0f);
	NotificationsSideTabButton->SetLeftClickCallback(SetNotificationsOpen);
	MainProgramUINode->AddChild(NotificationsSideTabButton);

	//  Load the side-bar inbox icon
	NotificationsIconNode = GUIObjectNode::CreateObjectNode("");
	NotificationsIconNode->SetDimensions(24, 24);
	NotificationsIconNode->SetPosition((NotificationsSideTabButton->GetWidth() / 2) - (NotificationsIconNode->GetWidth() / 2), (NotificationsSideTabButton->GetHeight() / 2) - (NotificationsIconNode->GetHeight() / 2));
	NotificationsSideTabButton->AddChild(NotificationsIconNode);
	SetNotificationCount(0);
}


void PrimaryDialogue::LoadLatestUploadsUI()
{
	//  Load the background strip behind the status bar
	auto latestUploadsContainer = GUIObjectNode::CreateObjectNode("./Assets/Textures/Pixel_White.png");
	latestUploadsContainer->SetColor(0.4f, 0.4f, 0.7f, 1.0f);
	latestUploadsContainer->SetDimensions(LatestUploadsWidth, LatestUploadsHeight);
	latestUploadsContainer->SetPosition(80, 40);
	MainProgramUINode->AddChild(latestUploadsContainer);

	auto latestUploadsTitle = GUILabel::CreateLabel(fontManager.GetFont("Arial"), "Latest Uploaded Files:", 10, 10, LatestUploadsWidth - 20, 30);
	latestUploadsTitle->SetColor(0.2f, 0.2f, 0.2f, 1.0f);
	latestUploadsContainer->AddChild(latestUploadsTitle);

	LatestUploadsListBox = GUIListBox::CreateTemplatedListBox("Standard", 5, 30, LatestUploadsWidth - 10, LatestUploadsHeight - 60, LatestUploadsWidth - 22, 2, 12, 12, 12, 12, 12, 24, 2);
	latestUploadsContainer->AddChild(LatestUploadsListBox);
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