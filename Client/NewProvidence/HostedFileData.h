#pragma once

#include <unordered_map>
#include <functional>

enum HostedFileType { FILETYPE_MUSIC, FILETYPE_VIDEO, FILETYPE_OTHER, FILE_TYPE_COUNT };
enum HostedFileSubtype
{
	FILETYPE_MUSIC_LFHHRBTRST, FILETYPE_MUSIC_EDM_DANCE, FILETYPE_MUSIC_OTHER,
	FILETYPE_VIDEO_TV, FILETYPE_VIDEO_MOVIE, FILETYPE_VIDEO_OTHER,
	FILETYPE_OTHER_MISCELLANEOUS,
	FILE_SUBTYPE_COUNT
};

constexpr unsigned int hash(const char* str, int h = 0)
{
	return !str[h] ? 5381 : (hash(str, h + 1) * 33) ^ str[h];
}

inline std::string GetFileTypeNameFromID(int32_t id)
{
	switch (id)
	{
	case FILETYPE_MUSIC:		return "MUSIC";
	case FILETYPE_VIDEO:		return "VIDEO";
	case FILETYPE_OTHER:		return "OTHER";
	default:					return "UNKNOWN";
	}
}

inline int32_t GetFileTypeIDFromName(std::string name)
{
	std::unordered_map<std::string, int32_t> dataMap;
	dataMap["MUSIC"] = FILETYPE_MUSIC;
	dataMap["VIDEO"] = FILETYPE_VIDEO;
	dataMap["OTHER"] = FILETYPE_OTHER;
	return (dataMap.find(name) == dataMap.end() ? -1 : dataMap[name]);
}

inline std::string GetFileSubTypeNameFromID(int32_t id)
{
	switch (id)
	{
	case FILETYPE_MUSIC_LFHHRBTRST:		return "MUSIC: LFHHRBTRST";
	case FILETYPE_MUSIC_EDM_DANCE:		return "MUSIC: EDM/DANCE";
	case FILETYPE_MUSIC_OTHER:			return "MUSIC: OTHER";
	case FILETYPE_VIDEO_TV:				return "MUSIC: TELEVISION";
	case FILETYPE_VIDEO_MOVIE:			return "VIDEO: MOVIE";
	case FILETYPE_VIDEO_OTHER:			return "VIDEO: OTHER";
	case FILETYPE_OTHER_MISCELLANEOUS:	return "OTHER: MISCELANEOUS";
	default:							return "UNKNOWN";
	}
}

inline int32_t GetFileSubTypeIDFromName(std::string name)
{
	std::unordered_map<std::string, int32_t> dataMap;
	dataMap["MUSIC: LFHHRBTRST"] = FILETYPE_MUSIC_LFHHRBTRST;
	dataMap["MUSIC: EDM/DANCE"] = FILETYPE_MUSIC_EDM_DANCE;
	dataMap["MUSIC:OTHER"] = FILETYPE_MUSIC_OTHER;
	dataMap["MUSIC: TELEVISION"] = FILETYPE_VIDEO_TV;
	dataMap["VIDEO: MOVIE"] = FILETYPE_VIDEO_MOVIE;
	dataMap["VIDEO: OTHER"] = FILETYPE_VIDEO_OTHER;
	dataMap["OTHER:MISCELANEOUS"] = FILETYPE_OTHER_MISCELLANEOUS;
	return (dataMap.find(name) == dataMap.end() ? -1 : dataMap[name]);
}

inline GUIObjectNode* GetFileTypeIconFromID(int32_t id)
{
	switch (id)
	{
	case FILETYPE_MUSIC:		return GUIObjectNode::CreateObjectNode("./Assets/Textures/MusicIcon.png");
	case FILETYPE_VIDEO:		return GUIObjectNode::CreateObjectNode("./Assets/Textures/VideoIcon.png");
	case FILETYPE_OTHER:		return GUIObjectNode::CreateObjectNode("./Assets/Textures/UnknownIcon.png");
	default:					return nullptr;
	}
}

inline GUIObjectNode* GetFileSubTypeIconFromID(int32_t id)
{
	switch (id)
	{
	case FILETYPE_MUSIC_LFHHRBTRST:			return GUIObjectNode::CreateObjectNode("./Assets/Textures/LHHRBTRSTIcon.png");
	case FILETYPE_MUSIC_EDM_DANCE:			return GUIObjectNode::CreateObjectNode("./Assets/Textures/UnknownIcon.png");
	case FILETYPE_MUSIC_OTHER:				return GUIObjectNode::CreateObjectNode("./Assets/Textures/UnknownIcon.png");
	case FILETYPE_VIDEO_TV:					return GUIObjectNode::CreateObjectNode("./Assets/Textures/UnknownIcon.png");
	case FILETYPE_VIDEO_MOVIE:				return GUIObjectNode::CreateObjectNode("./Assets/Textures/UnknownIcon.png");
	case FILETYPE_VIDEO_OTHER:				return GUIObjectNode::CreateObjectNode("./Assets/Textures/UnknownIcon.png");
	case FILETYPE_OTHER_MISCELLANEOUS:		return GUIObjectNode::CreateObjectNode("./Assets/Textures/UnknownIcon.png");
	default:								return nullptr;
	}
}

std::vector<std::string> GetListOfFileTypes(void)
{
	std::vector<std::string> typeList;
	for (auto i = 0; i < FILE_TYPE_COUNT; ++i)
		typeList.push_back(GetFileTypeNameFromID(i));
	return typeList;
}

