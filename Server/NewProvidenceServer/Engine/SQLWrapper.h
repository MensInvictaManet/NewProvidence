#pragma once

#include "sqlite3.h"
#include <unordered_map>

static int SQLErrorCallback(void *NotUsed, int argc, char **argv, char **azColName) {
	for (auto i = 0; i < argc; i++) printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	printf("\n");
	return 0;
}

typedef std::unordered_map<std::string, std::unordered_map<std::string, std::string>> SQLSelectData;

class SQLWrapper
{
private:
	SQLWrapper() {}
	~SQLWrapper() { CloseAllDatabases(); }

	inline sqlite3* GetDatabase(std::string dbName) { return (DatabaseMap.find(dbName) != DatabaseMap.end()) ? DatabaseMap[dbName] : nullptr; }
	inline void CloseDatabase(sqlite3* database) { sqlite3_close(database); }
	inline void CloseAllDatabases() { for (auto db : DatabaseMap) CloseDatabase(db.second); }

	std::unordered_map<std::string, sqlite3*> DatabaseMap;

public:

	static SQLWrapper& GetInstance() { static SQLWrapper INSTANCE; return INSTANCE; }

	inline void CloseDatabase(std::string dbName) { if (DatabaseMap.find(dbName) != DatabaseMap.end()) CloseDatabase(DatabaseMap[dbName]); }

	bool OpenDatabase(std::string dbName);
	bool CreateDatabaseTable(std::string dbName, std::string tableName, std::string tableElements);
	bool InsertIntoTable(std::string dbName, std::string tableName, std::string columnList, std::string entries);
	bool UpdateInTable(std::string dbName, std::string tableName, std::string setCommand, std::string filter);
	bool DeleteInTable(std::string dbName, std::string tableName, std::string filter);
	bool SelectFromTable(std::string dbName, std::string selection, std::string tableName, std::string filter, SQLSelectData& selectData, std::string identifier);
};

//  Instance to be utilized by anyone including this header
SQLWrapper& sqlWrapper = SQLWrapper::GetInstance();

bool SQLWrapper::OpenDatabase(std::string dbName)
{
	auto db = GetDatabase(dbName);
	if (db == nullptr) DatabaseMap[dbName] = nullptr;

	auto errorCode = sqlite3_open(dbName.c_str(), &DatabaseMap[dbName]);
	if (errorCode != SQLITE_OK) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(DatabaseMap[dbName]));
		CloseDatabase(DatabaseMap[dbName]);
		return false;
	}
	return true;
}

bool SQLWrapper::CreateDatabaseTable(std::string dbName, std::string tableName, std::string tableElements)
{
	auto commandString = "CREATE TABLE IF NOT EXISTS " + tableName + "(" + tableElements + ");";
	char* errorMessage = nullptr;
	
	auto errorCode = sqlite3_exec(DatabaseMap[dbName], commandString.c_str(), SQLErrorCallback, 0, &errorMessage);
	if (errorCode != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", errorMessage);
		sqlite3_free(errorMessage);
		return false;
	}
	return true;
}

bool SQLWrapper::InsertIntoTable(std::string dbName, std::string tableName, std::string columnList, std::string entries)
{
	auto commandString = "INSERT INTO " + tableName + " (" + columnList + ") VALUES (" + entries + ");";

	char* errorMessage = 0;
	auto errorCode = sqlite3_exec(DatabaseMap[dbName], commandString.c_str(), SQLErrorCallback, 0, &errorMessage);
	if (errorCode != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", errorMessage);
		sqlite3_free(errorMessage);
		return false;
	}
	return true;
}

bool SQLWrapper::UpdateInTable(std::string dbName, std::string tableName, std::string setCommand, std::string filter)
{
	auto commandString = "UPDATE " + tableName + "\nSET " + setCommand + "\n" + filter + ";";

	char* errorMessage = 0;
	auto errorCode = sqlite3_exec(DatabaseMap[dbName], commandString.c_str(), SQLErrorCallback, 0, &errorMessage);
	if (errorCode != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", errorMessage);
		sqlite3_free(errorMessage);
		return false;
	}
	return true;
}


bool SQLWrapper::DeleteInTable(std::string dbName, std::string tableName, std::string filter)
{
	auto commandString = "DELETE FROM " + tableName + "\n" + filter + ";";

	char* errorMessage = 0;
	auto errorCode = sqlite3_exec(DatabaseMap[dbName], commandString.c_str(), SQLErrorCallback, 0, &errorMessage);
	if (errorCode != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", errorMessage);
		sqlite3_free(errorMessage);
		return false;
	}
	return true;
}


bool SQLWrapper::SelectFromTable(std::string dbName, std::string selection, std::string tableName, std::string filter, SQLSelectData& selectData, std::string identifier)
{
	auto commandString = "SELECT " + selection + " FROM " + tableName + " " + filter;

	static SQLSelectData selectDataPassIn;
	selectDataPassIn.clear();
	auto lambda = [](void* identifier, int argc, char **argv, char **azColName) {
		//  Get the username
		auto idColumn = "";
		for (auto i = 0; i < argc; ++i) if (strcmp(azColName[i], ((std::string*)identifier)->c_str()) == 0) { idColumn = argv[i]; break; }
		for (auto i = 0; i < argc; i++) selectDataPassIn[idColumn][azColName[i]] =  argv[i];
		return 0;
	};

	char* errorMessage = 0;
	auto errorCode = sqlite3_exec(DatabaseMap[dbName], commandString.c_str(), lambda, &identifier, &errorMessage);
	if (errorCode != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", errorMessage);
		sqlite3_free(errorMessage);
		return false;
	}
	selectData = selectDataPassIn;
	return true;
}