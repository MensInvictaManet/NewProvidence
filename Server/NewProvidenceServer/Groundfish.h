#pragma once

#include <stdio.h>			/* printf, scanf, puts, NULL */
#include <stdlib.h>			/* srand, rand */
#include <time.h>			/* time */
#include <assert.h>			/* assert */
#include <fstream>			/* ifstream, ofstream */
#include <string>			/* string */
#include <unordered_map>	/* unordered_map */
#include <filesystem>		/* file_size */

#define FILE_ENCRYPTION_BYTES_PER_STEP		1024

typedef std::vector<unsigned char> EncryptedData;

namespace Groundfish
{
	struct GroundfishWordlist
	{
		unsigned long int ListVersion;
		unsigned char WordList[256][256];
		unsigned char ReverseWordList[256][256];
	};

	GroundfishWordlist CurrentWordList;
	unsigned int CurrentVersion = 0;

	EncryptedData Encrypt(const char* data, const int dataLength, const int wordListVersion = 0, unsigned char wordIndex = 0)
	{
		EncryptedData encryptedData;

		GroundfishWordlist& wordList = (wordListVersion == 0) ? CurrentWordList : CurrentWordList; // TODO: Update this to grab old versions
		unsigned int encryptionIndex = 0;

		//  Input the word list version number
		encryptedData.push_back(((unsigned char*)&wordListVersion)[0]);
		encryptedData.push_back(((unsigned char*)&wordListVersion)[1]);
		encryptedData.push_back(((unsigned char*)&wordListVersion)[2]);
		encryptedData.push_back(((unsigned char*)&wordListVersion)[3]);
		encryptionIndex += 4;

		encryptedData.push_back(((unsigned char*)&dataLength)[0]);
		encryptedData.push_back(((unsigned char*)&dataLength)[1]);
		encryptedData.push_back(((unsigned char*)&dataLength)[2]);
		encryptedData.push_back(((unsigned char*)&dataLength)[3]);
		encryptionIndex += 4;

		//  Encrypt and input the word index to start decryption at
		encryptedData.push_back(wordList.WordList[0][wordIndex]);
		encryptionIndex += 1;

		for (int i = 0; i < dataLength; ++i)
			encryptedData.push_back(wordList.WordList[wordIndex++][(unsigned char)data[i]]);

		return encryptedData;
	}

	bool EncryptAndMoveFile(std::string targetFileName, std::string newFileName, const int wordListVersion = 0, unsigned char wordIndex = 0)
	{
		GroundfishWordlist& wordList = (wordListVersion == 0) ? CurrentWordList : CurrentWordList; // TODO: Update this to grab old versions

		std::ifstream targetFile(targetFileName, std::ios_base::binary);
		assert(targetFile.good() && !targetFile.bad());

		std::ofstream newFile(newFileName, std::ios_base::binary);
		assert(newFile.good() && !newFile.bad());

		//  Get the file size in bytes for the unencrypted file
		uint64_t fileSize = std::filesystem::file_size(targetFileName);

		newFile.write((char*)&wordListVersion, sizeof(wordListVersion));
		newFile.write((char*)&fileSize, sizeof(fileSize));
		newFile.write((char*)&wordIndex, sizeof(wordIndex));

		uint64_t bytesRead = 0;
		uint64_t bytesToRead = 0;
		unsigned char readArray[1024] = "";
		while (targetFile.eof() == false)
		{
			bytesToRead = ((fileSize - bytesRead) > 1024) ? 1024 : fileSize - bytesRead;
			if (bytesToRead <= 0) break;
			targetFile.read((char*)readArray, bytesToRead);
			bytesRead += bytesToRead;
			for (int i = 0; i < bytesToRead; ++i)
				readArray[i] = (char)wordList.WordList[wordIndex++][(unsigned char)readArray[i]];

			newFile.write((char*)readArray, bytesToRead);
		}

		targetFile.close();
		newFile.close();

		return true;
	}

	EncryptedData Decrypt(const unsigned char* encrypted)
	{
		EncryptedData decryptedData;

		unsigned int encryptionIndex = 0;
		unsigned int wordListVersion = 0;
		unsigned int messageLength = 0;

		memcpy((void*)&wordListVersion, (const void*)&encrypted[encryptionIndex], 4);
		encryptionIndex += 4;

		memcpy((void*)&messageLength, (const void*)&encrypted[encryptionIndex], 4);
		encryptionIndex += 4;

		GroundfishWordlist& wordList = (wordListVersion == 0) ? CurrentWordList : CurrentWordList; // TODO: Update this to grab old versions

		std::unordered_map<unsigned char, unsigned char> characterMap;
		for (int i = 0; i < 256; ++i) characterMap[wordList.WordList[0][i]] = i;

		unsigned char wordIndex = characterMap[encrypted[encryptionIndex]];
		encryptionIndex += 1;

		for (unsigned int i = 0; i < messageLength; ++i)
			decryptedData.push_back((char)wordList.ReverseWordList[wordIndex++][(unsigned char)encrypted[encryptionIndex + i]]);

		return decryptedData;
	}

