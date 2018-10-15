#pragma once

#include "MessageIdentifiers.h"
#include "Engine/WinsockWrapper.h"

#define FILE_CHUNK_SIZE					1024
#define FILE_CHUNK_BUFFER_COUNT			500
#define FILE_SEND_BUFFER_SIZE			(FILE_CHUNK_SIZE * FILE_CHUNK_BUFFER_COUNT)
#define PORTION_COMPLETE_REMIND_TIME	0.1


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
	//  Generate the checksum of the buffer to send before it, so the client can confirm the full, unaltered message arrived
	auto checksum4 = sha256((char*)buffer, 1, chunkSize).substr(0, 4);

	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_FILE_PORTION, 0);
	winsockWrapper.WriteInt(chunkBufferIndex, 0);
	winsockWrapper.WriteInt(chunkIndex, 0);
	winsockWrapper.WriteInt(chunkSize, 0);
	winsockWrapper.WriteInt(4, 0);
	winsockWrapper.WriteChars((unsigned char*)checksum4.c_str(), 4, 0);
	winsockWrapper.WriteChars(buffer, chunkSize, 0);
	winsockWrapper.SendMessagePacket(socket, ip, port, 0);
}


void SendMessage_FileTransferPortionComplete(int portionIndex, int socket, const char* ip, const int port)
{
	//  Send a "File Transfer Portion Complete" message
	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_FILE_PORTION_COMPLETE, 0);
	winsockWrapper.WriteInt(portionIndex, 0);
	winsockWrapper.SendMessagePacket(socket, ip, port, 0);
}


void SendMessage_FileReceiveReady(int socket, const char* ip, const int port)
{
	//  Send a "File Receive Ready" message
	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_FILE_RECEIVE_READY, 0);
	winsockWrapper.SendMessagePacket(socket, ip, port, 0);
}


void SendMessage_FilePortionCompleteConfirmation(int portionIndex, int socket, const char* ip, const int port)
{
	//  Send a "File Portion Complete Confirmation" message
	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_FILE_PORTION_COMPLETE_CONFIRM, 0);
	winsockWrapper.WriteInt(portionIndex, 0);
	winsockWrapper.SendMessagePacket(socket, ip, port, 0);
}


