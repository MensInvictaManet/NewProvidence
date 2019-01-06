#pragma once

//  Incoming message type IDs (enum list syncronous with both client and server)
enum MessageIDs
{
	MESSAGE_ID_PING_REQUEST						= 0,	// Ping Request (server to client)
	MESSAGE_ID_PING_RESPONSE					= 1,	// Ping Return (client to server)
	MESSAGE_ID_ENCRYPTED_CHAT_STRING			= 2,	// Chat String [encrypted] (two-way)
	MESSAGE_ID_USER_LOGIN_REQUEST				= 3,	// User Login Request (client to server)
	MESSAGE_ID_USER_LOGIN_RESPONSE				= 4,	// User Login Response (server to client)
	MESSAGE_ID_USER_INBOX_AND_NOTIFICATIONS		= 5,	// User's inbox and notification data (server to client)
	MESSAGE_ID_REQUEST_HOSTED_FILE_LIST			= 6,	// The request for the list of the latest uploads on the server (client to server)
	MESSAGE_ID_HOSTED_FILE_LIST					= 7,	// The list of the latest uploads on the server (server to client)
	MESSAGE_ID_FILE_REQUEST						= 8,	// File Request (client to server)
	MESSAGE_ID_FILE_REQUEST_FAILED				= 9,	// File Request Failure message (server to client)
	MESSAGE_ID_FILE_SEND_INIT					= 10,	// File Send Initializer (two-way)
	MESSAGE_ID_FILE_SEND_FAILED					= 11,	// File Send Failuse message (server to client)
	MESSAGE_ID_FILE_RECEIVE_READY				= 12,	// File Receive Ready (two-way)
	MESSAGE_ID_FILE_PORTION						= 13,	// File Portion Send (two-way)
	MESSAGE_ID_FILE_PORTION_COMPLETE			= 14,	// File Portion Complete Check (two-way)
	MESSAGE_ID_FILE_CHUNKS_REMAINING			= 15,	// File Chunks Remaining (two-way)
	MESSAGE_ID_FILE_PORTION_COMPLETE_CONFIRM	= 16,	// File Portion Complete Confirm (two-way)
};

//  Login Response Identifiers
enum LoginResponseIdentifiers
{
	LOGIN_RESPONSE_SUCCESS = 0,
	LOGIN_RESPONSE_USERNAME_DOES_NOT_EXIST = 1,
	LOGIN_RESPONSE_PASSWORD_INCORRECT = 2,
	LOGIN_RESPONSE_VERSION_NUMBER_INCORRECT = 3,
	LOGIN_RESPONSE_USER_ALREADY_LOGGED_IN = 4
};

//  Login Response Strings
std::string LoginResponses[] =
{
	"Successfully logged in to server!",
	"Failed to log in to server. Username did not exist. Try again.",
	"Failed to log in to server. Password was incorrect. Try again.",
	"Failed to log in to server. Client version was incorrect. Contact admin."
	"Failed to log in to server. User is already logged in. Try again.",
};