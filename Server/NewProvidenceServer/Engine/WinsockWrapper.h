#pragma once

#include "Socket.h"
#include "SocketBuffer.h"

#include <windows.h>
#include <Wininet.h>
#include <Iphlpapi.h>
#include <vector>
#include <minwindef.h>
#include <assert.h>


class WinsockWrapper
{
public:
	typedef unsigned long ulong;

	static WinsockWrapper& GetInstance() { static WinsockWrapper INSTANCE; return INSTANCE; }

	//  Utilities
	static inline bool GetInternetConnected() { DWORD cstat; return (InternetGetConnectedState(&cstat, 0) != false); }
	static unsigned int ConvertIPtoUINT(const char* ipAddress) { sockaddr_in sa; inet_pton(AF_INET, ipAddress, &(sa.sin_addr)); return sa.sin_addr.S_un.S_addr; }
	static std::string ConvertUINTtoIP(unsigned int ipAddress) { char outputString[32]; inet_ntop(AF_INET, &ipAddress, outputString, INET_ADDRSTRLEN); return std::string(outputString); }

	void WinsockInitialize(unsigned int bufferCount = 1);
	void WinsockShutdown();

	//  Connections
	int TCPConnect(const char* ipAddress, int port, int mode);
	int TCPListen(int port, int maxConnections, int mode);
	int TCPAccept(int socketID, int mode);
	bool TCPConnected(int socketID);
	int UDPConnect(int port, int mode);
	bool SetNagle(int socketID, bool value);

	//  Miscelaneous
	int SendMessagePacket(int socketID, const char* ipAddress, int port, int bufferID);
	int ReceiveMessagePacket(int socketID, int bufferID);
	int PeekMessagePacket(int socketID, int len, int bufferID);
	int SetFormat(int socketID, int mode, char* separater);
	int SetSync(int socketID, int mode);
	bool CloseSocket(int socketID);
	int GetLastSocketError(int socketID);
	static char* GetMyHostName();
	static bool CompareIP(char* ipAddress, char* mask);
	static void SocketStart();
	static void SocketExit();
	static std::string GetMyHostIP(char* hostName);
	int GetSocketID(int socketID);

	//  IP Information
	std::string GetExteriorIP(int socketID);
	static char* GetLastInIP();
	static double GetLastInPort();

	// Buffer Write
	int WriteChar(unsigned char val, int bufferID);
	int WriteChars(unsigned char* val, int length, int bufferID);
	int WriteShort(short val, int bufferID);
	int WriteUnsignedShort(unsigned short val, int bufferID);
	int WriteInt(int val, int bufferID);
	int WriteUnsignedInt(unsigned int val, int bufferID);
	int WriteLongInt(uint64_t val, int bufferID);
	int WriteFloat(float val, int bufferID);
	int WriteDouble(double val, int bufferID);
	int WriteString(char* val, int bufferID);
	int WriteString(const char* val, int bufferID);

	// Buffer Read
	unsigned char ReadChar(int bufferID, bool peek = false);
	unsigned char* ReadChars(int bufferID, int length, bool peek = false);
	short ReadShort(int bufferID, bool peek = false);
	unsigned short ReadUnsignedShort(int bufferID, bool peek = false);
	int ReadInt(int bufferID, bool peek = false);
	unsigned int ReadUnsignedInt(int bufferID, bool peek = false);
	uint64_t ReadLongInt(int bufferID, bool peek = false);
	float ReadFloat(int bufferID, bool peek = false);
	double ReadDouble(int bufferID, bool peek = false);
	char* ReadString(int bufferID, bool peek = false);

	// Buffer Information
	int GetBufferPosition(bool readWrite, int bufferID);
	int ClearBuffer(int bufferID);
	int GetBufferSize(int bufferID);
	int SetBufferPosition(int pos, int bufferID);
	int GetBytesLeft(int bufferID);
	const char* GetMacAddress() const;
	bool GetBufferExists(int bufferID);

