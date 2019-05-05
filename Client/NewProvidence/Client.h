#pragma once

#include <filesystem>
#include "Groundfish.h"
#include "Engine/WinsockWrapper.h"
#include "Engine/SimpleMD5.h"
#include "Engine/SimpleSHA256.h"
#include "Engine/StringTools.h"
#include "FileSendAndReceive.h"
#include "HostedFileData.h"

constexpr auto VERSION_NUMBER			= "2019.03.02";
constexpr auto NEW_PROVIDENCE_IP		= "98.181.188.165";
constexpr auto NEW_PROVIDENCE_PORT		= 2347;

//  Outgoing message send functions
void SendMessage_PingResponse(int socket)
{
	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_PING_RESPONSE, 0);
	winsockWrapper.SendMessagePacket(socket, NEW_PROVIDENCE_IP, NEW_PROVIDENCE_PORT, 0);
}

void SendMessage_UserLoginRequest(EncryptedData& encryptedUsername, EncryptedData& encryptedPassword, int socket)
{
	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_USER_LOGIN_REQUEST, 0);
	winsockWrapper.WriteString(VERSION_NUMBER, 0);
	winsockWrapper.WriteInt(int(encryptedUsername.size()), 0);
	winsockWrapper.WriteChars(encryptedUsername.data(), int(encryptedUsername.size()), 0);
	winsockWrapper.WriteInt(int(encryptedPassword.size()), 0);
	winsockWrapper.WriteChars(encryptedPassword.data(), int(encryptedPassword.size()), 0);
	winsockWrapper.SendMessagePacket(socket, NEW_PROVIDENCE_IP, NEW_PROVIDENCE_PORT, 0);
}

void SendMessage_RequestHostedFileList(int startingIndex, int socket, EncryptedData username = EncryptedData(), HostedFileType type = FILE_TYPE_COUNT, HostedFileSubtype subtype = FILE_SUBTYPE_COUNT)
{
	//  Send a "Hosted File List Request" message
	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_REQUEST_HOSTED_FILE_LIST, 0);

	//  Define the user to request uploads from
	winsockWrapper.WriteUnsignedShort((uint16_t)(username.size()), 0);
	winsockWrapper.WriteChars(username.data(), username.size(), 0);

	//  Define the type and sub-type to filter for
	winsockWrapper.WriteChar(type, 0);
	winsockWrapper.WriteChar(subtype, 0);

	//  Define the starting index to begin the list at
	winsockWrapper.WriteUnsignedShort(uint16_t(startingIndex), 0);

	winsockWrapper.SendMessagePacket(socket, NEW_PROVIDENCE_IP, NEW_PROVIDENCE_PORT, 0);
}

void SendMessage_FileRequest(std::string fileID, int socket, const char* ip)
{
	//  Send a "File Request" message
	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_FILE_REQUEST, 0);
	winsockWrapper.WriteInt(fileID.length(), 0);
	winsockWrapper.WriteChars((unsigned char*)fileID.c_str(), fileID.length(), 0);
	winsockWrapper.SendMessagePacket(socket, NEW_PROVIDENCE_IP, NEW_PROVIDENCE_PORT, 0);
}

struct HostedFileEntry
{
	HostedFileType FileType;
	HostedFileSubtype FileSubType;
	std::string FileTitle;
	std::string FileUploader;

	HostedFileEntry(HostedFileType type, HostedFileSubtype subtype, std::string title, std::string uploader) :
		FileType(type),
		FileSubType(subtype),
		FileTitle(title),
		FileUploader(uploader)
	{}
};

class Client
{
private:
	int						ServerSocket = -1;
	FileEncryptTask*		FileEncrypt = nullptr;
	std::vector<FileDecryptTask*> FileDecryptList;
	FileReceiveTask*		FileReceive = nullptr;
	FileSendTask*			FileSend = nullptr;
	EncryptedData			EncryptedUsername;

	std::function<void(int, int, int)> LoginResponseCallback = nullptr;
	std::function<void(int, int)> InboxAndNotificationCountCallback = nullptr;
	std::function<void(std::vector<HostedFileEntry>)> LatestUploadsCallback = nullptr;
	std::function<void(std::string, std::string)> FileRequestFailureCallback = nullptr;
	std::function<void(std::string)> FileSendFailureCallback = nullptr;
	std::function<void(std::string)> FileRequestSuccessCallback = nullptr;

	std::vector<HostedFileEntry> HostedFilesList;

public:
	static Client& GetInstance() { static Client INSTANCE; return INSTANCE; }

