#pragma once

#include "MemoryManager.h"

#if USING_SDL_MIXER
#include <SDL_mixer.h>
#endif

class SoundWrapper
{
public:
	static SoundWrapper& GetInstance() { static SoundWrapper INSTANCE; return INSTANCE; }

	int loadSoundFile(const char* soundFileName, int identifier = -1);
	int loadMusicFile(const char* soundFileName, int identifier = -1);
	bool playSoundFile(int identifier);
	bool playMusicFile(int identifier);
	bool pauseSoundFile(int identifier);
	bool pauseMusicFile(int identifier);
	bool stopSoundFile(int identifier);
	bool stopMusicFile(int identifier);
	bool unloadSoundFile(int identifier);
	bool unloadMusicFile(int identifier);
	void Shutdown();

#if USING_SDL_MIXER
	static bool GetMusicPlaying() { return (Mix_PlayingMusic() != 0); }
	static bool GetMusicPaused() { return (Mix_PausedMusic() != 0); }

	static void ResumeMusic() { Mix_ResumeMusic(); }
#else
	static bool GetMusicPlaying() { return false; }
	static bool GetMusicPaused() { return false; }

	static void ResumeMusic() { }
#endif

private:
	SoundWrapper() {}
	~SoundWrapper();

	int FindFirstFreeSoundIndex();
	int FindFirstFreeMusicIndex();
	void unloadAllSoundFiles();

#if USING_SDL_MIXER
	std::map< int, std::pair<std::string, Mix_Chunk*> > soundDataList;
	std::map< int, std::pair<std::string, Mix_Music*> > musicDataList;
#endif
};

inline int SoundWrapper::loadSoundFile(const char* soundFileName, int identifier)
{
#if USING_SDL_MIXER
	if (identifier == -1) identifier = FindFirstFreeSoundIndex();

	auto nameString = std::string(soundFileName);
	auto iterID = soundDataList.find(identifier);
	if (iterID != soundDataList.end()) return ((*iterID).second.first.compare(nameString) == 0 ? identifier : -1);

	MANAGE_MEMORY_NEW("SoundWrapper", sizeof(Mix_Chunk));
	auto newChunk = Mix_LoadWAV(soundFileName);
	if (newChunk != nullptr)
	{
		soundDataList[identifier] = std::pair<std::string, Mix_Chunk*>(nameString, newChunk);
		return identifier;
	}
	else
		printf("Failed to load sound file: %s\n", soundFileName);
#endif
	return -1;
}

inline int SoundWrapper::loadMusicFile(const char* soundFileName, int identifier)
{
#if USING_SDL_MIXER
	if (identifier == -1) identifier = FindFirstFreeMusicIndex();

	auto nameString = std::string(soundFileName);
	auto iterID = musicDataList.find(identifier);
	if (iterID != musicDataList.end()) return ((*iterID).second.first.compare(nameString) == 0 ? identifier : -1);

	MANAGE_MEMORY_NEW("SoundWrapper", 100);
	auto newMusic = Mix_LoadMUS(soundFileName);
	if (newMusic != nullptr)
	{
		musicDataList[identifier] = std::pair<std::string, Mix_Music*>(nameString, newMusic);
		return identifier;
	}
	else
		printf("Failed to load music file: %s\n", soundFileName);
#endif
	return -1;
}

inline bool SoundWrapper::playSoundFile(int identifier)
{
#if USING_SDL_MIXER
	auto findIter = soundDataList.find(identifier);
	if (findIter == soundDataList.end()) return false;

	return (Mix_PlayChannel(-1, (*findIter).second.second, 0) != -1);
#else
	return false;
#endif
}

inline bool SoundWrapper::playMusicFile(int identifier)
{
#if USING_SDL_MIXER
	auto findIter = musicDataList.find(identifier);
	if (findIter == musicDataList.end()) return false;

	if (GetMusicPlaying() && GetMusicPaused())
	{
		ResumeMusic();
		return true;
	}
	else
	{
		Mix_HaltMusic();
		return (Mix_PlayMusic((*findIter).second.second, -1) != -1);
	}
#else
	return false;
#endif
}

inline bool SoundWrapper::pauseSoundFile(int identifier)
{
#if USING_SDL_MIXER
	auto findIter = soundDataList.find(identifier);
	if (findIter == soundDataList.end()) return false;

	Mix_Pause(-1);
#endif
	return true;
}

inline bool SoundWrapper::pauseMusicFile(int identifier)
{
#if USING_SDL_MIXER
	auto findIter = musicDataList.find(identifier);
	if (findIter == musicDataList.end()) return false;

	if (Mix_PlayingMusic()) Mix_PauseMusic();
#endif
	return true;
}

inline bool SoundWrapper::stopSoundFile(int identifier)
{
#if USING_SDL_MIXER
	auto findIter = soundDataList.find(identifier);
	if (findIter == soundDataList.end()) return false;

	Mix_HaltChannel(-1);
#endif
	return true;
}

inline bool SoundWrapper::stopMusicFile(int identifier)
{
#if USING_SDL_MIXER
	auto findIter = musicDataList.find(identifier);
	if (findIter == musicDataList.end()) return false;

	Mix_HaltMusic();
#endif
	return true;
}

inline bool SoundWrapper::unloadSoundFile(int identifier)
{
#if USING_SDL_MIXER
	auto findIter = soundDataList.find(identifier);
	if (findIter == soundDataList.end()) return false;

	MANAGE_MEMORY_DELETE("SoundWrapper", sizeof(Mix_Chunk));
	Mix_FreeChunk((*findIter).second.second);
	soundDataList.erase(findIter);
#endif
	return true;
}

inline bool SoundWrapper::unloadMusicFile(int identifier)
{
#if USING_SDL_MIXER
	auto findIter = musicDataList.find(identifier);
	if (findIter == musicDataList.end()) return false;

	MANAGE_MEMORY_DELETE("SoundWrapper", 100);
	Mix_FreeMusic((*findIter).second.second);
	musicDataList.erase(findIter);
#endif
	return true;
}

inline void SoundWrapper::Shutdown()
{
	unloadAllSoundFiles();
}

inline SoundWrapper::~SoundWrapper()
{
	Shutdown();
}

inline int SoundWrapper::FindFirstFreeSoundIndex()
{
#if USING_SDL_MIXER
	for (auto i = 0; ; ++i)
	{
		auto findIter = soundDataList.find(i);
		if (findIter == soundDataList.end()) return i;
	}
#endif
	return -1;
}

inline int SoundWrapper::FindFirstFreeMusicIndex()
{
#if USING_SDL_MIXER
	for (auto i = 0; ; ++i)
	{
		auto findIter = musicDataList.find(i);
		if (findIter == musicDataList.end()) return i;
	}
#endif
	return -1;
}

inline void SoundWrapper::unloadAllSoundFiles()
{
#if USING_SDL_MIXER
	while (!soundDataList.empty()) unloadSoundFile(soundDataList.begin()->first);
	while (!musicDataList.empty()) unloadMusicFile(musicDataList.begin()->first);
#endif
}

SoundWrapper& soundWrapper = SoundWrapper::GetInstance();