	int AddBuffer(SocketBuffer* b);
	int AddSocket(Socket* b);

private:
	WinsockWrapper();
	~WinsockWrapper() {}

	std::vector<SocketBuffer*> m_BufferList;
	std::vector<Socket*> m_SocketList;
	bool m_WinsockInitialized;
};

inline void WinsockWrapper::WinsockInitialize(unsigned int bufferCount)
{
	//  If we're already initialized, exit gracefully
	if (m_WinsockInitialized) return;

	//  Start up the Winsock library, requesting version 2.2
	WSADATA wsaData;
	(void)WSAStartup(MAKEWORD(2, 2), &wsaData);

	for (unsigned int i = 0; i < bufferCount; ++i)
	{
		MANAGE_MEMORY_NEW("WinsockWrapper", sizeof(SocketBuffer));
		AddBuffer(new SocketBuffer);
	}

	//  Set the flag to ensure we don't double-initialize Winsock
	m_WinsockInitialized = true;
}

inline void WinsockWrapper::WinsockShutdown()
{
	Socket::SockExit();

	while (!m_BufferList.empty())
	{
		MANAGE_MEMORY_DELETE("WinsockWrapper", sizeof(SocketBuffer));
		delete (*m_BufferList.begin());
		m_BufferList.erase(m_BufferList.begin());
	}
	while (!m_BufferList.empty())
	{
		MANAGE_MEMORY_DELETE("WinsockWrapper", sizeof(Socket));
		delete (*m_SocketList.begin());
		m_SocketList.erase(m_SocketList.begin());
	}

	m_BufferList.clear();
	m_SocketList.clear();
}

inline int WinsockWrapper::TCPConnect(const char* ipAddress, int port, int mode)
{
	MANAGE_MEMORY_NEW("WinsockWrapper", sizeof(Socket));
	auto socket = new Socket;
	if (socket->tcpconnect(ipAddress, port, mode)) return AddSocket(socket);

	MANAGE_MEMORY_DELETE("WinsockWrapper", sizeof(Socket));
	delete socket;
	return -1;
}

inline int WinsockWrapper::TCPListen(int port, int maxConnections, int mode)
{
	MANAGE_MEMORY_NEW("WinsockWrapper", sizeof(Socket));
	auto socket = new Socket;
	if (socket->tcplisten(port, maxConnections, mode)) return AddSocket(socket);

	MANAGE_MEMORY_DELETE("WinsockWrapper", sizeof(Socket));
	delete socket;
	return -1;
}

inline int WinsockWrapper::TCPAccept(int socketID, int mode)
{
	auto socket1 = m_SocketList[socketID];
	if (socket1 == nullptr)	return -1;

	auto socket2 = socket1->tcpaccept(mode);
	return ((socket2 != nullptr) ? AddSocket(socket2) : -1);
}

inline bool WinsockWrapper::TCPConnected(int socketID)
{
	auto socket = m_SocketList[socketID];
	return ((socket == nullptr) ? true : false);
}

inline int WinsockWrapper::UDPConnect(int port, int mode)
{
	auto socket = new Socket();
	if (socket->udpconnect(port, mode)) return AddSocket(socket);

	MANAGE_MEMORY_DELETE("WinsockWrapper", sizeof(Socket));
	delete socket;
	return -1;
}

inline bool WinsockWrapper::SetNagle(int socketID, bool value)
{
	auto socket = m_SocketList[socketID];
	if (socket == nullptr)	return false;
	if (value)	socket->setnagle(true);
	else		socket->setnagle(false);
	return true;
}

inline int WinsockWrapper::SendMessagePacket(int socketID, const char* ipAddress, int port, int bufferID)
{
	auto socket = m_SocketList[socketID];
	auto buffer = m_BufferList[bufferID];
	if (socket == nullptr) return -1;
	if (buffer == nullptr) return -2;
	auto size = socket->sendmessage(ipAddress, port, buffer);
	if (size < 0) return -socket->lasterror();
	return size;
}