	Client()	{}
	~Client()	{ Shutdown(); }

	inline int GetServerSocket(void) const { return ServerSocket; }
	inline EncryptedData GetUsername(void) const { return EncryptedUsername; }
	inline void SetLoginResponseCallback(const std::function<void(int, int, int)>& callback) { LoginResponseCallback = callback; }
	inline void SetInboxAndNotificationCountCallback(const std::function<void(int, int)>& callback) { InboxAndNotificationCountCallback = callback; }
	inline void SetLatestUploadsCallback(const std::function<void(std::vector<HostedFileEntry>)>& callback) { LatestUploadsCallback = callback; }
	inline void SetFileRequestFailureCallback(const std::function<void(std::string, std::string)>& callback) { FileRequestFailureCallback = callback; }
	inline void SetFileSendFailureCallback(const std::function<void(std::string)>& callback) { FileSendFailureCallback = callback; }
	inline void SetFileRequestSuccessCallback(const std::function<void(std::string)>& callback) { FileRequestSuccessCallback = callback; }
	inline void SetUsername(const EncryptedData& username) { EncryptedUsername = username; }

	inline void AddFileEncryptTask(std::string taskName, std::string unencryptedFileName, std::string encryptedFileName)
	{
		assert(FileEncrypt == nullptr);
		FileEncrypt = new FileEncryptTask(taskName, unencryptedFileName, encryptedFileName);
	}

	inline void AddFileDecryptTask(std::string taskName, std::string encryptedFileName, std::string unencryptedFileName)
	{
		auto decrypt = new FileDecryptTask(taskName, encryptedFileName, unencryptedFileName, true);
		FileDecryptList.push_back(decrypt);
	}

	inline void AddFileSendTask(std::string fileName, std::string fileTitle, std::string filePath, HostedFileType fileTypeID, HostedFileSubtype fileSubTypeID, int socketID, std::string ipAddress, const int port, bool deleteAfter = false)
	{
		assert(FileSend == nullptr);
		FileSend = new FileSendTask(fileName, fileTitle, filePath, fileTypeID, fileSubTypeID, socketID, ipAddress, port, deleteAfter);
	}

	bool Connect(void);

	inline bool IsFileBeingSent(void) const { return ((FileEncrypt != nullptr) || (FileSend != nullptr)); }
	inline bool IsFileBeingReceived(void) const { return ((!FileDecryptList.empty()) || (FileReceive != nullptr)); }

	void AddLatestUpload(int index, std::string upload, std::string uploader, HostedFileType type, HostedFileSubtype subtype);
	void DetectFilesInUploadFolder(std::string folder, std::vector<std::wstring>& fileList);
	void SendFileToServer(std::string fileName, std::string filePath, std::string fileTitle, HostedFileType fileTypeID, HostedFileSubtype fileSubTypeID);
	void ContinueFileEncryptions(void);
	void ContinueFileTransfers(void);
	void CancelFileSend(void);

	void Initialize(void);
	bool MainProcess(void);
	void Shutdown(void);

	bool ReadMessages(void);
};


bool Client::Connect(void)
{
	// Connect to the New Providence server
	ServerSocket = winsockWrapper.TCPConnect(NEW_PROVIDENCE_IP, NEW_PROVIDENCE_PORT, 1);
	return (ServerSocket >= 0);
}


void Client::AddLatestUpload(int index, std::string fileTitle, std::string uploader, HostedFileType type, HostedFileSubtype subtype)
{
	for (auto iter = HostedFilesList.begin(); iter != HostedFilesList.end(); ++iter)
		if ((*iter).FileTitle.compare(fileTitle) == 0) return;

	auto newEntry = HostedFileEntry(type, subtype, fileTitle, uploader);

	if (int(HostedFilesList.size()) > index)
	{
		auto iter = HostedFilesList.begin();
		for (int i = 0; i < index; ++i) iter++;
		HostedFilesList.insert(iter, newEntry);
	}
	else HostedFilesList.push_back(newEntry);
}


void Client::DetectFilesInUploadFolder(std::string folder, std::vector<std::wstring>& fileList)
{
	for (const auto& entry : std::filesystem::directory_iterator(folder))
	{
		auto path = entry.path().wstring();
		fileList.push_back(path);
	}
}


