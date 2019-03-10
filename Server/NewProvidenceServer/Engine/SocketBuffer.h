#pragma once

#include <algorithm>

#define RETURNVAL_BUFFER_SIZE 1024 * 128 // 128KB

class SocketBuffer
{
	static char m_ReturnValueBuffer[RETURNVAL_BUFFER_SIZE + 1];
public:
	char* m_BufferData;
	int m_BufferSize;
	int m_ReadPosition;
	int m_WritePosition;
	int m_BufferUtilizedCount;
	void StreamWrite(void *in, int size);
	void StreamRead(void* out, int size, bool peek);
	SocketBuffer();
	~SocketBuffer();
	int 				writechar(unsigned char a);
	int 				writeshort(short a);
	int 				writeushort(unsigned short a);
	int 				writeint(int a);
	int 				writeuint(unsigned int a);
	int					writelint(uint64_t a);
	int 				writefloat(float a);
	int 				writedouble(double a);
	int 				writechars(char*str);
	int 				writechars(const char*str);
	int 				writechars(char*str, int length);
	int 				writechars(const char*str, int length);
	int 				writestring(char*str);
	int 				writestring(const char*str);

	unsigned char		readchar(bool peek = false);
	short				readshort(bool peek = false);
	unsigned short		readushort(bool peek = false);
	int					readint(bool peek = false);
	unsigned int		readuint(bool peek = false);
	uint64_t			readlint(bool peek = false);
	float				readfloat(bool peek = false);
	double				readdouble(bool peek = false);
	char*				readchars(int len, bool peek = false);
	char*				readstring(bool peek = false);

	int bytesleft() const { return m_BufferUtilizedCount - m_ReadPosition; }
	void StreamSet(int pos);
	void clear();
	int addBuffer(char*, int);
	int addBuffer(SocketBuffer*);
	char operator[](int index) const;
};

constexpr auto SIZEOF_CHAR = sizeof(char);
constexpr auto SIZEOF_SHRT = sizeof(short);
constexpr auto SIZEOF_USRT = sizeof(unsigned short);
constexpr auto SIZEOF_INTE = sizeof(int);
constexpr auto SIZEOF_UINT = sizeof(unsigned int);
constexpr auto SIZEOF_LINT = sizeof(uint64_t);
constexpr auto SIZEOF_FLOT = sizeof(float);
constexpr auto SIZEOF_DOUB = sizeof(double);

char SocketBuffer::m_ReturnValueBuffer[RETURNVAL_BUFFER_SIZE + 1];

inline SocketBuffer::SocketBuffer()
{
	m_BufferSize = 30;
	MANAGE_MEMORY_NEW("WinsockWrapper", m_BufferSize);
	m_BufferData = new char[m_BufferSize];
	m_BufferUtilizedCount = 0;
	m_ReadPosition = 0;
	m_WritePosition = 0;
}

inline SocketBuffer::~SocketBuffer()
{
	if (m_BufferData != nullptr)
	{
		MANAGE_MEMORY_DELETE("WinsockWrapper", m_BufferSize);
		delete[] m_BufferData;
	}
}

inline void SocketBuffer::StreamWrite(void *in, int size)
{
	if (m_WritePosition + size >= m_BufferSize)
	{
		m_BufferSize = m_WritePosition + size + 30;
		if ((m_BufferData = static_cast<char*>(realloc(m_BufferData, m_BufferSize))) == nullptr) return;
	}
	memcpy(m_BufferData + m_WritePosition, in, size);
	m_WritePosition += size;
	if (m_WritePosition > m_BufferUtilizedCount) m_BufferUtilizedCount = m_WritePosition;
}

inline void SocketBuffer::StreamRead(void* out, int size, bool peek)
{
	if (m_ReadPosition + size > m_BufferUtilizedCount) size = m_BufferUtilizedCount - m_ReadPosition;
	if (size <= 0) return;
	memcpy(out, m_BufferData + m_ReadPosition, size);

	if (!peek)
	{
		m_ReadPosition += size;
	}
}

inline int SocketBuffer::writechar(unsigned char val)
{
	StreamWrite(&val, 1);
	return SIZEOF_CHAR;
}

inline int SocketBuffer::writeshort(short a)
{
	StreamWrite(&a, 2);
	return SIZEOF_SHRT;
}

