#pragma once

struct Events
{
	SDL_Event FileOpen = {};
	SDL_Event StartRaytrace = {};
	SDL_Event StopRender = {};

	SDL_Event RenderStarted = {};
	SDL_Event RaytraceStopped = {};

	SDL_Event RenderSampleDone = {};
};

extern "C" Events events;

void Events_Initialize();
