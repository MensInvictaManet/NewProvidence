#pragma once

#include "Engine/WinsockWrapper.h"
#include "Engine/SimpleMD5.h"
#include "Engine/SimpleSHA256.h"
#include "Groundfish.h"
#include "FileSendAndReceive.h"

#include <fstream>
#include <ctime>

#define NEW_PROVIDENCE_PORT				2347
#define MAX_LATEST_UPLOADS_COUNT		7

struct HostedFileData
{
	std::string FileTitleMD5;
	std::vector<unsigned char> EncryptedFileName;
	std::vector<unsigned char> EncryptedFileTitle;
	std::vector<unsigned char> EncryptedFileDescription;
	std::vector<unsigned char> EncryptedUploader;
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

	std::function<void(std::unordered_map<UserConnection*, bool>&)> UserConnectionListChangedCallback = nullptr;

	typedef std::pair<std::vector<unsigned char>, std::vector<unsigned char>> UserLoginDetails;
	std::unordered_map<std::string, UserLoginDetails> UserLoginDetailsList;
	std::unordered_map<std::string, HostedFileData> HostedFileDataList;
	std::unordered_map<UserConnection*, bool> UserConnectionsList;
	std::unordered_map<UserConnection*, FileSendTask*> FileSendTaskList; // NOTE: Key is the client ID, so we should limit them to one transfer in the future
	std::vector< std::vector<unsigned char> > LatestUploadsList;

public:
	Server() {}
	~Server() {}

	inline void SetUserConnectionListChangedCallback(const std::function<void(std::unordered_map<UserConnection*, bool>&)>& callback) { UserConnectionListChangedCallback = callback; }

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
	void BeginFileTransfer(HostedFileData& fileData, UserConnection* user);
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
	_wmkdir(L"_UserInbox");
	_wmkdir(L"_UserNotifications");
	_wmkdir(L"_HostedFiles");

	//  Load the list of latest uploads to the server
	LoadLatestUploadsList();

	//  Load all user login details into an accessible list
	LoadUserLoginDetails();
	//AddUserLoginDetails("Drew", "testpass1");
	//AddUserLoginDetails("Charlie", "testpass2");
	//AddUserLoginDetails("Emerson", "testpass3");
	//AddUserLoginDetails("Bex", "testpass4");

	//  Load the hosted files list
	LoadHostedFilesData();
	//AddHostedFileFromUnencrypted("TestImage1.png", "Test Image 1", "DESCRIPTION1");
	//AddHostedFileFromUnencrypted("TestImage2.png", "Test Image 2", "DESCRIPTION2");
	//AddHostedFileFromUnencrypted("TestImage3.png", "Test Image 3", "DESCRIPTION3");
	//AddHostedFileFromUnencrypted("TestImage4.png", "Test Image 4", "DESCRIPTION4");
	//AddHostedFileFromUnencrypted("TestImage5.png", "Test Image 5", "DESCRIPTION5");
	//AddHostedFileFromUnencrypted("TestImage6.png", "Test Image 6", "DESCRIPTION6");
	//AddHostedFileFromUnencrypted("TestImage7.png", "Test Image 7", "DESCRIPTION7");

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
	newConnection->UserIdentifier = "Unidentified User";
	newConnection->InboxCount = 0;
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