void Client::SendFileToServer(std::string fileName, std::string filePath, std::string fileTitle, HostedFileType fileTypeID, HostedFileSubtype fileSubTypeID)
{
	if (IsFileBeingSent()) return;

	auto titleMD5 = md5(fileTitle);
	auto hostedFileName = titleMD5 + ".hostedfile";

	//  Add a new FileEncryptTask to our list, so it can manage itself
	AddFileEncryptTask(fileTitle, filePath, hostedFileName);

	//  Add a new FileSendTask to our list, so it can manage itself (note: this will not start until the FileEncryptTask finishes
	AddFileSendTask(fileName, fileTitle, hostedFileName, fileTypeID, fileSubTypeID, GetServerSocket(), NEW_PROVIDENCE_IP, NEW_PROVIDENCE_PORT, true);
}


void Client::ContinueFileEncryptions(void)
{
	if (FileEncrypt != nullptr)
	{
		auto encryptComplete = FileEncrypt->Update();

		auto cryptEvent = FileCryptProgressEventData(FileEncrypt->TaskName, FileEncrypt->EncryptionPercentage, "Encrypt", "Client");
		eventManager.BroadcastEvent(&cryptEvent);

		if (encryptComplete)
		{
			delete FileEncrypt;
			FileEncrypt = nullptr;

#if FILE_TRANSFER_DEBUGGING
			debugConsole->AddDebugConsoleLine("FileEncryptTask deleted...");
#endif
		}
	}

	if (!FileDecryptList.empty())
	{
		for (auto task = FileDecryptList.begin(); task != FileDecryptList.end(); ++task)
		{
			auto decryptComplete = (*task)->Update();

			auto cryptEvent = FileCryptProgressEventData((*task)->TaskName, (*task)->DecryptionPercentage, "Decrypt", "Client");
			eventManager.BroadcastEvent(&cryptEvent);

			if (decryptComplete)
			{
				delete (*task);
				FileDecryptList.erase(task);

#if FILE_TRANSFER_DEBUGGING
				debugConsole->AddDebugConsoleLine("FileDecryptTask deleted...");
#endif

				//  Just break out and come back to this funtion next loop... don't worry about iterating over the erase
				break;
			}
		}
	}
}


void Client::ContinueFileTransfers(void)
{
	//  If we're currently encrypting a file, do not continue a file transfer
	if (FileEncrypt != nullptr) return;

	// If there is a file send task, send a file chunk for each one if the file transfer is ready
	if (FileSend == nullptr) return;

	//  If we haven't "started" the file send, do so now and return out
	if (FileSend->FileSendStarted == false)
	{
		FileSend->StartFileSend();
		return;
	}

	//  If we aren't sending the file yet, continue out and wait for a ready signal
	if (FileSend->GetFileTransferState() == FileSendTask::CHUNK_STATE_INITIALIZING) return;

	FileSend->SetFileTransferEndTime(gameSeconds);

	//  Create the file progress event, but don't broadcast yet
	auto fileProgressEvent = FileTransferProgressEventData(FileSend->GetFileTitle(), FileSend->GetPercentageComplete(), FileSend->GetTransferTime(), FileSend->GetFileSize(), FileSend->GetEstimatedSecondsRemaining(), "Upload", "Client");

	if (FileSend->GetFileTransferComplete())
	{
		//  If the file send is complete, delete the file send task and move on
		delete FileSend;
		FileSend = nullptr;

#if FILE_TRANSFER_DEBUGGING
		debugConsole->AddDebugConsoleLine("FileSendTask deleted...");
#endif
	}

	//  Broadcast the progress event after checking for a complete transfer, to avoid an overlap of FileSendTasks
	eventManager.BroadcastEvent(&fileProgressEvent);

	//  If we get this far, we have data to send. Send it and continue out
	if (FileSend != nullptr) FileSend->SendFileChunk();
}


void Client::CancelFileSend(void)
{
	delete FileSend;
	FileSend = nullptr;
}


void Client::Initialize(void)
{
	//  Seed the random number generator
	srand((unsigned int)(time(NULL)));

	//  Load the current Groundfish word list
	Groundfish::LoadCurrentWordList();

	//  Initialize the Winsock wrapper
	winsockWrapper.WinsockInitialize();
}

bool Client::MainProcess(void)
{
	if (ServerSocket < 0) return false;

	// Receive messages
	ReadMessages();

	//  Encrypt files
	ContinueFileEncryptions();

	//  Send files
	ContinueFileTransfers();

	return true;
}

void Client::Shutdown(void)
{
	if (ServerSocket == -1) return;
	closesocket(ServerSocket);
	ServerSocket = -1;
}

