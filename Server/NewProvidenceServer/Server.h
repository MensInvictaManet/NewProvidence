#pragma once

#include "Engine/WinsockWrapper.h"
#include "Engine/SimpleMD5.h"
#include "Groundfish.h"

#include <fstream>
#include <ctime>

#define SERVER_PORT						2347
#define FILE_CHUNK_SIZE					1024
#define FILE_CHUNK_BUFFER_COUNT			500
#define FILE_SEND_BUFFER_SIZE			(FILE_CHUNK_SIZE * FILE_CHUNK_BUFFER_COUNT)
#define MAXIMUM_PACKET_COUNT			10
#define PORTION_COMPLETE_REMIND_TIME	0.5
#define MAX_LATEST_UPLOADS_COUNT		3


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

struct HostedFileData
{
	std::string FileNameMD5;
	std::vector<unsigned char> EncryptedFileName;
	std::vector<unsigned char> EncryptedFileTitle;
	std::vector<unsigned char> EncryptedFileDescription;
};

struct UserConnection
{
	int				SocketID;
	std::string		IPAddress;
	int				PingCount;

	int				InboxCount;

	std::string		UserIdentifier;

	std::vector<std::string> NotificationsList;
};


//  Outgoing message send functions
void SendMessage_LoginResponse(bool success, UserConnection* user)
{
	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_USER_LOGIN_RESPONSE, 0);
	winsockWrapper.WriteChar((unsigned char)success, 0);

	if (success)
	{
		winsockWrapper.WriteInt(user->InboxCount, 0);
		winsockWrapper.WriteInt(int(user->NotificationsList.size()), 0);
	}

	winsockWrapper.SendMessagePacket(user->SocketID, user->IPAddress.c_str(), SERVER_PORT, 0);
}


void SendMessage_InboxAndNotifications(UserConnection* user)
{
	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_USER_INBOX_AND_NOTIFICATIONS, 0);
	winsockWrapper.WriteInt(user->InboxCount, 0);
	for (auto i = 0; i < user->InboxCount; ++i)
	{
		//  TODO: Write inbox item data
	}
	winsockWrapper.WriteInt(int(user->NotificationsList.size()), 0);
	for (auto notificationIter = user->NotificationsList.begin(); notificationIter != user->NotificationsList.end(); ++notificationIter)
	{
		winsockWrapper.WriteInt(int((*notificationIter).length()), 0);
		winsockWrapper.WriteChars((unsigned char*)(*notificationIter).c_str(), int((*notificationIter).length()), 0);
	}

	winsockWrapper.SendMessagePacket(user->SocketID, user->IPAddress.c_str(), SERVER_PORT, 0);
}

void SendMessage_LatestUploads(std::vector< std::vector<unsigned char> > latestUploads, UserConnection* user)
{
	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_LATEST_UPLOADS_LIST, 0);
	winsockWrapper.WriteInt(int(latestUploads.size()), 0);

	for (auto iter = latestUploads.begin(); iter != latestUploads.end(); ++iter)
	{
		winsockWrapper.WriteInt(int((*iter).size()), 0);
		winsockWrapper.WriteChars((*iter).data(), int((*iter).size()), 0);
	}

	winsockWrapper.SendMessagePacket(user->SocketID, user->IPAddress.c_str(), SERVER_PORT, 0);
}


void SendMessage_FileSendInitializer(std::string fileName, int fileSize, int socket, const char* ip)
{
	//  Encrypt the file name string using Groundfish
	std::vector<unsigned char> encryptedFilename = Groundfish::Encrypt(fileName.c_str(), int(fileName.length()) + 1, 0, rand() % 256);

	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_FILE_SEND_INIT, 0);
	winsockWrapper.WriteInt(int(encryptedFilename.size()), 0);
	winsockWrapper.WriteChars(encryptedFilename.data(), int(encryptedFilename.size()), 0);
	winsockWrapper.WriteInt(fileSize, 0);
	winsockWrapper.WriteInt(FILE_CHUNK_SIZE, 0);
	winsockWrapper.WriteInt(FILE_CHUNK_BUFFER_COUNT, 0);

	winsockWrapper.SendMessagePacket(socket, ip, SERVER_PORT, 0);
}


void SendMessage_FileSendChunk(int chunkBufferIndex, int chunkIndex, int chunkSize, unsigned char* buffer, int socket, const char* ip)
{
	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_FILE_PORTION, 0);
	winsockWrapper.WriteInt(chunkBufferIndex, 0);
	winsockWrapper.WriteInt(chunkIndex, 0);
	winsockWrapper.WriteInt(chunkSize, 0);
	winsockWrapper.WriteChars(buffer, chunkSize, 0);
	winsockWrapper.SendMessagePacket(socket, ip, SERVER_PORT, 0);
}