inline int WinsockWrapper::ReceiveMessagePacket(int socketID, int bufferID)
{
	auto socket = m_SocketList[socketID];
	auto buffer = m_BufferList[bufferID];
	if (socket == nullptr) return -1;
	if (buffer == nullptr) return -2;
	auto size = socket->receivemessage(buffer);

	if (size < 0)
	{
		auto error = socket->lasterror();
		if (error == 0)		return -1;
		if (error == 10054)	return 0;
		return -error;
	}
	return size;
}

inline int WinsockWrapper::PeekMessagePacket(int socketID, int len, int bufferID)
{
	auto socket = m_SocketList[socketID];
	auto buffer = m_BufferList[bufferID];
	if (socket == nullptr) return -1;
	if (buffer == nullptr) return -2;
	auto size = socket->peekmessage(len, buffer);
	if (size < 0)
	{
		auto error = socket->lasterror();
		return ((error == 10054) ? 0 : -error);
	}
	return size;
}

inline int WinsockWrapper::SetFormat(int socketID, int mode, char* separater)
{
	auto socket = m_SocketList[socketID];
	return ((socket == nullptr) ? -1 : socket->SetFormat(mode, separater));
}

inline int WinsockWrapper::SetSync(int socketID, int mode)
{
	if (socketID < 0) return -1;
	auto socket = m_SocketList[socketID];
	if (socket == nullptr) return -1;
	socket->setsync(mode);
	return 1;
}

inline bool WinsockWrapper::CloseSocket(int socketID)
{
	if (socketID < 0) return false;
	auto socket = m_SocketList[socketID];
	if (socket == nullptr) return false;
	MANAGE_MEMORY_DELETE("WinsockWrapper", sizeof(Socket));
	delete socket;
	m_SocketList[socketID] = nullptr;
	return true;
}

inline int WinsockWrapper::GetLastSocketError(int socketID)
{
	auto socket = m_SocketList[socketID];
	return ((socket == nullptr) ? -1 : -socket->lasterror());
}

inline char* WinsockWrapper::GetMyHostName()
{
	return Socket::myhost();
}

inline bool WinsockWrapper::CompareIP(char* ipAddress, char* mask)
{
	char* cp = nullptr;
	char* mp = nullptr;
	while ((*ipAddress) && (*mask != '*')) {
		if ((*mask != *ipAddress) && (*mask != '?')) {
			return false;
		}
		mask++;
		ipAddress++;
	}
	while (*ipAddress) {
		if (*mask == '*') {
			if (!*++mask) {
				return true;
			}
			mp = mask;
			cp = ipAddress + 1;
		}
		else if ((*mask == *ipAddress) || (*mask == '?')) {
			mask++;
			ipAddress++;
		}
		else {
			mask = mp;
			ipAddress = cp++;
		}
	}
	while (*mask == '*') {
		mask++;
	}
	return (!*mask);
}

inline void WinsockWrapper::SocketStart()
{
	Socket::SockStart();
}

inline void WinsockWrapper::SocketExit()
{
	Socket::SockExit();
}

inline std::string WinsockWrapper::GetMyHostIP(char* hostName)
{
	return Socket::GetHostIP(hostName);
}

inline int WinsockWrapper::GetSocketID(int socketID)
{
	auto socket = m_SocketList[socketID];
	return ((socket == nullptr) ? -1 : int(socket->m_SocketID));
}

inline std::string WinsockWrapper::GetExteriorIP(int socketID)
{
	auto socket = m_SocketList[socketID];
	return socket->tcpip();
}

inline char* WinsockWrapper::GetLastInIP()
{
	return Socket::lastinIP();
}

inline double WinsockWrapper::GetLastInPort()
{
	return Socket::lastinPort();
}