bool Client::ReadMessages(void)
{
	auto messageBufferSize = winsockWrapper.ReceiveMessagePacket(ServerSocket, 0);
	if (messageBufferSize == 0) return false;
	if (messageBufferSize < 0) return true;

	auto messageID = winsockWrapper.ReadChar(0);
	switch (messageID)
	{
	case MESSAGE_ID_PING_REQUEST:
	{
		SendMessage_PingResponse(ServerSocket);
	}
	break;

	case MESSAGE_ID_ENCRYPTED_CHAT_STRING:
	{
		int messageSize = winsockWrapper.ReadInt(0);

		//  Decrypt using Groundfish
		unsigned char encryptedChatString[256];
		memcpy(encryptedChatString, winsockWrapper.ReadChars(0, messageSize), messageSize);
		auto chatString = Groundfish::DecryptToString(encryptedChatString);
		//NewLine(chatString);
	}
	break;

	case MESSAGE_ID_USER_LOGIN_RESPONSE:
	{
		auto response = winsockWrapper.ReadInt(0);
		bool success = (response == LOGIN_RESPONSE_SUCCESS);
		auto inboxCount = success ? winsockWrapper.ReadInt(0) : 0;
		auto notificationCount = success ? winsockWrapper.ReadInt(0) : 0;
		if (LoginResponseCallback != nullptr) LoginResponseCallback(response, inboxCount, notificationCount);
	}
	break;

	case MESSAGE_ID_USER_INBOX_AND_NOTIFICATIONS:
	{
		auto inboxCount = winsockWrapper.ReadInt(0);
		for (auto i = 0; i < inboxCount; ++i)
		{
			//  TODO: Read inbox item data
		}

		char* notification;
		auto notificationsCount = winsockWrapper.ReadInt(0);
		for (auto i = 0; i < notificationsCount; ++i)
		{
			auto notificationLength = winsockWrapper.ReadInt(0);
			notification = (char*)winsockWrapper.ReadChars(0, notificationLength);
		}

		if (InboxAndNotificationCountCallback != nullptr) InboxAndNotificationCountCallback(0, notificationsCount);
	}
	break;

	case MESSAGE_ID_HOSTED_FILE_LIST:
	{
		auto uploadsStartIndex = int(winsockWrapper.ReadUnsignedShort(0));
		auto latestUploadCount = int(winsockWrapper.ReadUnsignedShort(0));
		HostedFilesList.clear();
		for (auto i = 0; i < latestUploadCount; ++i)
		{
			//  The file type and subtype
			auto type = HostedFileType(winsockWrapper.ReadChar(0));
			auto subtype = HostedFileSubtype(winsockWrapper.ReadChar(0));

			//  The encrypted file title
			auto ftSize = int(winsockWrapper.ReadChar(0));
			assert(ftSize != 0);
			auto ftData = winsockWrapper.ReadChars(0, ftSize);
			auto ftDataVector = EncryptedData(ftData, ftData + ftSize);
			auto decryptedTitleString = Groundfish::DecryptToString(ftDataVector.data());

			//  The encrypted file uploader
			auto fuSize = int(winsockWrapper.ReadChar(0));
			assert(fuSize != 0);
			auto fuData = winsockWrapper.ReadChars(0, fuSize);
			auto fuDataVector = EncryptedData(fuData, fuData + fuSize);
			auto decryptedUploaderString = Groundfish::DecryptToString(fuDataVector.data());

			AddLatestUpload(uploadsStartIndex++, decryptedTitleString, decryptedUploaderString, type, subtype);
		}

		if (LatestUploadsCallback != nullptr) LatestUploadsCallback(HostedFilesList);
	}
	break;


	case MESSAGE_ID_FILE_REQUEST_FAILED:
	{
		auto failedFileID = std::string(winsockWrapper.ReadString(0));
		auto failureReason = std::string(winsockWrapper.ReadString(0));
		if (FileRequestFailureCallback != nullptr) FileRequestFailureCallback(failedFileID, failureReason);
	}
	break;


	case MESSAGE_ID_FILE_SEND_INIT:
	{
		int fileNameSize = winsockWrapper.ReadInt(0);
		int fileTitleSize = winsockWrapper.ReadInt(0);
		int fileDescriptionSize = winsockWrapper.ReadInt(0);

		//  Decrypt the filename using Groundfish
		auto decryptedFileNamePure = Groundfish::DecryptToString(winsockWrapper.ReadChars(0, fileNameSize));
		auto decryptedFilename = "./_DownloadedFiles/" + decryptedFileNamePure;
		auto tempFilename = std::string(decryptedFilename) + std::string(".tempfile");

		//  Decrypt the filename using Groundfish
		auto decryptedFileTitle = Groundfish::DecryptToString(winsockWrapper.ReadChars(0, fileTitleSize));

		//  Decrypt the filename using Groundfish
		auto decryptedFileDescription = Groundfish::DecryptToString(winsockWrapper.ReadChars(0, fileDescriptionSize));

		//  Grab the file type and sub-type
		auto fileTypeID = HostedFileType(winsockWrapper.ReadUnsignedShort(0));
		auto fileSubTypeID = HostedFileSubtype(winsockWrapper.ReadUnsignedShort(0));

		//  Grab the file size, file chunk size, and buffer count
		auto fileSize = winsockWrapper.ReadLongInt(0);
		auto fileChunkSize = winsockWrapper.ReadLongInt(0);
		auto FileChunkBufferSize = winsockWrapper.ReadLongInt(0);

		//  Create a new file receive task
		(void)_wmkdir(L"_DownloadedFiles");
		FileReceive = new FileReceiveTask(decryptedFilename, decryptedFileTitle, decryptedFileDescription, fileTypeID, fileSubTypeID, fileSize, fileChunkSize, FileChunkBufferSize, tempFilename, 0, NEW_PROVIDENCE_IP, NEW_PROVIDENCE_PORT);
		FileReceive->SetDecryptWhenReceived(true);

		//  Respond to the file request success
		if (FileRequestSuccessCallback != nullptr) FileRequestSuccessCallback(decryptedFileNamePure);
	}
	break;

	case MESSAGE_ID_FILE_SEND_FAILED:
	{
		auto failureReason = std::string(winsockWrapper.ReadString(0));
		if (FileSendFailureCallback != nullptr) FileSendFailureCallback(failureReason);
	}
	break;

	case MESSAGE_ID_FILE_RECEIVE_READY:
	{
		assert(FileSend != nullptr);

		FileSend->SetFileTransferState(FileSendTask::CHUNK_STATE_SENDING);
	}
	break;

	case MESSAGE_ID_FILE_PORTION:
	{
		if (FileReceive == nullptr) break;

		if (FileReceive->ReceiveFileChunk())
		{
			assert(false); // ALERT: We should never be receiving a chunk after the file transfer is complete

			//  If ReceiveFile returns true, the transfer is complete
			delete FileReceive;
			FileReceive = nullptr;

#if FILE_TRANSFER_DEBUGGING
			debugConsole->AddDebugConsoleLine("File Receive Task deleted...");
#endif
		}
	}
	break;

	case MESSAGE_ID_FILE_PORTION_COMPLETE:
	{
		auto portionIndex = winsockWrapper.ReadLongInt(0);

		if (FileReceive == nullptr)
		{
			SendMessage_FilePortionCompleteConfirmation(portionIndex, ServerSocket, NEW_PROVIDENCE_IP, NEW_PROVIDENCE_PORT);
			break;
		}

		(void) _wmkdir(L"_DownloadedFiles");
		if (FileReceive->CheckFilePortionComplete(portionIndex))
		{
			FileReceive->SetFileTransferEndTime(gameSecondsF);

			if (FileReceive->GetFileTransferComplete())
			{
				if (FileReceive->GetDecryptWhenRecieved())
					AddFileDecryptTask(FileReceive->GetFileTitle(), FileReceive->GetTemporaryFileName(), FileReceive->GetFileName());

				delete FileReceive;
				FileReceive = nullptr;

#if FILE_TRANSFER_DEBUGGING
				debugConsole->AddDebugConsoleLine("File Receive Task deleted...");
#endif
				break;
			}
		}
	}
	break;

	case MESSAGE_ID_FILE_CHUNKS_REMAINING:
	{
		assert(FileSend != nullptr);

		auto chunkCount = winsockWrapper.ReadInt(0);
		std::unordered_map<int, bool> chunksRemaining;
		for (auto i = 0; i < chunkCount; ++i) chunksRemaining[winsockWrapper.ReadShort(0)] = true;
		FileSend->SetChunksRemaining(chunksRemaining);
	}
	break;

	case MESSAGE_ID_FILE_PORTION_COMPLETE_CONFIRM:
	{
		auto portionIndex = winsockWrapper.ReadLongInt(0);

		if (FileSend == nullptr) break;
		if (FileSend->GetFileTransferState() != FileSendTask::CHUNK_STATE_PENDING_COMPLETE) break;

		FileSend->ConfirmFilePortionSendComplete(portionIndex);
	}
	break;
	}

	return true;
}