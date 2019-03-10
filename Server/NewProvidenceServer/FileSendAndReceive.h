#pragma once

#include <thread>
#include <filesystem>
#include "Engine/WinsockWrapper.h"
#include "MessageIdentifiers.h"
#include "Groundfish.h"
#include "HostedFileData.h"


constexpr auto FILE_CHUNK_SIZE = 1024;
constexpr auto FILE_CHUNK_BUFFER_COUNT = 500;
constexpr auto FILE_SEND_BUFFER_SIZE = (FILE_CHUNK_SIZE * FILE_CHUNK_BUFFER_COUNT);
constexpr auto PORTION_COMPLETE_REMIND_TIME = 0.1;

constexpr auto UPLOAD_TITLE_MAX_LENGTH = 40;
constexpr auto ENCRYPTED_TITLE_MAX_SIZE = (UPLOAD_TITLE_MAX_LENGTH + 9);

#define FILE_TRANSFER_DEBUGGING				0
#if FILE_TRANSFER_DEBUGGING
#include "Engine/DebugConsole.h"
#endif


void SendMessage_FileSendInitializer(std::string fileName, std::string fileTitle, std::string fileDescription, HostedFileType fileTypeID, HostedFileSubtype fileSubTypeID, uint64_t fileSize, int socket, const char* ip, const int port)
{
	//  Encrypt the file name, title, and description string using Groundfish
	EncryptedData encryptedFilename = Groundfish::Encrypt(fileName.c_str(), int(fileName.length()), 0, rand() % 256);
	EncryptedData encryptedTitle = Groundfish::Encrypt(fileTitle.c_str(), int(fileTitle.length()), 0, rand() % 256);
	EncryptedData encryptedDescription = Groundfish::Encrypt(fileDescription.c_str(), int(fileDescription.length()), 0, rand() % 256);

	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_FILE_SEND_INIT, 0);
	winsockWrapper.WriteInt(int(encryptedFilename.size()), 0);
	winsockWrapper.WriteInt(int(encryptedTitle.size()), 0);
	winsockWrapper.WriteInt(int(encryptedDescription.size()), 0);
	winsockWrapper.WriteChars(encryptedFilename.data(), int(encryptedFilename.size()), 0);
	winsockWrapper.WriteChars(encryptedTitle.data(), int(encryptedTitle.size()), 0);
	winsockWrapper.WriteChars(encryptedDescription.data(), int(encryptedDescription.size()), 0);
	winsockWrapper.WriteUnsignedShort((short)(fileTypeID), 0);
	winsockWrapper.WriteUnsignedShort((short)(fileSubTypeID), 0);
	winsockWrapper.WriteLongInt(fileSize, 0);
	winsockWrapper.WriteLongInt(FILE_CHUNK_SIZE, 0);
	winsockWrapper.WriteLongInt(FILE_CHUNK_BUFFER_COUNT, 0);
	winsockWrapper.SendMessagePacket(socket, ip, port, 0);

#if FILE_TRANSFER_DEBUGGING
	debugConsole->AddDebugConsoleLine("Message Sent: MESSAGE_ID_FILE_SEND_INIT");
#endif
}


void SendMessage_FileSendChunk(uint64_t chunkBufferIndex, uint64_t chunkIndex, uint64_t chunkSize, unsigned char* buffer, int socket, const char* ip, const int port)
{
	//  Generate the checksum of the buffer to send before it, so the client can confirm the full, unaltered message arrived
	auto checksum4 = sha256((char*)buffer, 1, int(chunkSize)).substr(0, 4);

	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_FILE_PORTION, 0);
	winsockWrapper.WriteLongInt(chunkBufferIndex, 0);
	winsockWrapper.WriteLongInt(chunkIndex, 0);
	winsockWrapper.WriteLongInt(chunkSize, 0);
	winsockWrapper.WriteInt(4, 0);
	winsockWrapper.WriteChars((unsigned char*)checksum4.c_str(), 4, 0);
	winsockWrapper.WriteChars(buffer, int(chunkSize), 0);
	winsockWrapper.SendMessagePacket(socket, ip, port, 0);
}


