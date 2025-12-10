#pragma once

struct Events
{
	SDL_Event FileOpenEvent = {};
	SDL_Event StartRaytraceEvent = {};
	SDL_Event StopRaytraceEvent = {};

	SDL_Event RaytraceStartedEvent = {};
	SDL_Event RaytraceStoppedEvent = {};
};

extern "C" Events events;

void Events_Initialize();
