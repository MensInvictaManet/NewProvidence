#pragma once

#include <unordered_map>

enum HostedFileType { FILETYPE_MUSIC, FILETYPE_VIDEO, FILETYPE_GAMES, FILETYPE_OTHER, FILE_TYPE_COUNT };
enum HostedFileSubtype
{
	FILETYPE_MUSIC_LFHHRBTRST, FILETYPE_MUSIC_EDM_DANCE, FILETYPE_MUSIC_OTHER,
	FILETYPE_VIDEO_TV, FILETYPE_VIDEO_MOVIE, FILETYPE_VIDEO_PORN, FILETYPE_VIDEO_OTHER,
	FILETYPE_GAMES_MISCELLANEOUS,
	FILETYPE_OTHER_MISCELLANEOUS,
	FILE_SUBTYPE_COUNT
};

constexpr unsigned int hash(const char* str, int h = 0)
{
	return !str[h] ? 5381 : (hash(str, h + 1) * 33) ^ str[h];
}

inline std::string GetFileTypeNameFromID(HostedFileType id)
{
	switch (id)
	{
	case FILETYPE_MUSIC:		return "MUSIC";
	case FILETYPE_VIDEO:		return "VIDEO";
	case FILETYPE_GAMES:		return "GAMES";
	case FILETYPE_OTHER:		return "OTHER";
	case FILE_TYPE_COUNT:		return "NO TYPE";
	default:					return "UNKNOWN";
	}
}

inline HostedFileType GetFileTypeIDFromName(std::string name)
{
	std::unordered_map<std::string, HostedFileType> dataMap;
	dataMap["MUSIC"]	= FILETYPE_MUSIC;
	dataMap["VIDEO"]	= FILETYPE_VIDEO;
	dataMap["GAMES"]	= FILETYPE_GAMES;
	dataMap["OTHER"]	= FILETYPE_OTHER;
	dataMap["NO TYPE"]	= FILE_TYPE_COUNT;
	return (dataMap.find(name) == dataMap.end() ? HostedFileType(-1) : dataMap[name]);
}

inline std::string GetFileSubTypeNameFromID(HostedFileSubtype id)
{
	switch (id)
	{
	case FILETYPE_MUSIC_LFHHRBTRST:		return "MUSIC: LFHHRBTRST";
	case FILETYPE_MUSIC_EDM_DANCE:		return "MUSIC: EDM/DANCE";
	case FILETYPE_MUSIC_OTHER:			return "MUSIC: OTHER";
	case FILETYPE_VIDEO_TV:				return "VIDEO: TELEVISION";
	case FILETYPE_VIDEO_MOVIE:			return "VIDEO: MOVIE";
	case FILETYPE_VIDEO_PORN:			return "VIDEO: PORN";
	case FILETYPE_VIDEO_OTHER:			return "VIDEO: OTHER";
	case FILETYPE_GAMES_MISCELLANEOUS:	return "GAMES: MISCELANEOUS";
	case FILETYPE_OTHER_MISCELLANEOUS:	return "OTHER: MISCELANEOUS";
	case FILE_SUBTYPE_COUNT:			return "NO SUBTYPE";
	default:							return "UNKNOWN";
	}
}

inline HostedFileSubtype GetFileSubTypeIDFromName(std::string name)
{
	std::unordered_map<std::string, HostedFileSubtype> dataMap;
	dataMap["MUSIC: LFHHRBTRST"]	= FILETYPE_MUSIC_LFHHRBTRST;
	dataMap["MUSIC: EDM/DANCE"]		= FILETYPE_MUSIC_EDM_DANCE;
	dataMap["MUSIC: OTHER"]			= FILETYPE_MUSIC_OTHER;
	dataMap["VIDEO: TELEVISION"]	= FILETYPE_VIDEO_TV;
	dataMap["VIDEO: MOVIE"]			= FILETYPE_VIDEO_MOVIE;
	dataMap["VIDEO: PORN"]			= FILETYPE_VIDEO_PORN;
	dataMap["VIDEO: OTHER"]			= FILETYPE_VIDEO_OTHER;
	dataMap["GAMES: MISCELANEOUS"]	= FILETYPE_GAMES_MISCELLANEOUS;
	dataMap["OTHER: MISCELANEOUS"]	= FILETYPE_OTHER_MISCELLANEOUS;
	dataMap["NO SUBTYPE"]			= FILE_SUBTYPE_COUNT;
	return (dataMap.find(name) == dataMap.end() ? HostedFileSubtype(-1) : dataMap[name]);
}

