#pragma once

#include "Engine/SQLWrapper.h"

constexpr auto UserDatabaseName = "UserDatabase";

sqlite3* userDatabase;
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
	inline std::string CreateUserEntry(std::string username, std::string passwordHash) { return "(\"" + username + "\", \"" + passwordHash + "\")"; }

	inline bool OpenUserDatabaseConnection(void)	{ return sqlWrapper.OpenDatabase(UserDatabaseName); }
	inline void CloseUserDatabaseConnection(void)	{ sqlWrapper.CloseDatabase(UserDatabaseName); }

	bool CheckIfUserExists(std::string username)
	{
		std::transform(username.begin(), username.end(), username.begin(), ::tolower);
		if (username.empty()) return false;

		//  Grab the user database entry if it exists
		SQLSelectData selectData;
		sqlWrapper.SelectFromTable(UserDatabaseName, "*", "USERS", "WHERE USERNAME = '" + username + "'", selectData);

		if (selectData.size() != 1) return false;
		return (selectData.find(username) != selectData.end());
	}

	bool CheckUserPassword(std::string username, std::string passHash)
	{
		std::transform(username.begin(), username.end(), username.begin(), ::tolower);
		if (username.empty()) return false;
		if (passHash.empty()) return false;

		//  Grab the user database entry if it exists
		SQLSelectData selectData;
		sqlWrapper.SelectFromTable(UserDatabaseName, "*", "USERS", "WHERE USERNAME = '" + username + "'", selectData);

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
};