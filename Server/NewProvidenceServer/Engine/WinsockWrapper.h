#pragma once

#include "Socket.h"
#include "SocketBuffer.h"
#include "SimpleMD5.h"

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
	static bool GetInternetConnected();
	static unsigned int ConvertIPtoUINT(const char* ipAddress);
	static std::string ConvertUINTtoIP(unsigned int ipAddress);

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
	int ReceiveMessagePacket(int socketID, int len, int bufferID, int length_specific = 0);
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
	float ReadFloat(int bufferID, bool peek = false);
	double ReadDouble(int bufferID, bool peek = false);
	char* ReadString(int bufferID, bool peek = false);

	// Buffer Information
	int GetBufferPosition(bool readWrite, int bufferID);
	int ClearBuffer(int bufferID);
	int GetBufferSize(int bufferID);
	int SetBufferPosition(int pos, int bufferID);
	int GetBytesLeft(int bufferID);
	int CreateBuffer();
	bool FreeBuffer(int bufferID);
	bool CopyBuffer(int destinationID, int sourceID);
	bool CopyBuffer(int destinationID, int start, int len, int sourceID);
	const char* GetMacAddress() const;
	static const char* GetStringMD5(char* str);
	const char* GetBufferMD5(int bufferID) const;
	bool EncryptBuffer(char* pass, int bufferID);
	unsigned int GetBufferAdler32(int bufferID);
	bool GetBufferExists(int bufferID);

	// File Read/Write
	int FileOpen(char* name, int mode);
	int FileClose(int fileID);
	int FileWrite(int fileID, int bufferID);
	int FileRead(int fileID, int bytes, int bufferID);
	int FileGetPosition(int fileID);
	int FileSetPosition(int fileID, int pos);
	int FileGetSize(int fileID);

	int AddBuffer(SocketBuffer* b);
	int AddSocket(Socket* b);
	int AddFile(HANDLE b);

private:
	static HANDLE BinaryOpenFile(char* filename, int mode);
	static bool BinaryCloseFile(HANDLE hwnd);
	static int BinaryFileWrite(HANDLE hwnd, SocketBuffer* dataBuffer);
	static int BinaryFileRead(HANDLE hwnd, int size, SocketBuffer* out);
	static int BinaryGetPosition(HANDLE hwnd);
	static int BinarySetPosition(HANDLE hwnd, int offset);
	static int BinaryGetFileSize(HANDLE hwnd);

	WinsockWrapper();
	~WinsockWrapper();

	std::vector<SocketBuffer*> m_BufferList;
	std::vector<Socket*> m_SocketList;
	std::vector<HANDLE>  m_FileList;
	bool m_WinsockInitialized;
};

inline bool WinsockWrapper::GetInternetConnected()
{
	DWORD cstat;
	return InternetGetConnectedState(&cstat, 0) != false;
}

inline unsigned int WinsockWrapper::ConvertIPtoUINT(const char* ipAddress)
{
	struct sockaddr_in sa;
	inet_pton(AF_INET, ipAddress, &(sa.sin_addr));
	return sa.sin_addr.S_un.S_addr;
}

inline std::string WinsockWrapper::ConvertUINTtoIP(unsigned int ipAddress)
{
	char outputString[32];
	inet_ntop(AF_INET, &ipAddress, outputString, INET_ADDRSTRLEN);
	return std::string(outputString);
}

