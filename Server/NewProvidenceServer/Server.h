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
void SendMessage_LoginResponse(bool success, int socket, const char* ip)
{
	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_USER_LOGIN_RESPONSE, 0);
	winsockWrapper.WriteChar((unsigned char)success, 0);

	winsockWrapper.SendMessagePacket(socket, ip, SERVER_PORT, 0);
}


void SendMessage_FileSendInitializer(std::string fileName, int fileSize, int socket, const char* ip)
{
	//  Encrypt the file name string using Groundfish
	unsigned char encrypted[256];
	int messageSize = Groundfish::Encrypt(fileName.c_str(), encrypted, int(fileName.length()) + 1, 0, rand() % 256);

	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_FILE_SEND_INIT, 0);
	winsockWrapper.WriteInt(messageSize, 0);
	winsockWrapper.WriteChars(encrypted, messageSize, 0);
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

	struct UserConnection
	{
		int				SocketID;
		std::string		IPAddress;
		int				PingCount;

		std::string		UserIdentifier;
	};

	typedef std::pair<std::vector<unsigned char>, std::vector<unsigned char>> UserLoginDetails;
	std::unordered_map<std::string, UserLoginDetails> UserLoginDetailsList;
	std::unordered_map<UserConnection*, bool> UserConnectionsList;
	std::unordered_map<UserConnection*, FileSendTask*> FileSendTaskList; // NOTE: Key is the client ID, so we should limit them to one transfer in the future

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

	//  Load the current Groundfish word list
	Groundfish::LoadCurrentWordList();

	//  Initialize the Winsock wrapper
	winsockWrapper.WinsockInitialize();

	// Initialize the Listening Port for Clients
	ServerSocketHandle = winsockWrapper.TCPListen(SERVER_PORT, 10, 1);
	if (ServerSocketHandle == -1) return false;
	winsockWrapper.SetNagle(ServerSocketHandle, true);

	//  Load all user login details into an accessible list
	LoadUserLoginDetails();
	AddUserLoginDetails("Drew", "password");
	AddUserLoginDetails("Charlie", "testpass");

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
			unsigned char encrypted[256];
			memcpy(encrypted, winsockWrapper.ReadChars(0, messageSize), messageSize);
			char decrypted[256];
			Groundfish::Decrypt(encrypted, decrypted);

			std::string decryptedString(decrypted);
			if (decryptedString.find("download") != std::string::npos) BeginFileTransfer("fileToTransfer.jpeg", user);
			else SendChatString(decrypted);
		}
		break;

		case MESSAGE_ID_USER_LOGIN_REQUEST: // Player enters the server, sending their encrypted name and password
		{
			//  (int) Length of encrypted username (n1)
			//  (n1-size chars array) Encrypted username
			//  (int) Length of encrypted password (n2)
			//  (n2-size chars array) Encrypted password

			//  Grab the username size and then the encrypted username, and decrypt it
			int usernameSize = winsockWrapper.ReadInt(0);
			std::vector<unsigned char> decryptedUsername;
			auto encryptedUsername = winsockWrapper.ReadChars(0, usernameSize);
			Groundfish::Decrypt(encryptedUsername, decryptedUsername);
			std::string username;
			for (auto i = 0; i < int(decryptedUsername.size()); ++i) username += (char)(decryptedUsername[i]);

			//  Grab the password size and then the encrypted password, and decrypt it
			int passwordSize = winsockWrapper.ReadInt(0);
			std::vector<unsigned char> decryptedPassword;
			auto encryptedPassword = winsockWrapper.ReadChars(0, passwordSize);
			Groundfish::Decrypt(encryptedPassword, decryptedPassword);
			std::string password;
			for (auto i = 0; i < int(decryptedPassword.size()); ++i) password += (char)(decryptedPassword[i]);

			AttemptUserLogin(user, username, password);
		}
		break;

		case MESSAGE_ID_FILE_REQUEST:
		{
			char* fileName = winsockWrapper.ReadString(0);

			BeginFileTransfer(fileName, user);
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
	std::vector<unsigned char> userLoginComboVector;
	for (auto i = 0; i < int(username.length()); ++i) userLoginComboVector.push_back(username[i]);
	for (auto i = 0; i < int(password.length()); ++i) userLoginComboVector.push_back(password[i]);
	std::string userLoginCombo = username + password;
	auto userLoginMD5 = md5(userLoginComboVector);
	auto userIter = UserLoginDetailsList.find(userLoginMD5);

	//  if the user does not exist
	if (userIter == UserLoginDetailsList.end())
	{
		SendMessage_LoginResponse(false, user->SocketID, user->IPAddress.c_str());
	}
	else
	{
		SendMessage_LoginResponse(true, user->SocketID, user->IPAddress.c_str());
	}
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
	unsigned char encryptedUsername[32];
	unsigned char encryptedPassword[32];
	std::vector<unsigned char> encryptedUsernameVector;
	std::vector<unsigned char> encryptedPasswordVector;
	std::vector<unsigned char> unencryptedUsername;
	std::vector<unsigned char> unencryptedPassword;
	std::vector<unsigned char> loginDataCombo;

	auto fileSize = int(uldFile.tellg());
	uldFile.seekg(0, std::ios::end);
	fileSize = int(uldFile.tellg()) - fileSize;
	uldFile.seekg(0, std::ios::beg);

	while ((fileSize != 0) && (!uldFile.eof()))
	{
		int usernameLength = 0;
		int passwordLength = 0;
		uldFile.read((char*)(&usernameLength), sizeof(usernameLength));
		if (usernameLength == 0) break;
		uldFile.read((char*)(encryptedUsername), usernameLength);
		uldFile.read((char*)(&passwordLength), sizeof(passwordLength));
		uldFile.read((char*)(encryptedPassword), passwordLength);

		for (auto i = 0; i < usernameLength; ++i) encryptedUsernameVector.push_back(encryptedUsername[i]);
		for (auto i = 0; i < passwordLength; ++i) encryptedPasswordVector.push_back(encryptedPassword[i]);

		//  Get the MD5 of the decrypted username and password
		char decryptedUsername[256];
		Groundfish::Decrypt(encryptedUsername, decryptedUsername);
		char decryptedPassword[256];
		Groundfish::Decrypt(encryptedPassword, decryptedPassword);
		loginDataCombo.clear();
		for (auto i = 0; i < int(strlen(decryptedUsername)); ++i) loginDataCombo.push_back(decryptedUsername[i]);
		for (auto i = 0; i < int(strlen(decryptedPassword)); ++i) loginDataCombo.push_back(decryptedPassword[i]);
		std::string MD5String = md5(loginDataCombo);

		UserLoginDetailsList[MD5String] = UserLoginDetails(encryptedUsernameVector, encryptedPasswordVector);
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

		//  Write the size of the encrypted username
		auto encryptedUsernameSize = int(encryptedUsername.size());
		uldFile.write((const char*)&encryptedUsernameSize, sizeof(encryptedUsernameSize));

		//  Write the encrypted username
		uldFile.write((const char*)(encryptedUsername.data()), encryptedUsername.size());

		//  Write the size of the encrypted password
		auto encryptedPasswordSize = int(encryptedPassword.size());
		uldFile.write((const char*)&encryptedPasswordSize, sizeof(encryptedPasswordSize));

		//  Write the encrypted password
		uldFile.write((const char*)(encryptedPassword.data()), encryptedPassword.size());
	}

	uldFile.close();
}

