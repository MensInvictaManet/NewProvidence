#pragma once

#include "Engine/GUIObjectNode.h"
#include "Engine/GUILabel.h"
#include "Engine/GUIListBox.h"
#include "Client.h"
#include <vector>
#include <unordered_map>
#include <string>


struct FileUploadData
{
	std::string FileName = "";
	std::string FilePath = "";
	std::string FileTitle = "";
	HostedFileType FileTypeID = FILE_TYPE_COUNT;
	HostedFileSubtype FileSubTypeID = FILE_SUBTYPE_COUNT;

	FileUploadData() {}

	FileUploadData(std::string fileName, std::string filePath, std::string fileTitle, HostedFileType fileTypeID, HostedFileSubtype fileSubTypeID) :
		FileName(fileName),
		FilePath(filePath),
		FileTitle(fileTitle),
		FileTypeID(fileTypeID),
		FileSubTypeID(fileSubTypeID)
	{}

	FileUploadData& operator=(const FileUploadData& other) // copy assignment
	{
		if (this != &other) { // self-assignment check expected
			FileName = other.FileName;
			FilePath = other.FilePath;
			FileTitle = other.FileTitle;
			FileTypeID = other.FileTypeID;
			FileSubTypeID = other.FileSubTypeID;
		}
		return *this;
	}
};


class FileTransfersDialogue : public GUIObjectNode, EventListener
{
public:
	static FileTransfersDialogue* GetInstance() { static FileTransfersDialogue* INSTANCE = new FileTransfersDialogue; return INSTANCE; }

	FileTransfersDialogue();
	~FileTransfersDialogue();

	virtual void Update();
	void Shutdown();

	void LoadTransfersMenuUI();
	void OpenTransfersMenu();

	inline void SetUIVisible() { MenuUINode->SetVisible(true); }
	inline void SetUIHidden() { MenuUINode->SetVisible(false); }

	void AddDownloadToQueue(std::string fileTitle, HostedFileType fileTypeIndex, HostedFileSubtype fileSubTypeIndex);
	void AddUploadToQueue(FileUploadData& fileUploadData);
	void RemoveDownloadFromQueue(std::string fileTitle);
	void RemoveUploadFromQueue(std::string fileTitle);

	bool UpdateDownload(std::string downloadName, double progress, Color& barColor);
	void UpdateUpload(std::string downloadName, double progress, Color& barColor);

private:
	virtual void ReceiveEvent(EventData* eventData) override;

	void AddTransferEntryToList(GUIListBox* listbox, std::string entryTitle, HostedFileType fileTypeID, HostedFileSubtype fileSubtypeID);
	void RemoveDownloadFromQueueUI(std::string fileTitle);
	void RemoveUploadFromQueueUI(std::string fileTitle);

	GUIObjectNode* MenuUINode = nullptr;

	std::unordered_map<std::string, bool> QueuedDownloadsMap;
	std::vector<std::string> QueuedDownloadsList;
	std::unordered_map<std::string, FileUploadData> QueuedUploadsMap;
	std::vector<std::string> QueuedUploadsList;

	GUIListBox* DownloadQueueListBox;
	GUIListBox* UploadQueueListBox;
};


void SetTransferPercentage(std::string fileTitle, double progress, double totalTime, uint64_t fileSize, uint64_t timeRemaining, std::string transferType)
{
	if (fileTitle.empty()) return;

	int averageKBs = int((double(fileSize) / 1024.0) / totalTime);
	auto transfers = FileTransfersDialogue::GetInstance();
	auto download = (transferType.compare("Download") == 0);

	auto minutesRemaining = timeRemaining / 60;
	auto secondsRemaining = timeRemaining % 60;
	auto timeString = (minutesRemaining != 0) ? (std::to_string(minutesRemaining) + " minutes") : std::to_string(secondsRemaining) + " seconds";

	if (download)	transfers->UpdateDownload(fileTitle, progress, COLOR_LIGHTRED);
	else			transfers->UpdateUpload(fileTitle, progress, (progress >= 1.0) ? COLOR_LIGHTGREEN : COLOR_LIGHTRED);
}