			SendChatString(decryptedString.c_str());
		}
		break;

		case MESSAGE_ID_USER_LOGIN_REQUEST: // Player enters the server, sending their encrypted name and password
		{
			//  (int) Length of encrypted username (n1)
			//  (n1-size chars array) Encrypted username
			//  (int) Length of encrypted password (n2)
			//  (n2-size chars array) Encrypted password

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

			AttemptUserLogin(user, username, password);
		}
		break;

		case MESSAGE_ID_FILE_REQUEST:
		{
			auto fileTitle = std::string(winsockWrapper.ReadString(0));

			auto hostedFileIdentifier = md5(fileTitle);
			auto hostedFileIter = HostedFileDataList.find(hostedFileIdentifier);
			if (hostedFileIter == HostedFileDataList.end())
			{
				//  The file could not be found.
				SendMessage_FileRequestFailed(fileTitle, "The specified file was not found.", user);
				break;
			}
			else if (FileSendTaskList.find(user) != FileSendTaskList.end())
			{
				//  The user is already downloading something.
				SendMessage_FileRequestFailed(fileTitle, "User is currently already downloading a file.", user);
				break;
			}
			else
			{
				auto hostedFileData = (*hostedFileIter).second;
				BeginFileTransfer(hostedFileData, user);
			}
		}
		break;

		case MESSAGE_ID_FILE_RECEIVE_READY:
		{
			auto taskIter = FileSendTaskList.find(user);
			assert(taskIter != FileSendTaskList.end());

			auto task = (*taskIter).second;
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
	//  Lowercase the username and password
	std::transform(username.begin(), username.end(), username.begin(), ::tolower);
	std::transform(password.begin(), password.end(), password.begin(), ::tolower);

	//  Locate the user entry in the login details list, if possible
	auto loginDataMD5 = md5(username + password);
	auto userIter = UserLoginDetailsList.find(loginDataMD5);

	//  if the user does not exist
	if (userIter == UserLoginDetailsList.end())
	{
		SendMessage_LoginResponse(false, user);
		return;
	}

	SendMessage_LoginResponse(true, user);

	//  Set the user identifier, and read the Inbox and Notifications data for the user
	user->UserIdentifier = loginDataMD5;
	ReadUserInbox(user);
	ReadUserNotifications(user);
	SendMessage_InboxAndNotifications(user);
	SendMessage_LatestUploads(LatestUploadsList, user);

	if (UserConnectionListChangedCallback != nullptr) UserConnectionListChangedCallback(UserConnectionsList);
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
		std::string username = std::string((char*)decryptedUsername.data(), decryptedUsername.size());
		decryptedPassword = Groundfish::Decrypt(encryptedPassword);
		std::string password = std::string((char*)decryptedPassword.data(), decryptedPassword.size());

		//  Lowercase the username and password
		std::transform(username.begin(), username.end(), username.begin(), ::tolower);
		std::transform(password.begin(), password.end(), password.begin(), ::tolower);

		for (auto i = 0; i < usernameLength; ++i) encryptedUsernameVector.push_back(encryptedUsername[i]);
		for (auto i = 0; i < passwordLength; ++i) encryptedPasswordVector.push_back(encryptedPassword[i]);
		auto loginDataMD5 = md5(username + password);
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

void Server::AddHostedFileFromUnencrypted(std::string fileToAdd, std::string fileTitle, std::string fileDescription)
{
	//  Test that the file exists and is readable, and exit out if it is not
	std::ifstream targetFile(fileToAdd, std::ios_base::binary);
	if (!targetFile.good() || targetFile.bad()) return;
	targetFile.close();

	//  Find the file's primary name (no directories)
	std::string pureFileName = fileToAdd;
	if (fileToAdd.find_last_of('/') != -1) pureFileName = fileToAdd.substr(fileToAdd.find_last_of('/') + 1, fileToAdd.length() - fileToAdd.find_last_of('/') - 1);

	//  If the file already exists in the Hosted File Data List, return out
	auto fileTitleMD5 = md5(fileTitle);
	if (HostedFileDataList.find(fileTitleMD5) != HostedFileDataList.end()) return;

	//  Add the hosted file data to the hosted file data list
	HostedFileData newFile;
	newFile.FileTitleMD5 = fileTitleMD5;
	newFile.EncryptedFileName = Groundfish::Encrypt(pureFileName.c_str(), int(pureFileName.length()), 0, rand() % 256);
	newFile.EncryptedFileTitle = Groundfish::Encrypt(fileTitle.c_str(), int(fileTitle.length()), 0, rand() % 256);
	newFile.EncryptedFileDescription = Groundfish::Encrypt(fileDescription.c_str(), int(fileDescription.length()), 0, rand() % 256);
	newFile.EncryptedUploader = Groundfish::Encrypt("SERVER", strlen("SERVER"), 0, rand() % 256);
	HostedFileDataList[fileTitleMD5] = newFile;

	//  Add the hosted file data to the HostedFiles.data file and move to the end of it
	std::ofstream hfFile("HostedFiles.data", std::ios_base::app);
	assert(!hfFile.bad() && hfFile.good());

	int md5Length = int(newFile.FileTitleMD5.length());
	int encryptedFileNameLength = int(newFile.EncryptedFileName.size());
	int encryptedFileTitleLength = int(newFile.EncryptedFileTitle.size());
	int encryptedFileDescLength = int(newFile.EncryptedFileDescription.size());
	int encryptedFileUploaderLength = int(newFile.EncryptedUploader.size());

	//  Write the hosted file data into the file, then close it
	hfFile.write((const char*)&md5Length, sizeof(md5Length));
	hfFile.write((const char*)newFile.FileTitleMD5.c_str(), md5Length);
	hfFile.write((const char*)&encryptedFileNameLength, sizeof(encryptedFileNameLength));
	hfFile.write((const char*)newFile.EncryptedFileName.data(), encryptedFileNameLength);
	hfFile.write((const char*)&encryptedFileTitleLength, sizeof(encryptedFileTitleLength));
	hfFile.write((const char*)newFile.EncryptedFileTitle.data(), encryptedFileTitleLength);
	hfFile.write((const char*)&encryptedFileDescLength, sizeof(encryptedFileDescLength));
	hfFile.write((const char*)newFile.EncryptedFileDescription.data(), encryptedFileDescLength);
	hfFile.write((const char*)&encryptedFileUploaderLength, sizeof(encryptedFileUploaderLength));
	hfFile.write((const char*)newFile.EncryptedUploader.data(), encryptedFileUploaderLength);
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
	int fileUploaderLength = 0;
	unsigned char fileTitleMD5[128];
	unsigned char encryptedFileName[128];
	unsigned char encryptedFileTitle[128];
	unsigned char encryptedFileDesc[256];
	unsigned char encryptedFileUploader[256];

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
		fileUploaderLength = 0;

		//  Read in the hosted file data
		hfFile.read((char*)(&md5Length), sizeof(md5Length)); if (md5Length == 0) break;
		hfFile.read((char*)(fileTitleMD5), md5Length);
		hfFile.read((char*)(&fileNameLength), sizeof(fileNameLength));
		hfFile.read((char*)(encryptedFileName), fileNameLength);
		hfFile.read((char*)(&fileTitleLength), sizeof(fileTitleLength));
		hfFile.read((char*)(encryptedFileTitle), fileTitleLength);
		hfFile.read((char*)(&fileDescLength), sizeof(fileDescLength));
		hfFile.read((char*)(encryptedFileDesc), fileDescLength);
		hfFile.read((char*)(&fileUploaderLength), sizeof(fileUploaderLength));
		hfFile.read((char*)(encryptedFileUploader), fileUploaderLength);

		//  Create a Hosted File Data struct and store it off in memory
		auto decryptedFileTitle = Groundfish::Decrypt(encryptedFileTitle);
		auto decryptedFileTitleString = std::string((char*)decryptedFileTitle.data(), decryptedFileTitle.size());

		HostedFileData newFile;
		newFile.FileTitleMD5 = std::string((char*)fileTitleMD5, md5Length);
		for (auto i = 0; i < fileNameLength; ++i) newFile.EncryptedFileName.push_back(encryptedFileName[i]);
		for (auto i = 0; i < fileTitleLength; ++i) newFile.EncryptedFileTitle.push_back(encryptedFileTitle[i]);
		for (auto i = 0; i < fileDescLength; ++i) newFile.EncryptedFileDescription.push_back(encryptedFileDesc[i]);
		for (auto i = 0; i < fileUploaderLength; ++i) newFile.EncryptedUploader.push_back(encryptedFileUploader[i]);
		HostedFileDataList[newFile.FileTitleMD5] = newFile;
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
		if (task->GetFileTransferState() == FileSendTask::CHUNK_STATE_INITIALIZING) continue;

		//  If we have data to send, send it and continue out so we can keep sending it until we're done
		if (!task->SendFileChunk()) continue;

		//  If we've gotten this far, we're ready to delete the file send task, as it completed.
		delete task;
		FileSendTaskList.erase(taskIter);
		break;
	}
}


void Server::BeginFileTransfer(HostedFileData& fileData, UserConnection* user)
{
	//  Decrypt the file name and hosted file path
	auto decryptedFileNameVector = Groundfish::Decrypt(fileData.EncryptedFileName.data());
	auto fileName = std::string((char*)decryptedFileNameVector.data(), decryptedFileNameVector.size());
	auto filePath = "./_HostedFiles/" + fileData.FileTitleMD5 + ".hostedfile";

	//  Add a new FileSendTask to our list, so it can manage itself
	FileSendTask* newTask = new FileSendTask(fileName, filePath, user->SocketID, std::string(user->IPAddress), NEW_PROVIDENCE_PORT);
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
		winsockWrapper.SendMessagePacket(user->SocketID, user->IPAddress.c_str(), NEW_PROVIDENCE_PORT, 0);
	}
}