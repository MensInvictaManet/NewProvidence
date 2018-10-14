#pragma once

#include "MessageIdentifiers.h"
#include "Engine/WinsockWrapper.h"

#define FILE_CHUNK_SIZE					1024
#define FILE_CHUNK_BUFFER_COUNT			500
#define FILE_SEND_BUFFER_SIZE			(FILE_CHUNK_SIZE * FILE_CHUNK_BUFFER_COUNT)
#define PORTION_COMPLETE_REMIND_TIME	0.5


void SendMessage_FileSendInitializer(std::string fileName, int fileSize, int socket, const char* ip, const int port)
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

	winsockWrapper.SendMessagePacket(socket, ip, port, 0);
}


void SendMessage_FileSendChunk(int chunkBufferIndex, int chunkIndex, int chunkSize, unsigned char* buffer, int socket, const char* ip, const int port)
{
	assert(chunkSize > 0);
	auto checksum4 = sha256((char*)buffer, 1, chunkSize).substr(0, 4);

	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_FILE_PORTION, 0);
	winsockWrapper.WriteInt(chunkBufferIndex, 0);
	winsockWrapper.WriteInt(chunkIndex, 0);
	winsockWrapper.WriteInt(chunkSize, 0);
	assert(winsockWrapper.WriteInt(4, 0) == sizeof(int));
	assert(winsockWrapper.WriteChars((unsigned char*)checksum4.c_str(), 4, 0) == 4);
	assert(winsockWrapper.WriteChars(buffer, chunkSize, 0) == chunkSize);
	winsockWrapper.SendMessagePacket(socket, ip, port, 0);
	std::cout << chunkBufferIndex << " - " << chunkIndex << ": " << chunkSize << std::endl;
}


void SendMessage_FileTransferPortionComplete(int portionIndex, int socket, const char* ip, const int port)
{
	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_FILE_PORTION_COMPLETE, 0);
	winsockWrapper.WriteInt(portionIndex, 0);
	winsockWrapper.SendMessagePacket(socket, ip, port, 0);
}


void SendMessage_FileReceiveReady(std::string fileName, int socket, const char* ip, const int port)
{
	//  Send a "File Receive Ready" message
	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_FILE_RECEIVE_READY, 0);
	//  TODO: Encrypt the file name
	winsockWrapper.WriteString(fileName.c_str(), 0);
	winsockWrapper.SendMessagePacket(socket, ip, port, 0);
}


void SendMessage_FilePortionCompleteConfirmation(int portionIndex, int socket, const char* ip, const int port)
{
	//  Send a "File Portion Complete" message
	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_FILE_PORTION_COMPLETE_CONFIRM, 0);
	winsockWrapper.WriteInt(portionIndex, 0);
	winsockWrapper.SendMessagePacket(socket, ip, port, 0);
}


void SendMessage_FileChunksRemaining(std::unordered_map<int, bool>& chunksRemaining, int socket, const char* ip, const int port)
{
	//  Send a "File Portion Complete" message
	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_FILE_CHUNKS_REMAINING, 0);
	winsockWrapper.WriteInt(chunksRemaining.size(), 0);
	for (auto i = chunksRemaining.begin(); i != chunksRemaining.end(); ++i) winsockWrapper.WriteShort((short)((*i).first), 0);
	winsockWrapper.SendMessagePacket(socket, ip, port, 0);
}

//  FileSendTask class
class FileSendTask
{
public:
	FileSendTask(std::string fileName, std::string filePath, int socketID, std::string ipAddress, const int port) :
		FileName(fileName),
		SocketID(socketID),
		IPAddress(ipAddress),
		ConnectionPort(port),
		FileTransferReady(false),
		FileChunkTransferState(CHUNK_STATE_INITIALIZING),
		FilePortionIndex(0),
		LastMessageTime(clock())
	{
		//  Open the file, determine that the file handler is good or not, then save off the chunk send indicator map
		FileStream.open(filePath, std::ios_base::binary);
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
		SendMessage_FileSendInitializer(FileName, FileSize, SocketID, IPAddress.c_str(), ConnectionPort);
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
				SendMessage_FileTransferPortionComplete(FilePortionIndex, SocketID, IPAddress.c_str(), ConnectionPort);
				LastMessageTime = clock();
				return false;
			}
			return (FilePortionIndex >= FilePortionCount);
		}

		//  First, check that we have data left unsent in the current chunk buffer. If not, set us to "Pending Complete" on the current chunk buffer
		if (FileChunksToSend.begin() == FileChunksToSend.end())
		{
			FileChunkTransferState = CHUNK_STATE_PENDING_COMPLETE;
			SendMessage_FileTransferPortionComplete(FilePortionIndex, SocketID, IPAddress.c_str(), ConnectionPort);
			LastMessageTime = clock();
			return false;
		}


		//  Choose a portion of the file and commit it to a byte array
		auto chunkIndex = (*FileChunksToSend.begin()).first;
		auto chunkPosition = (FilePortionIndex * FILE_SEND_BUFFER_SIZE) + (FILE_CHUNK_SIZE * chunkIndex);
		int chunkSize = ((chunkPosition + FILE_CHUNK_SIZE) > FileSize) ? (FileSize - chunkPosition) : FILE_CHUNK_SIZE;

		//  Write the chunk buffer index, the index of the chunk, the size of the chunk, and then the chunk data
		SendMessage_FileSendChunk(FilePortionIndex, chunkIndex, chunkSize, (unsigned char*)FilePortionBuffer[chunkIndex], SocketID, IPAddress.c_str(), ConnectionPort);

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
	const int ConnectionPort;
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