inline int WinsockWrapper::WriteChar(unsigned char val, int bufferID)
{
	auto buffer = m_BufferList[bufferID];
	return ((buffer == nullptr) ? 0 : buffer->writechar(val));
}

inline int WinsockWrapper::WriteChars(unsigned char* val, int length, int bufferID)
{
	auto buffer = m_BufferList[bufferID];
	if (buffer == nullptr) return 0;

	return ((buffer == nullptr) ? 0 : buffer->writechars((char*)val, length));
}

inline int WinsockWrapper::WriteShort(short val, int bufferID)
{
	auto buffer = m_BufferList[bufferID];
	return ((buffer == nullptr) ? 0 : buffer->writeshort(val));
}

inline int WinsockWrapper::WriteUnsignedShort(unsigned short val, int bufferID)
{
	auto buffer = m_BufferList[bufferID];
	return ((buffer == nullptr) ? 0 : buffer->writeushort(val));
}

inline int WinsockWrapper::WriteInt(int val, int bufferID)
{
	auto buffer = m_BufferList[bufferID];
	return ((buffer == nullptr) ? 0 : buffer->writeint(val));
}

inline int WinsockWrapper::WriteUnsignedInt(unsigned int val, int bufferID)
{
	auto buffer = m_BufferList[bufferID];
	return ((buffer == nullptr) ? 0 : buffer->writeuint(val));
}

inline int WinsockWrapper::WriteLongInt(uint64_t val, int bufferID)
{
	auto buffer = m_BufferList[bufferID];
	return ((buffer == nullptr) ? 0 : buffer->writelint(val));
}

inline int WinsockWrapper::WriteFloat(float val, int bufferID)
{
	auto buffer = m_BufferList[bufferID];
	return ((buffer == nullptr) ? 0 : buffer->writefloat(val));
}

inline int WinsockWrapper::WriteDouble(double val, int bufferID)
{
	auto buffer = m_BufferList[bufferID];
	return ((buffer == nullptr) ? 0 : buffer->writedouble(val));
}

inline int WinsockWrapper::WriteString(char* val, int bufferID)
{
	auto buffer = m_BufferList[bufferID];
	return ((buffer == nullptr) ? 0 : buffer->writestring(val));
}

inline int WinsockWrapper::WriteString(const char* val, int bufferID)
{
	auto buffer = m_BufferList[bufferID];
	return ((buffer == nullptr) ? 0 : buffer->writestring(val));
}

inline unsigned char WinsockWrapper::ReadChar(int bufferID, bool peek)
{
	auto buffer = m_BufferList[bufferID];
	return ((buffer == nullptr) ? 0 : buffer->readchar(peek));
}

inline unsigned char* WinsockWrapper::ReadChars(int bufferID, int length, bool peek)
{
	auto buffer = m_BufferList[bufferID];
	return ((buffer == nullptr) ? 0 : (unsigned char*)buffer->readchars(length, peek));
}

inline short WinsockWrapper::ReadShort(int bufferID, bool peek)
{
	auto buffer = m_BufferList[bufferID];
	return ((buffer == nullptr) ? 0 : buffer->readshort(peek));
}

inline unsigned short WinsockWrapper::ReadUnsignedShort(int bufferID, bool peek)
{
	auto buffer = m_BufferList[bufferID];
	return ((buffer == nullptr) ? 0 : buffer->readushort(peek));
}

inline int WinsockWrapper::ReadInt(int bufferID, bool peek)
{
	auto buffer = m_BufferList[bufferID];
	return ((buffer == nullptr) ? 0 : buffer->readint(peek));
}

inline unsigned int WinsockWrapper::ReadUnsignedInt(int bufferID, bool peek)
{
	auto buffer = m_BufferList[bufferID];
	return ((buffer == nullptr) ? 0 : buffer->readuint(peek));
}

