#pragma once

#include "Engine/WinsockWrapper.h"
#include "Engine/SimpleMD5.h"
#include "Engine/SimpleSHA256.h"
#include "Groundfish.h"
#include "FileSendAndReceive.h"
#include "HostedFileData.h"

#include <fstream>
#include <ctime>

#define VERSION_NUMBER					"2018.12.21"

#define NEW_PROVIDENCE_PORT				2347

#define LATEST_UPLOADS_SENT_COUNT		20

#define PING_INTERVAL_TIME				5.0
#define PINGS_BEFORE_DISCONNECT			30

std::list<HostedFileData> HostedFileDataList;


inline bool HostedFileExists(std::string checksum) {
	for (auto iter = HostedFileDataList.begin(); iter != HostedFileDataList.end(); ++iter)
		if ((*iter).FileTitleChecksum.compare(checksum) == 0)
			return true;
	return false;
}


inline std::list<HostedFileData>::iterator GetHostedFileIterByTitleChecksum(std::string checksum) {
	for (auto iter = HostedFileDataList.begin(); iter != HostedFileDataList.end(); ++iter)
		if ((*iter).FileTitleChecksum.compare(checksum) == 0)
			return iter;
	return HostedFileDataList.end();
}


struct UserLoginDetails
{
	std::vector<unsigned char> EncryptedUserName;
	std::vector<unsigned char> EncryptedPassword;

	UserLoginDetails()
	{
		EncryptedUserName.clear();
		EncryptedPassword.clear();
	}
	UserLoginDetails(std::vector<unsigned char> username, std::vector<unsigned char> password) : EncryptedUserName(username), EncryptedPassword(password) {}

	void WriteToFile(std::ofstream& outFile)
	{
		//  Write the username data length
		int usernameSize = EncryptedUserName.size();
		outFile.write((char*)&usernameSize, sizeof(usernameSize));

		//  Write the username data
		outFile.write((char*)EncryptedUserName.data(), usernameSize);

		//  Write the password data length
		int passwordSize = EncryptedUserName.size();
		outFile.write((char*)&passwordSize, sizeof(passwordSize));

		//  Write the password data
		outFile.write((char*)EncryptedPassword.data(), passwordSize);
	}

	bool ReadFromFile(std::ifstream& inFile)
	{
		//  Read the username length
		int usernameSize;
		inFile.read((char*)&usernameSize, sizeof(usernameSize));
		if (inFile.eof()) return false;

		//  Read the username data
		char readInData[512];
		inFile.read(readInData, usernameSize);
		EncryptedUserName.clear();
		for (auto i = 0; i < usernameSize; ++i) EncryptedUserName.push_back((unsigned char)readInData[i]);

		//  Read the password length
		int passwordSize;
		inFile.read((char*)&passwordSize, sizeof(passwordSize));
		if (inFile.eof()) return false;

		//  Read the username data
		inFile.read(readInData, passwordSize);
		EncryptedPassword.clear();
		for (auto i = 0; i < passwordSize; ++i) EncryptedPassword.push_back((unsigned char)readInData[i]);

		return true;
	}
};


bool CompareUploadsByTimeAdded(const HostedFileData& first, const HostedFileData& second)
{
	auto comparison = first.FileUploadTime.compare(second.FileUploadTime);
	return (comparison == 1);
}


struct UserConnection
{
	enum LoginStatus { LOGIN_STATUS_CONNECTED, LOGIN_STATUS_LOGGED_IN, LOGIN_STATUS_COUNT };
	const std::string UserStatusStrings[LOGIN_STATUS_COUNT] = { "Connected", "Logged In" };

	UserConnection() :
		SocketID(-1),
		IPAddress(""),
		LastPingTime(gameSeconds),
		LastPingRequest(0.0),
		InboxCount(0),
		UserIdentifier("UNKNOWN ID"),
		Username("UNKNOWN USER"),
		LoginStatus(LOGIN_STATUS_CONNECTED),
		StatusString("Connected"),
		UserFileSendTask(nullptr),
		UserFileReceiveTask(nullptr)
	{}

	UserConnection(int socketID, std::string ipAddress) :
		SocketID(socketID),
		IPAddress(ipAddress),
		LastPingTime(gameSeconds),
		LastPingRequest(0.0),
		InboxCount(0),
		UserIdentifier("UNKNOWN ID"),
		Username("UNKNOWN USER"),
		LoginStatus(LOGIN_STATUS_CONNECTED),
		StatusString("Connected"),
		UserFileSendTask(nullptr),
		UserFileReceiveTask(nullptr)
	{}

	inline void UpdatePingTime() { LastPingTime = gameSeconds; UpdatePingRequestTime(); }
	inline void UpdatePingRequestTime() { LastPingRequest = gameSeconds; }
	inline std::string GetLoginStatusString() const { return UserStatusStrings[LoginStatus]; }
	inline void SetStatusIdle(int secondsSinceActive = 0) { StatusString = GetLoginStatusString() + ", Idle (last activity " + std::to_string(secondsSinceActive) + " seconds ago)"; }
	inline void SetStatusDownloading(std::string checksum, float percent, int kbps) { StatusString = "Downloading file " + checksum + " [" + std::to_string(int(percent * 100.0f)) + "% @" + std::to_string(kbps) + " KB/s]"; }

