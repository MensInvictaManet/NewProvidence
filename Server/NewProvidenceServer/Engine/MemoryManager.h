#pragma once

#define MEMORY_MANAGER_ACTIVE true

#if !MEMORY_MANAGER_ACTIVE
#define MANAGE_MEMORY_NEW(stringType, sizetAmount) NULL;
#define MANAGE_MEMORY_DELETE(stringType, sizetAmount) NULL;
#else
#define MANAGE_MEMORY_NEW(stringPoolType, sizetAmount) memoryManager.ManageMemoryNew(stringPoolType, sizetAmount);
#define MANAGE_MEMORY_DELETE(stringPoolType, sizetAmount) memoryManager.ManageMemoryDelete(stringPoolType, sizetAmount);

#include <map>
#include <fstream>

class MemoryManager
{
public:
	static MemoryManager& GetInstance() { static MemoryManager INSTANCE; return INSTANCE; }

	void ManageMemoryNew(std::string poolType, size_t Amount);
	void ManageMemoryDelete(std::string poolType, size_t Amount);
	void OutputMemoryData(const char* fileName);

	unsigned int GetMemoryPoolCount() const { return int(m_MemoryPoolList.size()); }
	std::string GetMemoryPoolNameAtIndex(int index) const;
	int GetMemoryPoolAmountAtIndex(int index) const;

	void Shutdown();

private:
	MemoryManager();
	~MemoryManager();

	std::map<std::string, int> m_MemoryPoolList;
	int m_TotalMemoryUsed;
};

inline void MemoryManager::ManageMemoryNew(std::string poolType, size_t amount)
{
	if (m_MemoryPoolList.find(poolType) == m_MemoryPoolList.end()) m_MemoryPoolList[poolType] = 0;
	m_MemoryPoolList[poolType] += int(amount);
	m_TotalMemoryUsed += int(amount);
}

inline void MemoryManager::ManageMemoryDelete(std::string poolType, size_t amount)
{
	if (m_MemoryPoolList.find(poolType) == m_MemoryPoolList.end()) m_MemoryPoolList[poolType] = 0;
	m_MemoryPoolList[poolType] -= int(amount);
	if (m_MemoryPoolList[poolType] < 0 && m_MemoryPoolList[poolType] >= -int(amount)) printf("MemoryManager has gone negative on the %s pool.\n", poolType.c_str());

	m_TotalMemoryUsed -= int(amount);
	if (m_TotalMemoryUsed < 0 && m_TotalMemoryUsed >= -int(amount)) printf("MemoryManager has gone negative on the total memory count.\n");
}


inline void MemoryManager::OutputMemoryData(const char* fileName)
{
	std::ofstream memoryOutput(fileName, std::ios_base::binary);
	if (!memoryOutput.good() || memoryOutput.bad())
	{
		printf("MemoryManager has failed to output it's data.\n");
		return;
	}

	printf("Memory Management Data:\n");
	for (auto iter = m_MemoryPoolList.begin(); iter != m_MemoryPoolList.end(); ++iter)
	{
		printf("\"%s\":  %d\n", (*iter).first.c_str(), (*iter).second);
	}
	memoryOutput.close();
}

inline std::string MemoryManager::GetMemoryPoolNameAtIndex(int index) const
{
	auto i = 0;
	for (auto iter = m_MemoryPoolList.begin(); iter != m_MemoryPoolList.end(); ++iter, ++i)
	{
		if (i != index) continue;
		return (*iter).first;
	}

	printf("Memory Manager error: Attempting to find invalid memory pool.\n");
	return "";
}

inline int MemoryManager::GetMemoryPoolAmountAtIndex(int index) const
{
	auto i = 0;
	for (auto iter = m_MemoryPoolList.begin(); iter != m_MemoryPoolList.end(); ++iter, ++i)
	{
		if (i != index) continue;
		return (*iter).second;
	}

	printf("Memory Manager error: Attempting to find invalid memory pool.\n");
	return 0;
}

inline void MemoryManager::Shutdown()
{
	if (m_TotalMemoryUsed > 0) printf("MemoryManager still managing %d bytes of memory. Perhaps it wasn't shut down last.\n", m_TotalMemoryUsed);
	m_MemoryPoolList.clear();
}

inline MemoryManager::MemoryManager() : 
	m_TotalMemoryUsed(0)
{

}

inline MemoryManager::~MemoryManager()
{
}

//  Instance to be utilized by anyone including this header
MemoryManager& memoryManager = MemoryManager::GetInstance();
#endif