void SendMessage_FileTransferPortionComplete(uint64_t portionIndex, int socket, const char* ip, const int port)
{
	//  Send a "File Transfer Portion Complete" message
	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_FILE_PORTION_COMPLETE, 0);
	winsockWrapper.WriteLongInt(portionIndex, 0);
	winsockWrapper.SendMessagePacket(socket, ip, port, 0);

#if FILE_TRANSFER_DEBUGGING
	debugConsole->AddDebugConsoleLine("Message Sent: MESSAGE_ID_FILE_PORTION_COMPLETE");
#endif
}


void SendMessage_FileReceiveReady(int socket, const char* ip, const int port)
{
	//  Send a "File Receive Ready" message
	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_FILE_RECEIVE_READY, 0);
	winsockWrapper.SendMessagePacket(socket, ip, port, 0);

#if FILE_TRANSFER_DEBUGGING
	debugConsole->AddDebugConsoleLine("Message Sent: MESSAGE_ID_FILE_RECEIVE_READY");
#endif
}


void SendMessage_FilePortionCompleteConfirmation(uint64_t portionIndex, int socket, const char* ip, const int port)
{
	//  Send a "File Portion Complete Confirmation" message
	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_FILE_PORTION_COMPLETE_CONFIRM, 0);
	winsockWrapper.WriteLongInt(portionIndex, 0);
	winsockWrapper.SendMessagePacket(socket, ip, port, 0);

#if FILE_TRANSFER_DEBUGGING
	debugConsole->AddDebugConsoleLine("Message Sent: MESSAGE_ID_FILE_PORTION_COMPLETE_CONFIRM");
#endif
}


void SendMessage_FileChunksRemaining(std::unordered_map<uint64_t, bool>& chunksRemaining, int socket, const char* ip, const int port)
{
	//  Send a "File Chunks Remaining" message
	winsockWrapper.ClearBuffer(0);
	winsockWrapper.WriteChar(MESSAGE_ID_FILE_CHUNKS_REMAINING, 0);
	winsockWrapper.WriteInt(chunksRemaining.size(), 0);
	for (auto i = chunksRemaining.begin(); i != chunksRemaining.end(); ++i) winsockWrapper.WriteShort((short)((*i).first), 0);
	winsockWrapper.SendMessagePacket(socket, ip, port, 0);

#if FILE_TRANSFER_DEBUGGING
	debugConsole->AddDebugConsoleLine("Message Sent: MESSAGE_ID_FILE_CHUNKS_REMAINING");
#endif
}

//  FileSendTask class
class FileSendTask
{
public:
	enum FileChunkSendState
	{
		CHUNK_STATE_INITIALIZING = 0,
		CHUNK_STATE_READY_TO_SEND = 1,
		CHUNK_STATE_SENDING = 2,
		CHUNK_STATE_PENDING_COMPLETE = 3,
		CHUNK_STATE_COMPLETE = 4,
	};

	bool FileSendStarted;

private:
	const std::string FileName;
	const std::string FileTitle;
	const std::string FilePath;
	const HostedFileType FileTypeID;
	const HostedFileSubtype FileSubTypeID;
	const int SocketID;
	const std::string IPAddress;
	const int ConnectionPort;

	FileChunkSendState FileChunkTransferState;
	uint64_t FilePortionIndex;
	clock_t LastMessageTime;

	uint64_t FileSize;
	std::ifstream FileStream;

	uint64_t FilePortionCount;
	uint64_t FileChunkCount;
	std::unordered_map<uint64_t, bool> FileChunksToSend;
	char FilePortionBuffer[FILE_CHUNK_BUFFER_COUNT][FILE_CHUNK_SIZE];

	double TransferStartTime;
	double TransferEndTime;