inline GUIObjectNode* GetFileTypeIconFromID(HostedFileType id)
{
	switch (id)
	{
	case FILETYPE_MUSIC:		return GUIObjectNode::CreateObjectNode("./Assets/Textures/MusicIcon.png");
	case FILETYPE_VIDEO:		return GUIObjectNode::CreateObjectNode("./Assets/Textures/VideoIcon.png");
	case FILETYPE_GAMES:		return GUIObjectNode::CreateObjectNode("./Assets/Textures/GamesIcon.png");
	case FILETYPE_OTHER:		return GUIObjectNode::CreateObjectNode("./Assets/Textures/UnknownIcon.png");
	default:					assert(false); return GUIObjectNode::CreateObjectNode("./Assets/Textures/UnknownIcon.png");;
	}
}

inline GUIObjectNode* GetFileSubTypeIconFromID(HostedFileSubtype id)
{
	switch (id)
	{
	case FILETYPE_MUSIC_LFHHRBTRST:			return GUIObjectNode::CreateObjectNode("./Assets/Textures/LHHRBTRSTIcon.png");
	case FILETYPE_MUSIC_EDM_DANCE:			return GUIObjectNode::CreateObjectNode("./Assets/Textures/UnknownIcon.png");
	case FILETYPE_MUSIC_OTHER:				return GUIObjectNode::CreateObjectNode("./Assets/Textures/UnknownIcon.png");
	case FILETYPE_VIDEO_TV:					return GUIObjectNode::CreateObjectNode("./Assets/Textures/TelevisionIcon.png");
	case FILETYPE_VIDEO_MOVIE:				return GUIObjectNode::CreateObjectNode("./Assets/Textures/MovieIcon.png");
	case FILETYPE_VIDEO_PORN:				return GUIObjectNode::CreateObjectNode("./Assets/Textures/PornIcon.png");
	case FILETYPE_VIDEO_OTHER:				return GUIObjectNode::CreateObjectNode("./Assets/Textures/UnknownIcon.png");
	case FILETYPE_GAMES_MISCELLANEOUS:		return GUIObjectNode::CreateObjectNode("./Assets/Textures/UnknownIcon.png");
	case FILETYPE_OTHER_MISCELLANEOUS:		return GUIObjectNode::CreateObjectNode("./Assets/Textures/UnknownIcon.png");
	default:								return nullptr;
	}
}

std::vector<std::string> GetListOfFileTypes(void)
{
	std::vector<std::string> typeList;
	for (auto i = 0; i < FILE_TYPE_COUNT; ++i)
		typeList.push_back(GetFileTypeNameFromID(HostedFileType(i)));
	return typeList;
}

std::vector<std::string> GetListOfFileSubTypes(void)
{
	std::vector<std::string> typeList;
	for (auto i = 0; i < FILE_SUBTYPE_COUNT; ++i)
		typeList.push_back(GetFileSubTypeNameFromID(HostedFileSubtype(i)));
	return typeList;
}

std::vector<std::string> GetListOfSpecificFileSubTypes(std::string typeName)
{
	std::vector<std::string> typeList;
	for (auto i = 0; i < FILE_SUBTYPE_COUNT; ++i)
		if (GetFileSubTypeNameFromID(HostedFileSubtype(i)).compare(0, typeName.length(), typeName.c_str()) == 0)
			typeList.push_back(GetFileSubTypeNameFromID(HostedFileSubtype(i)));
	return typeList;
}

inline int CompareEncryptedData(EncryptedData& vec1, EncryptedData& vec2, bool caseSensitive = true)
{
	auto string1 = Groundfish::DecryptToString(vec1.data());
	auto string2 = Groundfish::DecryptToString(vec2.data());

	if (caseSensitive == false)
	{
		std::transform(string1.begin(), string1.end(), string1.begin(), ::tolower);
		std::transform(string2.begin(), string2.end(), string2.begin(), ::tolower);
	}

	return string1.compare(string2);
}

struct HostedFileData
{
	std::string FileTitleChecksum;
	EncryptedData EncryptedFileName;
	EncryptedData EncryptedFileTitle;
	EncryptedData EncryptedFileDescription;
	EncryptedData EncryptedUploader;
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