void SendMessage_FileChunksRemaining(std::unordered_map<int, bool>& chunksRemaining, int socket, const char* ip, const int port)
{
	//  Send a "File Chunks Remaining" message
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
	enum FileChunkSendState
	{
		CHUNK_STATE_INITIALIZING = 0,
		CHUNK_STATE_SENDING = 1,
		CHUNK_STATE_PENDING_COMPLETE = 2,
		CHUNK_STATE_COMPLETE = 3,
	};

private:
	const std::string FileName;
	const int SocketID;
	const std::string IPAddress;
	const int ConnectionPort;

	FileChunkSendState FileChunkTransferState;
	int FilePortionIndex;
	clock_t LastMessageTime;

	int FileSize;
	std::ifstream FileStream;

	int FilePortionCount;
	int FileChunkCount;
	std::unordered_map<int, bool> FileChunksToSend;
	char FilePortionBuffer[FILE_CHUNK_BUFFER_COUNT][FILE_CHUNK_SIZE];

public:
	//  Accessors & Modifiers
	inline bool GetFileSendComplete(void) const { return (FilePortionIndex >= FilePortionCount); }
	inline int GetFileTransferState() const { return FileChunkTransferState; }
	inline void SetFileTransferState(int state) { FileChunkTransferState = (FileChunkSendState)state; }

	FileSendTask(std::string fileName, std::string filePath, int socketID, std::string ipAddress, const int port) :
		FileName(fileName),
		SocketID(socketID),
		IPAddress(ipAddress),
		ConnectionPort(port),
		FileChunkTransferState(CHUNK_STATE_INITIALIZING),
		FilePortionIndex(0),
		LastMessageTime(clock())
	{
		//  Open the file we're sending and ensure the file handler is valid
		FileStream.open(filePath, std::ios_base::binary);
		assert(FileStream.good() && !FileStream.bad());

		//  Determine the size of the file we're sending
		FileSize = int(FileStream.tellg());
		FileStream.seekg(0, std::ios::end);
		FileSize = int(FileStream.tellg()) - FileSize;

		//  Determine the file chunk count and file portion count
		FileChunkCount = FileSize / FILE_CHUNK_SIZE;
		if ((FileSize % FILE_CHUNK_SIZE) != 0) FileChunkCount += 1;

		FilePortionCount = FileChunkCount / FILE_CHUNK_BUFFER_COUNT;
		if ((FileChunkCount % FILE_CHUNK_BUFFER_COUNT) != 0) FilePortionCount += 1;

		//  Buffer a portion of the file in preparation of sending it
		BufferFilePortion(FilePortionIndex);

		//  Send a "File Send Initializer" message
		SendMessage_FileSendInitializer(FileName, FileSize, SocketID, IPAddress.c_str(), ConnectionPort);
	}

	~FileSendTask()
	{
		FileStream.close();
	}

	void BufferFilePortion(int filePortionIndex)
	{
		//  Determine the values needed to buffer the data (we might need less than the full buffer)
		auto portionPosition = filePortionIndex * FILE_SEND_BUFFER_SIZE;
		auto portionByteCount = ((portionPosition + FILE_SEND_BUFFER_SIZE) > FileSize) ? (FileSize - portionPosition) : FILE_SEND_BUFFER_SIZE;
		auto portionbufferCount = (((portionByteCount % FILE_CHUNK_SIZE) == 0) ? (portionByteCount / FILE_CHUNK_SIZE) : ((portionByteCount / FILE_CHUNK_SIZE) + 1));

		//  Empty the entire FilePortionBuffer
		memset(FilePortionBuffer, NULL, FILE_SEND_BUFFER_SIZE);

		//  Go through each chunk of the file and place it in the file buffer until we either fill the buffer or run out of file
		FileChunksToSend.clear();
		for (auto i = 0; i < portionbufferCount; ++i)
		{
			//  Seek to the beginning of the chunk we're loading
			auto chunkPosition = portionPosition + (i * FILE_CHUNK_SIZE);
			FileStream.seekg(chunkPosition);

			//  Determine how much data to read for this chunk (we might need less than the full buffer) and read it
			auto chunkFill = (((chunkPosition + FILE_CHUNK_SIZE) > FileSize) ? (FileSize - chunkPosition) : FILE_CHUNK_SIZE);
			FileStream.read(FilePortionBuffer[i], chunkFill);

			//  Set an indicator that this chunk must be sent into a list
			FileChunksToSend[i] = true;
		}
	}

	void SendFileChunk()
	{
		//  Double-check we're not calling this even though our file send is complete
		if (GetFileSendComplete()) return;

		//  If we're pending completion, wait and send completion confirmation only if we've reached the end
		if (FileChunkTransferState == CHUNK_STATE_PENDING_COMPLETE)
		{
			//  If enough time has gone by without response from the last one, send the FileTransferPortionComplete message again
			auto secondsSinceLastMessage = double(clock() - LastMessageTime) / CLOCKS_PER_SEC;
			if (secondsSinceLastMessage > PORTION_COMPLETE_REMIND_TIME)
			{
				SendMessage_FileTransferPortionComplete(FilePortionIndex, SocketID, IPAddress.c_str(), ConnectionPort);
				LastMessageTime = clock();
			}
			return;
		}

		//  First, check that we have data left unsent in the current chunk buffer. If not, set us to "Pending Complete" on the current chunk buffer
		if (FileChunksToSend.begin() == FileChunksToSend.end())
		{
			FileChunkTransferState = CHUNK_STATE_PENDING_COMPLETE;
			SendMessage_FileTransferPortionComplete(FilePortionIndex, SocketID, IPAddress.c_str(), ConnectionPort);
			LastMessageTime = clock();
			return;
		}

		//  Determine the values needed to access the data (we might need less than the full buffer)
		auto chunkIndex = (*FileChunksToSend.begin()).first;
		auto chunkPosition = (FilePortionIndex * FILE_SEND_BUFFER_SIZE) + (FILE_CHUNK_SIZE * chunkIndex);
		int chunkByteCount = ((chunkPosition + FILE_CHUNK_SIZE) > FileSize) ? (FileSize - chunkPosition) : FILE_CHUNK_SIZE;

		//  Write the chunk buffer index, the index of the chunk, the size of the chunk, and then the chunk data
		SendMessage_FileSendChunk(FilePortionIndex, chunkIndex, chunkByteCount, (unsigned char*)FilePortionBuffer[chunkIndex], SocketID, IPAddress.c_str(), ConnectionPort);

		//  Delete the portionIter to signal we've completed sending it
		FileChunksToSend.erase(chunkIndex);
	}

	void SetChunksRemaining(std::unordered_map<int, bool>& chunksRemaining)
	{
		//  Given a list from the file receiver, reset the FileChunksToSend list so we can re-send the necessary file chunks
		FileChunksToSend.clear();
		for (auto i = chunksRemaining.begin(); i != chunksRemaining.end(); ++i) FileChunksToSend[(*i).first] = true;
		FileChunkTransferState = CHUNK_STATE_SENDING;
	}

	void ConfirmFilePortionSendComplete(int portionIndex)
	{
		//  If this message is not regarding the current file portion index, ignore it
		if (FilePortionIndex != portionIndex) return;

		//  If we've reached the end of the file, don't attempt to buffer anything, and leave the state PENDING COMPLETE
		if (++FilePortionIndex >= FilePortionCount)  return;

		//  Buffer the next file portion for sending
		BufferFilePortion(FilePortionIndex);
	}
};