	bool DeleteAfter;

public:
	//  Accessors & Modifiers
	inline std::string GetFileName() const { return FileName; }
	inline std::string GetFileTitle() const { return FileTitle; }
	inline uint64_t GetFileSize() const { return FileSize; }
	inline bool GetFileTransferComplete(void) const { return (FilePortionIndex >= FilePortionCount); }
	inline int GetFileTransferState() const { return FileChunkTransferState; }
	inline void SetFileTransferState(int state) { FileChunkTransferState = (FileChunkSendState)state; }
	inline uint64_t GetFileTransferBytesCompleted() const { return (FilePortionIndex * FILE_SEND_BUFFER_SIZE); }
	inline double GetPercentageComplete() const { return (double)(GetFileTransferBytesCompleted()) / (double)(FileSize); }
	inline double GetEstimatedTransferSpeed() const { return (float(GetFileTransferBytesCompleted()) / (std::max<float>(float(gameSeconds - TransferStartTime), 0.01f))); }
	inline void SetFileTransferEndTime(double endTime) { TransferEndTime = endTime; }
	inline double GetTransferTime() { return TransferEndTime - TransferStartTime; }
	inline uint64_t GetFilePortionsRemaining() const { return (FilePortionCount - FilePortionIndex); }
	inline uint64_t GetEstimatedSecondsRemaining() const { auto estimate = GetEstimatedTransferSpeed(); return ((estimate == 0) ? 100 : uint64_t(double(GetFilePortionsRemaining() * FILE_SEND_BUFFER_SIZE) / GetEstimatedTransferSpeed())); }

	FileSendTask(std::string fileName, std::string fileTitle, std::string filePath, HostedFileType fileTypeID, HostedFileSubtype fileSubTypeID, int socketID, std::string ipAddress, const int port, bool deleteAfter = false) :
		FileSendStarted(false),
		FileName(fileName),
		FileTitle(fileTitle),
		FilePath(filePath),
		FileTypeID(fileTypeID),
		FileSubTypeID(fileSubTypeID),
		SocketID(socketID),
		IPAddress(ipAddress),
		ConnectionPort(port),
		FileChunkTransferState(CHUNK_STATE_INITIALIZING),
		FilePortionIndex(0),
		LastMessageTime(clock()),
		TransferStartTime(gameSeconds),
		TransferEndTime(gameSeconds + 0.1),
		DeleteAfter(deleteAfter)
	{
		//  Initialize the file portion buffer
		memset(FilePortionBuffer, 0, FILE_SEND_BUFFER_SIZE);

#if FILE_TRANSFER_DEBUGGING
		debugConsole->AddDebugConsoleLine("FileSendTask created!");
#endif
	}

	~FileSendTask()
	{
		FileStream.close();

		if (DeleteAfter) std::remove(FilePath.c_str());
	}

	void StartFileSend()
	{
		FileSendStarted = true;

		//  Open the file we're sending and ensure the file handler is valid
		FileStream.open(FilePath, std::ios_base::binary);
		assert(FileStream.good() && !FileStream.bad());

		//  Get the file size in bytes for the file
		FileSize = std::filesystem::file_size(FilePath);

		//  Determine the file chunk count and file portion count
		FileChunkCount = FileSize / FILE_CHUNK_SIZE;
		if ((FileSize % FILE_CHUNK_SIZE) != 0) FileChunkCount += 1;

		FilePortionCount = FileChunkCount / FILE_CHUNK_BUFFER_COUNT;
		if ((FileChunkCount % FILE_CHUNK_BUFFER_COUNT) != 0) FilePortionCount += 1;

		//  Buffer a portion of the file in preparation of sending it
		BufferFilePortion(FilePortionIndex);

		//  Send a "File Send Initializer" message
		SendMessage_FileSendInitializer(FileName, FileTitle, "FILE DESCRIPTION", FileTypeID, FileSubTypeID, FileSize, SocketID, IPAddress.c_str(), ConnectionPort);
	}