void SetCryptPercentage(std::string fileTitle, double percent, std::string cryptType)
{
	auto encryption = (cryptType.compare("Encrypt") == 0);
	auto transfers = FileTransfersDialogue::GetInstance();

	if (encryption)
	{
		if (!fileTitle.empty()) transfers->UpdateUpload(fileTitle, std::min<double>(1.0, percent), COLOR_LIGHTBLUE);
	}
	else
	{
		if (!fileTitle.empty()) transfers->UpdateDownload(fileTitle, std::min<double>(1.0, percent), (percent >= 1.0) ? COLOR_LIGHTGREEN : COLOR_LIGHTBLUE);
	}
}


FileTransfersDialogue::FileTransfersDialogue() :
	DownloadQueueListBox(nullptr),
	UploadQueueListBox(nullptr)
{
	LoadTransfersMenuUI();

	eventManager.AddEventListener("FileTransferProgress", this);
	eventManager.AddEventListener("FileCryptProgress", this);
}


FileTransfersDialogue::~FileTransfersDialogue()
{
	Shutdown();
}


inline void FileTransfersDialogue::Update()
{
	if (!QueuedUploadsList.empty() && !Client::GetInstance().IsFileBeingSent())
	{
		assert(QueuedUploadsMap.find(QueuedUploadsList[0]) != QueuedUploadsMap.end());
		auto uploadData = QueuedUploadsMap[QueuedUploadsList[0]];
		Client::GetInstance().SendFileToServer(uploadData.FileName, uploadData.FilePath, uploadData.FileTitle, uploadData.FileTypeID, uploadData.FileSubTypeID);
	}

	GUIObjectNode::Update();
}


inline void FileTransfersDialogue::Shutdown()
{
}


void FileTransfersDialogue::OpenTransfersMenu()
{
	//  Make the upload UI visible
	MenuUINode->SetVisible(true);
}

void FileTransfersDialogue::AddDownloadToQueue(std::string fileTitle, HostedFileType fileTypeID, HostedFileSubtype fileSubTypeID)
{
	auto queuedDownload = QueuedDownloadsMap.find(fileTitle);
	if (queuedDownload != QueuedDownloadsMap.end()) return;

	if (QueuedDownloadsList.empty())
		SendMessage_FileRequest(fileTitle, Client::GetInstance().GetServerSocket(), NEW_PROVIDENCE_IP);

	//  Add an entry in the QueuedDownloads map and list
	QueuedDownloadsMap[fileTitle] = true;
	QueuedDownloadsList.push_back(fileTitle);

	AddTransferEntryToList(DownloadQueueListBox, fileTitle, fileTypeID, fileSubTypeID);
}

void FileTransfersDialogue::AddUploadToQueue(FileUploadData& uploadData)
{
	auto queuedDownload = QueuedUploadsMap.find(uploadData.FileTitle);
	if (queuedDownload != QueuedUploadsMap.end()) return;

	//  Add an entry in the QueuedUploads map and list
	QueuedUploadsMap[uploadData.FileTitle] = uploadData;
	QueuedUploadsList.push_back(uploadData.FileTitle);

	AddTransferEntryToList(UploadQueueListBox, uploadData.FileTitle, uploadData.FileTypeID, uploadData.FileSubTypeID);
}

void FileTransfersDialogue::RemoveDownloadFromQueue(std::string fileTitle)
{
	auto queuedItem = QueuedDownloadsMap.find(fileTitle);
	if (queuedItem == QueuedDownloadsMap.end()) return;

	//  Erase the download queue item from both the map and the list
	QueuedDownloadsMap.erase(queuedItem);
	for (auto i = QueuedDownloadsList.begin(); i != QueuedDownloadsList.end(); ++i)
	{
		if ((*i) == fileTitle)
		{
			QueuedDownloadsList.erase(i);
			break;
		}
	}

	RemoveDownloadFromQueueUI(fileTitle);
}