void SendMessage_FileTransferPortionComplete(int portionIndex, int socket, const char* ip)
{
	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_FILE_PORTION_COMPLETE, 0);
	winsockWrapper.WriteInt(portionIndex, 0);
	winsockWrapper.SendMessagePacket(socket, ip, SERVER_PORT, 0);
}


void AddUserInboxMessage(std::string userID, std::string inboxMessage)
{
}


void AddUserNotification(std::string userID, std::string notification)
{
	std::string notificationsFilename = "./_UserNotifications/" + userID + ".notifications";
	std::ofstream notificationsFile(notificationsFilename, std::ios_base::binary);
	assert(notificationsFile.good() && !notificationsFile.bad());

	auto notificationSize = int(notification.length());
	notificationsFile.write((char*)&notificationSize, sizeof(notificationSize));
	notificationsFile.write(notification.c_str(), notificationSize);

	notificationsFile.close();
}


void ReadUserInbox(UserConnection* user)
{
	return;
	std::string inboxFilename = "./_UserInbox/" + user->UserIdentifier + ".inbox";
	std::ifstream inboxFile(inboxFilename, std::ios_base::binary);
	assert(inboxFile.good() && !inboxFile.bad());

	inboxFile.close();
}


void ReadUserNotifications(UserConnection* user)
{
	std::string notificationsFilename = "./_UserNotifications/" + user->UserIdentifier + ".notifications";
	std::ifstream notificationsFile(notificationsFilename, std::ios_base::binary);
	assert(notificationsFile.good() && !notificationsFile.bad());

	//  Get the file size by reading the beginning and end memory positions
	auto fileSize = int(notificationsFile.tellg());
	notificationsFile.seekg(0, std::ios::end);
	fileSize = int(notificationsFile.tellg()) - fileSize;
	notificationsFile.seekg(0, std::ios::beg);

	char notification[256];
	while ((fileSize != 0) && !notificationsFile.eof())
	{
		int notificationSize = 0;
		memset(notification, 0, 256);
		notificationsFile.read((char*)&notificationSize, sizeof(notificationSize));
		if (notificationsFile.eof()) break;
		notificationsFile.read(notification, notificationSize);
		notification[notificationSize] = 0;
		user->NotificationsList.push_back(std::string(notification));
	}

	notificationsFile.close();
}

//  FileSendTask class
class FileSendTask
{
public:
	FileSendTask(std::string fileName, int socketID, std::string ipAddress) :
		FileName(fileName),
		SocketID(socketID),
		IPAddress(ipAddress),
		FileTransferReady(false),
		FileChunkTransferState(CHUNK_STATE_INITIALIZING),
		FilePortionIndex(0),
		LastMessageTime(clock())
	{
		//  Open the file, determine that the file handler is good or not, then save off the chunk send indicator map
		FileStream.open(FileName.c_str(), std::ios_base::binary);
		assert(FileStream.good() && !FileStream.bad());

		//  Get the file size by reading the beginning and end memory positions
		FileSize = int(FileStream.tellg());
		FileStream.seekg(0, std::ios::end);
		FileSize = int(FileStream.tellg()) - FileSize;

		//  Determine the file portion count
		FileChunkCount = FileSize / FILE_CHUNK_SIZE;
		if ((FileSize % FILE_CHUNK_SIZE) != 0) FileChunkCount += 1;

		FilePortionCount = FileChunkCount / FILE_CHUNK_BUFFER_COUNT;
		if ((FileChunkCount % FILE_CHUNK_BUFFER_COUNT) != 0) FilePortionCount += 1;

		BufferFilePortions(FilePortionIndex, FILE_CHUNK_BUFFER_COUNT);

		//  Send a "File Send Initializer" message
		SendMessage_FileSendInitializer(FileName, FileSize, SocketID, IPAddress.c_str());
	}

	~FileSendTask()
	{
		FileStream.close();
	}

	void BufferFilePortions(int chunkBufferIndex, int portionCount)
	{
		auto portionPosition = chunkBufferIndex * FILE_SEND_BUFFER_SIZE;
		auto bytesBuffering = ((portionPosition + FILE_SEND_BUFFER_SIZE) > FileSize) ? (FileSize - portionPosition) : FILE_SEND_BUFFER_SIZE;
		auto bufferCount = (((bytesBuffering % FILE_CHUNK_SIZE) == 0) ? (bytesBuffering / FILE_CHUNK_SIZE) : ((bytesBuffering / FILE_CHUNK_SIZE) + 1));

		//  Empty the entire FilePortionBuffer
		memset(FilePortionBuffer, NULL, FILE_SEND_BUFFER_SIZE);

		//  Determine the amount of data we're actually buffering, if the file would end before reading the entire size

		//  Go through each chunk of the file and place it in the file buffer until we either fill the buffer or run out of file
		FileChunksToSend.clear();
		for (auto i = 0; i < bufferCount; ++i)
		{
			auto chunkPosition = portionPosition + (i * FILE_CHUNK_SIZE);
			FileStream.seekg(chunkPosition);

			auto chunkFill = (((chunkPosition + FILE_CHUNK_SIZE) > FileSize) ? (FileSize - chunkPosition) : FILE_CHUNK_SIZE);
			FileStream.read(FilePortionBuffer[i], chunkFill);

			FileChunksToSend[i] = true;
		}
	}

