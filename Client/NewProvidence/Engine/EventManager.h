#pragma once

#include <string>
#include <unordered_map>
#include <vector>

struct EventData
{
	std::string EventType;
	std::string EventSender;
	EventData(std::string type, std::string sender) : EventType(type), EventSender(sender) {}
};

class EventListener
{
public:
	virtual void ReceiveEvent(EventData* eventData) = 0;
};

class EventManager
{
private:
	EventManager();
	~EventManager();

	std::unordered_map< std::string, std::vector<EventListener*> > EventListenerList;

public:
	static EventManager& GetInstance() { static EventManager INSTANCE; return INSTANCE; }

	inline void AddEventListener(std::string eventType, EventListener* listener) { EventListenerList[eventType].push_back(listener); }

	void RemoveEventListener(std::string eventType, EventListener* listener)
	{
		if (EventListenerList.find(eventType) == EventListenerList.end()) return;
		auto listenerList = EventListenerList[eventType];
		for (auto i = listenerList.begin(); i != listenerList.end(); ++i)
		{
			if ((*i) != listener) continue;
			i = listenerList.erase(i);
			break;
		}
	}

	void BroadcastEvent(EventData* eventData)
	{
		if (EventListenerList.find(eventData->EventType) == EventListenerList.end()) return;
		auto listenerList = EventListenerList[eventData->EventType];
		for (auto i = listenerList.begin(); i != listenerList.end(); ++i) (*i)->ReceiveEvent(eventData);
	}
};


inline EventManager::EventManager()
{
}

inline EventManager::~EventManager()
{
}

//  Instance to be utilized by anyone including this header
EventManager& eventManager = EventManager::GetInstance();

///// Basic Events All Projects Can Use
struct FileDropEventData : public EventData
{
	std::string File;
	bool IsFolder;

	FileDropEventData(std::string file, bool isFolder, std::string sender) :
		EventData::EventData("FileDrop", sender),
		File(file),
		IsFolder(isFolder)
	{}
};