class FileReceiveTask
{
private:
	const std::string FileName;
	const int FileSize;
	const int FileChunkSize;
	const int FileChunkBufferSize;
	const std::string TempFileName;
	const int SocketID;
	const std::string IPAddress;
	const int ConnectionPort;

	int FilePortionIndex;
	bool FileDownloadComplete;

	int FileChunkCount;
	int FilePortionCount;
	std::unordered_map<int, bool> FileChunksToReceive;
	std::ofstream FileStream;
	double DownloadTime;

	//  NOTE: The file chunk data buffer is set to a size of FILE_CHUNK_SIZE, which assumes this header is the same on sender and receiver.
	unsigned char FileChunkDataBuffer[FILE_CHUNK_SIZE];

public:
	//  Accessors & Modifiers
	inline int GetFileSize() const					{ return FileSize; }
	inline bool GetFileDownloadComplete() const		{ return FileDownloadComplete; }
	inline float GetPercentageComplete() const		{ return float(FilePortionIndex * FileChunkSize * FileChunkBufferSize) / float(FileSize); }
	inline double GetDownloadTime() const			{ return DownloadTime; }

	inline void ResetChunksToReceiveMap(int chunkCount) { FileChunksToReceive.clear(); for (auto i = 0; i < chunkCount; ++i)  FileChunksToReceive[i] = true; }

	inline void CreateTemporaryFile(const std::string tempFileName, const int tempFileSize) const {
		std::ofstream outputFile(TempFileName, std::ios::binary | std::ios::trunc | std::ios_base::beg);
		assert(outputFile.good() && !outputFile.bad());
		outputFile.seekp(FileSize - 1);
		outputFile.write("", 1);
		outputFile.close();
	}

	FileReceiveTask(std::string fileName, const int fileSize, int fileChunkSize, int fileChunkBufferSize, std::string tempFilePath, int socketID, std::string ipAddress, const int port) :
		FileName(fileName),
		FileSize(fileSize),
		FileChunkSize(fileChunkSize),
		FileChunkBufferSize(fileChunkBufferSize),
		TempFileName(tempFilePath),
		SocketID(socketID),
		IPAddress(ipAddress),
		ConnectionPort(port),
		FilePortionIndex(0),
		FileDownloadComplete(false)
	{
		//  Determine the count of file chunks and file portions we'll be receiving
		FileChunkCount = ((FileSize % FileChunkSize) == 0) ? (FileSize / FileChunkSize) : ((FileSize / FileChunkSize) + 1);
		FilePortionCount = ((FileChunkCount % FileChunkBufferSize) == 0) ? (FileChunkCount / FileChunkBufferSize) : ((FileChunkCount / FileChunkBufferSize) + 1);

		//  Reset the FileChunksToReceive list, which we use to confirm we've successfully received all chunks in a file portion
		ResetChunksToReceiveMap((FileChunkCount > FileChunkBufferSize) ? FileChunkBufferSize : FileChunkCount);

		//  Create a temporary file of the proper size based on the sender's file description
		CreateTemporaryFile(TempFileName, FileSize);

		//  Open the temporary file, this time keeping the file handle open for later writing
		FileStream.open(TempFileName, std::ios_base::binary | std::ios_base::out | std::ios_base::in);
		assert(FileStream.good() && !FileStream.bad());

		//  Set the download time to the starting seconds (we'll subtract this number from current time when the download finishes)
		DownloadTime = gameSeconds;

		//  Send a signal to the file sender that we're ready to receive the file
		SendMessage_FileReceiveReady(SocketID, IPAddress.c_str(), ConnectionPort);
	}