	bool SendFileChunk()
	{
		//  If we're pending completion, wait and send completion confirmation only if we've reached the end
		if (FileChunkTransferState == CHUNK_STATE_PENDING_COMPLETE)
		{
			auto seconds = double(clock() - LastMessageTime) / CLOCKS_PER_SEC;
			if (seconds > PORTION_COMPLETE_REMIND_TIME)
			{
				SendMessage_FileTransferPortionComplete(FilePortionIndex, SocketID, IPAddress.c_str());
				LastMessageTime = clock();
				return false;
			}
			return (FilePortionIndex >= FilePortionCount);
		}

		//  First, check that we have data left unsent in the current chunk buffer. If not, set us to "Pending Complete" on the current chunk buffer
		if (FileChunksToSend.begin() == FileChunksToSend.end())
		{
			FileChunkTransferState = CHUNK_STATE_PENDING_COMPLETE;
			SendMessage_FileTransferPortionComplete(FilePortionIndex, SocketID, IPAddress.c_str());
			LastMessageTime = clock();
			return false;
		}


		//  Choose a portion of the file and commit it to a byte array
		auto chunkIndex = (*FileChunksToSend.begin()).first;
		auto chunkPosition = (FilePortionIndex * FILE_SEND_BUFFER_SIZE) + (FILE_CHUNK_SIZE * chunkIndex);
		int chunkSize = ((chunkPosition + FILE_CHUNK_SIZE) > FileSize) ? (FileSize - chunkPosition) : FILE_CHUNK_SIZE;

		//  Write the chunk buffer index, the index of the chunk, the size of the chunk, and then the chunk data
		SendMessage_FileSendChunk(FilePortionIndex, chunkIndex, chunkSize, (unsigned char*)FilePortionBuffer[chunkIndex], SocketID, IPAddress.c_str());

		//  Delete the portionIter to signal we've completed sending it
		FileChunksToSend.erase(chunkIndex);

		return false;
	}

	void SetChunksRemaining(std::unordered_map<int, bool>& chunksRemaining)
	{
		FileChunksToSend.clear();
		for (auto i = chunksRemaining.begin(); i != chunksRemaining.end(); ++i) FileChunksToSend[(*i).first] = true;
		FileChunkTransferState = CHUNK_STATE_SENDING;
	}

	void FilePortionComplete(int portionIndex)
	{
		//  If this is just a duplicate message we're receiving, ignore it
		if (FilePortionIndex > portionIndex) return;

		//  If we've reached the end of the file, don't attempt to buffer anything, and leave the state PENDING COMPLETE
		if (++FilePortionIndex >= FilePortionCount)  return;

		BufferFilePortions(FilePortionIndex, FILE_CHUNK_BUFFER_COUNT);
	}

	inline bool GetFileTransferReady() const { return FileTransferReady; }
	inline void SetFileTransferReady(bool ready) { FileTransferReady = ready; }
	inline int GetFileTransferState() const { return FileChunkTransferState; }
	inline void SetFileTransferState(int state) { FileChunkTransferState = (FileChunkSendState)state; }

	std::string FileName;
	int FileSize;
	int SocketID;
	std::string IPAddress;
	std::ifstream FileStream;

	enum FileChunkSendState
	{
		CHUNK_STATE_INITIALIZING = 0,
		CHUNK_STATE_SENDING = 1,
		CHUNK_STATE_PENDING_COMPLETE = 2,
		CHUNK_STATE_COMPLETE = 3,
	};

	int FilePortionIndex;
	bool FileTransferReady;
	FileChunkSendState FileChunkTransferState;
	int FilePortionCount;
	int FileChunkCount;
	std::unordered_map<int, bool> FileChunksToSend;
	char FilePortionBuffer[FILE_CHUNK_BUFFER_COUNT][FILE_CHUNK_SIZE];
	clock_t LastMessageTime;
};

class Server
{
private:
	int		ServerSocketHandle;