void Server::AddUserLoginDetails(std::string username, std::string password)
{
	//  Grab the MD5 of the combination of the unencrypted username and unencrypted password
	std::vector<unsigned char> loginDetailsVector;
	for (auto i = 0; i < int(username.size()); ++i) loginDetailsVector.push_back((unsigned char)(username[i]));
	for (auto i = 0; i < int(password.size()); ++i) loginDetailsVector.push_back((unsigned char)(password[i]));
	auto loginMD5 = md5(loginDetailsVector);

	//  Check if the MD5 already links to a user login data entry, and if so exit out
	if (UserLoginDetailsList.find(loginMD5) != UserLoginDetailsList.end()) return;

	//  If it's a new entry, encrypt the username and password, place both encrypted arrays in a vector of unsigned chars
	unsigned char encryptedUsername[256];
	unsigned char encryptedPassword[256];
	std::vector<unsigned char> usernameVector;
	std::vector<unsigned char> passwordVector;
	auto usernameSize = Groundfish::Encrypt(username.c_str(), encryptedUsername, int(username.length()), 0, rand() % 256);
	auto passwordSize = Groundfish::Encrypt(password.c_str(), encryptedPassword, int(password.length()), 0, rand() % 256);
	for (auto i = 0; i < usernameSize; ++i) usernameVector.push_back((unsigned char)(encryptedUsername[i]));
	for (auto i = 0; i < passwordSize; ++i) passwordVector.push_back((unsigned char)(encryptedPassword[i]));
	
	//  Add the user data to the login details list, then save off the new user login details list
	UserLoginDetailsList[loginMD5] = UserLoginDetails(usernameVector, passwordVector);
	SaveUserLoginDetails();
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
	FileSendTask* newTask = new FileSendTask(std::string(fileName), user->SocketID, std::string(user->IPAddress));
	FileSendTaskList[user] = newTask;
}

void Server::SendChatString(const char* chatString)
{
	for (auto userIter = UserConnectionsList.begin(); userIter != UserConnectionsList.end(); ++userIter)
	{
		auto user = (*userIter).first;

		//  Encrypt the string using Groundfish
		unsigned char encrypted[256];
		int messageSize = Groundfish::Encrypt(chatString, encrypted, int(strlen(chatString)) + 1, 0, rand() % 256);

		winsockWrapper.ClearBuffer(0);
		winsockWrapper.WriteChar(MESSAGE_ID_ENCRYPTED_CHAT_STRING, 0);
		winsockWrapper.WriteInt(messageSize, 0);
		winsockWrapper.WriteChars(encrypted, messageSize, 0);
		winsockWrapper.SendMessagePacket(user->SocketID, user->IPAddress.c_str(), SERVER_PORT, 0);
	}
}