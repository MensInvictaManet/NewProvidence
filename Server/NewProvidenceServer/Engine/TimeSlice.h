#pragma once

#include <algorithm>

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