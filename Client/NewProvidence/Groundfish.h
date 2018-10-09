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
	};

	GroundfishWordlist CurrentWordList;
	unsigned int CurrentVersion = 0;

	int Encrypt(const char* data, unsigned char* encrypted, const int dataLength, const int wordListVersion = 0, unsigned char wordIndex = 0)
	{
		GroundfishWordlist& wordList = (wordListVersion == 0) ? CurrentWordList : CurrentWordList; // TODO: Update this to grab old versions
		unsigned int encryptionIndex = 0;

		//  Input the word list version number
		memcpy(&encrypted[encryptionIndex], (const void*)&wordListVersion, 4);
		encryptionIndex += 4;

		memcpy(&encrypted[encryptionIndex], (const void*)&dataLength, 4);
		encryptionIndex += 4;

		//  Encrypt and input the word index to start decryption at
		memcpy(&encrypted[encryptionIndex], (const void*)&wordList.WordList[0][wordIndex], sizeof(wordList.WordList[0][wordIndex]));
		encryptionIndex += 1;

		for (int i = 0; i < dataLength; ++i)
			encrypted[encryptionIndex + i] = wordList.WordList[wordIndex++][(unsigned char)data[i]];

		return encryptionIndex + dataLength;
	}

	void Decrypt(const unsigned char* encrypted, char* decrypted)
	{
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

			decrypted[i] = characterMap[(unsigned char)encrypted[encryptionIndex + i]];
		}
	}

	void Decrypt(const unsigned char* encrypted, std::vector<unsigned char>& decrypted)
	{
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

			decrypted.push_back(characterMap[(unsigned char)encrypted[encryptionIndex + i]]);
		}
	}

	void SaveWordList(GroundfishWordlist& savedList, std::string filename)
	{
		std::ofstream wordlistOutput(filename.c_str(), std::ofstream::out | std::ifstream::binary);
		assert(!wordlistOutput.bad() && wordlistOutput.good());

		wordlistOutput.write((char*)(&CurrentWordList), sizeof(GroundfishWordlist));

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

		//  For each word, confirm that we have every character
		for (int i = 0; i < 256; ++i)
		{
			for (int j = 0; j < 256; ++j)
			{
				bool characterFound = false;
				for (int k = 0; k < 256; ++k)
				{
					if (newList.WordList[i][k] == j)
					{
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