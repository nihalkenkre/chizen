#pragma once

struct Events
{
	SDL_Event FileOpen = {};
	SDL_Event StartRaytrace = {};
	SDL_Event StopRaytrace = {};

	SDL_Event RaytraceStarted = {};
	SDL_Event RaytraceStopped = {};

	SDL_Event RaytraceSampleDone = {};
};

extern "C" Events events;

void Events_Initialize();
