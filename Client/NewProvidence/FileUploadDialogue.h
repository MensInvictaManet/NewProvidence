#pragma once

#include "Engine/GUIObjectNode.h"
#include "Engine/GUILabel.h"
#include "Engine/GUIListBox.h"
#include "Engine/GUIEditBox.h"
#include "Engine/GUIDropDown.h"
#include "Engine/GUIButton.h"
#include "Engine/StringTools.h"
#include "Engine/DebugConsole.h"
#include "HostedFileData.h"
#include "FileSendAndReceive.h"
#include "Client.h"
#include <string>

GUIListBox* UploadFolderItemsListBox = nullptr;
GUILabel* UploadFileNameLabel = nullptr;
GUIEditBox* UploadFileTitleEditBox = nullptr;
GUIDropDown* UploadFileTypeDropDown = nullptr;
GUIDropDown* UploadFileSubTypeDropDown = nullptr;
std::string SelectedUploadFileName = "";

GUIObjectNode* UploadErrorBG = nullptr;
GUILabel* UploadErrorLabel = nullptr;

void UpdateUploadFolderList(void)
{
	if (UploadFolderItemsListBox == nullptr) return;
	UploadFolderItemsListBox->ClearItems();

	std::string uploadFolder("_FilesToUpload");
	std::vector<std::wstring> fileList;
	Client& client = Client::GetInstance();
	client.DetectFilesInUploadFolder(uploadFolder, fileList);

	auto entryX = UploadFolderItemsListBox->GetWidth() / 2;
	auto entryY = 12;
	auto entryH = UploadFolderItemsListBox->GetEntryHeight();
	auto entryW = UploadFolderItemsListBox->GetWidth();

	for (auto iter = fileList.begin(); iter != fileList.end(); ++iter)
	{
		auto shortName = ws2s(*iter).substr(uploadFolder.length() + 1, (*iter).length() - uploadFolder.length() - 1);

		if (s2ws(ws2s(*iter)).compare((*iter)) != 0)
		{
			debugConsole->AddDebugConsoleLine("File found with unrecognized characters in File Upload folder. File ignored: " + shortName);
			continue;
		}

		auto itemLabel = GUILabel::CreateLabel("Arial", shortName.c_str(), entryX, entryY, entryW, entryH, UI_JUSTIFY_CENTER);
		itemLabel->SetObjectName(shortName);
		UploadFolderItemsListBox->AddItem(itemLabel);
	}
}


void UpdateUploadSubTypeList(GUIObjectNode* object)
{
	UploadFileSubTypeDropDown->ClearItems();
	auto fileTypeNameSelected = ((GUILabel*)(UploadFileTypeDropDown->GetSelectedItem()))->GetText();
	auto fileSubTypeX = UploadFileSubTypeDropDown->GetWidth() / 2;
	auto fileSubTypeY = UploadFileSubTypeDropDown->GetHeight() / 2 - 6;
	auto fileSubTypeList = GetListOfSpecificFileSubTypes(fileTypeNameSelected);
	for (auto iter = fileSubTypeList.begin(); iter != fileSubTypeList.end(); ++iter)
		UploadFileSubTypeDropDown->AddItem(GUILabel::CreateLabel("Arial", (*iter).c_str(), fileSubTypeX, fileSubTypeY, 200, 20, UI_JUSTIFY_CENTER));
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


void SetUploadErrorMessage(std::string statusBarMessage, bool error = false)
{
	if (UploadErrorLabel == nullptr) return;

	UploadErrorLabel->SetText(statusBarMessage);

	if (error) UploadErrorBG->SetColor(0.8f, 0.1f, 0.1f, 1.0f);
	else UploadErrorBG->SetColor(0.1f, 0.4f, 0.7f, 0.0f);
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
		SetUploadErrorMessage("Upload Attempt Failed! File selected does not exist. Did you recently delete it?", true);
		return;
	}

	//  If there is no access to the given file title, or the title is blank, return out
	if (UploadFileTitleEditBox == nullptr) return;
	auto fileToUploadTitle = UploadFileTitleEditBox->GetText();
	if (fileToUploadTitle.length() == 0) return;
	if (fileToUploadTitle.length() > UPLOAD_TITLE_MAX_LENGTH)
	{
		SetUploadErrorMessage("Upload Attempt Failed! File title can not be longer than " + std::to_string(UPLOAD_TITLE_MAX_LENGTH) + " characters. Try again...", true);
		return;
	}

	//  Get the file type and sub-type, ensuring they match
	auto fileTypeString = ((GUILabel*)(UploadFileTypeDropDown->GetSelectedItem()))->GetText();
	auto fileSubTypeString = ((GUILabel*)(UploadFileSubTypeDropDown->GetSelectedItem()))->GetText();
	int fileTypeID = GetFileTypeIDFromName(fileTypeString);
	int fileSubTypeID = GetFileSubTypeIDFromName(fileSubTypeString);

	//  Attempt to start an upload of the file to the server
	SetUploadErrorMessage("", false);
	Client& client = Client::GetInstance();
	client.SendFileToServer(SelectedUploadFileName, fileToUpload, fileToUploadTitle, fileTypeID, fileSubTypeID);
}


