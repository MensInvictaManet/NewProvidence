#pragma once

#include <stdio.h>			/* printf, scanf, puts, NULL */
#include <stdlib.h>			/* srand, rand */
#include <time.h>			/* time */
#include <assert.h>			/* assert */
#include <fstream>			/* ifstream, ofstream */
#include <string>			/* string */
#include <unordered_map>	/* unordered_map */

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

	std::vector<unsigned char> Encrypt(const char* data, const int dataLength, const int wordListVersion = 0, unsigned char wordIndex = 0)
	{
		std::vector<unsigned char> encryptedData;

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
		unsigned int encryptionIndex = 0;

		std::ifstream targetFile(targetFileName, std::ios_base::binary);
		assert(targetFile.good() && !targetFile.bad());

		std::ofstream newFile(newFileName, std::ios_base::binary);
		assert(newFile.good() && !newFile.bad());

		//  Grab the file size of the unencrypted file
		auto fileSize = int(targetFile.tellg());
		targetFile.seekg(0, std::ios::end);
		fileSize = int(targetFile.tellg()) - fileSize;
		targetFile.seekg(0, std::ios::beg);

		newFile.write((char*)&wordListVersion, sizeof(wordListVersion));
		newFile.write((char*)&fileSize, sizeof(fileSize));
		newFile.write((char*)&wordIndex, sizeof(wordIndex));

		int bytesRead = 0;
		int bytesToRead = 0;
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

	std::vector<unsigned char> Decrypt(const unsigned char* encrypted)
	{
		std::vector<unsigned char> decryptedData;

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
		{
			for (int j = 0; j < 256; ++j) characterMap[wordList.WordList[wordIndex][j]] = j;
			wordIndex++;

			decryptedData.push_back(characterMap[(unsigned char)encrypted[encryptionIndex + i]]);
		}

		return decryptedData;
	}

	void DecryptAndMoveFile(std::string targetFileName, std::string newFileName, bool deleteOld)
	{
		std::ifstream targetFile(targetFileName, std::ios_base::binary);
		assert(targetFile.good() && !targetFile.bad());

		std::ofstream newFile(newFileName, std::ios_base::binary);
		assert(newFile.good() && !newFile.bad());

		int wordListVersion = 0;
		int newFileSize = 0;
		unsigned char wordIndex = 0;

		auto fileSize = int(targetFile.tellg());
		targetFile.seekg(0, std::ios::end);
		fileSize = int(targetFile.tellg()) - fileSize;
		targetFile.seekg(0, std::ios::beg);

		targetFile.read((char*)&wordListVersion, sizeof(wordListVersion));
		targetFile.read((char*)&newFileSize, sizeof(newFileSize));
		targetFile.read((char*)&wordIndex, sizeof(wordIndex));

		GroundfishWordlist& wordList = (wordListVersion == 0) ? CurrentWordList : CurrentWordList; // TODO: Update this to grab old versions

		int bytesRead = 0;
		int bytesToRead = 0;
		unsigned char readArray[1024] = "";
		while (targetFile.eof() == false)
		{
			bytesToRead = ((fileSize - bytesRead) > 1024) ? 1024 : fileSize - bytesRead;
			if (bytesToRead <= 0) break;
			targetFile.read((char*)readArray, bytesToRead);
			bytesRead += bytesToRead;
			for (int i = 0; i < bytesToRead; ++i)
				readArray[i] = (char)wordList.ReverseWordList[wordIndex++][(unsigned char)readArray[i]];

			newFile.write((char*)readArray, bytesToRead);
		}

		targetFile.close();
		newFile.close();

		if (deleteOld) std::remove(targetFileName.c_str());
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