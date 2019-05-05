#pragma once

#include "Engine/SQLWrapper.h"
#include "HostedFileData.h"

constexpr auto UserDatabaseName = "UserDatabase";
constexpr auto FileDatabaseName = "FileDatabase";

sqlite3* userDatabase;
sqlite3* fileDatabase;
static SQLSelectData selectDataPassIn;

int NPSQL_ErrorCallback(void *NotUsed, int argc, char **argv, char **azColName)
{
	selectDataPassIn.clear();
	//  Get the username
	auto username = "";
	for (auto i = 0; i < argc; ++i) if (strcmp(azColName[i], "USERNAME") == 0) { username = argv[i]; break; }
	for (auto i = 0; i < argc; i++) selectDataPassIn[username][azColName[i]] = argv[i];
	return 0;
};

namespace NPSQL {
	inline bool OpenUserDatabaseConnection(void)	{ return sqlWrapper.OpenDatabase(UserDatabaseName); }
	inline void CloseUserDatabaseConnection(void)	{ sqlWrapper.CloseDatabase(UserDatabaseName); }
	inline bool OpenFileDatabaseConnection(void) { return sqlWrapper.OpenDatabase(FileDatabaseName); }
	inline void CloseFileDatabaseConnection(void) { sqlWrapper.CloseDatabase(FileDatabaseName); }

	bool CreateUserTable(void) {
		return sqlWrapper.CreateDatabaseTable(UserDatabaseName, "USERS", "USERNAME TEXT PRIMARY KEY NOT NULL, PASSWORD_HASH TEXT NOT NULL");
	}

	bool CreateFileTable(void) {
		return sqlWrapper.CreateDatabaseTable(FileDatabaseName, "FILES", "CHECKSUM TEXT PRIMARY KEY NOT NULL, NAME TEXT, TITLE TEXT, DESCRIPTION TEXT, UPLOADER TEXT, FILESIZE INT, UPLOADTIME TEXT, TYPEID INT, SUBTYPEID INT");
	}

	inline std::string CreateUserEntry(std::string username, std::string passwordHash) { return "'" + username + "', '" + passwordHash + "'"; }
	inline std::string CreateFileEntry(std::string checksum, std::string fileName, std::string fileTitle, std::string fileDesc, std::string uploader, uint64_t fileSize, std::string uploadTime, int typeID, int subTypeID)
	{
		std::string entry = "";
		entry += "'" + checksum + "', ";
		entry += "'" + fileName + "', ";
		entry += "'" + fileTitle + "', ";
		entry += "'" + fileDesc + "', ";
		entry += "'" + uploader + "', ";
		entry += std::to_string(fileSize) + ", ";
		entry += "'" + uploadTime + "', ";
		entry += std::to_string(typeID) + ", ";
		entry += std::to_string(subTypeID);
		return entry;
	}

	bool CheckIfUserExists(std::string username)
	{
		std::transform(username.begin(), username.end(), username.begin(), ::tolower);
		if (username.empty()) return false;

		//  Grab the user database entry if it exists
		SQLSelectData selectData;
		sqlWrapper.SelectFromTable(UserDatabaseName, "*", "USERS", "WHERE USERNAME = '" + username + "'", selectData, "USERNAME");

		return (selectData.size() != 0);
	}

	bool CheckUserPassword(std::string username, std::string passHash)
	{
		std::transform(username.begin(), username.end(), username.begin(), ::tolower);
		if (username.empty()) return false;
		if (passHash.empty()) return false;

		//  Grab the user database entry if it exists
		SQLSelectData selectData;
		sqlWrapper.SelectFromTable(UserDatabaseName, "*", "USERS", "WHERE USERNAME = '" + username + "'", selectData, "USERNAME");

		if (selectData.size() != 1) return false;

		auto userDataEntry = selectData.find(username);
		if (userDataEntry == selectData.end()) return false;

		auto passHashEntry = (*userDataEntry).second.find("PASSWORD_HASH");
		if (passHashEntry == (*userDataEntry).second.end()) return false;

		return ((*passHashEntry).second == passHash);
	}

	bool SetUserPassword(std::string username, std::string passHash)
	{
		std::transform(username.begin(), username.end(), username.begin(), ::tolower);
		if (username.empty()) return false;
		if (passHash.empty()) return false;

		//  Update the user's password hash
		return sqlWrapper.UpdateInTable(UserDatabaseName, "USERS", "PASSWORD_HASH = '" + passHash + "'", "WHERE USERNAME = '" + username + "'");
	}

	bool RemoveUser(std::string username)
	{
		std::transform(username.begin(), username.end(), username.begin(), ::tolower);
		if (username.empty()) return false;

		return sqlWrapper.DeleteInTable(UserDatabaseName, "USERS", "WHERE USERNAME = '" + username + "'");
	}