	typedef std::pair<std::vector<unsigned char>, std::vector<unsigned char>> UserLoginDetails;
	std::unordered_map<std::string, UserLoginDetails> UserLoginDetailsList;
	std::unordered_map<std::string, HostedFileData> HostedFileDataList;
	std::unordered_map<UserConnection*, bool> UserConnectionsList;
	std::unordered_map<UserConnection*, FileSendTask*> FileSendTaskList; // NOTE: Key is the client ID, so we should limit them to one transfer in the future
	std::vector< std::vector<unsigned char> > LatestUploadsList;

public:
	Server() {}
	~Server() {}

	bool Initialize(void);
	void MainProcess(void);
	void Shutdown(void);

private:
	void AddClient(int socketID, std::string ipAddress);
	void RemoveClient(UserConnection* user);
	void AcceptNewClients(void);
	void ReceiveMessages(void);
	void AttemptUserLogin(UserConnection* user, std::string& username, std::string& password);

	void LoadUserLoginDetails(void);
	void SaveUserLoginDetails(void);
	void AddUserLoginDetails(std::string username, std::string password);

	void AddHostedFileFromUnencrypted(std::string fileToAdd, std::string fileTitle, std::string fileDescription);
	void LoadHostedFilesData(void);
	void AddLatestUpload(std::vector<unsigned char> newUpload);
	void LoadLatestUploadsList(void);
	void SaveLatestUploadsList(void);

	void ContinueFileTransfers(void);
	void BeginFileTransfer(const char* fileName, UserConnection* user);
	void SendChatString(const char* chatString);

	inline const std::unordered_map<UserConnection*, bool> GetUserList(void) const { return UserConnectionsList; }
	inline std::string	GetClientIP(UserConnection* user) { return user->IPAddress; }
};

bool Server::Initialize(void)
{
	//  Seed the random number generator
	srand((unsigned int)(time(NULL)));

	//Groundfish::CreateWordList(Groundfish::CurrentWordList);

	//  Load the current Groundfish word list
	Groundfish::LoadCurrentWordList();

	//  Initialize the Winsock wrapper
	winsockWrapper.WinsockInitialize();

	// Initialize the Listening Port for Clients
	ServerSocketHandle = winsockWrapper.TCPListen(SERVER_PORT, 10, 1);
	if (ServerSocketHandle == -1) return false;
	winsockWrapper.SetNagle(ServerSocketHandle, true);

	//  ensure there is an Inbox, Notifications, and Files folder
	_wmkdir(L"_UserInbox");
	_wmkdir(L"_UserNotifications");
	_wmkdir(L"_HostedFiles");

	//  Load the list of latest uploads to the server
	LoadLatestUploadsList();

	//  Load all user login details into an accessible list
	LoadUserLoginDetails();
	AddUserLoginDetails("Drew", "password");
	AddUserLoginDetails("Charlie", "testpass");

	//  Load the hosted files list
	LoadHostedFilesData();
	//AddHostedFileFromUnencrypted("fileToTransfer.png", "Test file", "This is a test image file");
	//Groundfish::DecryptAndMoveFile("./_HostedFiles/" + md5("Test file") + ".hostedfile", "./_HostedFiles/fileToTransfer.jpeg");

	return true;
}


void Server::MainProcess(void)
{
	// Accept Incoming Connections
	AcceptNewClients();

	// Receive messages
	ReceiveMessages();

	//  Send files
	ContinueFileTransfers();
}


void Server::Shutdown(void)
{
	//  Close out the server socket
	winsockWrapper.CloseSocket(ServerSocketHandle);
}


////////////////////////////////////////
//	Client Connection Functions
////////////////////////////////////////

void Server::AddClient(int socketID, std::string ipAddress)
{
	auto newConnection = new UserConnection;
	newConnection->SocketID = socketID;
	newConnection->IPAddress = ipAddress;
	newConnection->PingCount = 0;
	newConnection->UserIdentifier = "";
	newConnection->InboxCount = 0;
	UserConnectionsList[newConnection] = true;
}

void Server::RemoveClient(UserConnection* user)
{
	auto userIter = UserConnectionsList.find(user);
	assert(userIter != UserConnectionsList.end());

	delete user;
	UserConnectionsList.erase(userIter);
}

void Server::AcceptNewClients(void)
{
	auto newClient = winsockWrapper.TCPAccept(ServerSocketHandle, 1);
	while (newClient >= 0)
	{
		//  Add the new client using his 
		AddClient(newClient, winsockWrapper.GetExteriorIP(newClient));

		// Check for another client connection
		newClient = winsockWrapper.TCPAccept(ServerSocketHandle, 1);
	}
}