	int				SocketID;
	std::string		IPAddress;
	double			LastPingTime;
	double			LastPingRequest;

	int				InboxCount;

	std::string		UserIdentifier;
	std::string		Username;
	LoginStatus		LoginStatus;
	std::string		StatusString;

	std::vector<std::string> NotificationsList;

	FileSendTask*		UserFileSendTask = nullptr;
	FileReceiveTask*	UserFileReceiveTask = nullptr;
};


//  Outgoing message send functions
void SendMessage_PingRequest(UserConnection* user)
{
	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_PING_REQUEST, 0);
	winsockWrapper.SendMessagePacket(user->SocketID, user->IPAddress.c_str(), NEW_PROVIDENCE_PORT, 0);
}


void SendMessage_LoginResponse(LoginResponseIdentifiers response, UserConnection* user)
{
	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_USER_LOGIN_RESPONSE, 0);
	winsockWrapper.WriteInt((int)response, 0);

	if (response == LOGIN_RESPONSE_SUCCESS)
	{
		winsockWrapper.WriteInt(user->InboxCount, 0);
		winsockWrapper.WriteInt(int(user->NotificationsList.size()), 0);
	}

	winsockWrapper.SendMessagePacket(user->SocketID, user->IPAddress.c_str(), NEW_PROVIDENCE_PORT, 0);
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

	winsockWrapper.SendMessagePacket(user->SocketID, user->IPAddress.c_str(), NEW_PROVIDENCE_PORT, 0);
}


void SendMessage_LatestUploads(std::list<HostedFileData>& hostedFileDataList, UserConnection* user, int startIndex = 0)
{
	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_LATEST_UPLOADS_LIST, 0);

	//  Find out the index range we're going to send.
	auto endIndex = std::min(int(hostedFileDataList.size() - 1), startIndex + LATEST_UPLOADS_SENT_COUNT - 1);
	auto listSize = std::max(0, endIndex - startIndex + 1);

	winsockWrapper.WriteUnsignedShort(uint16_t(startIndex), 0);
	winsockWrapper.WriteUnsignedShort(uint16_t(listSize), 0);

	if (listSize > 0)
	{
		int iterIndex = 0;
		for (auto iter = hostedFileDataList.begin(); iter != hostedFileDataList.end(); ++iter)
		{
			if (iterIndex++ < startIndex) continue;
			if (--listSize < 0) break;
			auto title = (*iter).EncryptedFileTitle;
			assert(title.size() <= ENCRYPTED_TITLE_MAX_SIZE);
			winsockWrapper.WriteChar((unsigned char)(title.size()), 0);
			winsockWrapper.WriteChars(title.data(), int(title.size()), 0);
		}
	}

	winsockWrapper.SendMessagePacket(user->SocketID, user->IPAddress.c_str(), NEW_PROVIDENCE_PORT, 0);
}


void SendMessage_FileRequestFailed(std::string fileID, std::string failureReason, UserConnection* user)
{
	//  Send a "File Request" message
	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_FILE_REQUEST_FAILED, 0);
	winsockWrapper.WriteString(fileID.c_str(), 0);
	winsockWrapper.WriteString(failureReason.c_str(), 0);
	winsockWrapper.SendMessagePacket(user->SocketID, user->IPAddress.c_str(), NEW_PROVIDENCE_PORT, 0);
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



class Server
{
private:
	int		ServerSocketHandle;

	std::function<void(const std::list<HostedFileData >&)> HostedFileListChangedCallback = nullptr;
	std::function<void(std::unordered_map<UserConnection*, bool>&)> UserConnectionListChangedCallback = nullptr;

	std::unordered_map<std::string, UserLoginDetails> UserLoginDetailsList;
	std::unordered_map<UserConnection*, bool> UserConnectionsList;

	inline UserConnection* FindUserByUserID(std::string userID)
	{
		for (auto iter = UserConnectionsList.begin(); iter != UserConnectionsList.end(); ++iter)
			if ((*iter).first->UserIdentifier == userID) return (*iter).first;
		return nullptr;
	}

public:
	Server() :
		ServerSocketHandle(-1)
	{}

	~Server() {}

	inline void SetHostedFileListChangedCallback(const std::function<void(const std::list<HostedFileData>& hostedFileDataList)>& callback) { HostedFileListChangedCallback = callback; }
	inline void SetUserConnectionListChangedCallback(const std::function<void(std::unordered_map<UserConnection*, bool>&)>& callback) { UserConnectionListChangedCallback = callback; }
	inline const std::list<HostedFileData>& GetHostedFileDataList() const { return HostedFileDataList; }

	bool Initialize(void);
	void MainProcess(void);
	void Shutdown(void);

	void DeleteHostedFile(std::string fileChecksum);

private:
	void AddClient(int socketID, std::string ipAddress);
	void RemoveClient(UserConnection* user);
	void AcceptNewClients(void);
	void ReceiveMessages(void);
	void PingConnectedUsers(void);
	void AttemptUserLogin(UserConnection* user, std::string& username, std::string& password);

	void LoadUserLoginDetails(void);
	void SaveUserLoginDetails(void);
	void AddUserLoginDetails(std::string username, std::string password);