inline uint64_t WinsockWrapper::ReadLongInt(int bufferID, bool peek)
{
	auto buffer = m_BufferList[bufferID];
	return ((buffer == nullptr) ? 0 : buffer->readlint(peek));
}

inline float WinsockWrapper::ReadFloat(int bufferID, bool peek)
{
	auto buffer = m_BufferList[bufferID];
	return ((buffer == nullptr) ? 0 : buffer->readfloat(peek));
}

inline double WinsockWrapper::ReadDouble(int bufferID, bool peek)
{
	auto buffer = m_BufferList[bufferID];
	return ((buffer == nullptr) ? 0 : buffer->readdouble(peek));
}

inline char* WinsockWrapper::ReadString(int bufferID, bool peek)
{
	auto buffer = m_BufferList[bufferID];
	return ((buffer == nullptr) ? 0 : buffer->readstring(peek));
}

inline int WinsockWrapper::GetBufferPosition(bool readWrite, int bufferID)
{
	auto buffer = m_BufferList[bufferID];
	if (buffer == nullptr) return 0;
	return (readWrite ? buffer->m_ReadPosition : buffer->m_WritePosition);
}

inline int WinsockWrapper::ClearBuffer(int bufferID)
{
	auto buffer = m_BufferList[bufferID];
	if (buffer == nullptr) return 0;
	buffer->clear();
	return 1;
}

inline int WinsockWrapper::GetBufferSize(int bufferID)
{
	auto buffer = m_BufferList[bufferID];
	return ((buffer == nullptr) ? 0 : buffer->m_BufferUtilizedCount);
}

inline int WinsockWrapper::SetBufferPosition(int pos, int bufferID)
{
	auto buffer = m_BufferList[bufferID];
	if (buffer == nullptr) return 0;
	buffer->m_ReadPosition = pos;
	buffer->m_WritePosition = pos;
	return pos;
}

inline int WinsockWrapper::GetBytesLeft(int bufferID)
{
	auto buffer = m_BufferList[bufferID];
	return ((buffer == nullptr) ? 0 : buffer->bytesleft());
}

inline const char* WinsockWrapper::GetMacAddress() const
{
	static char mac_address[32];
	IP_ADAPTER_INFO AdapterInfo[16];
	DWORD dwBufLen = sizeof(AdapterInfo);
	auto dwStatus = GetAdaptersInfo(AdapterInfo, &dwBufLen);
	assert(dwStatus == ERROR_SUCCESS);
	auto pAdapterInfo = AdapterInfo;
	if (!pAdapterInfo) return nullptr;

	sprintf_s(mac_address, 31, "%02X-%02X-%02X-%02X-%02X-%02X", AdapterInfo->Address[0], AdapterInfo->Address[1], AdapterInfo->Address[2], AdapterInfo->Address[3], AdapterInfo->Address[4], AdapterInfo->Address[5]);
	return mac_address;
}

inline bool WinsockWrapper::GetBufferExists(int bufferID)
{
	auto buffer = m_BufferList[bufferID];
	return (buffer != nullptr);
}

inline int WinsockWrapper::AddBuffer(SocketBuffer* b)
{
	for (unsigned int i = 0; i < m_BufferList.size(); i++)
		if (m_BufferList[i] == nullptr)
		{
			m_BufferList[i] = b;
			return i;
		}

	m_BufferList.push_back(b);
	return int(m_BufferList.size()) - 1;
}

inline int WinsockWrapper::AddSocket(Socket* b)
{
	for (auto i = 0; i < int(m_SocketList.size()); i++)
		if (m_SocketList[i] == nullptr)
		{
			m_SocketList[i] = b;
			return i;
		}

	m_SocketList.push_back(b);
	return int(m_SocketList.size()) - 1;
}


inline WinsockWrapper::WinsockWrapper() :
	m_WinsockInitialized(false)
{

}

//  Instance to be utilized by anyone including this header
WinsockWrapper& winsockWrapper = WinsockWrapper::GetInstance();