void Server::ReceiveMessages(void)
{
	for (auto userIter = UserConnectionsList.begin(); userIter != UserConnectionsList.end(); ++userIter)
	{
		auto user = (*userIter).first;
		auto messageBufferSize = winsockWrapper.ReceiveMessagePacket(user->SocketID, 0, 0);

		//  If no message has arrived, move on to the next connection
		if (messageBufferSize < 0) continue;

		//  If we receive a disconnection indicator, remove the user and move on to the next connection
		if (messageBufferSize == 0)
		{
			RemoveClient(user);
			break;
		}

		char messageID = winsockWrapper.ReadChar(0);
		switch (messageID)
		{
		case MESSAGE_ID_PING_RESPONSE:
		{
			//  NO DATA

			user->PingCount = 0;
		}
		break;

		case MESSAGE_ID_ENCRYPTED_CHAT_STRING:
		{
			//  (int) Message Size [N]
			//  (N-sized char array) Encrypted String

			int messageSize = winsockWrapper.ReadInt(0);

			//  Decrypt using Groundfish
			unsigned char encryptedChatString[256];
			memcpy(encryptedChatString, winsockWrapper.ReadChars(0, messageSize), messageSize);
			std::vector<unsigned char> descryptedChatString = Groundfish::Decrypt(encryptedChatString);
			std::string decryptedString((char*)descryptedChatString.data(), descryptedChatString.size());

			if (decryptedString.find("download") != std::string::npos) BeginFileTransfer("fileToTransfer.jpeg", user);
			else SendChatString(decryptedString.c_str());
		}
		break;

		case MESSAGE_ID_USER_LOGIN_REQUEST: // Player enters the server, sending their encrypted name and password
		{
			//  (int) Length of encrypted username (n1)
			//  (n1-size chars array) Encrypted username
			//  (int) Length of encrypted password (n2)
			//  (n2-size chars array) Encrypted password

			//  Grab the username size and then the encrypted username
			int usernameSize = winsockWrapper.ReadInt(0);
			auto encryptedUsername = winsockWrapper.ReadChars(0, usernameSize);

			//  Decrypt the username
			std::vector<unsigned char> decryptedUsername = Groundfish::Decrypt(encryptedUsername);
			std::string username = std::string((char*)decryptedUsername.data(), decryptedUsername.size());

			//  Grab the password size and then the encrypted password
			int passwordSize = winsockWrapper.ReadInt(0);
			auto encryptedPassword = winsockWrapper.ReadChars(0, passwordSize);

			//  Decrypt the password
			std::vector<unsigned char> decryptedPassword = Groundfish::Decrypt(encryptedPassword);
			std::string password = std::string((char*)decryptedPassword.data(), decryptedPassword.size());

			AttemptUserLogin(user, username, password);
		}
		break;

		case MESSAGE_ID_FILE_REQUEST:
		{
			char* fileTitle = winsockWrapper.ReadString(0);
			auto hostedFileIdentifier = md5(fileTitle);
			auto hostedFileIter = HostedFileDataList.find(hostedFileIdentifier);

			if (hostedFileIter == HostedFileDataList.end())
			{
				//  TODO: Send back a failed file retrieval message, then break out
			}
			else
			{
				auto hostedFileData = (*hostedFileIter).second;

				std::vector<unsigned char> decryptedFileName = Groundfish::Decrypt(hostedFileData.EncryptedFileName.data());
				BeginFileTransfer((char*)decryptedFileName.data(), user);
			}
		}
		break;

		case MESSAGE_ID_FILE_RECEIVE_READY:
		{
			char* fileName = winsockWrapper.ReadString(0);
			auto taskIter = FileSendTaskList.find(user);
			assert(taskIter != FileSendTaskList.end());

			auto task = (*taskIter).second;
			task->SetFileTransferReady(true);
			task->SetFileTransferState(FileSendTask::CHUNK_STATE_SENDING);
		}
		break;

		case MESSAGE_ID_FILE_PORTION_COMPLETE_CONFIRM:
		{
			auto taskIter = FileSendTaskList.find(user);
			assert(taskIter != FileSendTaskList.end());

			auto task = (*taskIter).second;
			assert(task->GetFileTransferState() == FileSendTask::CHUNK_STATE_PENDING_COMPLETE);

			auto portionIndex = winsockWrapper.ReadInt(0);
			task->FilePortionComplete(portionIndex);
		}
		break;

		case MESSAGE_ID_FILE_CHUNKS_REMAINING:
		{
			auto taskIter = FileSendTaskList.find(user);
			assert(taskIter != FileSendTaskList.end());

			auto task = (*taskIter).second;
			auto chunkCount = winsockWrapper.ReadInt(0);
			std::unordered_map<int, bool> chunksRemaining;
			for (auto i = 0; i < chunkCount; ++i) chunksRemaining[winsockWrapper.ReadShort(0)] = true;
			task->SetChunksRemaining(chunksRemaining);
		}
		break;
		}
	}
}


