#pragma once

struct Events
{
	SDL_Event FileOpen = {};
	SDL_Event StartRaytrace = {};
	SDL_Event StopRaytrace = {};

	SDL_Event RaytraceStarted = {};
	SDL_Event RaytraceStopped = {};
};

extern "C" Events events;

void Events_Initialize();
