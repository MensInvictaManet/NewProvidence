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
	MESSAGE_ID_FILE_REQUEST						= 5,	// File Request (client to server)
	MESSAGE_ID_FILE_SEND_INIT					= 6,	// File Send Initializer (two-way)
	MESSAGE_ID_FILE_RECEIVE_READY				= 7,	// File Receive Ready (two-way)
	MESSAGE_ID_FILE_PORTION						= 8,	// File Portion Send (two-way)
	MESSAGE_ID_FILE_PORTION_COMPLETE			= 9,	// File Portion Complete Check (two-way)
	MESSAGE_ID_FILE_CHUNKS_REMAINING			= 10,	// File Chunks Remaining (two-way)
	MESSAGE_ID_FILE_PORTION_COMPLETE_CONFIRM	= 11,	// File Portion Complete Confirm (two-way)
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

	std::function<void(bool)> LoginResponseCallback = nullptr;

public:
	Client()	{}
	~Client()	{}

	inline int GetServerSocket(void) const { return ServerSocket; }
	inline void SetLoginResponseCallback(const std::function<void(bool)>& callback) { LoginResponseCallback = callback; }

	bool Connect();
	void SendLoginData();

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

void Client::SendLoginData()
{
	//  Send file request to the server
	SendMessage_FileRequest("fileToTransfer.jpeg", ServerSocket, NEW_PROVIDENCE_IP);
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

		//  Decrypt using Groundfish and post to the chat
		unsigned char encrypted[256];
		memcpy(encrypted, winsockWrapper.ReadChars(0, messageSize), messageSize);
		char decrypted[256];
		Groundfish::Decrypt(encrypted, decrypted);
		//NewLine(decrypted);
	}
	break;

	case MESSAGE_ID_USER_LOGIN_RESPONSE:
	{
		auto success = bool(winsockWrapper.ReadChar(0));
		if (LoginResponseCallback != nullptr) LoginResponseCallback(success);
	}
	break;


	case MESSAGE_ID_FILE_SEND_INIT:
	{
		int fileNameSize = winsockWrapper.ReadInt(0);

		//  Decrypt using Groundfish and save as the filename
		unsigned char encryptedFileName[256];
		memcpy(encryptedFileName, winsockWrapper.ReadChars(0, fileNameSize), fileNameSize);
		char decryptedFileName[256];
		Groundfish::Decrypt(encryptedFileName, decryptedFileName);

		//  Grab the file size, file chunk size, and buffer count
		auto fileSize = winsockWrapper.ReadInt(0);
		auto fileChunkSize = winsockWrapper.ReadInt(0);
		auto FileChunkBufferSize = winsockWrapper.ReadInt(0);

		//  Create a new file receive task
		FileReceive = new FileReceiveTask(decryptedFileName, fileSize, fileChunkSize, FileChunkBufferSize, 0, NEW_PROVIDENCE_IP);

		//  Output the file name
		std::string newString = "Downloading file: " + std::string(decryptedFileName) + " (size: " + std::to_string(fileSize) + ")";
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