void Server::AttemptUserLogin(UserConnection* user, std::string& username, std::string& password)
{
	//  Locate the user entry in the login details list, if possible
	auto userMD5 = md5(username);
	auto userIter = UserLoginDetailsList.find(userMD5);

	//  if the user does not exist
	if (userIter == UserLoginDetailsList.end())
	{
		SendMessage_LoginResponse(false, user);
		return;
	}

	SendMessage_LoginResponse(true, user);

	//  Set the user identifier, and read the Inbox and Notifications data for the user
	user->UserIdentifier = userMD5;
	ReadUserInbox(user);
	ReadUserNotifications(user);
	SendMessage_InboxAndNotifications(user);
	SendMessage_LatestUploads(LatestUploadsList, user);
}

////////////////////////////////////////
//	Program Functionality
////////////////////////////////////////
void Server::LoadUserLoginDetails(void)
{
	std::ifstream uldFile("UserLoginDetails.data");
	assert(!uldFile.bad() && uldFile.good());

	int usernameLength = 0;
	int passwordLength = 0;
	unsigned char encryptedUsername[64];
	unsigned char encryptedPassword[64];
	std::vector<unsigned char> encryptedUsernameVector;
	std::vector<unsigned char> encryptedPasswordVector;
	std::vector<unsigned char> decryptedUsername;
	std::vector<unsigned char> decryptedPassword;

	auto fileSize = int(uldFile.tellg());
	uldFile.seekg(0, std::ios::end);
	fileSize = int(uldFile.tellg()) - fileSize;
	uldFile.seekg(0, std::ios::beg);

	while ((fileSize != 0) && (!uldFile.eof()))
	{
		usernameLength = 0;
		passwordLength = 0;

		uldFile.read((char*)(&usernameLength), sizeof(usernameLength));
		if (usernameLength == 0) break;
		uldFile.read((char*)(encryptedUsername), usernameLength);
		uldFile.read((char*)(&passwordLength), sizeof(passwordLength));
		uldFile.read((char*)(encryptedPassword), passwordLength);

		//  Get the MD5 of the decrypted username and password
		decryptedUsername = Groundfish::Decrypt(encryptedUsername);
		decryptedPassword = Groundfish::Decrypt(encryptedPassword);
		auto loginDataMD5 = md5(std::string((const char*)decryptedUsername.data(), decryptedUsername.size()));

		for (auto i = 0; i < usernameLength; ++i) encryptedUsernameVector.push_back(encryptedUsername[i]);
		for (auto i = 0; i < passwordLength; ++i) encryptedPasswordVector.push_back(encryptedPassword[i]);
		UserLoginDetailsList[loginDataMD5] = UserLoginDetails(encryptedUsernameVector, encryptedPasswordVector);
	}

	uldFile.close();
}

void Server::SaveUserLoginDetails(void)
{
	std::ofstream uldFile("UserLoginDetails.data", std::ios_base::trunc);
	assert(!uldFile.bad() && uldFile.good());

	for (auto userIter = UserLoginDetailsList.begin(); userIter != UserLoginDetailsList.end(); ++userIter)
	{
		auto encryptedUsername = (*userIter).second.first;
		auto encryptedPassword = (*userIter).second.second;
		auto encryptedUsernameSize = int(encryptedUsername.size());
		auto encryptedPasswordSize = int(encryptedPassword.size());

		//  Write the size of the encrypted username, then the encrypted username itself
		uldFile.write((const char*)&encryptedUsernameSize, sizeof(encryptedUsernameSize));
		uldFile.write((const char*)(encryptedUsername.data()), encryptedUsername.size());

		//  Write the size of the encrypted password, then the encrypted password itself
		uldFile.write((const char*)&encryptedPasswordSize, sizeof(encryptedPasswordSize));
		uldFile.write((const char*)(encryptedPassword.data()), encryptedPassword.size());
	}

	uldFile.close();
}

void Server::AddUserLoginDetails(std::string username, std::string password)
{
	//  Check if the MD5 of the unencrypted login details already links to a user login data entry, and if so exit out
	auto loginMD5 = md5(username);
	if (UserLoginDetailsList.find(loginMD5) != UserLoginDetailsList.end()) return;

	//  If it's a new entry, encrypt the username and password, place both encrypted arrays in a vector of unsigned chars
	std::vector<unsigned char> usernameVector = Groundfish::Encrypt(username.c_str(), int(username.length()), 0, rand() % 256);
	std::vector<unsigned char> passwordVector = Groundfish::Encrypt(password.c_str(), int(password.length()), 0, rand() % 256);

	//  Open and close the Inbox and Notifications file for this user, to create them
	std::ofstream userInboxFile("./_UserInbox/" + loginMD5 + ".inbox");
	assert(userInboxFile.good() && !userInboxFile.bad());
	userInboxFile.close();
	std::ofstream userNotificationFile("./_UserNotifications/" + loginMD5 + ".notifications");
	assert(userNotificationFile.good() && !userNotificationFile.bad());
	userNotificationFile.close();

	//  Add a new notification for a welcoming message
	AddUserNotification(loginMD5, "Welcome to New Providence");
	
	//  Add the user data to the login details list, then save off the new user login details list
	UserLoginDetailsList[loginMD5] = UserLoginDetails(usernameVector, passwordVector);
	SaveUserLoginDetails();
}