void FileTransfersDialogue::RemoveUploadFromQueue(std::string fileTitle)
{
	auto queuedItem = QueuedUploadsMap.find(fileTitle);
	if (queuedItem == QueuedUploadsMap.end()) return;

	//  Erase the download queue item from both the map and the list
	QueuedUploadsMap.erase(queuedItem);
	for (auto i = QueuedUploadsList.begin(); i != QueuedUploadsList.end(); ++i)
	{
		if ((*i) == fileTitle)
		{
			QueuedUploadsList.erase(i);
			break;
		}
	}

	RemoveUploadFromQueueUI(fileTitle);
}


bool FileTransfersDialogue::UpdateDownload(std::string entryName, double progress, Color& barColor)
{
	auto entry = DownloadQueueListBox->GetItemByName(entryName);
	if (entry == nullptr)
	{
		debugConsole->AddDebugConsoleLine("Attempted to update download (" + entryName + ") but could not find it [" + getDoubleStringRounded(progress * 100, 2) + "%]");
		return false;
	}

	auto progressBar = static_cast<GUIProgressBar*>(entry->GetChildByName("ProgressBar"));
	if (progressBar == nullptr)
	{
		debugConsole->AddDebugConsoleLine("Attempted to update download (" + entryName + ") but could not locate it's ProgressBar");
		return false;
	}

	if (progressBar->GetProgress() != float(progress))
	{
		progressBar->SetBarColor(barColor);
		progressBar->SetVisible(true);
		progressBar->SetProgress(float(progress));

		if (progress >= 1.0 && barColor == COLOR_LIGHTGREEN)
		{
			this->RemoveDownloadFromQueue(entryName);
			if (!QueuedDownloadsList.empty())
			{
				SendMessage_FileRequest(QueuedDownloadsList[0], Client::GetInstance().GetServerSocket(), NEW_PROVIDENCE_IP);
				debugConsole->AddDebugConsoleLine("Attempting to begin the next queued download");
			}
		}
	}

	return true;
}


void FileTransfersDialogue::UpdateUpload(std::string entryName, double progress, Color& barColor)
{
	auto entry = UploadQueueListBox->GetItemByName(entryName);
	if (entry == nullptr)
	{
		debugConsole->AddDebugConsoleLine("Attempted to update upload (" + entryName + ") but could not find it");
		return;
	}

	auto progressBar = static_cast<GUIProgressBar*>(entry->GetChildByName("ProgressBar"));
	if (progressBar == nullptr)
	{
		debugConsole->AddDebugConsoleLine("Attempted to update upload (" + entryName + ") but could not locate it's ProgressBar");
		return;
	}

	progressBar->SetBarColor(barColor);
	progressBar->SetVisible(true);
	progressBar->SetProgress(float(progress));

	//  TODO: Fix this using the events system...
	if (progress >= 1.0 && barColor == COLOR_LIGHTGREEN)
	{
		this->RemoveUploadFromQueue(entryName);
		for (auto iter = QueuedUploadsList.begin(); iter != QueuedUploadsList.end();)
		{
			if (QueuedUploadsMap.find((*iter)) == QueuedUploadsMap.end())
			{
				debugConsole->AddDebugConsoleLine("Item \"" + (*iter) + " not found in QueuedUploadsMap");
				this->RemoveUploadFromQueueUI((*iter));
				iter = QueuedUploadsList.erase(iter);
				continue;
			}
			break;
		}
	}
}


void FileTransfersDialogue::ReceiveEvent(EventData* eventData)
{
	if (eventData->EventType.compare("FileTransferProgress") == 0)
	{
		auto fpEvent = dynamic_cast<FileTransferProgressEventData*>(eventData);
		SetTransferPercentage(fpEvent->FileTitle, fpEvent->Progress, fpEvent->TotalTime, fpEvent->FileSize, fpEvent->TimeRemaining, fpEvent->TransferType);
	}
	else if (eventData->EventType.compare("FileCryptProgress") == 0)
	{
		auto cpEvent = dynamic_cast<FileCryptProgressEventData*>(eventData);
		if (cpEvent != nullptr) SetCryptPercentage(cpEvent->FileTitle, cpEvent->Progress, cpEvent->CryptType);
	}
}


