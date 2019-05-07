#pragma once

#include "Engine/WinsockWrapper.h"
#include "Engine/SimpleMD5.h"
#include "Engine/SimpleSHA256.h"
#include "Groundfish.h"
#include "FileSendAndReceive.h"
#include "HostedFileData.h"
#include "NPSQL.h"

#include <fstream>
#include <ctime>

constexpr auto VERSION_NUMBER				= "2019.03.02";

constexpr auto NEW_PROVIDENCE_PORT			= 2347;

constexpr auto LATEST_UPLOADS_SENT_COUNT	= 20;
constexpr auto PING_INTERVAL_TIME			= 5.0;
constexpr auto PINGS_BEFORE_DISCONNECT		= 30;

struct UserLoginDetails
{
	EncryptedData EncryptedUserName;
	EncryptedData EncryptedPassword;

	UserLoginDetails()
	{
		EncryptedUserName.clear();
		EncryptedPassword.clear();
	}
	UserLoginDetails(EncryptedData username, EncryptedData password) : EncryptedUserName(username), EncryptedPassword(password) {}
};


bool CompareUploadsByTimeAdded(const HostedFileData& first, const HostedFileData& second)
{
	auto comparison = second.FileUploadTime.compare(first.FileUploadTime);
	return (comparison == 1);
}


struct UserConnection
{
	enum UserStatusID { USER_STATUS_CONNECTED, USER_STATUS_LOGGED_IN, USER_STATUS_DOWNLOADING, USER_STATUS_UPLOADING, USER_STATUS_COUNT };
	const std::string UserStatusStrings[USER_STATUS_COUNT] = { "Connected", "Logged In", "Downloading", "Uploading" };

	UserConnection() :
		SocketID(-1),
		IPAddress(""),
		LastPingTime(gameSeconds),
		LastPingRequest(0.0),
		InboxCount(0),
		UserIdentifier("UNKNOWN ID"),
		Username("UNKNOWN USER"),
		UserStatus(USER_STATUS_CONNECTED),
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
		UserStatus(USER_STATUS_CONNECTED),
		StatusString("Connected"),
		UserFileSendTask(nullptr),
		UserFileReceiveTask(nullptr)
	{}

	inline void UpdatePingTime() { LastPingTime = gameSeconds; UpdatePingRequestTime(); }
	inline void UpdatePingRequestTime() { LastPingRequest = gameSeconds; }
	inline std::string GetUserStatusString() const { return UserStatusStrings[UserStatus]; }
	inline void SetStatusIdle(int secondsSinceActive = 0) { StatusString = GetUserStatusString() + ", Idle (last activity " + std::to_string(secondsSinceActive) + " seconds ago)"; }
	inline void SetStatusTransferring(bool download, std::string checksum, float percent, int kbps) { StatusString = StatusString = GetUserStatusString() + " file " + checksum + " [" + std::to_string(int(percent * 100.0f)) + "% @" + std::to_string(kbps) + " KB/s]"; }

	int				SocketID;
	std::string		IPAddress;
	double			LastPingTime;
	double			LastPingRequest;

	int				InboxCount;

	std::string		UserIdentifier;
	std::string		Username;
	UserStatusID	UserStatus;
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


void SendMessage_HostedFileList(UserConnection* user, int startIndex = 0, EncryptedData encryptedUsername = EncryptedData(), HostedFileType type = FILE_TYPE_COUNT, HostedFileSubtype subtype = FILE_SUBTYPE_COUNT)
{
	//  Message composition:
	//  - (1 byte) unsigned char representing the message ID
	//  - (2 bytes) unsigned short representing the starting index
	//  - (2 bytes) unsigned short representing the list size (max 20)
	//  - (1440 bytes) (1 + 1 + 1 + 49 + 1 + 19) x [list max (20)]
	//
	//  Max message size = 1045 bytes

	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_HOSTED_FILE_LIST, 0);