/*
struct HostedFileData
{
	std::string FileNameMD5;
	std::vector<unsigned char> EncryptedFileName;
	std::vector<unsigned char> EncryptedFileTitle;
	std::vector<unsigned char> EncryptedFileDescription;
};

std::unordered_map<std::string, HostedFileData> HostedFileDataList;
*/

void Server::AddHostedFileFromUnencrypted(std::string fileToAdd, std::string fileTitle, std::string fileDescription)
{
	//  Find the file's primary name (no directories)
	std::string pureFileName = fileToAdd;
	if (fileToAdd.find_last_of('/') != -1) pureFileName = fileToAdd.substr(fileToAdd.find_last_of('/') + 1, fileToAdd.length() - fileToAdd.find_last_of('/') - 1);

	//  If the file already exists in the Hosted File Data List, return out
	auto fileTitleMD5 = md5(fileTitle);
	if (HostedFileDataList.find(fileTitleMD5) != HostedFileDataList.end()) return;

	//  Add the hosted file data to the hosted file data list
	HostedFileData newFile;
	newFile.FileNameMD5 = md5(pureFileName);
	newFile.EncryptedFileName = Groundfish::Encrypt(pureFileName.c_str(), int(pureFileName.length()), 0, rand() % 256);
	newFile.EncryptedFileTitle = Groundfish::Encrypt(fileTitle.c_str(), int(fileTitle.length()), 0, rand() % 256);
	newFile.EncryptedFileDescription = Groundfish::Encrypt(fileDescription.c_str(), int(fileDescription.length()), 0, rand() % 256);
	HostedFileDataList[fileTitleMD5] = newFile;

	//  Add the hosted file data to the HostedFiles.data file and move to the end of it
	std::ofstream hfFile("HostedFiles.data");
	assert(!hfFile.bad() && hfFile.good());

	int md5Length = int(newFile.FileNameMD5.length());
	int encryptedFileNameLength = int(newFile.EncryptedFileName.size());
	int encryptedFileTitleLength = int(newFile.EncryptedFileTitle.size());
	int encryptedFileDescLength = int(newFile.EncryptedFileDescription.size());

	//  Write the hosted file data into the file, then close it
	hfFile.write((const char*)&md5Length, sizeof(md5Length));
	hfFile.write((const char*)newFile.FileNameMD5.c_str(), md5Length);
	hfFile.write((const char*)&encryptedFileNameLength, sizeof(encryptedFileNameLength));
	hfFile.write((const char*)newFile.EncryptedFileName.data(), encryptedFileNameLength);
	hfFile.write((const char*)&encryptedFileTitleLength, sizeof(encryptedFileTitleLength));
	hfFile.write((const char*)newFile.EncryptedFileTitle.data(), encryptedFileTitleLength);
	hfFile.write((const char*)&encryptedFileDescLength, sizeof(encryptedFileDescLength));
	hfFile.write((const char*)newFile.EncryptedFileDescription.data(), encryptedFileDescLength);
	hfFile.close();

	Groundfish::EncryptAndMoveFile(fileToAdd, "./_HostedFiles/" + fileTitleMD5 + ".hostedfile");
	AddLatestUpload(newFile.EncryptedFileTitle);
}