	std::string DecryptToString(const unsigned char* encrypted)
	{
		auto decryptedVector = Decrypt(encrypted);
		return std::string((char*)decryptedVector.data(), decryptedVector.size());
	}

	void SaveWordList(GroundfishWordlist& savedList, std::string filename)
	{
		std::ofstream wordlistOutput(filename.c_str(), std::ofstream::out | std::ifstream::binary);
		assert(!wordlistOutput.bad() && wordlistOutput.good());

		wordlistOutput.write((char*)(&savedList), sizeof(GroundfishWordlist));

		wordlistOutput.close();
	}

	void LoadWordList(GroundfishWordlist& wordList, int index = -1)
	{
		std::string filename;
		if (index == -1) filename += "Groundfish.words";
		else
		{
			filename += "WordLists/";
			filename += std::to_string(index);
			filename += ".words";
		}

		std::ifstream wordlistInput(filename.c_str(), std::ifstream::in | std::ifstream::binary);
		assert(!wordlistInput.bad() && wordlistInput.good());

		wordlistInput.read((char*)(&wordList), sizeof(GroundfishWordlist));
		wordlistInput.close();
	}

	void LoadCurrentWordList()
	{
		LoadWordList(CurrentWordList);
		CurrentVersion = CurrentWordList.ListVersion;
	}

	void CreateWordList(GroundfishWordlist& newList)
	{
		//  Seed the random number generator
		srand((unsigned int)(time(NULL)));

		//  Create the word list and fill it in sequential order for each word
		for (int i = 0; i < 256; ++i)
			for (int j = 0; j < 256; ++j)
				newList.WordList[i][j] = j;

		//  For each word, randomize the order of the characters
		for (int i = 0; i < 256; ++i)
		{
			for (int j = 0; j < 256; ++j)
			{
				int other = rand() % 256;
				if (other == j) continue;
				newList.WordList[i][j] ^= newList.WordList[i][other];
				newList.WordList[i][other] ^= newList.WordList[i][j];
				newList.WordList[i][j] ^= newList.WordList[i][other];
			}
		}

		//  For each word, confirm that we have every character, and fill in the reverse word list
		for (int i = 0; i < 256; ++i)
		{
			for (int j = 0; j < 256; ++j)
			{
				bool characterFound = false;
				for (int k = 0; k < 256; ++k)
				{
					if (newList.WordList[i][k] == j)
					{
						newList.ReverseWordList[i][j] = k;
						characterFound = true;
						break;
					}
				}
				assert(characterFound);
			}
		}

		newList.ListVersion = (++CurrentVersion);
		SaveWordList(newList, "Groundfish.words");
	}

	void ArchiveWordList(GroundfishWordlist& archivedList)
	{
		std::string filename;
		filename += "WordLists/";
		filename += std::to_string(archivedList.ListVersion);
		filename += ".words";

		SaveWordList(archivedList, filename);
	}

	void UpdateWordList()
	{
		ArchiveWordList(CurrentWordList);
		CreateWordList(CurrentWordList);
	}
}

struct FileEncryptTask
{
	const std::string TaskName;
	const std::string TargetFileName;
	const std::string NewFileName;
	const int WordListVersion;
	unsigned char WordIndex;

	Groundfish::GroundfishWordlist& WordList;

	std::ifstream FileStreamIn;
	std::ofstream FileStreamOut;
	uint64_t FileInSize;
	uint64_t BytesRead;

	bool EncryptionComplete;

	double EncryptionPercentage;

	FileEncryptTask(std::string taskName, std::string targetFileName, std::string newFileName, const int wordListVersion = 0, unsigned char wordStartingIndex = 0) :
		TaskName(taskName),
		TargetFileName(targetFileName),
		NewFileName(newFileName),
		WordListVersion(wordListVersion),
		WordIndex(wordStartingIndex),
		WordList(Groundfish::CurrentWordList),
		BytesRead(0),
		EncryptionComplete(false),
		EncryptionPercentage(0.0)
	{
		//  Open the target file, and ensure it is a valid file
		FileStreamIn = std::ifstream(TargetFileName, std::ios_base::binary);
		assert(FileStreamIn.good() && !FileStreamIn.bad());

		//  Open the new file, and ensure it is a valid file
		FileStreamOut = std::ofstream(NewFileName, std::ios_base::binary);
		assert(FileStreamOut.good() && !FileStreamOut.bad());

		//  Get the file size in bytes for the unencrypted file
		FileInSize = std::filesystem::file_size(TargetFileName);

		//  Write out the file information for the output file
		FileStreamOut.write((char*)&WordListVersion, sizeof(WordListVersion));
		FileStreamOut.write((char*)&FileInSize, sizeof(FileInSize));
		FileStreamOut.write((char*)&WordIndex, sizeof(WordIndex));
	}

