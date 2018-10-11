#pragma once

#include "Groundfish.h"
#include "Engine/WinsockWrapper.h"

#define NEW_PROVIDENCE_IP		"98.181.188.165"
#define NEW_PROVIDENCE_PORT		2347

//  Incoming message type IDs (enum list syncronous with both client and server)
enum MessageIDs
{
	MESSAGE_ID_PING_REQUEST						= 0,	// Ping Request (server to client)
	MESSAGE_ID_PING_RESPONSE					= 1,	// Ping Return (client to server)
	MESSAGE_ID_ENCRYPTED_CHAT_STRING			= 2,	// Chat String [encrypted] (two-way)
	MESSAGE_ID_USER_LOGIN_REQUEST				= 3,	// User Login Request (client to server)
	MESSAGE_ID_USER_LOGIN_RESPONSE				= 4,	// User Login Response (server to client)
	MESSAGE_ID_USER_INBOX_AND_NOTIFICATIONS		= 5,	// User's inbox and notification data (server to client)	
	MESSAGE_ID_LATEST_UPLOADS_LIST				= 6,	// The list of the latest uploads on the server (server to client)
	MESSAGE_ID_FILE_REQUEST						= 7,	// File Request (client to server)
	MESSAGE_ID_FILE_SEND_INIT					= 8,	// File Send Initializer (two-way)
	MESSAGE_ID_FILE_RECEIVE_READY				= 9,	// File Receive Ready (two-way)
	MESSAGE_ID_FILE_PORTION						= 10,	// File Portion Send (two-way)
	MESSAGE_ID_FILE_PORTION_COMPLETE			= 11,	// File Portion Complete Check (two-way)
	MESSAGE_ID_FILE_CHUNKS_REMAINING			= 12,	// File Chunks Remaining (two-way)
	MESSAGE_ID_FILE_PORTION_COMPLETE_CONFIRM	= 13,	// File Portion Complete Confirm (two-way)
};


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

void SendMessage_FileRequest(std::string fileName, int socket, const char* ip)
{
	//  Send a "File Request" message
	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_FILE_REQUEST, 0);
	winsockWrapper.WriteString(fileName.c_str(), 0);
	winsockWrapper.SendMessagePacket(socket, NEW_PROVIDENCE_IP, NEW_PROVIDENCE_PORT, 0);
}

void SendMessage_FileReceiveReady(std::string fileName, int socket, const char* ip)
{
	//  Send a "File Receive Ready" message
	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_FILE_RECEIVE_READY, 0);
	//  TODO: Encrypt the file name
	winsockWrapper.WriteString(fileName.c_str(), 0);
	winsockWrapper.SendMessagePacket(socket, NEW_PROVIDENCE_IP, NEW_PROVIDENCE_PORT, 0);
}

void SendMessage_FilePortionCompleteConfirmation(int portionIndex, int socket, const char* ip)
{
	//  Send a "File Portion Complete" message
	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_FILE_PORTION_COMPLETE_CONFIRM, 0);
	winsockWrapper.WriteInt(portionIndex, 0);
	winsockWrapper.SendMessagePacket(socket, NEW_PROVIDENCE_IP, NEW_PROVIDENCE_PORT, 0);
}

void SendMessage_FileChunksRemaining(std::unordered_map<int, bool>& chunksRemaining, int socket, const char* ip)
{
	//  Send a "File Portion Complete" message
	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_FILE_CHUNKS_REMAINING, 0);
	winsockWrapper.WriteInt(chunksRemaining.size(), 0);
	for (auto i = chunksRemaining.begin(); i != chunksRemaining.end(); ++i) winsockWrapper.WriteShort((short)((*i).first), 0);
	winsockWrapper.SendMessagePacket(socket, NEW_PROVIDENCE_IP, NEW_PROVIDENCE_PORT, 0);
}

struct FileReceiveTask
{
	FileReceiveTask(const char* fileName, int fileSize, int fileChunkSize, int fileChunkBufferSize, int socketID, std::string ipAddress) :
		FileName(fileName),
		FileSize(fileSize),
		FileChunkSize(fileChunkSize),
		FileChunkBufferSize(fileChunkBufferSize),
		SocketID(socketID),
		IPAddress(ipAddress),
		FilePortionIndex(0),
		FileDownloadComplete(false)
	{
		FileChunkCount = FileSize / FileChunkSize;
		if ((FileSize % FileChunkSize) != 0) FileChunkCount += 1;
		auto nextChunkCount = (FileChunkCount > (FileChunkBufferSize)) ? FileChunkBufferSize : FileChunkCount;
		ResetChunksToReceiveMap(nextChunkCount);
		FilePortionCount = FileChunkCount / FileChunkBufferSize;
		if ((FileChunkCount % FileChunkBufferSize) != 0) FilePortionCount += 1;

		//  Create a file of the proper size based on the server's description
		std::ofstream outputFile(fileName, std::ios::binary | std::ios::trunc | std::ios_base::beg);
		outputFile.seekp(fileSize - 1);
		outputFile.write("", 1);
		outputFile.close();

		//  Open the file again, this time keeping the file handle open for later writing
		FileStream.open(FileName.c_str(), std::ios_base::binary | std::ios_base::out | std::ios_base::in);
		assert(FileStream.good() && !FileStream.bad());

		FileReceiveBuffer = new char[FileChunkSize];
		SendMessage_FileReceiveReady(FileName, SocketID, IPAddress.c_str());
	}

	~FileReceiveTask()
	{
		delete[] FileReceiveBuffer;
	}