	//  Grab the file data list and find out the list size we're going to send.
	std::list<HostedFileData> hostedFileDataList;
	NPSQL::GetHostedFileList(hostedFileDataList, startIndex, LATEST_UPLOADS_SENT_COUNT);
	hostedFileDataList.sort(CompareUploadsByTimeAdded);
	auto listSize = hostedFileDataList.size();

	//  Determine if we have enough uploads by the given user to fill the list size. If not, decrement the list size
	if ((encryptedUsername.size() != 0) || (type != FILE_TYPE_COUNT) || (subtype != FILE_SUBTYPE_COUNT))
	{
		auto uploadCount = 0;
		for (auto iter = hostedFileDataList.begin(); iter != hostedFileDataList.end(); ++iter)
		{
			if ((encryptedUsername.size() != 0) && CompareEncryptedData(encryptedUsername, (*iter).EncryptedUploader, false) != 0) continue;
			if ((type != FILE_TYPE_COUNT) && ((*iter).FileType != type)) continue;
			if ((subtype != FILE_SUBTYPE_COUNT) && ((*iter).FileSubType != subtype)) continue;

			++uploadCount;
		}
		listSize = std::max<int>(std::min<int>(listSize, (uploadCount - startIndex)), 0);
	}

	winsockWrapper.WriteUnsignedShort(uint16_t(startIndex), 0);
	winsockWrapper.WriteUnsignedShort(uint16_t(listSize), 0);