	bool Update()
	{
		if (EncryptionComplete) return true;

		uint64_t bytesToRead = 0;
		unsigned char readArray[FILE_ENCRYPTION_BYTES_PER_STEP] = "";

		//  Determine how many bytes to read this step
		bytesToRead = ((FileInSize - BytesRead) > FILE_ENCRYPTION_BYTES_PER_STEP) ? FILE_ENCRYPTION_BYTES_PER_STEP : FileInSize - BytesRead;

		//  If we've gotten this far, we should grab the specified amount of data, encrypt it, and write it to the new file
		FileStreamIn.read((char*)readArray, bytesToRead);
		BytesRead += bytesToRead;
		for (int i = 0; i < bytesToRead; ++i) readArray[i] = (char)WordList.WordList[WordIndex++][(unsigned char)readArray[i]];
		FileStreamOut.write((char*)readArray, bytesToRead);

		EncryptionPercentage = double(BytesRead) / double(FileInSize);

		//  If there is no more information to read, we're done encrypting
		if (FileStreamIn.eof() || (BytesRead == FileInSize))
		{
			FileStreamIn.close();
			FileStreamOut.close();
			EncryptionComplete = true;
			return true;
		}

		return false;
	}
};

struct FileDecryptTask
{
	const std::string TaskName;
	const std::string TargetFileName;
	const std::string NewFileName;
	const bool DeleteOldFile;
	int WordListVersion;
	unsigned char WordIndex;

	Groundfish::GroundfishWordlist& WordList;

	std::ifstream FileStreamIn;
	std::ofstream FileStreamOut;
	uint64_t FileInSize;
	uint64_t BytesRead;

	bool DecryptionComplete;

	double DecryptionPercentage;

	FileDecryptTask(std::string taskName, std::string targetFileName, std::string newFileName, const bool deleteOldFile, const int wordListVersion = 0, unsigned char wordStartingIndex = 0) :
		TaskName(taskName),
		TargetFileName(targetFileName),
		NewFileName(newFileName),
		DeleteOldFile(deleteOldFile),
		WordList(Groundfish::CurrentWordList),
		BytesRead(0),
		DecryptionComplete(false),
		DecryptionPercentage(0.0)
	{
		//  Open the target file, and ensure it is a valid file
		FileStreamIn = std::ifstream(TargetFileName, std::ios_base::binary);
		assert(FileStreamIn.good() && !FileStreamIn.bad());

		//  Open the new file, and ensure it is a valid file
		FileStreamOut = std::ofstream(NewFileName, std::ios_base::binary);
		if (!FileStreamOut.good() || FileStreamOut.bad())
		{
			debugConsole->AddDebugConsoleLine("Attempted to open " + NewFileName + " for writing, but failed. Did you have the file open?");
		}

		//  Write out the file information for the output file
		FileStreamIn.read((char*)&WordListVersion, sizeof(WordListVersion));
		FileStreamIn.read((char*)&FileInSize, sizeof(FileInSize));
		FileStreamIn.read((char*)&WordIndex, sizeof(WordIndex));
	}

	bool Update()
	{
		if (!FileStreamOut.good() || FileStreamOut.bad()) return true;
		if (DecryptionComplete) return true;

		uint64_t bytesToRead = 0;
		unsigned char readArray[FILE_ENCRYPTION_BYTES_PER_STEP] = "";

		//  Determine how many bytes to read this step
		bytesToRead = ((FileInSize - BytesRead) > FILE_ENCRYPTION_BYTES_PER_STEP) ? FILE_ENCRYPTION_BYTES_PER_STEP : FileInSize - BytesRead;

		//  If we've gotten this far, we should grab the specified amount of data, decrypt it, and write it to the new file
		FileStreamIn.read((char*)readArray, bytesToRead);
		BytesRead += bytesToRead;
		for (int i = 0; i < bytesToRead; ++i) readArray[i] = (char)WordList.ReverseWordList[WordIndex++][(unsigned char)readArray[i]];
		FileStreamOut.write((char*)readArray, bytesToRead);

		DecryptionPercentage = double(BytesRead) / double(FileInSize);

		//  If there is no more information to read, we're done decrypting
		if (FileStreamIn.eof() || (BytesRead == FileInSize))
		{
			FileStreamIn.close();
			FileStreamOut.close();
			if (DeleteOldFile) std::filesystem::remove(TargetFileName.c_str());
			DecryptionComplete = true;
			return true;
		}

		return false;
	}
};