	~FileReceiveTask()
	{
		FileStream.close();
	}


	bool ReceiveFileChunk()
	{
		//  Clear out the File Chunk Data Buffer
		memset(FileChunkDataBuffer, 0, FILE_CHUNK_SIZE);
		std::string chunkChecksum;

		//  Get the file chunk data from the message, as well as a checksum to check it against
		auto filePortionIndex = winsockWrapper.ReadInt(0);
		auto chunkIndex = winsockWrapper.ReadInt(0);
		auto chunkSize = winsockWrapper.ReadInt(0);
		assert(chunkSize <= FILE_CHUNK_SIZE);
		auto checksumSize = winsockWrapper.ReadInt(0);
		chunkChecksum = std::string((char*)winsockWrapper.ReadChars(0, checksumSize), checksumSize);
		memcpy(FileChunkDataBuffer, winsockWrapper.ReadChars(0, chunkSize), chunkSize);

		//  If the file download is already complete, return out
		if (FileDownloadComplete) return true;

		//  If we're receiving a chunk from a portion other than what we're currently on, return out
		if (filePortionIndex != FilePortionIndex) return false;

		//  Ensure we haven't already received this chunk. If we have, return out
		auto iter = FileChunksToReceive.find(chunkIndex);
		if (iter == FileChunksToReceive.end()) return false;

		//  Check the checksum against the data. If they differ, return out
		if (sha256((char*)FileChunkDataBuffer, 1, chunkSize).substr(0, 4) != chunkChecksum) return false;

		//  If the data is new and valid, seek to the appropriate position and write it to the temporary file
		FileStream.seekp((filePortionIndex * FileChunkSize * FileChunkBufferSize) + (chunkIndex * FileChunkSize));
		FileStream.write((char*)FileChunkDataBuffer, chunkSize);

		//  Remove the chunk index from the list of chunks to receive, and return out
		FileChunksToReceive.erase(iter);
		return false;
	}

	bool CheckFilePortionComplete(int portionIndex)
	{
		//  If we are being requested to check a portion we're not on right now, ignore it and return out
		if (portionIndex != FilePortionIndex) return false;

		//  If there are still chunks we haven't received in this portion, send a list to the server and return out
		if (FileChunksToReceive.size() != 0)
		{
			SendMessage_FileChunksRemaining(FileChunksToReceive, SocketID, IPAddress.c_str(), ConnectionPort);
			return false;
		}

		//  If there are no chunks to receive, send a confirmation that this file portion is complete
		SendMessage_FilePortionCompleteConfirmation(FilePortionIndex, SocketID, IPAddress.c_str(), ConnectionPort);
		
		//  Iterate to the next file portion, and if we've completed all portions in the file, complete the download and decrypt the file
		if (++FilePortionIndex == FilePortionCount)
		{
			DownloadTime = gameSeconds - DownloadTime;
			FileDownloadComplete = true;
			FileStream.close();
			Groundfish::DecryptAndMoveFile(TempFileName, FileName, true);
		}
		else
		{
			//  Reset the chunk list to ensure we're waiting on the right number of chunks for the next portion
			auto chunksProcessed = FilePortionIndex * FileChunkBufferSize;
			auto nextChunkCount = (FileChunkCount > (chunksProcessed + FileChunkBufferSize)) ? FileChunkBufferSize : (FileChunkCount - chunksProcessed);
			ResetChunksToReceiveMap(nextChunkCount);
		}

		return true;
	}
};
