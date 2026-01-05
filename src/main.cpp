#include "app.hpp"
#include "utils.hpp"
#include "vulkan_interface.hpp"
#include "imgui_state.hpp"
#include "vulkan_objects.hpp"
#include "display.hpp"

#include "events.hpp"

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv)
{
	SDL_CHECK(SDL_Init(SDL_INIT_VIDEO));

	SDL_Window* window = SDL_CreateWindow("Chizen", 1280, 720, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);

	if (window == nullptr)
	{
		SDL_Log("%s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	static App app(window);
	*appstate = &app;

	SDL_CHECK(ImGui_ImplSDL3_InitForVulkan(window));

	Events_Initialize();

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
	App* app = reinterpret_cast<App*>(appstate);
	if (SDL_GetWindowFlags(app->GetWindow()) & (SDL_WINDOW_MINIMIZED | SDL_WINDOW_HIDDEN | SDL_WINDOW_OCCLUDED))
	{
		return SDL_APP_CONTINUE;
	}

	if (event->type == SDL_EVENT_QUIT)
	{
		app->StopRendering();
		return SDL_APP_SUCCESS;
	}
	else {
		ImGui_ImplSDL3_ProcessEvent(event);
		app->ProcessEvent(event);
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
	App* app = reinterpret_cast<App*>(appstate);

	if (SDL_GetWindowFlags(app->GetWindow()) & (SDL_WINDOW_MINIMIZED | SDL_WINDOW_HIDDEN | SDL_WINDOW_OCCLUDED))
	{
		return SDL_APP_CONTINUE;
	}

	app->Iterate();

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
	ImGui_ImplSDL3_Shutdown();
}
