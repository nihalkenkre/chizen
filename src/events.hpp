#pragma once

struct Events
{
	SDL_Event FileOpen = {};
	SDL_Event StartRender = {};
	SDL_Event StopRender = {};

	SDL_Event RenderStarted = {};
	SDL_Event RenderStopped = {};

	SDL_Event RenderSampleDone = {};
	SDL_Event ReloadShaders = {};
};

extern "C" Events events;

void Events_Initialize();