	void AddHostedFileFromEncrypted(std::string fileToAdd, std::string fileTitle, std::string fileDescription, int32_t fileTypeID, int32_t fileSubTypeID);
	void AddHostedFileFromUnencrypted(std::string fileToAdd, std::string fileTitle, std::string fileDescription);
	void SaveHostedFileList();
	void LoadHostedFilesData(void);
	void SendOutLatestUploadsList(void);

	void ContinueFileTransfers(void);
	void BeginFileTransfer(HostedFileData& fileData, UserConnection* user);
	void UpdateFileTransferPercentage(UserConnection* user);
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
	ServerSocketHandle = winsockWrapper.TCPListen(NEW_PROVIDENCE_PORT, 10, 1);
	if (ServerSocketHandle == -1) return false;
	winsockWrapper.SetNagle(ServerSocketHandle, true);

	//  ensure there is an Inbox, Notifications, and Files folder
	(void)  _wmkdir(L"_UserInbox");
	(void) _wmkdir(L"_UserNotifications");
	(void) _wmkdir(L"_HostedFiles");

	//  Load the hosted files list
	LoadHostedFilesData();

	//  Load all user login details into an accessible list
	LoadUserLoginDetails();
	
	//  Add default test users if they aren't already on the list
	AddUserLoginDetails("Drew", "testpass");
	AddUserLoginDetails("Charlie", "testpass");
	AddUserLoginDetails("Emerson", "testpass");
	AddUserLoginDetails("Bex", "testpass");
	AddUserLoginDetails("Jason", "testpass");

	//  Load the default hosted files if they aren't already loaded
	//AddHostedFileFromUnencrypted("_FilesToHost/TestImage1.png", "Test Image 1", "DESCRIPTION 1");
	//AddHostedFileFromUnencrypted("_FilesToHost/TestImage2.png", "Test Image 2", "DESCRIPTION 2");
	//AddHostedFileFromUnencrypted("_FilesToHost/TestImage3.png", "Test Image 3", "DESCRIPTION 3");
	//AddHostedFileFromUnencrypted("_FilesToHost/The Thirteenth Floor (1999 - 1080p).mp4", "The Thirteenth Floor (1999 - 1080p)", "A computer scientist running a virtual reality simulation of 1937 becomes the primary suspect when his colleague and mentor is murdered.");
	//AddHostedFileFromUnencrypted("_FilesToHost/Purity Ring - Lofticries.mp3", "Purity Ring - Lofticries", "From the album \"Shrines\"");

	return true;
}


void Server::MainProcess(void)
{
	// Accept Incoming Connections
	AcceptNewClients();

	// Receive messages
	ReceiveMessages();

	// Ping connected users
	PingConnectedUsers();

	//  Send files
	ContinueFileTransfers();
}


void Server::Shutdown(void)
{
	//  Close out the server socket
	winsockWrapper.CloseSocket(ServerSocketHandle);
}


void Server::DeleteHostedFile(std::string fileChecksum)
{
	//  If the file does not exist, exit out
	auto iter = GetHostedFileIterByTitleChecksum(fileChecksum);
	assert(iter != HostedFileDataList.end());
	if (iter == HostedFileDataList.end()) return;

	//  Delete the file from the _HostedFiles folder
	auto hostedFileName = "_HostedFiles/" + (*iter).FileTitleChecksum + ".hostedfile";
	std::remove(hostedFileName.c_str());

	//  Updated the Hosted File Data List
	HostedFileDataList.erase(iter);
	SaveHostedFileList();
	if (HostedFileListChangedCallback != nullptr) HostedFileListChangedCallback(GetHostedFileDataList());
	SendOutLatestUploadsList();
}


////////////////////////////////////////
//	Client Connection Functions
////////////////////////////////////////

void Server::AddClient(int socketID, std::string ipAddress)
{
	auto newConnection = new UserConnection(socketID, ipAddress);
	UserConnectionsList[newConnection] = true;
	
	if (UserConnectionListChangedCallback != nullptr) UserConnectionListChangedCallback(UserConnectionsList);
}

