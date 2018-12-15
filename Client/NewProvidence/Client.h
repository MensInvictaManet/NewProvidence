#pragma once

#include "Groundfish.h"
#include "Engine/WinsockWrapper.h"
#include "Engine/SimpleMD5.h"
#include "Engine/SimpleSHA256.h"
#include "FileSendAndReceive.h"

#define VERSION_NUMBER					"2018.12.15"
#define NEW_PROVIDENCE_IP				"98.181.188.165"
#define NEW_PROVIDENCE_PORT				2347

//  Outgoing message send functions
void SendMessage_PingResponse(int socket)
{
	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_PING_RESPONSE, 0);
	winsockWrapper.SendMessagePacket(socket, NEW_PROVIDENCE_IP, NEW_PROVIDENCE_PORT, 0);
}

void SendMessage_UserLoginRequest(std::vector<unsigned char>& encryptedUsername, std::vector<unsigned char>& encryptedPassword, int socket)
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

void SendMessage_FileRequest(std::string fileID, int socket, const char* ip)
{
	//  Send a "File Request" message
	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_FILE_REQUEST, 0);
	winsockWrapper.WriteString(fileID.c_str(), 0);
	winsockWrapper.SendMessagePacket(socket, NEW_PROVIDENCE_IP, NEW_PROVIDENCE_PORT, 0);
}

class Client
{
private:
	int					ServerSocket = -1;
	std::string			UserName;
	FileReceiveTask*	FileReceive = NULL;
	FileSendTask*		FileSend = NULL;

	std::function<void(int, int, int)> LoginResponseCallback = nullptr;
	std::function<void(int, int)> InboxAndNotificationCountCallback = nullptr;
	std::function<void(std::vector<std::string>)> LatestUploadsCallback = nullptr;
	std::function<void(std::string, std::string)> FileRequestFailureCallback = nullptr;
	std::function<void(std::string)> FileRequestSuccessCallback = nullptr;
	std::function<void(float, double, int, bool)> TransferPercentCompleteCallback = nullptr;
	std::vector<std::string> LatestUploadsList;

public:
	Client()	{}
	~Client()	{ Shutdown(); }

	inline int GetServerSocket(void) const { return ServerSocket; }
	inline void SetLoginResponseCallback(const std::function<void(int, int, int)>& callback) { LoginResponseCallback = callback; }
	inline void SetInboxAndNotificationCountCallback(const std::function<void(int, int)>& callback) { InboxAndNotificationCountCallback = callback; }
	inline void SetLatestUploadsCallback(const std::function<void(std::vector<std::string>)>& callback) { LatestUploadsCallback = callback; }
	inline void SetFileRequestFailureCallback(const std::function<void(std::string, std::string)>& callback) { FileRequestFailureCallback = callback; }
	inline void SetFileRequestSuccessCallback(const std::function<void(std::string)>& callback) { FileRequestSuccessCallback = callback; }
	inline void SetTransferPercentCompleteCallback(const std::function<void(float, double, int, bool)>& callback) { TransferPercentCompleteCallback = callback; }