	inline void ResetChunksToReceiveMap(int chunkCount) { FilePortionsToReceive.clear(); for (auto i = 0; i < chunkCount; ++i)  FilePortionsToReceive[i] = true; }


	bool ReceiveFile(std::string& progress)
	{
		auto filePortionIndex = winsockWrapper.ReadInt(0);
		auto chunkIndex = winsockWrapper.ReadInt(0);
		auto chunkSize = winsockWrapper.ReadInt(0);
		unsigned char* chunkData = winsockWrapper.ReadChars(0, chunkSize);

		if (filePortionIndex != FilePortionIndex)
		{
			return false;
		}

		if (FileDownloadComplete) return true;

		auto iter = FilePortionsToReceive.find(chunkIndex);
		if (iter == FilePortionsToReceive.end()) return false;

		auto chunkPosition = (filePortionIndex * FileChunkSize * FileChunkBufferSize) + (chunkIndex * FileChunkSize) + 0;
		FileStream.seekp(chunkPosition);
		FileStream.write((char*)chunkData, chunkSize);

		progress = "Downloaded Portion [" + std::to_string(chunkPosition) + " to " + std::to_string(chunkPosition + chunkSize - 1) + "]";
		assert(iter != FilePortionsToReceive.end());
		FilePortionsToReceive.erase(iter);

		return false;
	}

	bool CheckFilePortionComplete(int portionIndex)
	{
		if (portionIndex != FilePortionIndex) return false;

		if (FilePortionsToReceive.size() != 0)
		{
			SendMessage_FileChunksRemaining(FilePortionsToReceive, SocketID, IPAddress.c_str());
			return false;
		}
		else
		{
			SendMessage_FilePortionCompleteConfirmation(FilePortionIndex, SocketID, IPAddress.c_str());
			if (++FilePortionIndex == FilePortionCount)
			{
				FileDownloadComplete = true;
				FileStream.close();
			}

			//  Reset the chunk list to ensure we're waiting on the right number of chunks for the next portion
			auto chunksProcessed = FilePortionIndex * FileChunkBufferSize;
			auto nextChunkCount = (FileChunkCount > (chunksProcessed + FileChunkBufferSize)) ? FileChunkBufferSize : (FileChunkCount - chunksProcessed);
			ResetChunksToReceiveMap(nextChunkCount);

			return true;
		}
	}

	inline bool GetFileDownloadComplete() const { return FileDownloadComplete; }
	inline float GetPercentageComplete() const { return float(FilePortionIndex) / float(FilePortionCount) * 100.0f; }

	std::string FileName;
	int FileSize;
	int SocketID;
	std::string IPAddress;
	int FilePortionCount;
	int FileChunkCount;
	const int FileChunkSize;
	const int FileChunkBufferSize;
	std::ofstream FileStream;

	int FilePortionIndex;
	char* FileReceiveBuffer;
	std::unordered_map<int, bool> FilePortionsToReceive;
	bool FileDownloadComplete;
};

class Client
{
private:
	int					ServerSocket = -1;
	std::string			UserName;
	FileReceiveTask*	FileReceive = NULL;

	std::function<void(bool, int, int)> LoginResponseCallback = nullptr;
	std::function<void(int, int)> InboxAndNotificationCountCallback = nullptr;
	std::function<void(std::vector<std::string>)> LatestUploadsCallback = nullptr;
	std::vector<std::string> LatestUploadsList;

public:
	Client()	{}
	~Client()	{}

	inline int GetServerSocket(void) const { return ServerSocket; }
	inline void SetLoginResponseCallback(const std::function<void(bool, int, int)>& callback) { LoginResponseCallback = callback; }
	inline void SetInboxAndNotificationCountCallback(const std::function<void(int, int)>& callback) { InboxAndNotificationCountCallback = callback; }
	inline void SetLatestUploadsCallback(const std::function<void(std::vector<std::string>)>& callback) { LatestUploadsCallback = callback; }

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


	case MESSAGE_ID_FILE_SEND_INIT:
	{
		int fileNameSize = winsockWrapper.ReadInt(0);

		//  Decrypt using Groundfish and save as the filename
		unsigned char encryptedFileName[256];
		memcpy(encryptedFileName, winsockWrapper.ReadChars(0, fileNameSize), fileNameSize);
		std::vector<unsigned char> decryptedFileName = Groundfish::Decrypt(encryptedFileName);
		std::string decryptedFileNameString = std::string((char*)decryptedFileName.data(), decryptedFileName.size());

		//  Grab the file size, file chunk size, and buffer count
		auto fileSize = winsockWrapper.ReadInt(0);
		auto fileChunkSize = winsockWrapper.ReadInt(0);
		auto FileChunkBufferSize = winsockWrapper.ReadInt(0);

		//  Create a new file receive task
		FileReceive = new FileReceiveTask(decryptedFileNameString.c_str(), fileSize, fileChunkSize, FileChunkBufferSize, 0, NEW_PROVIDENCE_IP);

		//  Output the file name
		std::string newString = "Downloading file: " + decryptedFileNameString + " (size: " + std::to_string(fileSize) + ")";
		//NewLine(newString.c_str());
	}
	break;

	case MESSAGE_ID_FILE_PORTION:
	{
		if (FileReceive == NULL) break;

		std::string progressString = "ERROR";
		if (FileReceive->ReceiveFile(progressString))
		{
			//  If ReceiveFile returns true, the transfer is complete
			delete FileReceive;
			FileReceive = NULL;
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
			auto percentComplete = "File Portion Complete: (" + std::to_string(FileReceive->GetPercentageComplete()) + "%)";
			//NewLine(percentComplete.c_str());

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