void Server::LoadHostedFilesData(void)
{
	std::ifstream hfFile("HostedFiles.data");
	assert(!hfFile.bad() && hfFile.good());

	int md5Length = 0;
	int fileNameLength = 0;
	int fileTitleLength = 0;
	int fileDescLength = 0;
	unsigned char fileNameMD5[128];
	unsigned char encryptedFileName[128];
	unsigned char encryptedFileTitle[128];
	unsigned char encryptedFileDesc[256];

	auto fileSize = int(hfFile.tellg());
	hfFile.seekg(0, std::ios::end);
	fileSize = int(hfFile.tellg()) - fileSize;
	hfFile.seekg(0, std::ios::beg);

	while ((fileSize != 0) && (!hfFile.eof()))
	{
		md5Length = 0;
		fileNameLength = 0;
		fileTitleLength = 0;
		fileDescLength = 0;

		//  Read in the hosted file data
		hfFile.read((char*)(&md5Length), sizeof(md5Length)); if (md5Length == 0) break;
		hfFile.read((char*)(fileNameMD5), md5Length);
		hfFile.read((char*)(&fileNameLength), sizeof(fileNameLength));
		hfFile.read((char*)(encryptedFileName), fileNameLength);
		hfFile.read((char*)(&fileTitleLength), sizeof(fileTitleLength));
		hfFile.read((char*)(encryptedFileTitle), fileTitleLength);
		hfFile.read((char*)(&fileDescLength), sizeof(fileDescLength));
		hfFile.read((char*)(encryptedFileDesc), fileDescLength);

		//  Create a Hosted File Data struct and store it off in memory
		auto decryptedFileTitle = Groundfish::Decrypt(encryptedFileTitle);
		auto decryptedFileTitleString = std::string((char*)decryptedFileTitle.data(), decryptedFileTitle.size());

		HostedFileData newFile;
		newFile.FileNameMD5 = std::string((char*)fileNameMD5);
		for (auto i = 0; i < fileNameLength; ++i) newFile.EncryptedFileName.push_back(encryptedFileName[i]);
		for (auto i = 0; i < fileTitleLength; ++i) newFile.EncryptedFileTitle.push_back(encryptedFileTitle[i]);
		for (auto i = 0; i < fileDescLength; ++i) newFile.EncryptedFileDescription.push_back(encryptedFileDesc[i]);
		HostedFileDataList[md5(decryptedFileTitleString)] = newFile;
	}

	hfFile.close();
}

void Server::AddLatestUpload(std::vector<unsigned char> newUpload)
{
	LatestUploadsList.push_back(newUpload);
	while (LatestUploadsList.size() > MAX_LATEST_UPLOADS_COUNT) LatestUploadsList.erase(LatestUploadsList.begin());
	SaveLatestUploadsList();
}

void Server::LoadLatestUploadsList()
{
	std::ifstream lulFile("LatestUploads.data", std::ios_base::binary);
	assert(lulFile.good() && !lulFile.bad());

	unsigned char upload[256];
	int size = 0;
	while (!lulFile.eof())
	{
		size = 0;
		memset(upload, 0, 256);
		lulFile.read((char*)&size, sizeof(size)); if (lulFile.eof()) break;
		lulFile.read((char*)&upload, size);

		std::vector<unsigned char> newUpload;
		for (auto i = 0; i < size; ++i) newUpload.push_back(upload[i]);
		LatestUploadsList.push_back(newUpload);
	}
	
	lulFile.close();
}

void Server::SaveLatestUploadsList(void)
{
	std::ofstream lulFile("LatestUploads.data", std::ios_base::binary | std::ios_base::trunc);
	assert(lulFile.good() && !lulFile.bad());

	for (auto iter = LatestUploadsList.begin(); iter != LatestUploadsList.end(); ++iter)
	{
		int size = (*iter).size();
		lulFile.write((char*)&size, sizeof(size));
		lulFile.write((char*)((*iter).data()), size);
	}

	lulFile.close();
}

void Server::ContinueFileTransfers(void)
{
	//  Find all file transfers and send a file chunk for each one if the file transfer is ready
	for (auto taskIter = FileSendTaskList.begin(); taskIter != FileSendTaskList.end(); ++taskIter)
	{
		auto task = (*taskIter).second;

		//  If we aren't ready to send the file, continue out and wait for a ready signal
		if (task->GetFileTransferReady() == false) continue;

		//  If we have data to send, send it and continue out so we can keep sending it until we're done
		if (!task->SendFileChunk()) continue;

		//  If we've gotten this far, we're ready to delete the file send task, as it completed.
		delete task;
		FileSendTaskList.erase(taskIter);
		break;
	}
}


void Server::BeginFileTransfer(const char* fileName, UserConnection* user)
{
	//  Add a new FileSendTask to our list, so it can manage itself
	FileSendTask* newTask = new FileSendTask("./_HostedFiles/" + std::string(fileName), user->SocketID, std::string(user->IPAddress));
	FileSendTaskList[user] = newTask;
}

void Server::SendChatString(const char* chatString)
{
	for (auto userIter = UserConnectionsList.begin(); userIter != UserConnectionsList.end(); ++userIter)
	{
		auto user = (*userIter).first;

		//  Encrypt the string using Groundfish
		std::vector<unsigned char> encryptedChatString = Groundfish::Encrypt(chatString, int(strlen(chatString)) + 1, 0, rand() % 256);

		winsockWrapper.ClearBuffer(0);
		winsockWrapper.WriteChar(MESSAGE_ID_ENCRYPTED_CHAT_STRING, 0);
		winsockWrapper.WriteInt(int(encryptedChatString.size()), 0);
		winsockWrapper.WriteChars(encryptedChatString.data(), int(encryptedChatString.size()), 0);
		winsockWrapper.SendMessagePacket(user->SocketID, user->IPAddress.c_str(), SERVER_PORT, 0);
	}
}