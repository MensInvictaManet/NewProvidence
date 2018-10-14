#pragma once

#include "Groundfish.h"
#include "Engine/WinsockWrapper.h"
#include "Engine/SimpleMD5.h"
#include "Engine/SimpleSHA256.h"
#include "FileSendAndReceive.h"

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

	std::function<void(bool, int, int)> LoginResponseCallback = nullptr;
	std::function<void(int, int)> InboxAndNotificationCountCallback = nullptr;
	std::function<void(std::vector<std::string>)> LatestUploadsCallback = nullptr;
	std::function<void(std::string, std::string)> FileRequestFailureCallback = nullptr;
	std::function<void(std::string)> FileRequestSuccessCallback = nullptr;
	std::function<void(float, double, int)> DownloadPercentCompleteCallback = nullptr;
	std::vector<std::string> LatestUploadsList;

public:
	Client()	{}
	~Client()	{}

	inline int GetServerSocket(void) const { return ServerSocket; }
	inline void SetLoginResponseCallback(const std::function<void(bool, int, int)>& callback) { LoginResponseCallback = callback; }
	inline void SetInboxAndNotificationCountCallback(const std::function<void(int, int)>& callback) { InboxAndNotificationCountCallback = callback; }
	inline void SetLatestUploadsCallback(const std::function<void(std::vector<std::string>)>& callback) { LatestUploadsCallback = callback; }
	inline void SetFileRequestFailureCallback(const std::function<void(std::string, std::string)>& callback) { FileRequestFailureCallback = callback; }
	inline void SetFileRequestSuccessCallback(const std::function<void(std::string)>& callback) { FileRequestSuccessCallback = callback; }
	inline void SetDownloadPercentCompleteCallback(const std::function<void(float, double, int)>& callback) { DownloadPercentCompleteCallback = callback; }

	bool Connect();

	void Initialize();
	bool MainProcess();
	void Shutdown();

	bool ReadMessages(void);
};

bool Client::Connect()
{
	// Connect to the New Providence server
	ServerSocket = winsockWrapper.TCPConnect(NEW_PROVIDENCE_IP, NEW_PROVIDENCE_PORT, 1);
	return (ServerSocket >= 0);
}

void Client::Initialize()
{
	//  Seed the random number generator
	srand((unsigned int)(time(NULL)));

	//  Load the current Groundfish word list
	Groundfish::LoadCurrentWordList();

	//  Initialize the Winsock wrapper
	winsockWrapper.WinsockInitialize();
}

bool Client::MainProcess()
{
	if (ServerSocket < 0) return false;
	
	ReadMessages();
	return true;
}

void Client::Shutdown()
{
	closesocket(ServerSocket);
}

bool Client::ReadMessages(void)
{
	auto messageBufferSize = winsockWrapper.ReceiveMessagePacket(ServerSocket, 0, 0);
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
		auto success = bool(winsockWrapper.ReadChar(0));
		auto inboxCount = success ? winsockWrapper.ReadInt(0) : 0;
		auto notificationCount = success ? winsockWrapper.ReadInt(0) : 0;
		if (LoginResponseCallback != nullptr) LoginResponseCallback(success, inboxCount, notificationCount);
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
		auto decryptedFileName = Groundfish::Decrypt(winsockWrapper.ReadChars(0, fileNameSize));
		auto decryptedFileNameString = std::string((char*)decryptedFileName.data(), decryptedFileName.size());

		//  Grab the file size, file chunk size, and buffer count
		auto fileSize = winsockWrapper.ReadInt(0);
		auto fileChunkSize = winsockWrapper.ReadInt(0);
		auto FileChunkBufferSize = winsockWrapper.ReadInt(0);

		//  Create a new file receive task
		FileReceive = new FileReceiveTask(decryptedFileNameString, fileSize, fileChunkSize, FileChunkBufferSize, 0, NEW_PROVIDENCE_IP, NEW_PROVIDENCE_PORT);

		//  Output the file name
		std::string newString = "Downloading file: " + decryptedFileNameString + " (size: " + std::to_string(fileSize) + ")";
		//NewLine(newString.c_str());

		if (FileRequestSuccessCallback != nullptr) FileRequestSuccessCallback("test");
	}
	break;

	case MESSAGE_ID_FILE_PORTION:
	{
		if (FileReceive == nullptr) break;

		std::string progressString = "ERROR";
		if (FileReceive->ReceiveFile(progressString))
		{
			//  If ReceiveFile returns true, the transfer is complete
			delete FileReceive;
			FileReceive = nullptr;
		}
		//NewLine(progressString.c_str());
	}
	break;

	case MESSAGE_ID_FILE_PORTION_COMPLETE:
	{
		auto portionIndex = winsockWrapper.ReadInt(0);

		if (FileReceive == nullptr) break;
		if (FileReceive->CheckFilePortionComplete(portionIndex))
		{
			if (DownloadPercentCompleteCallback != nullptr) DownloadPercentCompleteCallback(FileReceive->GetPercentageComplete(), FileReceive->DownloadTime, FileReceive->FileSize);

			if (FileReceive->GetFileDownloadComplete())
			{
				delete FileReceive;
				FileReceive = nullptr;
				break;
			}
		}
	}
	break;
	}

	return true;
}