	bool RegisterUser(std::string username, std::string passHash)
	{
		std::transform(username.begin(), username.end(), username.begin(), ::tolower);
		if (username.empty()) return false;
		if (passHash.empty()) return false;

		return sqlWrapper.InsertIntoTable(UserDatabaseName, "USERS", "USERNAME, PASSWORD_HASH", CreateUserEntry(username, passHash));
	}

	bool AddFileData(HostedFileData& hfd)
	{
		auto columnList = "CHECKSUM, NAME, TITLE, DESCRIPTION, UPLOADER, FILESIZE, UPLOADTIME, TYPEID, SUBTYPEID";
		auto entries = CreateFileEntry(hfd.FileTitleChecksum, hfd.getFileNameString(), hfd.getFileTitleString(), hfd.getFileDescString(), hfd.getFileUploaderString(), hfd.FileSize, hfd.FileUploadTime, hfd.FileType, hfd.FileSubType);
		
		return sqlWrapper.InsertIntoTable(FileDatabaseName, "FILES", columnList, entries);
	}

	bool CheckIfFileExists(std::string checksum)
	{
		if (checksum.empty()) return false;

		//  Grab the user database entry if it exists
		SQLSelectData selectData;
		sqlWrapper.SelectFromTable(FileDatabaseName, "*", "FILES", "WHERE CHECKSUM = '" + checksum + "'", selectData, "CHECKSUM");

		return (selectData.size() != 0);
	}

	bool GetFileData(std::string checksum, HostedFileData& out)
	{
		//  Grab the user database entry if it exists
		SQLSelectData selectData;
		sqlWrapper.SelectFromTable(FileDatabaseName, "*", "FILES", "WHERE CHECKSUM = '" + checksum + "'", selectData, "CHECKSUM");

		if (selectData.size() != 1) return false;

		auto dataEntry = selectData.find(checksum);
		if (dataEntry == selectData.end()) return false;

		out.FileTitleChecksum = (*(*dataEntry).second.find("CHECKSUM")).second;
		out.LoadFileNameFromString((*(*dataEntry).second.find("NAME")).second);
		out.LoadFileTitleFromString((*(*dataEntry).second.find("TITLE")).second);
		out.LoadFileDescFromString((*(*dataEntry).second.find("DESCRIPTION")).second);
		out.LoadUploaderFromString((*(*dataEntry).second.find("UPLOADER")).second);
		out.FileSize = atoll((*(*dataEntry).second.find("FILESIZE")).second.c_str());
		out.FileUploadTime = (*(*dataEntry).second.find("UPLOADTIME")).second;
		out.FileType = HostedFileType(atoi((*(*dataEntry).second.find("TYPEID")).second.c_str()));
		out.FileSubType = HostedFileSubtype(atoi((*(*dataEntry).second.find("SUBTYPEID")).second.c_str()));

		return true;
	}

	bool RemoveFile(std::string fileChecksum)
	{
		return sqlWrapper.DeleteInTable(FileDatabaseName, "FILES", "WHERE CHECKSUM = '" + fileChecksum + "'");
	}

	bool GetHostedFileList(std::list<HostedFileData>& outList, int startIndex, int count)
	{	
		//  Grab the user database entry if it exists
		SQLSelectData selectData;
		if (sqlWrapper.SelectFromTable(FileDatabaseName, "*", "FILES", "ORDER BY UPLOADTIME ASC LIMIT " + std::to_string(count) + " OFFSET " + std::to_string(startIndex), selectData, "CHECKSUM") == false) return false;

		outList.clear();
		if (selectData.size() == 0) return true;
		for (auto dataEntry : selectData)
		{
			HostedFileData fileData;
			fileData.FileTitleChecksum = (*dataEntry.second.find("CHECKSUM")).second;
			fileData.LoadFileNameFromString((*dataEntry.second.find("NAME")).second);
			fileData.LoadFileTitleFromString((*dataEntry.second.find("TITLE")).second);
			fileData.LoadFileDescFromString((*dataEntry.second.find("DESCRIPTION")).second);
			fileData.LoadUploaderFromString((*dataEntry.second.find("UPLOADER")).second);
			fileData.FileSize = atoll((*dataEntry.second.find("FILESIZE")).second.c_str());
			fileData.FileUploadTime = (*dataEntry.second.find("UPLOADTIME")).second;
			fileData.FileType = HostedFileType(atoi((*dataEntry.second.find("TYPEID")).second.c_str()));
			fileData.FileSubType = HostedFileSubtype(atoi((*dataEntry.second.find("SUBTYPEID")).second.c_str()));
			outList.push_back(fileData);
		}

		return true;
	}
};