void FileTransfersDialogue::AddTransferEntryToList(GUIListBox* listbox, std::string entryTitle, HostedFileType fileTypeID, HostedFileSubtype fileSubtypeID)
{
	auto newListing = GUIObjectNode::CreateObjectNode("");
	newListing->SetObjectName(entryTitle);

	auto fileTypeIcon = GetFileTypeIconFromID(fileTypeID);
	fileTypeIcon->SetPosition(6, 6);
	fileTypeIcon->SetDimensions(20, 20);
	newListing->AddChild(fileTypeIcon);

	auto fileSubtypeIcon = GetFileSubTypeIconFromID(fileSubtypeID);
	fileSubtypeIcon->SetPosition(26, 6);
	fileSubtypeIcon->SetDimensions(20, 20);
	newListing->AddChild(fileSubtypeIcon);

	auto fileTitleLabel = GUILabel::CreateLabel("Arial-12-White", entryTitle.c_str(), 64, 8, 300, 20, UI_JUSTIFY_LEFT);
	fileTitleLabel->SetObjectName("FileTitle");
	newListing->AddChild(fileTitleLabel);

	auto fileStatusLabel = GUILabel::CreateLabel("Arial-12-White", "QUEUED", 460, 8, 300, 20, UI_JUSTIFY_LEFT);
	fileStatusLabel->SetObjectName("FileStatus");
	fileStatusLabel->SetVisible(true);
	newListing->AddChild(fileStatusLabel);

	auto fileProgressBar = GUIProgressBar::CreateTemplatedProgressBar("Standard", 600, 4, 280, 24);
	fileProgressBar->SetObjectName("ProgressBar");
	fileProgressBar->SetFont("Arial");
	fileProgressBar->SetShowStringWhenComplete(true, "COMPLETE");
	fileProgressBar->SetVisible(false);
	newListing->AddChild(fileProgressBar);

	//auto fileCancelButton = GUIButton::CreateTemplatedButton("Standard", 890, 4, 80, 24);
	//fileCancelButton->SetObjectName("CancelButton");
	//fileCancelButton->SetFont("Arial");
	//fileCancelButton->SetText("Cancel");
	//fileCancelButton->SetVisible(true);
	//newListing->AddChild(fileCancelButton);

	listbox->AddItem(newListing);
}


void FileTransfersDialogue::RemoveDownloadFromQueueUI(std::string fileTitle)
{
	auto listing = DownloadQueueListBox->GetItemByName(fileTitle);
	if (listing == nullptr) return;

	auto cancelButton = listing->GetChildByName("CancelButton");
	if (cancelButton) cancelButton->SetVisible(false);

	listing->SetObjectName("");
	//DownloadQueueListBox->RemoveItem(listing);
}


void FileTransfersDialogue::RemoveUploadFromQueueUI(std::string fileTitle)
{
	auto listing = UploadQueueListBox->GetItemByName(fileTitle);
	if (listing == nullptr) return;

	listing->SetObjectName("");
	//UploadQueueListBox->RemoveItem(listing);
}


void FileTransfersDialogue::LoadTransfersMenuUI()
{
	if (MenuUINode != nullptr) return;
	MenuUINode = GUIObjectNode::CreateObjectNode("");
	MenuUINode->SetVisible(false);
	AddChild(MenuUINode);

	auto currentDownloadsLabel = GUILabel::CreateLabel("Arial-12-White", "Current Download Queue:", 34, 92, 200, 20);
	currentDownloadsLabel->SetColorBytes(160, 160, 160, 255);
	MenuUINode->AddChild(currentDownloadsLabel);

	DownloadQueueListBox = GUIListBox::CreateTemplatedListBox("Standard", 32, 70, 1000, 300, 982, 0, 18, 18, 18, 18, 18, 24, 2);
	DownloadQueueListBox->SetColorBytes(87, 28, 87, 255);
	MenuUINode->AddChild(DownloadQueueListBox);

	UploadQueueListBox = GUIListBox::CreateTemplatedListBox("Standard", 32, 400, 1000, 300, 982, 0, 18, 18, 18, 18, 18, 24, 2);
	UploadQueueListBox->SetColorBytes(87, 28, 87, 255);
	MenuUINode->AddChild(UploadQueueListBox);
}