class FileUploadDialogue : public GUIObjectNode
{
public:
	static FileUploadDialogue* GetInstance() { static FileUploadDialogue* INSTANCE = new FileUploadDialogue; return INSTANCE; }

	FileUploadDialogue();
	~FileUploadDialogue();

	virtual void Update();
	void Shutdown();

	void LoadUploadMenuUI();
	void OpenUploadMenu();

	inline void SetUIVisible() { UploadMenuUINode->SetVisible(true); }
	inline void SetUIHidden() { UploadMenuUINode->SetVisible(false); }

private:
	void LoadUploadErrorUI();

	GUIObjectNode* UploadMenuUINode = nullptr;
	GUIObjectNode* UploadErrorNode = nullptr;
};


FileUploadDialogue::FileUploadDialogue()
{
	LoadUploadMenuUI();
}


FileUploadDialogue::~FileUploadDialogue()
{
	Shutdown();
}


inline void FileUploadDialogue::Update()
{
	GUIObjectNode::Update();
}


inline void FileUploadDialogue::Shutdown()
{
}


void FileUploadDialogue::OpenUploadMenu()
{
	//  Make the upload UI visible
	UploadMenuUINode->SetVisible(true);

	//  Run a new detection of items in the uploads folder
	UpdateUploadFolderList();

	//  Set the upload error bar back to default
	SetUploadErrorMessage("", false);
}


void FileUploadDialogue::LoadUploadMenuUI()
{
	if (UploadMenuUINode != nullptr) return;
	UploadMenuUINode = GUIObjectNode::CreateObjectNode("");
	UploadMenuUINode->SetVisible(false);
	AddChild(UploadMenuUINode);

	LoadUploadErrorUI();

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
		UploadFileTypeDropDown->AddItem(GUILabel::CreateLabel("Arial", (*iter).c_str(), fileTypeX, fileTypeY, 200, 24, UI_JUSTIFY_CENTER));
	UploadFileTypeDropDown->SetItemSelectCallback(UpdateUploadSubTypeList);
	UploadMenuUINode->AddChild(UploadFileTypeDropDown);

	auto fileSubTypeLabel = GUILabel::CreateLabel("Arial-12-White", "File Type:", 530, 386, 200, 20);
	fileSubTypeLabel->SetColorBytes(160, 160, 160, 255);
	UploadMenuUINode->AddChild(fileSubTypeLabel);

	UploadFileSubTypeDropDown = GUIDropDown::CreateTemplatedDropDown("Standard", 824, 378, 420, 40, 386, 10, 16, 16);
	UpdateUploadSubTypeList(UploadFileSubTypeDropDown);
	UploadMenuUINode->AddChild(UploadFileSubTypeDropDown);
}


void FileUploadDialogue::LoadUploadErrorUI()
{
	if (UploadErrorNode != nullptr) return;
	UploadErrorNode = GUIObjectNode::CreateObjectNode("");
	UploadMenuUINode->AddChild(UploadErrorNode);

	//  Load the background strip behind the status bar
	auto uploadErrorBarMiddle = 880;
	auto uploadErrorBarWidth = 740;
	auto UploadErrorStripHeight = 24;
	auto uploadErrorStripBottom = 606;
	auto uploadErrorStripY = (uploadErrorStripBottom - UploadErrorStripHeight);
	UploadErrorBG = GUIObjectNode::CreateObjectNode("./Assets/Textures/Pixel_TransparentGray.png");
	UploadErrorBG->SetColor(0.1f, 0.4f, 0.7f, 1.0f);
	UploadErrorBG->SetDimensions(uploadErrorBarWidth, UploadErrorStripHeight);
	UploadErrorBG->SetPosition(uploadErrorBarMiddle - (uploadErrorBarWidth / 2), uploadErrorStripY);
	UploadErrorNode->AddChild(UploadErrorBG);

	//  Load the status bar text label
	UploadErrorLabel = GUILabel::CreateLabel(fontManager.GetFont("Arial-12-White"), "Test", -100, 0, uploadErrorBarWidth, UploadErrorStripHeight);
	UploadErrorLabel->SetPosition(uploadErrorBarMiddle, uploadErrorStripY + 5);
	UploadErrorLabel->SetJustification(UI_JUSTIFY_CENTER);
	UploadErrorNode->AddChild(UploadErrorLabel);
	SetUploadErrorMessage("", false);
}