inline int SocketBuffer::writeushort(unsigned short a)
{
	StreamWrite(&a, 2);
	return SIZEOF_USRT;
}
inline int SocketBuffer::writeint(int a)
{
	StreamWrite(&a, 4);
	return SIZEOF_INTE;
}
inline int SocketBuffer::writeuint(unsigned int a)
{
	StreamWrite(&a, 4);
	return SIZEOF_UINT;
}

inline int SocketBuffer::writelint(uint64_t a)
{
	StreamWrite(&a, 8);
	return SIZEOF_LINT;
}

inline int SocketBuffer::writefloat(float a)
{
	StreamWrite(&a, 4);
	return SIZEOF_FLOT;
}
inline int SocketBuffer::writedouble(double a)
{
	StreamWrite(&a, 8);
	return SIZEOF_DOUB;
}

inline int SocketBuffer::writechars(char*str)
{
	auto len = int(strlen(str));
	StreamWrite(str, len);
	return len;
}

inline int SocketBuffer::writechars(const char*str)
{
	int len = int(strlen(str));
	StreamWrite((void*)(str), len);
	return len;
}

inline int SocketBuffer::writechars(char*str, int length)
{
	StreamWrite(str, length);
	return length;
}

inline int SocketBuffer::writechars(const char*str, int length)
{
	StreamWrite((void*)(str), length);
	return length;
}

inline int SocketBuffer::writestring(char *str)
{
	auto len = writechars(str);
	return len + writechar('\0');
}

inline int SocketBuffer::writestring(const char *str)
{
	auto len = writechars(str);
	return len + writechar('\0');
}

inline unsigned char SocketBuffer::readchar(bool peek)
{
	unsigned char b;
	StreamRead(&b, 1, peek);
	return b;
}

inline char* SocketBuffer::readchars(int len, bool peek)
{
	if (len < 0) return nullptr;
	StreamRead(&m_ReturnValueBuffer, len, peek);
	return m_ReturnValueBuffer;
}

inline short SocketBuffer::readshort(bool peek)
{
	short b;
	StreamRead(&b, 2, peek);
	return b;
}

inline unsigned short SocketBuffer::readushort(bool peek)
{
	unsigned short b;
	StreamRead(&b, 2, peek);
	return b;
}

inline int SocketBuffer::readint(bool peek)
{
	int b;
	StreamRead(&b, 4, peek);
	return b;
}

inline unsigned int SocketBuffer::readuint(bool peek)
{
	unsigned int b;
	StreamRead(&b, 4, peek);
	return b;
}

inline uint64_t SocketBuffer::readlint(bool peek)
{
	uint64_t b;
	StreamRead(&b, 8, peek);
	return b;
}

inline float SocketBuffer::readfloat(bool peek)
{
	float b;
	StreamRead(&b, 4, peek);
	return b;
}
inline double SocketBuffer::readdouble(bool peek)
{
	double b;
	StreamRead(&b, 8, peek);
	return b;
}

inline char* SocketBuffer::readstring(bool peek)
{
	int i;
	for (i = m_ReadPosition; i < m_BufferUtilizedCount; i++)
		if (m_BufferData[i] == '\0') break;

	if (i == m_BufferUtilizedCount) return nullptr;
	i -= m_ReadPosition;
	i = std::min<int>(RETURNVAL_BUFFER_SIZE, i);
	StreamRead(&m_ReturnValueBuffer, i + 1, peek);
	return m_ReturnValueBuffer;
}

inline int SocketBuffer::addBuffer(SocketBuffer *buffer)
{
	StreamWrite(buffer->m_BufferData, buffer->m_BufferUtilizedCount);
	return buffer->m_BufferUtilizedCount;
}

inline int SocketBuffer::addBuffer(char *m_BufferData, int len)
{
	StreamWrite(m_BufferData, len);
	return len;
}

inline void SocketBuffer::clear()
{
	if (m_BufferSize > 30)
	{
		MANAGE_MEMORY_DELETE("WinsockWrapper", sizeof(m_BufferData));
		delete[] m_BufferData;
		m_BufferSize = 30;
		MANAGE_MEMORY_NEW("WinsockWrapper", m_BufferSize);
		m_BufferData = new char[m_BufferSize];
	}
	m_BufferUtilizedCount = 0;
	m_ReadPosition = 0;
	m_WritePosition = 0;
}

inline void SocketBuffer::StreamSet(int pos)
{
	m_ReadPosition = 0;
	m_WritePosition = 0;
}

inline char SocketBuffer::operator [](int i) const
{
	return ((i < 0 || i >= m_BufferUtilizedCount) ? '\0' : m_BufferData[i]);
}