	bool Connect(void);
	void SendFileToServer(std::string filePath, std::string fileTitle);
	void ContinueFileTransfers(void);

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

void Client::SendFileToServer(std::string filePath, std::string fileTitle)
{
	if (FileSend != nullptr) return;

	//  Add a new FileSendTask to our list, so it can manage itself
	FileSend = new FileSendTask(filePath, fileTitle, filePath, ServerSocket, NEW_PROVIDENCE_IP, NEW_PROVIDENCE_PORT);
}

void Client::ContinueFileTransfers(void)
{
	// If there is a file send task, send a file chunk for each one if the file transfer is ready
	if (FileSend == nullptr) return;

	//  If we aren't ready to send the file, continue out and wait for a ready signal
	if (FileSend->GetFileTransferState() == FileSendTask::CHUNK_STATE_INITIALIZING) return;

	FileSend->SetFileTransferEndTime(gameSeconds);
	if (TransferPercentCompleteCallback != nullptr) TransferPercentCompleteCallback(FileSend->GetPercentageComplete(), FileSend->GetTransferTime(), FileSend->GetFileSize(), false);

	if (FileSend->GetFileTransferComplete())
	{
		//  If the file send is complete, delete the file send task and move on
		delete FileSend;
		FileSend = nullptr;

#if FILE_TRANSFER_DEBUGGING
		debugConsole->AddDebugConsoleLine("FileSendTask deleted...");
#endif
		return;
	}

	//  If we get this far, we have data to send. Send it and continue out
	FileSend->SendFileChunk();
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
		std::vector<unsigned char> descryptedChatString = Groundfish::Decrypt(encryptedChatString);
		std::string decryptedString((char*)descryptedChatString.data(), descryptedChatString.size());
		//NewLine(decryptedString);
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

	case MESSAGE_ID_LATEST_UPLOADS_LIST:
	{
		auto latestUploadCount = winsockWrapper.ReadInt(0);
		for (auto i = 0; i < latestUploadCount; ++i)
		{
			auto size = winsockWrapper.ReadInt(0);
			unsigned char* data = winsockWrapper.ReadChars(0, size);
			auto decryptedTitle = Groundfish::Decrypt(data);
			LatestUploadsList.push_back(std::string((char*)(decryptedTitle.data()), int(decryptedTitle.size())));
		}

		if (LatestUploadsCallback != nullptr) LatestUploadsCallback(LatestUploadsList);
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

		//  Decrypt using Groundfish and save as the filename
		auto decryptedFileNameVector = Groundfish::Decrypt(winsockWrapper.ReadChars(0, fileNameSize));
		auto decryptedFileNamePure = std::string((char*)decryptedFileNameVector.data(), decryptedFileNameVector.size());
		auto decryptedFilename = "./_DownloadedFiles/" + decryptedFileNamePure;

		//  Grab the file size, file chunk size, and buffer count
		auto fileSize = winsockWrapper.ReadInt(0);
		auto fileChunkSize = winsockWrapper.ReadInt(0);
		auto FileChunkBufferSize = winsockWrapper.ReadInt(0);

		//  Create a new file receive task
		(void)_wmkdir(L"_DownloadedFiles");
		FileReceive = new FileReceiveTask(decryptedFilename, fileSize, fileChunkSize, FileChunkBufferSize, "./_DownloadedFiles/_download.tempfile", 0, NEW_PROVIDENCE_IP, NEW_PROVIDENCE_PORT);

		//  Respond to the file request success
		if (FileRequestSuccessCallback != nullptr) FileRequestSuccessCallback(decryptedFileNamePure);
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
			debugConsole->AddDebugConsoleLine("FileReceiveTask deleted...");
#endif
		}
	}
	break;

	case MESSAGE_ID_FILE_PORTION_COMPLETE:
	{
		auto portionIndex = winsockWrapper.ReadInt(0);

		if (FileReceive == nullptr)
		{
			SendMessage_FilePortionCompleteConfirmation(portionIndex, ServerSocket, NEW_PROVIDENCE_IP, NEW_PROVIDENCE_PORT);
			break;
		}

		(void) _wmkdir(L"_DownloadedFiles");
		if (FileReceive->CheckFilePortionComplete(portionIndex))
		{
			FileReceive->SetFileTransferEndTime(gameSecondsF);
			if (TransferPercentCompleteCallback != nullptr) TransferPercentCompleteCallback(FileReceive->GetPercentageComplete(), FileReceive->GetTransferTime(), FileReceive->GetFileSize(), true);

			if (FileReceive->GetFileTransferComplete())
			{
				delete FileReceive;
				FileReceive = nullptr;

#if FILE_TRANSFER_DEBUGGING
				debugConsole->AddDebugConsoleLine("FileReceiveTask deleted...");
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
		auto portionIndex = winsockWrapper.ReadInt(0);

		if (FileSend == nullptr) break;
		if (FileSend->GetFileTransferState() != FileSendTask::CHUNK_STATE_PENDING_COMPLETE) break;

		FileSend->ConfirmFilePortionSendComplete(portionIndex);
	}
	break;
	}

	return true;
}