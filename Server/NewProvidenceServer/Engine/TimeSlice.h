#pragma once

#include <algorithm>
#include <time.h>

static Uint32 gameTicksUint = 0;
static Uint32 frameTicksUint = 0;
static double gameSeconds = 0.0;
static float gameSecondsF = 0.0f;
static double frameSeconds = 0.0;
static float frameSecondsF = 0.0f;
static unsigned int averageFPS = 0;

#define TICKS_TO_SECONDS(ticks) double(ticks) / 1000.0
#define SECONDS_TO_TICKS(seconds) Uint32(seconds * 1000.0)

inline void DetermineAverageFPS(double newFrameSeconds)
{
	static unsigned int frameIndex = 0;
	static double lastGameSeconds = 0.0;

	++frameIndex;
	if (gameSeconds > lastGameSeconds + 1.0)
	{
		averageFPS = (unsigned int)(double(frameIndex) / (gameSeconds - lastGameSeconds));
		frameIndex = 0;
		lastGameSeconds = gameSeconds;
	}
}

inline void DetermineTimeSlice()
{
	// Get the time slice
	static Uint32 lastGameTicksUint = 0;

	gameTicksUint = SDL_GetTicks();
	gameSeconds = TICKS_TO_SECONDS(gameTicksUint);
	frameTicksUint = std::min<Uint32>(gameTicksUint - lastGameTicksUint, 1000);
	frameSeconds = float(TICKS_TO_SECONDS(frameTicksUint));
	lastGameTicksUint = gameTicksUint;

	DetermineAverageFPS(frameSeconds);
	frameSecondsF = float(frameSeconds);
	gameSecondsF = float(gameSeconds);
}

inline std::string GetCurrentTimeString()
{
	auto currentTime = time(nullptr);

	tm timeBuffer;
	auto localTime = localtime_s(&timeBuffer, &currentTime);
	timeBuffer.tm_year += 1900;
	timeBuffer.tm_mon += 1;
	if ((timeBuffer.tm_mon) > 12)
	{
		timeBuffer.tm_mon -= 12;
		timeBuffer.tm_year += 1;
	}

	auto year = std::to_string(timeBuffer.tm_year);
	auto month = std::to_string(timeBuffer.tm_mon);
	if (month.length() == 1) month = "0" + month;
	auto day = std::to_string(timeBuffer.tm_mday);
	if (day.length() == 1) day = "0" + day;
	auto date = year + "-" + month + "-" + day;

	auto hour = std::to_string(timeBuffer.tm_hour);
	if (hour.length() == 1) hour = "0" + hour;
	auto minute = std::to_string(timeBuffer.tm_min);
	if (minute.length() == 1) minute = "0" + minute;
	auto second = std::to_string(timeBuffer.tm_sec);
	if (second.length() == 1) second = "0" + second;
	auto time = hour + ":" + minute + ":" + second;


	return std::string(date + " | " + time);
}