void Server::RemoveClient(UserConnection* user)
{
	auto userIter = UserConnectionsList.find(user);
	assert(userIter != UserConnectionsList.end());

	delete user;
	UserConnectionsList.erase(userIter);

	if (UserConnectionListChangedCallback != nullptr) UserConnectionListChangedCallback(UserConnectionsList);
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
		auto messageBufferSize = winsockWrapper.ReceiveMessagePacket(user->SocketID, 0);

		//  If no message has arrived, move on to the next connection
		if (messageBufferSize < 0) continue;

		//  If we receive a disconnection indicator, remove the user and move on to the next connection
		if (messageBufferSize == 0)
		{
			RemoveClient(user);
			break;
		}

		//  Update the last time we heard from this user
		user->UpdatePingTime();

		char messageID = winsockWrapper.ReadChar(0);
		switch (messageID)
		{
		case MESSAGE_ID_PING_RESPONSE:
		{
			//  NO DATA

			user->SetStatusIdle(0);
			if (UserConnectionListChangedCallback != nullptr) UserConnectionListChangedCallback(UserConnectionsList);
			//  Do nothing, as we've already updated the last ping time of the user
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

			SendChatString(decryptedString.c_str());
		}
		break;

		case MESSAGE_ID_USER_LOGIN_REQUEST: // Player enters the server, sending their encrypted name and password
		{
			//  (int) Length of encrypted username (n1)
			//  (n1-size chars array) Encrypted username
			//  (int) Length of encrypted password (n2)
			//  (n2-size chars array) Encrypted password

			//  Grab the current client version number to ensure they have the up-to-date version
			std::string versionString = winsockWrapper.ReadString(0);

			//  Grab the username size and encrypted username, then the password size and encrypted password
			auto usernameSize = winsockWrapper.ReadInt(0);
			auto encryptedUsername = winsockWrapper.ReadChars(0, usernameSize);
			auto encryptedUsernameVector = std::vector<unsigned char>(encryptedUsername, encryptedUsername + usernameSize);

			auto passwordSize = winsockWrapper.ReadInt(0);
			auto encryptedPassword = winsockWrapper.ReadChars(0, passwordSize);
			auto encryptedPasswordVector = std::vector<unsigned char>(encryptedPassword, encryptedPassword + passwordSize);

			//  Decrypt the username and password
			std::vector<unsigned char> decryptedUsername = Groundfish::Decrypt(encryptedUsernameVector.data());
			std::string username = std::string((char*)decryptedUsername.data(), decryptedUsername.size());
			std::vector<unsigned char> decryptedPassword = Groundfish::Decrypt(encryptedPasswordVector.data());
			std::string password = std::string((char*)decryptedPassword.data(), decryptedPassword.size());

			if (versionString.compare(VERSION_NUMBER) != 0)
			{
				SendMessage_LoginResponse(LOGIN_RESPONSE_VERSION_NUMBER_INCORRECT, user);
				break;
			}

			AttemptUserLogin(user, username, password);
		}
		break;

		case MESSAGE_ID_REQUEST_LATEST_UPLOADS:
		{
			auto startingIndex = int(winsockWrapper.ReadUnsignedShort(0));
			SendMessage_LatestUploads(HostedFileDataList, user, startingIndex);
		}
		break;

		case MESSAGE_ID_FILE_REQUEST:
		{
			auto fileNameLength = winsockWrapper.ReadInt(0);
			auto fileTitle = std::string((char*)winsockWrapper.ReadChars(0, fileNameLength), fileNameLength);

			auto hostedFileIdentifier = md5(fileTitle);
			auto hostedFileIter = GetHostedFileIterByTitleChecksum(hostedFileIdentifier);
			if (hostedFileIter == HostedFileDataList.end())
			{
				//  The file could not be found.
				SendMessage_FileRequestFailed(fileTitle, "The specified file was not found.", user);
				break;
			}
			else if (user->UserFileSendTask != nullptr)
			{
				//  The user is already downloading something.
				SendMessage_FileRequestFailed(fileTitle, "User is currently already downloading a file.", user);
				break;
			}
			else
			{
				auto hostedFileData = (*hostedFileIter);
				BeginFileTransfer(hostedFileData, user);
				break;
			}
		}
		break;

		case MESSAGE_ID_FILE_SEND_INIT:
		{
			auto fileNameSize = winsockWrapper.ReadInt(0);
			auto fileTitleSize = winsockWrapper.ReadInt(0);
			auto fileDescriptionSize = winsockWrapper.ReadInt(0);

			//  Decrypt the file name using Groundfish and save it off
			auto decryptedFileNameVector = Groundfish::Decrypt(winsockWrapper.ReadChars(0, fileNameSize));
			auto decryptedFileNamePure = std::string((char*)decryptedFileNameVector.data(), decryptedFileNameVector.size());
			auto decryptedFileName = "./_DownloadedFiles/" + decryptedFileNamePure;

			//  Decrypt the file title using Groundfish and save it off
			auto decryptedFileTitleVector = Groundfish::Decrypt(winsockWrapper.ReadChars(0, fileTitleSize));
			auto decryptedFileTitle = std::string((char*)decryptedFileTitleVector.data(), decryptedFileTitleVector.size());
			assert(decryptedFileTitle.length() <= UPLOAD_TITLE_MAX_LENGTH);

			//  Decrypt the file description using Groundfish and save it off
			auto decryptedFileDescriptionVector = Groundfish::Decrypt(winsockWrapper.ReadChars(0, fileDescriptionSize));
			auto decryptedFileDescription = std::string((char*)decryptedFileDescriptionVector.data(), decryptedFileDescriptionVector.size());

			//  grab the file type and sub type
			auto fileTypeID = winsockWrapper.ReadShort(0);
			auto fileSubTypeID = winsockWrapper.ReadShort(0);

			//  Grab the file size, file chunk size, and buffer count
			auto fileSize = winsockWrapper.ReadLongInt(0);
			auto fileChunkSize = winsockWrapper.ReadLongInt(0);
			auto fileChunkBufferCount = winsockWrapper.ReadLongInt(0);

			if (user->UserFileReceiveTask != nullptr) return;

			//  Create a new file receive task
			(void) _wmkdir(L"_DownloadedFiles");
			user->UserFileReceiveTask = new FileReceiveTask(decryptedFileName, decryptedFileTitle, decryptedFileDescription, fileTypeID, fileSubTypeID, fileSize, fileChunkSize, fileChunkBufferCount, "./_DownloadedFiles/_download.tempfile", user->SocketID, user->IPAddress, NEW_PROVIDENCE_PORT);
			user->UserFileReceiveTask->SetDecryptWhenReceived(false);
		}
		break;

		case MESSAGE_ID_FILE_RECEIVE_READY:
		{
			auto task = user->UserFileSendTask;
			assert(task != nullptr);

			task->SetFileTransferState(FileSendTask::CHUNK_STATE_SENDING);
		}
		break;

		case MESSAGE_ID_FILE_PORTION:
		{
			if (user->UserFileReceiveTask == nullptr) break;

			if (user->UserFileReceiveTask->ReceiveFileChunk())
			{
				assert(false); // ALERT: We should never be receiving a chunk after the file download is complete

				//  If ReceiveFile returns true, the transfer is complete
				delete user->UserFileReceiveTask;
				user->UserFileReceiveTask = nullptr;

#if FILE_TRANSFER_DEBUGGING
				debugConsole->AddDebugConsoleLine("FileReceiveTask deleted...");
#endif
			}
		}
		break;

		case MESSAGE_ID_FILE_PORTION_COMPLETE:
		{
			auto portionIndex = winsockWrapper.ReadInt(0);
			auto fileTask = user->UserFileReceiveTask;

			if (fileTask == nullptr)
			{
				SendMessage_FilePortionCompleteConfirmation(portionIndex, user->SocketID, user->IPAddress.c_str(), NEW_PROVIDENCE_PORT);
				break;
			}

			(void) _wmkdir(L"_DownloadedFiles");
			if (fileTask->CheckFilePortionComplete(portionIndex))
			{
				//if (DownloadPercentCompleteCallback != nullptr) DownloadPercentCompleteCallback(FileReceive->GetPercentageComplete(), FileReceive->GetDownloadTime(), FileReceive->GetFileSize());

				if (fileTask->GetFileTransferComplete())
				{
					AddHostedFileFromEncrypted(fileTask->GetFileName(), fileTask->GetFileTitle(), fileTask->GetFileDescription(), fileTask->GetFileTypeID(), fileTask->GetFileSubTypeID());

					delete user->UserFileReceiveTask;
					user->UserFileReceiveTask = nullptr;

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
			auto task = user->UserFileSendTask;
			assert(task != nullptr);

			auto chunkCount = winsockWrapper.ReadInt(0);
			std::unordered_map<int, bool> chunksRemaining;
			for (auto i = 0; i < chunkCount; ++i) chunksRemaining[winsockWrapper.ReadShort(0)] = true;
			task->SetChunksRemaining(chunksRemaining);
		}
		break;

		case MESSAGE_ID_FILE_PORTION_COMPLETE_CONFIRM:
		{
			auto portionIndex = winsockWrapper.ReadLongInt(0);

			auto task = user->UserFileSendTask;
			if (task == nullptr) break;

			if (task->GetFileTransferState() != FileSendTask::CHUNK_STATE_PENDING_COMPLETE) break;

			task->ConfirmFilePortionSendComplete(portionIndex);

			//  Update the user list to reflect if we've updated % of file transferred
			UpdateFileTransferPercentage(user);
			if (UserConnectionListChangedCallback != nullptr) UserConnectionListChangedCallback(UserConnectionsList);
		}
		break;

		default:
			debugConsole->AddDebugConsoleLine("Unknown message ID received: " + std::to_string(messageID));
			//assert(false);
			break;
		}
	}
}


void Server::PingConnectedUsers(void)
{
	for (auto userIter = UserConnectionsList.begin(); userIter != UserConnectionsList.end(); ++userIter)
	{
		auto user = (*userIter).first;
		auto timeSinceLastPing = gameSeconds - user->LastPingTime;
		auto timeSinceLastRequest = gameSeconds - user->LastPingRequest;
		if (timeSinceLastRequest < PING_INTERVAL_TIME) continue;
		if (timeSinceLastPing < PING_INTERVAL_TIME) continue;

		if (timeSinceLastPing > (PINGS_BEFORE_DISCONNECT * PING_INTERVAL_TIME))
		{
			RemoveClient(user);
			break;
		}
		else
		{
			SendMessage_PingRequest(user);
			user->UpdatePingRequestTime();
			user->SetStatusIdle(int(gameSeconds - user->LastPingTime));
			if (UserConnectionListChangedCallback != nullptr) UserConnectionListChangedCallback(UserConnectionsList);
		}
	}
}


void Server::AttemptUserLogin(UserConnection* user, std::string& username, std::string& password)
{
	//  Lowercase the username and password
	std::transform(username.begin(), username.end(), username.begin(), ::tolower);
	std::transform(password.begin(), password.end(), password.begin(), ::tolower);

	//  Locate the user entry in the login details list, if possible
	auto loginDataChecksum = md5(username + password);

	//  If the user does not exist, send a failure message and return out
	if (UserLoginDetailsList.find(loginDataChecksum) == UserLoginDetailsList.end())
	{
		SendMessage_LoginResponse(LOGIN_RESPONSE_PASSWORD_INCORRECT, user);
		return;
	}

	//  If the username given is already assigned to a connected user, send a failure message and return out
	if (FindUserByUserID(loginDataChecksum) != nullptr)
	{
		SendMessage_LoginResponse(LOGIN_RESPONSE_USER_ALREADY_LOGGED_IN, user);
		return;
	}

	SendMessage_LoginResponse(LOGIN_RESPONSE_SUCCESS, user);

	//  Set the user identifier and name
	user->UserIdentifier = loginDataChecksum;
	user->Username = username;
	user->LoginStatus = UserConnection::LOGIN_STATUS_LOGGED_IN;
	user->SetStatusIdle();

	//  Read the Inbox and Notifications data for the user
	ReadUserInbox(user);
	ReadUserNotifications(user);
	SendMessage_InboxAndNotifications(user);
	SendMessage_LatestUploads(HostedFileDataList, user);

	if (UserConnectionListChangedCallback != nullptr) UserConnectionListChangedCallback(UserConnectionsList);
}

////////////////////////////////////////
//	Program Functionality
////////////////////////////////////////
void Server::LoadUserLoginDetails(void)
{
	//  Open the file as an ofstream to ensure it exists
	std::ofstream uldFileCreate("UserLoginDetails.data", std::ios_base::binary | std::ios_base::app);
	assert(uldFileCreate.good() && !uldFileCreate.bad());
	uldFileCreate.close();

	//  Open the file as an ifstream for reading
	std::ifstream uldFile("UserLoginDetails.data", std::ios_base::binary);
	assert(!uldFile.bad() && uldFile.good());

	std::vector<unsigned char> decryptedUsername;
	std::vector<unsigned char> decryptedPassword;

	//  Get the file size by reading the beginning and end memory positions
	auto fileSize = int(uldFile.tellg());
	uldFile.seekg(0, std::ios::end);
	fileSize = int(uldFile.tellg()) - fileSize;
	uldFile.seekg(0, std::ios::beg);

	while (fileSize > 0 && !uldFile.eof())
	{
		UserLoginDetails newEntry;
		if (!newEntry.ReadFromFile(uldFile)) break;
		if (uldFile.eof()) break;

		//  Get the MD5 of the decrypted username and password
		decryptedUsername = Groundfish::Decrypt(newEntry.EncryptedUserName.data());
		std::string username = std::string((char*)decryptedUsername.data(), decryptedUsername.size());
		decryptedPassword = Groundfish::Decrypt(newEntry.EncryptedPassword.data());
		std::string password = std::string((char*)decryptedPassword.data(), decryptedPassword.size());
		auto loginDataMD5 = md5(username + password);

		UserLoginDetailsList[loginDataMD5] = newEntry;
	}

	uldFile.close();
}

void Server::SaveUserLoginDetails(void)
{
	std::ofstream uldFile("UserLoginDetails.data", std::ios_base::binary | std::ios_base::trunc);
	assert(!uldFile.bad() && uldFile.good());

	for (auto userIter = UserLoginDetailsList.begin(); userIter != UserLoginDetailsList.end(); ++userIter)
	{
		auto encryptedUsername = (*userIter).second.EncryptedUserName;
		auto encryptedPassword = (*userIter).second.EncryptedPassword;
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
	//  Lowercase the username and password
	std::transform(username.begin(), username.end(), username.begin(), ::tolower);
	std::transform(password.begin(), password.end(), password.begin(), ::tolower);

	//  Check if the MD5 of the unencrypted login details already links to a user login data entry, and if so exit out
	auto loginDataMD5 = md5(username + password);
	if (UserLoginDetailsList.find(loginDataMD5) != UserLoginDetailsList.end()) return;

	//  If it's a new entry, encrypt the username and password, place both encrypted arrays in a vector of unsigned chars
	std::vector<unsigned char> usernameVector = Groundfish::Encrypt(username.c_str(), int(username.length()), 0, rand() % 256);
	std::vector<unsigned char> passwordVector = Groundfish::Encrypt(password.c_str(), int(password.length()), 0, rand() % 256);

	//  Open and close the Inbox and Notifications file for this user, to create them
	std::ofstream userInboxFile("./_UserInbox/" + loginDataMD5 + ".inbox");
	assert(userInboxFile.good() && !userInboxFile.bad());
	userInboxFile.close();
	std::ofstream userNotificationFile("./_UserNotifications/" + loginDataMD5 + ".notifications");
	assert(userNotificationFile.good() && !userNotificationFile.bad());
	userNotificationFile.close();

	//  Add a new notification for a welcoming message
	AddUserNotification(loginDataMD5, "Welcome to New Providence");
	
	//  Add the user data to the login details list, then save off the new user login details list
	UserLoginDetailsList[loginDataMD5] = UserLoginDetails(usernameVector, passwordVector);
	SaveUserLoginDetails();
}


void Server::AddHostedFileFromEncrypted(std::string fileToAdd, std::string fileTitle, std::string fileDescription, int32_t fileTypeID, int32_t fileSubTypeID)
{
	assert(fileTitle.length() <= UPLOAD_TITLE_MAX_LENGTH);

	//  Test that the file exists and is readable, and exit out if it is not
	std::ifstream targetFile(fileToAdd, std::ios_base::binary);
	if (!targetFile.good() || targetFile.bad()) return;
	auto fileSize = int(targetFile.tellg());
	targetFile.seekg(0, std::ios::end);
	fileSize = int(targetFile.tellg()) - fileSize;
	targetFile.seekg(0, std::ios::beg);
	targetFile.close();

	//  Find the file's primary name (no directories)
	std::string pureFileName = fileToAdd;
	if (fileToAdd.find_last_of('/') != -1) pureFileName = fileToAdd.substr(fileToAdd.find_last_of('/') + 1, fileToAdd.length() - fileToAdd.find_last_of('/') - 1);

	//  If the file already exists in the Hosted File Data List, return out
	auto fileTitleMD5 = md5(fileTitle);
	auto fileDataIter = GetHostedFileIterByTitleChecksum(fileTitleMD5);
	assert(fileDataIter == HostedFileDataList.end());
	if (fileDataIter != HostedFileDataList.end()) return;

	//  Add the hosted file data to the hosted file data list, then save the hosted file data list
	HostedFileData newFile;
	newFile.FileTitleChecksum = fileTitleMD5;
	newFile.EncryptedFileName = Groundfish::Encrypt(pureFileName.c_str(), int(pureFileName.length()), 0, rand() % 256);
	newFile.EncryptedFileTitle = Groundfish::Encrypt(fileTitle.c_str(), int(fileTitle.length()), 0, rand() % 256);
	newFile.EncryptedFileDescription = Groundfish::Encrypt(fileDescription.c_str(), int(fileDescription.length()), 0, rand() % 256);
	newFile.EncryptedUploader = Groundfish::Encrypt("SERVER", strlen("SERVER"), 0, rand() % 256);
	newFile.FileSize = fileSize;
	newFile.FileUploadTime = GetCurrentTimeString();
	newFile.FileType = HostedFileType(fileTypeID);
	newFile.FileSubType = HostedFileSubtype(fileSubTypeID);

	HostedFileDataList.push_back(newFile);
	HostedFileDataList.sort(CompareUploadsByTimeAdded);
	SaveHostedFileList();
	HostedFileListChangedCallback(HostedFileDataList);

	//  If the file is not already in /_HostedFiles then move it in
	auto hostedFileName = "./_HostedFiles/" + fileTitleMD5 + ".hostedfile";
	std::ifstream uldFile(hostedFileName);
	auto fileExists = (!uldFile.bad() && uldFile.good());
	uldFile.close();
	if (!fileExists) std::rename(fileToAdd.c_str(), hostedFileName.c_str());

	SendOutLatestUploadsList();
}


void Server::AddHostedFileFromUnencrypted(std::string fileToAdd, std::string fileTitle, std::string fileDescription)
{
	assert(fileTitle.length() <= UPLOAD_TITLE_MAX_LENGTH);

	//  Test that the file exists and is readable, and exit out if it is not
	std::ifstream targetFile(fileToAdd, std::ios_base::binary);
	if (!targetFile.good() || targetFile.bad()) return;
	auto fileSize = int(targetFile.tellg());
	targetFile.seekg(0, std::ios::end);
	fileSize = int(targetFile.tellg()) - fileSize;
	targetFile.seekg(0, std::ios::beg);
	targetFile.close();

	//  Find the file's primary name (no directories)
	std::string pureFileName = fileToAdd;
	if (fileToAdd.find_last_of('/') != -1) pureFileName = fileToAdd.substr(fileToAdd.find_last_of('/') + 1, fileToAdd.length() - fileToAdd.find_last_of('/') - 1);

	//  If the file already exists in the Hosted File Data List, return out
	auto fileTitleMD5 = md5(fileTitle);
	if (HostedFileExists(fileTitleMD5)) return;

	//  Add the hosted file data to the hosted file data list, then save the hosted file data list
	HostedFileData newFile;
	newFile.FileTitleChecksum = fileTitleMD5;
	newFile.EncryptedFileName = Groundfish::Encrypt(pureFileName.c_str(), int(pureFileName.length()), 0, rand() % 256);
	newFile.EncryptedFileTitle = Groundfish::Encrypt(fileTitle.c_str(), int(fileTitle.length()), 0, rand() % 256);
	newFile.EncryptedFileDescription = Groundfish::Encrypt(fileDescription.c_str(), int(fileDescription.length()), 0, rand() % 256);
	newFile.EncryptedUploader = Groundfish::Encrypt("SERVER", strlen("SERVER"), 0, rand() % 256);
	newFile.FileSize = fileSize;
	newFile.FileUploadTime = GetCurrentTimeString();
	HostedFileDataList.push_back(newFile);
	HostedFileDataList.sort(CompareUploadsByTimeAdded);
	SaveHostedFileList();

	//  If the file is not already in /_HostedFiles then encrypt it and move it
	auto hostedFileName = "./_HostedFiles/" + fileTitleMD5 + ".hostedfile";
	std::ifstream uldFile(hostedFileName);
	auto fileExists = (!uldFile.bad() && uldFile.good());
	uldFile.close();
	if (!fileExists) Groundfish::EncryptAndMoveFile(fileToAdd, hostedFileName);

	SendOutLatestUploadsList();
}


void Server::SaveHostedFileList()
{
	//  Open the file as an ofstream to ensure it exists
	std::ofstream hfFileCreate("HostedFiles.data", std::ios_base::binary | std::ios_base::app);
	assert(hfFileCreate.good() && !hfFileCreate.bad());
	hfFileCreate.close();

	//  Open the file as an ifstream for reading
	std::ofstream hfFile("HostedFiles.data", std::ios_base::binary | std::ios_base::trunc);
	assert(!hfFile.bad() && hfFile.good());

	//  Write the data for all hosted files
	for (auto iter = HostedFileDataList.begin(); iter != HostedFileDataList.end(); ++iter)
		(*iter).WriteToFile(hfFile);

	hfFile.close();
}


void Server::LoadHostedFilesData(void)
{
	//  Open the file as an ofstream to ensure it exists
	std::ofstream hfFileCreate("HostedFiles.data", std::ios_base::binary | std::ios_base::app);
	assert(hfFileCreate.good() && !hfFileCreate.bad());
	hfFileCreate.close();

	//  Open the file as an ifstream for reading
	std::ifstream hfFile("HostedFiles.data", std::ios_base::binary);
	assert(!hfFile.bad() && hfFile.good());

	//  Get the file size by reading the beginning and end memory positions
	auto fileSize = int(hfFile.tellg());
	hfFile.seekg(0, std::ios::end);
	fileSize = int(hfFile.tellg()) - fileSize;
	hfFile.seekg(0, std::ios::beg);

	//  Read the data for all hosted files
	while (fileSize > 0 && !hfFile.eof())
	{
		HostedFileData newEntry;
		if (!newEntry.ReadFromFile(hfFile)) break;
		if (hfFile.eof()) break;

		HostedFileDataList.push_back(newEntry);
		HostedFileDataList.sort(CompareUploadsByTimeAdded);
	}

	hfFile.close();
}


void Server::SendOutLatestUploadsList(void)
{
	//  Send the latest uploads list to each connected user
	for (auto userIter = UserConnectionsList.begin(); userIter != UserConnectionsList.end(); ++userIter)
	{
		auto user = (*userIter).first;

		//  If the user isn't logged in yet, skip over them. They'll get an update when they log in
		if (user->LoginStatus != UserConnection::LOGIN_STATUS_LOGGED_IN) continue;

		SendMessage_LatestUploads(HostedFileDataList, user);
	}
}


void Server::ContinueFileTransfers(void)
{
	//  Find all file transfers and send a file chunk for each one if the file transfer is ready
	for (auto userIter = UserConnectionsList.begin(); userIter != UserConnectionsList.end(); ++userIter)
	{
		auto user = (*userIter).first;
		if (user->UserFileSendTask == nullptr) continue;

		//  If we aren't ready to send the file, continue out and wait for a ready signal
		if (user->UserFileSendTask->GetFileTransferState() == FileSendTask::CHUNK_STATE_INITIALIZING) continue;

		if (user->UserFileSendTask->GetFileTransferComplete())
		{
			//  If the file send is complete, delete the file send task and move on
			delete user->UserFileSendTask;
			user->UserFileSendTask = nullptr;

#if FILE_TRANSFER_DEBUGGING
			debugConsole->AddDebugConsoleLine("FileSendTask deleted...");
#endif

			user->SetStatusIdle();
			if (UserConnectionListChangedCallback != nullptr) UserConnectionListChangedCallback(UserConnectionsList);
			break;
		}

		//  If we get this far, we have data to send. Send it and continue out
		user->UserFileSendTask->SendFileChunk();
	}
}


void Server::BeginFileTransfer(HostedFileData& fileData, UserConnection* user)
{
	//  Decrypt the file name and hosted file path
	auto decryptedFileNameVector = Groundfish::Decrypt(fileData.EncryptedFileName.data());
	auto fileName = std::string((char*)decryptedFileNameVector.data(), decryptedFileNameVector.size());
	auto filePath = "./_HostedFiles/" + fileData.FileTitleChecksum + ".hostedfile";

	//  Get the file type and sub-type
	auto fileTypeID = fileData.FileType;
	auto fileSubTypeID = fileData.FileSubType;

	//  Decrypt the file title
	auto decryptedFileTitleVector = Groundfish::Decrypt(fileData.EncryptedFileTitle.data());
	auto fileTitle = std::string((char*)decryptedFileTitleVector.data(), decryptedFileTitleVector.size());

	//  Add a new FileSendTask to our list, so it can manage itself
	FileSendTask* newTask = new FileSendTask(fileName, fileTitle, filePath, fileTypeID, fileSubTypeID, user->SocketID, std::string(user->IPAddress), NEW_PROVIDENCE_PORT);
	user->UserFileSendTask = newTask;
	UpdateFileTransferPercentage(user);
}


void Server::UpdateFileTransferPercentage(UserConnection* user)
{
	auto fileTitleMD5 = md5(user->UserFileSendTask->GetFileTitle());
	auto fileDataIter = GetHostedFileIterByTitleChecksum(fileTitleMD5);
	if (fileDataIter == HostedFileDataList.end()) return;
	auto fileData = (*fileDataIter);
	user->SetStatusDownloading(fileData.FileTitleChecksum, float(user->UserFileSendTask->GetPercentageComplete()), int(float(user->UserFileSendTask->GetEstimatedTransferSpeed()) / 1024.0f));
	if (UserConnectionListChangedCallback != nullptr) UserConnectionListChangedCallback(UserConnectionsList);
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
		winsockWrapper.SendMessagePacket(user->SocketID, user->IPAddress.c_str(), NEW_PROVIDENCE_PORT, 0);
	}
}