	void BufferFilePortion(uint64_t filePortionIndex)
	{
#if FILE_TRANSFER_DEBUGGING
		debugConsole->AddDebugConsoleLine("FileSendTask: Buffering file portion...");
#endif

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
		if (GetFileTransferComplete()) return;

		//  If we're pending completion, wait and send completion confirmation only if we've reached the end
		if (GetFileTransferState() == CHUNK_STATE_PENDING_COMPLETE)
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
#if FILE_TRANSFER_DEBUGGING
			debugConsole->AddDebugConsoleLine("CHUNK_STATE_PENDING_COMPLETE");
#endif
			SetFileTransferState(CHUNK_STATE_PENDING_COMPLETE);
			SendMessage_FileTransferPortionComplete(FilePortionIndex, SocketID, IPAddress.c_str(), ConnectionPort);
			LastMessageTime = clock();
			return;
		}

		//  Determine the values needed to access the data (we might need less than the full buffer)
		auto chunkIndex = uint64_t((*FileChunksToSend.begin()).first);
		auto chunkPosition = uint64_t((FilePortionIndex * FILE_SEND_BUFFER_SIZE) + (FILE_CHUNK_SIZE * chunkIndex));
		auto chunkByteCount = uint64_t(((chunkPosition + FILE_CHUNK_SIZE) > FileSize) ? (FileSize - chunkPosition) : FILE_CHUNK_SIZE);

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
		SetFileTransferState(CHUNK_STATE_SENDING);
	}

	void ConfirmFilePortionSendComplete(uint64_t portionIndex)
	{
#if FILE_TRANSFER_DEBUGGING
		debugConsole->AddDebugConsoleLine("FileSendTask ConfirmFilePortionSendComplete");
#endif

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
	const std::string FileTitle;
	const std::string FileDescription;
	const HostedFileType FileTypeID;
	const HostedFileSubtype FileSubTypeID;

	const uint64_t FileSize;
	const uint64_t FileChunkSize;
	const uint64_t FileChunkBufferCount;
	const std::string TempFileName;
	const int SocketID;
	const std::string IPAddress;
	const int ConnectionPort;

	uint64_t FilePortionIndex;
	bool FileTransferComplete;

	bool DecryptWhenReceived;
	uint64_t FileChunkCount;
	uint64_t FilePortionCount;
	std::unordered_map<uint64_t, bool> FileChunksToReceive;
	std::ofstream FileStream;
	uint64_t CurrentPortionChunkCount;

	double TransferStartTime;
	double TransferEndTime;

	//  NOTE: The file chunk data buffer is set to a size of FILE_CHUNK_SIZE, which assumes this header is the same on sender and receiver.
	unsigned char FileChunkDataBuffer[FILE_CHUNK_SIZE];

public:
	//  Accessors & Modifiers
	inline std::string GetFileName() const { return FileName; }
	inline std::string GetFileTitle() const { return FileTitle; }
	inline std::string GetFileDescription() const { return FileDescription; }
	inline HostedFileType GetFileTypeID() const { return FileTypeID; }
	inline HostedFileSubtype GetFileSubTypeID() const { return FileSubTypeID; }
	inline uint64_t GetFileSize() const { return FileSize; }
	inline bool GetFileTransferComplete() const { return FileTransferComplete; }
	inline bool GetDecryptWhenRecieved() const { return DecryptWhenReceived; }
	inline uint64_t GetFileSendBufferSize() const { return FileChunkSize * FileChunkBufferCount; }
	inline std::string GetTemporaryFileName() const { return TempFileName; }
	inline double GetPortionPartComplete() const { return double(CurrentPortionChunkCount - FileChunksToReceive.size()) / double(CurrentPortionChunkCount); }
	inline double GetPercentageComplete() const { return (double(FilePortionIndex) + GetPortionPartComplete()) * double(GetFileSendBufferSize()) / double(FileSize); }
	inline void SetFileTransferEndTime(double endTime) { TransferEndTime = endTime; }
	inline double GetTransferTime() { return TransferEndTime - TransferStartTime; }
	inline uint64_t GetFileTransferBytesCompleted() const { return (FilePortionIndex * GetFileSendBufferSize()); }
	inline double GetEstimatedTransferSpeed() const { return (float(GetFileTransferBytesCompleted()) / (std::max<float>(float(gameSeconds - TransferStartTime), 0.01f))); }
	inline uint64_t GetFilePortionsRemaining() const { return (FilePortionCount - FilePortionIndex); }
	inline uint64_t GetEstimatedSecondsRemaining() const { return uint64_t(double(GetFilePortionsRemaining() * GetFileSendBufferSize()) / GetEstimatedTransferSpeed()); }

	inline void SetDecryptWhenReceived(bool decrypt) { DecryptWhenReceived = decrypt; }
	inline void ResetChunksToReceiveMap(uint64_t chunkCount) {
		FileChunksToReceive.clear(); for (auto i = 0; i < int(chunkCount); ++i)  FileChunksToReceive[i] = true; CurrentPortionChunkCount = chunkCount;
	}

	inline void CreateTemporaryFile(const std::string tempFileName, const uint64_t tempFileSize) const {
		std::ofstream outputFile(tempFileName, std::ios::binary | std::ios::trunc | std::ios_base::beg);
		assert(outputFile.good() && !outputFile.bad());
		outputFile.seekp(tempFileSize - 1);
		outputFile.write("", 1);
		outputFile.close();
	}

	FileReceiveTask(std::string fileName, std::string fileTitle, std::string fileDescription, HostedFileType fileTypeID, HostedFileSubtype fileSubTypeID, uint64_t fileSize, uint64_t fileChunkSize, uint64_t fileChunkBufferCount, std::string tempFilePath, int socketID, std::string ipAddress, const int port) :
		FileName(fileName),
		FileTitle(fileTitle),
		FileDescription(fileDescription),
		FileTypeID(fileTypeID),
		FileSubTypeID(fileSubTypeID),
		FileSize(fileSize),
		FileChunkSize(fileChunkSize),
		FileChunkBufferCount(fileChunkBufferCount),
		CurrentPortionChunkCount(fileChunkBufferCount),
		TempFileName(tempFilePath),
		SocketID(socketID),
		IPAddress(ipAddress),
		ConnectionPort(port),
		FilePortionIndex(0),
		FileTransferComplete(false),
		TransferStartTime(gameSeconds),
		TransferEndTime(gameSeconds + 0.1)
	{
		//  Initialize member variable data arrays
		memset(FileChunkDataBuffer, 0, FILE_CHUNK_SIZE);

		//  Determine the count of file chunks and file portions we'll be receiving
		FileChunkCount = ((FileSize % FileChunkSize) == 0) ? (FileSize / FileChunkSize) : ((FileSize / FileChunkSize) + 1);
		FilePortionCount = ((FileChunkCount % FileChunkBufferCount) == 0) ? (FileChunkCount / FileChunkBufferCount) : ((FileChunkCount / FileChunkBufferCount) + 1);

		//  Reset the FileChunksToReceive list, which we use to confirm we've successfully received all chunks in a file portion
		ResetChunksToReceiveMap((FileChunkCount > FileChunkBufferCount) ? fileChunkBufferCount : FileChunkCount);

		//  Create a temporary file of the proper size based on the sender's file description
		CreateTemporaryFile(TempFileName, FileSize);

		//  Open the temporary file, this time keeping the file handle open for later writing
		FileStream.open(TempFileName, std::ios_base::binary | std::ios_base::out | std::ios_base::in);
		assert(FileStream.good() && !FileStream.bad());

#if FILE_TRANSFER_DEBUGGING
		debugConsole->AddDebugConsoleLine("File Receive Task created!");
#endif

		//  Send a signal to the file sender that we're ready to receive the file
		SendMessage_FileReceiveReady(SocketID, IPAddress.c_str(), ConnectionPort);
	}

	~FileReceiveTask()
	{
		if (FileStream.is_open()) FileStream.close();
	}


	bool ReceiveFileChunk()
	{
		//  Clear out the File Chunk Data Buffer
		memset(FileChunkDataBuffer, 0, FILE_CHUNK_SIZE);
		std::string chunkChecksum;

		//  Get the file chunk data from the message, as well as a checksum to check it against
		auto filePortionIndex = winsockWrapper.ReadLongInt(0);
		assert(filePortionIndex >= 0);
		assert(filePortionIndex <= FilePortionCount);
		auto chunkIndex = winsockWrapper.ReadLongInt(0);
		assert(chunkIndex >= 0);
		assert(chunkIndex < FILE_CHUNK_BUFFER_COUNT);
		auto chunkSize = winsockWrapper.ReadLongInt(0);
		assert(chunkSize >= 0);
		assert(chunkSize <= FILE_CHUNK_SIZE);
		auto checksumSize = winsockWrapper.ReadInt(0);
		assert(checksumSize >= 0);
		assert(checksumSize == 4);
		chunkChecksum = std::string((char*)winsockWrapper.ReadChars(0, checksumSize), checksumSize);
		memcpy(FileChunkDataBuffer, winsockWrapper.ReadChars(0, int(chunkSize)), int(chunkSize));

		//  If the file transfer is already complete, return out
		if (FileTransferComplete) return true;

		//  If we're receiving a chunk from a portion other than what we're currently on, return out
		if (filePortionIndex != FilePortionIndex) return false;

		//  Ensure we haven't already received this chunk. If we have, return out
		auto iter = FileChunksToReceive.find(chunkIndex);
		if (iter == FileChunksToReceive.end()) return false;

		//  Check the checksum against the data. If they differ, return out
		if (sha256((char*)FileChunkDataBuffer, 1, int(chunkSize)).substr(0, 4) != chunkChecksum) return false;

		//  If the data is new and valid, seek to the appropriate position and write it to the temporary file
		FileStream.seekp((filePortionIndex * FileChunkSize * FileChunkBufferCount) + (chunkIndex * FileChunkSize));
		FileStream.write((char*)FileChunkDataBuffer, chunkSize);

		//  Remove the chunk index from the list of chunks to receive, and return out
		FileChunksToReceive.erase(iter);

		auto fileProgressEvent = FileTransferProgressEventData(GetFileTitle(), GetPercentageComplete(), GetTransferTime(), GetFileSize(), GetEstimatedSecondsRemaining(), "Download", "FileSendAndReceive");
		eventManager.BroadcastEvent(&fileProgressEvent);

		return false;
	}

	bool CheckFilePortionComplete(uint64_t portionIndex)
	{
		//  If we are being requested to check a portion we're not on right now, ignore it and return out
		if (portionIndex != FilePortionIndex)
		{
			if (portionIndex < FilePortionIndex)
				SendMessage_FilePortionCompleteConfirmation(portionIndex, SocketID, IPAddress.c_str(), ConnectionPort);
			return false;
		}

		//  If there are still chunks we haven't received in this portion, send a list to the server and return out
		if (FileChunksToReceive.size() != 0)
		{
			SendMessage_FileChunksRemaining(FileChunksToReceive, SocketID, IPAddress.c_str(), ConnectionPort);
			return false;
		}

		//  If there are no chunks to receive, send a confirmation that this file portion is complete
		SendMessage_FilePortionCompleteConfirmation(FilePortionIndex, SocketID, IPAddress.c_str(), ConnectionPort);

		//  Iterate to the next file portion, and if we've completed all portions in the file, complete the transfer and decrypt the file
		if (++FilePortionIndex == FilePortionCount)
		{
			FileTransferComplete = true;
			FileStream.close();
			std::remove(FileName.c_str());
#if FILE_TRANSFER_DEBUGGING
			debugConsole->AddDebugConsoleLine("File Receive Task complete!");
#endif

			if (DecryptWhenReceived)
			{
				return true;
			}
			else std::rename(TempFileName.c_str(), FileName.c_str());
		}
		else
		{
			//  Reset the chunk list to ensure we're waiting on the right number of chunks for the next portion
			auto chunksProcessed = FilePortionIndex * FileChunkBufferCount;
			auto nextChunkCount = (FileChunkCount > (chunksProcessed + FileChunkBufferCount)) ? FileChunkBufferCount : (FileChunkCount - chunksProcessed);
			ResetChunksToReceiveMap(nextChunkCount);
		}

		return true;
	}
};