	inline bool IsIdentical(HostedFileData& other)
	{
		if (FileTitleChecksum.compare(other.FileTitleChecksum) != 0) return false;
		if (strcmp((char*)(EncryptedFileName.data()), (char*)(other.EncryptedFileName.data())) != 0) return false;
		if (strcmp((char*)(EncryptedFileTitle.data()), (char*)(other.EncryptedFileTitle.data())) != 0) return false;
		if (strcmp((char*)(EncryptedFileDescription.data()), (char*)(other.EncryptedFileDescription.data())) != 0) return false;
		if (strcmp((char*)(EncryptedUploader.data()), (char*)(other.EncryptedUploader.data())) != 0) return false;
		if (FileSize != other.FileSize) return false;
		if (FileUploadTime.compare(other.FileUploadTime) != 0) return false;
		if (FileType != other.FileType) return false;
		if (FileSubType != other.FileSubType) return false;
		return true;
	}

	inline int hexCharToDecimal(char digit) { if (digit >= '0' && digit <= '9') return int(digit) - int('0'); else return int(digit) - int('a') + 10; }

	inline int hexStringToDecimal(std::string hex, int digits)
	{
		int num = 0;
		for (auto i = 0; i < digits; ++i) num += (int(std::pow(16, digits - i - 1)) * hexCharToDecimal(hex[i]));
		return num;
	}

	inline void LoadFileNameFromString(std::string str) {
		if ((str.size() & 1) != 0) { return; }
		EncryptedFileName.clear();
		str = str.substr(2, str.size() - 2);
		while (str.size() > 0) { EncryptedFileName.push_back((unsigned char)(hexStringToDecimal(str, 2))); str = str.substr(2, str.size() - 2); }
	}

	inline void LoadFileTitleFromString(std::string str) {
		if ((str.size() & 1) != 0) { return; }
		EncryptedFileTitle.clear();
		str = str.substr(2, str.size() - 2);
		while (str.size() > 0) { EncryptedFileTitle.push_back((unsigned char)(hexStringToDecimal(str, 2))); str = str.substr(2, str.size() - 2); }
	}

	inline void LoadFileDescFromString(std::string str) {
		if ((str.size() & 1) != 0) { return; }
		EncryptedFileDescription.clear();
		str = str.substr(2, str.size() - 2);
		while (str.size() > 0) { EncryptedFileDescription.push_back((unsigned char)(hexStringToDecimal(str, 2))); str = str.substr(2, str.size() - 2); }
	}

	inline void LoadUploaderFromString(std::string str) {
		if ((str.size() & 1) != 0) { return; }
		EncryptedUploader.clear();
		str = str.substr(2, str.size() - 2);
		while (str.size() > 0) { EncryptedUploader.push_back((unsigned char)(hexStringToDecimal(str, 2))); str = str.substr(2, str.size() - 2); }
	}

	inline std::string getFileNameString()
	{
		auto toHex = [](int x) { std::ostringstream oss; oss << std::hex << x; auto zero = (oss.str().length() == 1); return (zero ? "0" : "") + oss.str(); };
		std::string str = "0x";
		for (size_t i = 0; i < EncryptedFileName.size(); ++i) { str += toHex(int(EncryptedFileName[i])); }
		return str;
	}

	inline std::string getFileTitleString()
	{
		auto toHex = [](int x) { std::ostringstream oss; oss << std::hex << x; auto zero = (oss.str().length() == 1); return (zero ? "0" : "") + oss.str(); };
		std::string str = "0x";
		for (size_t i = 0; i < EncryptedFileTitle.size(); ++i) { str += toHex(int(EncryptedFileTitle[i])); }
		return str;
	}

	inline std::string getFileDescString()
	{
		auto toHex = [](int x) { std::ostringstream oss; oss << std::hex << x; auto zero = (oss.str().length() == 1); return (zero ? "0" : "") + oss.str(); };
		std::string str = "0x";
		for (size_t i = 0; i < EncryptedFileDescription.size(); ++i) { str += toHex(int(EncryptedFileDescription[i])); }
		return str;
	}

	inline std::string getFileUploaderString()
	{
		auto toHex = [](int x) { std::ostringstream oss; oss << std::hex << x; auto zero = (oss.str().length() == 1); return (zero ? "0" : "") + oss.str(); };
		std::string str = "0x";
		for (size_t i = 0; i < EncryptedUploader.size(); ++i) { str += toHex(int(EncryptedUploader[i])); }
		return str;
	}
};