class FileReceiveTask
{
public:
	FileReceiveTask(std::string fileName, int fileSize, int fileChunkSize, int fileChunkBufferSize, int socketID, std::string ipAddress, const int port) :
		FileName("./_DownloadedFiles/" + fileName),
		FileSize(fileSize),
		FileChunkSize(fileChunkSize),
		FileChunkBufferSize(fileChunkBufferSize),
		SocketID(socketID),
		IPAddress(ipAddress),
		ConnectionPort(port),
		FilePortionIndex(0),
		FileDownloadComplete(false)
	{
		_wmkdir(L"_DownloadedFiles");

		FileChunkCount = FileSize / FileChunkSize;
		if ((FileSize % FileChunkSize) != 0) FileChunkCount += 1;
		auto nextChunkCount = (FileChunkCount > (FileChunkBufferSize)) ? FileChunkBufferSize : FileChunkCount;
		ResetChunksToReceiveMap(nextChunkCount);
		FilePortionCount = FileChunkCount / FileChunkBufferSize;
		if ((FileChunkCount % FileChunkBufferSize) != 0) FilePortionCount += 1;

		//  Create a file of the proper size based on the server's description
		std::ofstream outputFile("./_DownloadedFiles/_download.tempfile", std::ios::binary | std::ios::trunc | std::ios_base::beg);
		outputFile.seekp(fileSize - 1);
		outputFile.write("", 1);
		outputFile.close();

		//  Open the file again, this time keeping the file handle open for later writing
		FileStream.open("./_DownloadedFiles/_download.tempfile", std::ios_base::binary | std::ios_base::out | std::ios_base::in);
		assert(FileStream.good() && !FileStream.bad());

		DownloadTime = gameSeconds;
		SendMessage_FileReceiveReady(FileName, SocketID, IPAddress.c_str(), ConnectionPort);
	}

	~FileReceiveTask() {}

	inline void ResetChunksToReceiveMap(int chunkCount) { FilePortionsToReceive.clear(); for (auto i = 0; i < chunkCount; ++i)  FilePortionsToReceive[i] = true; }

	bool ReceiveFile(std::string& progress)
	{
		unsigned char chunkData[1024];
		std::string chunkChecksum;

		auto filePortionIndex = winsockWrapper.ReadInt(0);
		auto chunkIndex = winsockWrapper.ReadInt(0);
		auto chunkSize = winsockWrapper.ReadInt(0);
		auto checksumSize = winsockWrapper.ReadInt(0);
		chunkChecksum = std::string((char*)winsockWrapper.ReadChars(0, checksumSize), checksumSize);
		memcpy(chunkData, winsockWrapper.ReadChars(0, chunkSize), chunkSize);

		if (filePortionIndex != FilePortionIndex)
		{
			return false;
		}

		if (FileDownloadComplete) return true;

		auto iter = FilePortionsToReceive.find(chunkIndex);
		if (iter == FilePortionsToReceive.end()) return false;
		if (sha256((char*)chunkData, 1, chunkSize).substr(0, 4) != chunkChecksum) return false;

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
			SendMessage_FileChunksRemaining(FilePortionsToReceive, SocketID, IPAddress.c_str(), ConnectionPort);
			return false;
		}
		else
		{
			SendMessage_FilePortionCompleteConfirmation(FilePortionIndex, SocketID, IPAddress.c_str(), ConnectionPort);
			if (++FilePortionIndex == FilePortionCount)
			{
				DownloadTime = gameSeconds - DownloadTime;
				FileDownloadComplete = true;
				FileStream.close();
				_wmkdir(L"_DownloadedFiles");
				Groundfish::DecryptAndMoveFile("./_DownloadedFiles/_download.tempfile", FileName, true);
			}

			//  Reset the chunk list to ensure we're waiting on the right number of chunks for the next portion
			auto chunksProcessed = FilePortionIndex * FileChunkBufferSize;
			auto nextChunkCount = (FileChunkCount > (chunksProcessed + FileChunkBufferSize)) ? FileChunkBufferSize : (FileChunkCount - chunksProcessed);
			ResetChunksToReceiveMap(nextChunkCount);

			return true;
		}
	}

	inline bool GetFileDownloadComplete() const { return FileDownloadComplete; }
	inline float GetPercentageComplete() const { return float(FilePortionIndex) / float(FilePortionCount); }

	std::string FileName;
	int FileSize;
	int SocketID;
	std::string IPAddress;
	const int ConnectionPort;
	int FilePortionCount;
	int FileChunkCount;
	const int FileChunkSize;
	const int FileChunkBufferSize;
	std::ofstream FileStream;
	double DownloadTime;

	int FilePortionIndex;
	std::unordered_map<int, bool> FilePortionsToReceive;
	bool FileDownloadComplete;
};
