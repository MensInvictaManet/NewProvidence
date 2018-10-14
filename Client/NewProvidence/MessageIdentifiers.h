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
	MESSAGE_ID_LATEST_UPLOADS_LIST				= 6,	// The list of the latest uploads on the server (server to client)
	MESSAGE_ID_FILE_REQUEST						= 7,	// File Request (client to server)
	MESSAGE_ID_FILE_REQUEST_FAILED				= 8,	// File Request Failure message (server to client)
	MESSAGE_ID_FILE_SEND_INIT					= 9,	// File Send Initializer (two-way)
	MESSAGE_ID_FILE_RECEIVE_READY				= 10,	// File Receive Ready (two-way)
	MESSAGE_ID_FILE_PORTION						= 11,	// File Portion Send (two-way)
	MESSAGE_ID_FILE_PORTION_COMPLETE			= 12,	// File Portion Complete Check (two-way)
	MESSAGE_ID_FILE_CHUNKS_REMAINING			= 13,	// File Chunks Remaining (two-way)
	MESSAGE_ID_FILE_PORTION_COMPLETE_CONFIRM	= 14,	// File Portion Complete Confirm (two-way)
};