	if (listSize > 0)
	{
		int iterIndex = 0;
		for (auto iter = hostedFileDataList.begin(); iter != hostedFileDataList.end(); ++iter)
		{
			//  Filter any entries that don't match our filter specifications
			if ((encryptedUsername.size() != 0) && CompareEncryptedData(encryptedUsername, (*iter).EncryptedUploader, false) != 0) continue;
			if ((type != FILE_TYPE_COUNT) && ((*iter).FileType != type)) continue;
			if ((subtype != FILE_SUBTYPE_COUNT) && ((*iter).FileSubType != subtype)) continue;

			if (iterIndex++ < startIndex) continue;

			if (--listSize < 0) break;
			auto title = (*iter).EncryptedFileTitle;
			assert(title.size() <= ENCRYPTED_TITLE_MAX_SIZE);
			winsockWrapper.WriteChar((unsigned char)((*iter).FileType), 0);
			winsockWrapper.WriteChar((unsigned char)((*iter).FileSubType), 0);

			winsockWrapper.WriteChar((unsigned char)(title.size()), 0);
			winsockWrapper.WriteChars(title.data(), int(title.size()), 0);

			winsockWrapper.WriteChar(uint8_t((*iter).EncryptedUploader.size()), 0);
			winsockWrapper.WriteChars((*iter).EncryptedUploader.data(), (*iter).EncryptedUploader.size(), 0);
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


void SendMessage_FileSendInitFailed(std::string failureReason, UserConnection* user)
{
	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_FILE_SEND_FAILED, 0);
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
	std::string UserDatabaseName = "UserDatabase";

	std::function<void()> HostedFileListChangedCallback = nullptr;
	std::function<void(std::unordered_map<UserConnection*, bool>)> UserConnectionListChangedCallback = nullptr;

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

	inline void SetHostedFileListChangedCallback(const std::function<void()>& callback) { HostedFileListChangedCallback = callback; }
	inline void SetUserConnectionListChangedCallback(const std::function<void(std::unordered_map<UserConnection*, bool>)>& callback) { UserConnectionListChangedCallback = callback; }
	
	bool Initialize(void);
	void MainProcess(void);
	void Shutdown(void);

	void DeleteHostedFile(std::string fileChecksum);
	inline UserConnection* GetUserConnectionByIP(std::string ip) const { for (auto i = UserConnectionsList.begin(); i != UserConnectionsList.end(); ++i) if ((*i).first->IPAddress == ip) return (*i).first; return nullptr; }

	void AddUserLoginDetails(std::string username, std::string password);

private:
	void AddClient(int socketID, std::string ipAddress);
	void RemoveClient(UserConnection* user);
	void AcceptNewClients(void);
	void ReceiveMessages(void);
	void PingConnectedUsers(void);
	void AttemptUserLogin(UserConnection* user, std::string& username, std::string& password);

	void AddHostedFileFromEncrypted(std::string fileToAdd, std::string fileTitle, std::string fileDescription, int32_t fileTypeID, int32_t fileSubTypeID, UserConnection* user);
	void AddHostedFileFromUnencrypted(std::string fileToAdd, std::string fileTitle, std::string fileDescription);
	void SendOutHostedFileList(void);

	void ContinueFileTransfers(void);
	void BeginFileTransfer(HostedFileData& fileData, UserConnection* user);
	void UpdateFileTransferPercentage(bool download, UserConnection* user);
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

	// Open the user and file database connections, and ensure we have the primary tables
	NPSQL::OpenUserDatabaseConnection();
	NPSQL::OpenFileDatabaseConnection();
	NPSQL::CreateUserTable();
	NPSQL::CreateFileTable();

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
	//  Close the user and file database connections
	NPSQL::CloseUserDatabaseConnection();
	NPSQL::CloseFileDatabaseConnection();

	//  Close out the server socket
	winsockWrapper.CloseSocket(ServerSocketHandle);
}


void Server::DeleteHostedFile(std::string fileChecksum)
{
	//  If the file does not exist, exit out
	if (NPSQL::CheckIfFileExists(fileChecksum) == false) return;

	//  Delete the file from the _HostedFiles folder
	auto hostedFileName = "_HostedFiles/" + fileChecksum + ".hostedfile";
	std::remove(hostedFileName.c_str());

	//  Updated the Hosted File Data List
	NPSQL::RemoveFile(fileChecksum);
	if (HostedFileListChangedCallback != nullptr) HostedFileListChangedCallback();
	SendOutHostedFileList();
}


void Server::AddUserLoginDetails(std::string username, std::string password)
{
	//  Lowercase the username and password
	std::transform(username.begin(), username.end(), username.begin(), ::tolower);
	std::transform(password.begin(), password.end(), password.begin(), ::tolower);

	//  Check if the MD5 of the unencrypted login details already links to a user login data entry, and if so exit out
	auto loginDataMD5 = md5(username + password);
	if (NPSQL::CheckIfUserExists(username)) return;

	//  If it's a new entry, encrypt the username and password, place both encrypted arrays in a vector of unsigned chars
	EncryptedData usernameVector = Groundfish::Encrypt(username.c_str(), int(username.length()), 0, rand() % 256);
	EncryptedData passwordVector = Groundfish::Encrypt(password.c_str(), int(password.length()), 0, rand() % 256);

	//  Open and close the Inbox and Notifications file for this user, to create them
	std::ofstream userInboxFile("./_UserInbox/" + loginDataMD5 + ".inbox");
	assert(userInboxFile.good() && !userInboxFile.bad());
	userInboxFile.close();
	std::ofstream userNotificationFile("./_UserNotifications/" + loginDataMD5 + ".notifications");
	assert(userNotificationFile.good() && !userNotificationFile.bad());
	userNotificationFile.close();

	//  Add a new notification for a welcoming message
	AddUserNotification(loginDataMD5, "Welcome to New Providence");

	//  Register the new user details to the user database
	NPSQL::RegisterUser(username, sha256(password));
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

	if (user->UserStatus != UserConnection::USER_STATUS_CONNECTED)
		debugConsole->AddDebugConsoleLine(GetCurrentTimeString() + " - User logged out: " + user->Username);

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
				std::string decryptedString = Groundfish::DecryptToString(encryptedChatString);

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

				//  Grab the username size and encrypted username, and decrypt it, then the password size and encrypted password, and decrypt it
				auto usernameSize = winsockWrapper.ReadInt(0);
				auto usernameArray = winsockWrapper.ReadChars(0, usernameSize);
				std::string username = Groundfish::DecryptToString(usernameArray);

				auto passwordSize = winsockWrapper.ReadInt(0);
				auto passwordArray = winsockWrapper.ReadChars(0, passwordSize);
				std::string password = Groundfish::DecryptToString(passwordArray);

				if (versionString.compare(VERSION_NUMBER) != 0)
				{
					SendMessage_LoginResponse(LOGIN_RESPONSE_VERSION_NUMBER_INCORRECT, user);
					break;
				}

				AttemptUserLogin(user, username, password);
			}
			break;

			case MESSAGE_ID_REQUEST_HOSTED_FILE_LIST:
			{
				//  Read the username to filter the list by (if any)
				auto usernameSize = winsockWrapper.ReadUnsignedShort(0);
				auto encryptedUsername = winsockWrapper.ReadChars(0, usernameSize);
				auto encryptedUsernameVec = EncryptedData(encryptedUsername, encryptedUsername + usernameSize);

				//  Read the type and subtype to filter for
				auto type = HostedFileType(winsockWrapper.ReadChar(0));
				auto subtype = HostedFileSubtype(winsockWrapper.ReadChar(0));

				//  Read the starting index to begin the list at
				auto startingIndex = int(winsockWrapper.ReadUnsignedShort(0));

				//  Send a hosted file data list with the given filters
				SendMessage_HostedFileList(user, startingIndex, encryptedUsernameVec, type, subtype);
			}
			break;

			case MESSAGE_ID_FILE_REQUEST:
			{
				auto fileNameLength = winsockWrapper.ReadInt(0);
				auto fileTitle = std::string((char*)winsockWrapper.ReadChars(0, fileNameLength), fileNameLength);

				HostedFileData fileData;
				if (NPSQL::GetFileData(md5(fileTitle), fileData) == false)
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
					BeginFileTransfer(fileData, user);
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
				auto decryptedFileNamePure = Groundfish::DecryptToString(winsockWrapper.ReadChars(0, fileNameSize));
				auto decryptedFileName = "./_DownloadedFiles/" + decryptedFileNamePure;

				//  Decrypt the file title using Groundfish and save it off
				auto decryptedFileTitle = Groundfish::DecryptToString(winsockWrapper.ReadChars(0, fileTitleSize));
				assert(decryptedFileTitle.length() <= UPLOAD_TITLE_MAX_LENGTH);

				//  Determine whether a file with that title already exists in the hosted file list
				if (NPSQL::CheckIfFileExists(md5(decryptedFileTitle)))
				{
					SendMessage_FileSendInitFailed("A file with that title already exists on the server. Try again.", user);
					return;
				}

				//  Decrypt the file description using Groundfish and save it off
				auto decryptedFileDescription = Groundfish::DecryptToString(winsockWrapper.ReadChars(0, fileDescriptionSize));

				//  grab the file type and sub type
				auto fileTypeID = HostedFileType(winsockWrapper.ReadUnsignedShort(0));
				auto fileSubTypeID = HostedFileSubtype(winsockWrapper.ReadUnsignedShort(0));

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

				//  Update the user's status message to show uploading status
				auto titleChecksum = md5(user->UserFileReceiveTask->GetFileTitle());
				auto percentageComplete = user->UserFileReceiveTask->GetPercentageComplete();
				auto transferSpeed = int(float(user->UserFileReceiveTask->GetEstimatedTransferSpeed()) / 1024.0f);
				user->UserStatus = UserConnection::USER_STATUS_UPLOADING;
				user->SetStatusTransferring(false, titleChecksum, float(percentageComplete), transferSpeed);
				if (UserConnectionListChangedCallback != nullptr) UserConnectionListChangedCallback(UserConnectionsList);
			}
			break;

			case MESSAGE_ID_FILE_PORTION_COMPLETE:
			{
				auto portionIndex = winsockWrapper.ReadInt(0);
				auto receiveTask = user->UserFileReceiveTask;

				if (receiveTask == nullptr)
				{
					SendMessage_FilePortionCompleteConfirmation(portionIndex, user->SocketID, user->IPAddress.c_str(), NEW_PROVIDENCE_PORT);
					break;
				}

				(void) _wmkdir(L"_DownloadedFiles");
				if (receiveTask->CheckFilePortionComplete(portionIndex))
				{
					//if (DownloadPercentCompleteCallback != nullptr) DownloadPercentCompleteCallback(FileReceive->GetPercentageComplete(), FileReceive->GetDownloadTime(), FileReceive->GetFileSize());

					if (receiveTask->GetFileTransferComplete())
					{
						AddHostedFileFromEncrypted(receiveTask->GetFileName(), receiveTask->GetFileTitle(), receiveTask->GetFileDescription(), receiveTask->GetFileTypeID(), receiveTask->GetFileSubTypeID(), user);
						debugConsole->AddDebugConsoleLine("Added hosted file: \"" + receiveTask->GetFileTitle() + "\"");

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
				UpdateFileTransferPercentage(true, user);
				if (UserConnectionListChangedCallback != nullptr) UserConnectionListChangedCallback(UserConnectionsList);
			}
			break;

			default:
			{
				std::string debugLine = "Unknown message ID received: " + std::to_string(messageID) + " from user " + user->Username;
				debugConsole->AddDebugConsoleLine(debugLine);
				//assert(false);
			}
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

	//  If the user does not exist, send a failure message and return out
	if (!NPSQL::CheckUserPassword(username, sha256(password)))
	{
		SendMessage_LoginResponse(LOGIN_RESPONSE_PASSWORD_INCORRECT, user);
		return;
	}

	//  Locate the user entry in the login details list, if possible
	auto loginDataChecksum = md5(username + password);

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
	user->UserStatus = UserConnection::USER_STATUS_LOGGED_IN;
	user->SetStatusIdle();
	debugConsole->AddDebugConsoleLine(GetCurrentTimeString() + " - User logged in: " + user->Username);

	//  Read the Inbox and Notifications data for the user
	ReadUserInbox(user);
	ReadUserNotifications(user);
	SendMessage_InboxAndNotifications(user);
	SendMessage_HostedFileList(user);

	if (UserConnectionListChangedCallback != nullptr) UserConnectionListChangedCallback(UserConnectionsList);
}


////////////////////////////////////////
//	Program Functionality
////////////////////////////////////////
void Server::AddHostedFileFromEncrypted(std::string fileToAdd, std::string fileTitle, std::string fileDescription, int32_t fileTypeID, int32_t fileSubTypeID, UserConnection* user)
{
	assert(fileTitle.length() <= UPLOAD_TITLE_MAX_LENGTH);

	//  Test that the file exists and is readable, and exit out if it is not
	//  If the file isn't valid, attempt to re-open it for a quarter of a second
	bool fileValid = false;
	std::ifstream targetFile(fileToAdd, std::ios_base::binary);
	float seconds = gameSecondsF;
	while (!targetFile.good() && targetFile.bad())
	{
		targetFile = std::ifstream(fileToAdd, std::ios_base::binary);
		DetermineTimeSlice();
		assert(gameSecondsF < seconds + 0.25f);
		if (gameSecondsF > seconds + 0.25f) return;
	}
	targetFile.close();

	uint64_t fileSize = 0;
	try {
		fileSize = std::filesystem::file_size(fileToAdd);
	}
	catch (const std::exception& e) {
		std::cerr << "ERROR: " << e.what() << std::endl;
	}
	//  Get the file size in bytes for the file

	//  Find the file's primary name (no directories)
	std::string pureFileName = fileToAdd;
	if (fileToAdd.find_last_of('/') != -1) pureFileName = fileToAdd.substr(fileToAdd.find_last_of('/') + 1, fileToAdd.length() - fileToAdd.find_last_of('/') - 1);

	//  If the file already exists in the Hosted File Data List, return out
	auto fileTitleMD5 = md5(fileTitle);
	if (NPSQL::CheckIfFileExists(md5(fileTitle))) return;

	//  Add the hosted file data to the hosted file data list, then save the hosted file data list
	HostedFileData newFile;
	newFile.FileTitleChecksum = fileTitleMD5;
	newFile.EncryptedFileName = Groundfish::Encrypt(pureFileName.c_str(), int(pureFileName.length()), 0, rand() % 256);
	newFile.EncryptedFileTitle = Groundfish::Encrypt(fileTitle.c_str(), int(fileTitle.length()), 0, rand() % 256);
	newFile.EncryptedFileDescription = Groundfish::Encrypt(fileDescription.c_str(), int(fileDescription.length()), 0, rand() % 256);
	newFile.EncryptedUploader = Groundfish::Encrypt(user->Username.c_str(), strlen(user->Username.c_str()), 0, rand() % 256);
	newFile.FileSize = fileSize;
	newFile.FileUploadTime = GetCurrentTimeString();
	newFile.FileType = HostedFileType(fileTypeID);
	newFile.FileSubType = HostedFileSubtype(fileSubTypeID);

	//  If the file is not already in /_HostedFiles then move it in
	auto hostedFileName = "./_HostedFiles/" + fileTitleMD5 + ".hostedfile";
	std::ifstream uldFile(hostedFileName);
	auto fileExists = (!uldFile.bad() && uldFile.good());
	uldFile.close();
	if (!fileExists) std::rename(fileToAdd.c_str(), hostedFileName.c_str());

	NPSQL::AddFileData(newFile);
	if (HostedFileListChangedCallback != nullptr) HostedFileListChangedCallback();

	SendOutHostedFileList();
}


void Server::AddHostedFileFromUnencrypted(std::string fileToAdd, std::string fileTitle, std::string fileDescription)
{
	assert(fileTitle.length() <= UPLOAD_TITLE_MAX_LENGTH);

	//  Test that the file exists and is readable, and exit out if it is not
	//  If the file isn't valid, attempt to re-open it for a quarter of a second
	bool fileValid = false;
	std::ifstream targetFile(fileToAdd, std::ios_base::binary);
	float seconds = gameSecondsF;
	while (targetFile.good() && targetFile.bad())
	{
		targetFile = std::ifstream(fileToAdd, std::ios_base::binary);
		DetermineTimeSlice();
		assert(gameSecondsF < seconds + 0.1f);
		if (gameSecondsF > seconds + 0.1f) return;
	}
	targetFile.close();

	//  Get the file size in bytes for the file
	uint64_t fileSize = std::filesystem::file_size(fileToAdd);

	//  Find the file's primary name (no directories)
	std::string pureFileName = fileToAdd;
	if (fileToAdd.find_last_of('/') != -1) pureFileName = fileToAdd.substr(fileToAdd.find_last_of('/') + 1, fileToAdd.length() - fileToAdd.find_last_of('/') - 1);

	//  If the file already exists in the Hosted File Data List, return out
	auto fileTitleMD5 = md5(fileTitle);
	if (NPSQL::CheckIfFileExists(fileTitleMD5)) return;

	//  Add the hosted file data to the hosted file data list, then save the hosted file data list
	HostedFileData newFile;
	newFile.FileTitleChecksum = fileTitleMD5;
	newFile.EncryptedFileName = Groundfish::Encrypt(pureFileName.c_str(), int(pureFileName.length()), 0, rand() % 256);
	newFile.EncryptedFileTitle = Groundfish::Encrypt(fileTitle.c_str(), int(fileTitle.length()), 0, rand() % 256);
	newFile.EncryptedFileDescription = Groundfish::Encrypt(fileDescription.c_str(), int(fileDescription.length()), 0, rand() % 256);
	newFile.EncryptedUploader = Groundfish::Encrypt("SERVER", strlen("SERVER"), 0, rand() % 256);
	newFile.FileSize = fileSize;
	newFile.FileUploadTime = GetCurrentTimeString();
	NPSQL::AddFileData(newFile);

	//  If the file is not already in /_HostedFiles then encrypt it and move it
	auto hostedFileName = "./_HostedFiles/" + fileTitleMD5 + ".hostedfile";
	std::ifstream uldFile(hostedFileName);
	auto fileExists = (!uldFile.bad() && uldFile.good());
	uldFile.close();
	if (!fileExists) Groundfish::EncryptAndMoveFile(fileToAdd, hostedFileName);

	SendOutHostedFileList();
}


void Server::SendOutHostedFileList(void)
{
	//  Send the latest uploads list to each connected user
	for (auto userIter = UserConnectionsList.begin(); userIter != UserConnectionsList.end(); ++userIter)
	{
		auto user = (*userIter).first;

		//  If the user isn't logged in yet, skip over them. They'll get an update when they log in
		if (user->UserStatus == UserConnection::USER_STATUS_CONNECTED) continue;

		SendMessage_HostedFileList(user);
	}
}


void Server::ContinueFileTransfers(void)
{
	//  Find all file transfers and send a file chunk for each one if the file transfer is ready
	for (auto userIter = UserConnectionsList.begin(); userIter != UserConnectionsList.end(); ++userIter)
	{
		auto user = (*userIter).first;
		if (user->UserFileSendTask == nullptr) continue;

		//  If we haven't "started" the file send, do so now and return out
		if (user->UserFileSendTask->FileSendStarted == false)
		{
			user->UserFileSendTask->StartFileSend();
			continue;
		}

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
	//  Decrypt the file name, the hosted file path, and the file title
	auto fileName = Groundfish::DecryptToString(fileData.EncryptedFileName.data());
	auto filePath = "./_HostedFiles/" + fileData.FileTitleChecksum + ".hostedfile";
	auto fileTitle = Groundfish::DecryptToString(fileData.EncryptedFileTitle.data());

	//  Get the file type and sub-type
	auto fileTypeID = fileData.FileType;
	auto fileSubTypeID = fileData.FileSubType;

	//  Add a new FileSendTask to our list, so it can manage itself
	FileSendTask* newTask = new FileSendTask(fileName, fileTitle, filePath, fileTypeID, fileSubTypeID, user->SocketID, std::string(user->IPAddress), NEW_PROVIDENCE_PORT);
	user->UserFileSendTask = newTask;
	UpdateFileTransferPercentage(true, user);
}


void Server::UpdateFileTransferPercentage(bool download, UserConnection* user)
{
	auto fileTitle = (download ? user->UserFileSendTask->GetFileTitle() : user->UserFileReceiveTask->GetFileTitle());
	auto percentComplete = (download ? user->UserFileSendTask->GetPercentageComplete() : user->UserFileReceiveTask->GetPercentageComplete());
	auto transferSpeed = (download ? user->UserFileSendTask->GetEstimatedTransferSpeed() : user->UserFileReceiveTask->GetEstimatedTransferSpeed());

	HostedFileData fileData;
	NPSQL::GetFileData(md5(fileTitle), fileData);
	
	//  Update the user and the UI user list
	user->UserStatus = download ? UserConnection::USER_STATUS_DOWNLOADING : UserConnection::USER_STATUS_UPLOADING;
	user->SetStatusTransferring(download, fileData.FileTitleChecksum, float(percentComplete), int(float(transferSpeed) / 1024.0f));
	if (UserConnectionListChangedCallback != nullptr) UserConnectionListChangedCallback(UserConnectionsList);
}


void Server::SendChatString(const char* chatString)
{
	for (auto userIter = UserConnectionsList.begin(); userIter != UserConnectionsList.end(); ++userIter)
	{
		auto user = (*userIter).first;

		//  Encrypt the string using Groundfish
		EncryptedData encryptedChatString = Groundfish::Encrypt(chatString, int(strlen(chatString)) + 1, 0, rand() % 256);

		winsockWrapper.ClearBuffer(0);
		winsockWrapper.WriteChar(MESSAGE_ID_ENCRYPTED_CHAT_STRING, 0);
		winsockWrapper.WriteInt(int(encryptedChatString.size()), 0);
		winsockWrapper.WriteChars(encryptedChatString.data(), int(encryptedChatString.size()), 0);
		winsockWrapper.SendMessagePacket(user->SocketID, user->IPAddress.c_str(), NEW_PROVIDENCE_PORT, 0);
	}
}