std::vector<std::string> GetListOfFileSubTypes(void)
{
	std::vector<std::string> typeList;
	for (auto i = 0; i < FILE_SUBTYPE_COUNT; ++i)
		typeList.push_back(GetFileSubTypeNameFromID(i));
	return typeList;
}

struct HostedFileData
{

	std::string FileTitleChecksum;
	std::vector<unsigned char> EncryptedFileName;
	std::vector<unsigned char> EncryptedFileTitle;
	std::vector<unsigned char> EncryptedFileDescription;
	std::vector<unsigned char> EncryptedUploader;
	uint64_t FileSize;
	std::string FileUploadTime;
	HostedFileType FileType;
	HostedFileSubtype FileSubType;

	HostedFileData()
	{
		FileTitleChecksum = "";
		EncryptedFileName.clear();
		EncryptedFileTitle.clear();
		EncryptedFileDescription.clear();
		EncryptedUploader.clear();
		FileSize = 0;
		FileUploadTime = "2018-01-01 - 12:00:00";
		FileType = FILETYPE_OTHER;
		FileSubType = FILETYPE_OTHER_MISCELLANEOUS;
	}

	void WriteToFile(std::ofstream& outFile)
	{
		//  Write the checksum length
		int checksumSize = FileTitleChecksum.length();
		outFile.write((char*)&checksumSize, sizeof(checksumSize));

		//  Write the checksum
		outFile.write(FileTitleChecksum.c_str(), checksumSize);

		//  Write the encrypted file name length
		int efnSize = EncryptedFileName.size();
		outFile.write((char*)&efnSize, sizeof(efnSize));

		//  Write the encrypted file name
		outFile.write((char*)EncryptedFileName.data(), efnSize);

		//  Write the encrypted file title length
		int eftSize = EncryptedFileTitle.size();
		outFile.write((char*)&eftSize, sizeof(eftSize));

		//  Write the encrypted file title
		outFile.write((char*)EncryptedFileTitle.data(), eftSize);

		//  Write the encrypted file description length
		int efdSize = EncryptedFileDescription.size();
		outFile.write((char*)&efdSize, sizeof(efdSize));

		//  Write the encrypted file description
		outFile.write((char*)EncryptedFileDescription.data(), efdSize);

		//  Write the encrypted file uploader length
		int efuSize = EncryptedUploader.size();
		outFile.write((char*)&efuSize, sizeof(efuSize));

		//  Write the encrypted file uploader
		outFile.write((char*)EncryptedUploader.data(), efuSize);

		//  Write the file size
		outFile.write((char*)&FileSize, sizeof(FileSize));

		//  Write the file upload time string size
		int uploadTimeLength = int(FileUploadTime.length());
		outFile.write((char*)(&uploadTimeLength), sizeof(uploadTimeLength));

		//  Write the file upload time string
		outFile.write((char*)FileUploadTime.c_str(), uploadTimeLength);

		//  Write the file type and sub-type
		outFile.write((char*)&FileType, sizeof(FileType));
		outFile.write((char*)&FileSubType, sizeof(FileSubType));

	}

	bool ReadFromFile(std::ifstream& inFile)
	{
		//  Read the checksum length
		int checksumSize;
		inFile.read((char*)&checksumSize, sizeof(checksumSize));
		if (inFile.eof()) return false;

		//  Read the checksum
		char readInData[512];
		inFile.read(readInData, checksumSize);
		FileTitleChecksum = std::string(readInData, checksumSize);

		//  Read the encrypted file name length
		int efnSize;
		inFile.read((char*)&efnSize, sizeof(efnSize));

		//  Read the encrypted file name
		inFile.read(readInData, efnSize);
		for (auto i = 0; i < efnSize; ++i) EncryptedFileName.push_back((unsigned char)readInData[i]);

		//  Read the encrypted file title length
		int eftSize;
		inFile.read((char*)&eftSize, sizeof(eftSize));

		//  Read the encrypted file title
		inFile.read(readInData, eftSize);
		for (auto i = 0; i < eftSize; ++i) EncryptedFileTitle.push_back((unsigned char)readInData[i]);

		//  Read the encrypted file description length
		int efdSize;
		inFile.read((char*)&efdSize, sizeof(efdSize));

		//  Read the encrypted file name
		inFile.read(readInData, efdSize);
		for (auto i = 0; i < efdSize; ++i) EncryptedFileDescription.push_back((unsigned char)readInData[i]);

		//  Read the encrypted file uploader length
		int efuSize;
		inFile.read((char*)&efuSize, sizeof(efuSize));

		//  Read the encrypted file uploader
		inFile.read(readInData, efuSize);
		for (auto i = 0; i < efuSize; ++i) EncryptedUploader.push_back((unsigned char)readInData[i]);

		//  Read the file size
		inFile.read((char*)&FileSize, sizeof(FileSize));

		//  Read the file upload time length
		int futLength;
		inFile.read((char*)&futLength, sizeof(futLength));

		//  Read the encrypted file uploader
		inFile.read(readInData, futLength);
		FileUploadTime.clear();
		for (auto i = 0; i < futLength; ++i) FileUploadTime += readInData[i];

		//  Read the file type and sub-type
		inFile.read((char*)&FileType, sizeof(FileType));
		inFile.read((char*)&FileSubType, sizeof(FileSubType));

		return true;
	}
};