inline void WinsockWrapper::WinsockInitialize(unsigned int bufferCount)
{
	//  If we're already initialized, exit gracefully
	if (m_WinsockInitialized) return;

	//  Start up the Winsock library, requesting version 2.2
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

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
	for (unsigned int i = 0; i < m_FileList.size(); ++i) BinaryCloseFile(m_FileList[i]);

	m_BufferList.clear();
	m_SocketList.clear();
	m_FileList.clear();
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

inline int WinsockWrapper::ReceiveMessagePacket(int socketID, int len, int bufferID, int length_specific)
{
	auto socket = m_SocketList[socketID];
	auto buffer = m_BufferList[bufferID];
	if (socket == nullptr) return -1;
	if (buffer == nullptr) return -2;
	auto size = socket->receivemessage(len, buffer, length_specific);
	if (size < 0)
	{
		auto error = socket->lasterror();
		if (error == 10054) return 0;
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

inline int WinsockWrapper::CreateBuffer()
{
	MANAGE_MEMORY_NEW("WinsockWrapper", sizeof(SocketBuffer));
	auto buffer = new SocketBuffer;
	return AddBuffer(buffer);
}

inline bool WinsockWrapper::FreeBuffer(int bufferID)
{
	if (bufferID == 0) return false;
	auto buff = m_BufferList[bufferID];
	if (buff == nullptr) return false;
	MANAGE_MEMORY_DELETE("WinsockWrapper", sizeof(SocketBuffer));
	delete buff;
	m_BufferList[bufferID] = nullptr;
	return true;
}

inline bool WinsockWrapper::CopyBuffer(int destinationID, int sourceID)
{
	auto destination = m_BufferList[destinationID];
	if (destination == nullptr) return false;
	auto source = m_BufferList[sourceID];
	if (source == nullptr) return false;
	destination->addBuffer(source);
	return true;
}

inline bool WinsockWrapper::CopyBuffer(int destinationID, int start, int len, int sourceID)
{
	auto destination = m_BufferList[destinationID];
	if (destination == nullptr) return false;
	auto source = m_BufferList[sourceID];
	if (source == nullptr) return false;
	destination->addBuffer(source->m_BufferData + start, len);
	return true;
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

inline const char* WinsockWrapper::GetStringMD5(char* str)
{
	return MD5(str).hexdigest().c_str();
}

inline const char* WinsockWrapper::GetBufferMD5(int bufferID) const
{
	auto buffer = m_BufferList[bufferID];
	return MD5(buffer->m_BufferData).hexdigest().c_str();
}

inline bool WinsockWrapper::EncryptBuffer(char* pass, int bufferID)
{
	auto buffer = m_BufferList[bufferID];
	if (buffer == nullptr) return false;


	auto *inp = buffer->m_BufferData;
	unsigned int inplen = buffer->m_BufferUtilizedCount;
	char KeyBox[257];
	auto keylen = std::min<int>(int(strlen(pass)), 256);
	if (keylen <= 0) return false;
	ulong i, j, t, x;
	char temp;
	j = t = 0;
	for (i = 0; i < 256; i++)
		KeyBox[i] = char(i);
	for (i = 0; i < 256; i++)
	{
		j = (j + ulong(KeyBox[i]) + pass[i % keylen]) % 256;
		temp = KeyBox[i];
		KeyBox[i] = KeyBox[j];
		KeyBox[j] = temp;
	}
	i = j = 0;
	for (x = 0; x < inplen; x++)
	{
		i = (i + 1U) % 256;
		j = (j + ulong(KeyBox[i])) % 256;
		temp = KeyBox[i];
		KeyBox[i] = KeyBox[j];
		KeyBox[j] = temp;
		t = (ulong(KeyBox[i]) + ulong(KeyBox[j])) % 256;
		inp[x] = (inp[x] ^ KeyBox[t]);
	}

	return true;
}

inline unsigned int WinsockWrapper::GetBufferAdler32(int bufferID)
{
	auto buffer = m_BufferList[bufferID];
	if (buffer == nullptr) return 0;

	auto bufferData = buffer->m_BufferData;
	unsigned int len = buffer->m_BufferUtilizedCount;
	unsigned int a = 1, b = 0;
	while (len) {
		auto tlen = len > 5550 ? 5550 : len;
		len -= tlen;
		do {
			a += *bufferData++;
			b += a;
		} while (--tlen);
		a = (a & 0xffff) + (a >> 16) * (65536 - 65521);
		b = (b & 0xffff) + (b >> 16) * (65536 - 65521);
	}
	if (a >= 65521)
		a -= 65521;
	b = (b & 0xffff) + (b >> 16) * (65536 - 65521);
	if (b >= 65521)
		b -= 65521;
	return b << 16 | a;
}

inline bool WinsockWrapper::GetBufferExists(int bufferID)
{
	auto buffer = m_BufferList[bufferID];
	return (buffer != nullptr);
}

inline int WinsockWrapper::FileOpen(char* name, int mode)
{
	auto file = BinaryOpenFile(name, mode);
	return ((file != nullptr) ? AddFile(file) : -1);
}

inline int WinsockWrapper::FileClose(int fileID)
{
	auto file = m_FileList[fileID];
	if (file == nullptr) return -1;
	m_FileList[fileID] = nullptr;
	return BinaryCloseFile(file);
}

inline int WinsockWrapper::FileWrite(int fileID, int bufferID)
{
	auto file = m_FileList[fileID];
	if (file == nullptr) return -1;
	auto buffer = m_BufferList[bufferID];
	if (buffer == nullptr) return -1;
	return BinaryFileWrite(file, buffer);
}

inline int WinsockWrapper::FileRead(int fileID, int bytes, int bufferID)
{
	auto file = m_FileList[fileID];
	if (file == nullptr) return -1;
	auto buffer = m_BufferList[bufferID];
	return ((buffer == nullptr) ? -1 : BinaryFileRead(file, bytes, buffer));
}

inline int WinsockWrapper::FileGetPosition(int fileID)
{
	auto file = m_FileList[fileID];
	return ((file == nullptr) ? -1 : BinaryGetPosition(file));
}

inline int WinsockWrapper::FileSetPosition(int fileID, int pos)
{
	auto file = m_FileList[fileID];
	return ((file == nullptr) ? -1 : BinarySetPosition(file, pos));
}

inline int WinsockWrapper::FileGetSize(int fileID)
{
	auto file = m_FileList[fileID];
	return ((file == nullptr) ? -1 : BinaryGetFileSize(file));
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

inline int WinsockWrapper::AddFile(HANDLE b)
{
	for (auto i = 0; i < int(m_FileList.size()); i++)
		if (m_FileList[i] == nullptr)
		{
			m_FileList[i] = b;
			return i;
		}

	m_FileList.push_back(b);
	return int(m_FileList.size()) - 1;
}

inline HANDLE WinsockWrapper::BinaryOpenFile(char* filename, int mode)
{
	DWORD access;
	access = GENERIC_READ | GENERIC_WRITE;
	if (mode == 0) access = GENERIC_READ;
	if (mode == 1) access = GENERIC_WRITE;
	return CreateFileA(filename, access, FILE_SHARE_READ,
		nullptr,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		nullptr);
}

inline bool WinsockWrapper::BinaryCloseFile(HANDLE hwnd)
{
	if (hwnd == nullptr) return false;
	return (CloseHandle(hwnd) != 0);
}

inline int WinsockWrapper::BinaryFileWrite(HANDLE hwnd, SocketBuffer* dataBuffer)
{
	DWORD bytes_written;
	WriteFile(hwnd, dataBuffer->m_BufferData + dataBuffer->m_ReadPosition, dataBuffer->m_BufferUtilizedCount - dataBuffer->m_ReadPosition, &bytes_written, nullptr);
	return int(bytes_written);
}

inline int WinsockWrapper::BinaryFileRead(HANDLE hwnd, int size, SocketBuffer* out)
{
	DWORD bytes_read;
	MANAGE_MEMORY_NEW("WinsockWrapper", size);
	auto b = new char[size];
	ReadFile(hwnd, b, size, &bytes_read, nullptr);
	out->StreamWrite(b, bytes_read);
	MANAGE_MEMORY_DELETE("WinsockWrapper", size);
	delete [] b;
	return int(bytes_read);
}

inline int WinsockWrapper::BinaryGetPosition(HANDLE hwnd)
{
	return SetFilePointer(hwnd, 0, nullptr, FILE_CURRENT);
}

inline int WinsockWrapper::BinarySetPosition(HANDLE hwnd, int offset)
{
	return SetFilePointer(hwnd, offset, nullptr, FILE_BEGIN);
}

inline int WinsockWrapper::BinaryGetFileSize(HANDLE hwnd)
{
	return GetFileSize(hwnd, nullptr);
}

inline WinsockWrapper::WinsockWrapper() :
	m_WinsockInitialized(false)
{

}

inline WinsockWrapper::~WinsockWrapper()
{

}

//  Instance to be utilized by anyone including this header
WinsockWrapper& winsockWrapper = WinsockWrapper::GetInstance();