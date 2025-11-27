#include "app.hpp"
#include "utils.hpp"

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv)
{
	SDL_CHECK(SDL_Init(SDL_INIT_VIDEO));

	SDL_Window* window = SDL_CreateWindow("Chizen", 1280, 720, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);

	if (window == nullptr)
	{
		SDL_Log("%s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	static App app(window, std::filesystem::path(std::string(argv[0])).parent_path().string());
	*appstate = &app;

	SDL_CHECK(ImGui_ImplSDL3_InitForVulkan(window));

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
	App* app = reinterpret_cast<App*>(appstate);

	ImGui_ImplSDL3_ProcessEvent(event);
	ImGuiIO& io = ImGui::GetIO();

	if (SDL_GetWindowFlags(app->GetWindow()) & SDL_WINDOW_MINIMIZED)
	{
		return SDL_APP_CONTINUE;
	}

	if (event->type == SDL_EVENT_QUIT)
	{
		return SDL_APP_SUCCESS;
	}
	else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
	{
		if (!io.WantCaptureMouse)
		{
			app->IsTrackingMouse() = true;

			app->GetLastMousePosition()[0] = event->motion.x;
			app->GetLastMousePosition()[1] = event->motion.y;
		}
	}
	else if (event->type == SDL_EVENT_MOUSE_MOTION)
	{
		if (!io.WantCaptureMouse)
		{
			if (app->IsTrackingMouse())
			{
				app->GetDeltaMousePosition()[0] += ((app->GetLastMousePosition()[0] - event->motion.x) / static_cast<float>(app->GetVulkanInterface()->GetSurface()->GetSurfaceCapabilities().surfaceCapabilities.currentExtent.width)) * 2;
				app->GetDeltaMousePosition()[1] += ((app->GetLastMousePosition()[1] - event->motion.y) / static_cast<float>(app->GetVulkanInterface()->GetSurface()->GetSurfaceCapabilities().surfaceCapabilities.currentExtent.height)) * 2;

				app->GetLastMousePosition()[0] = event->motion.x;
				app->GetLastMousePosition()[1] = event->motion.y;
			}
		}
	}
	else if (event->type == SDL_EVENT_MOUSE_BUTTON_UP)
	{
		if (!io.WantCaptureMouse)
		{
			app->GetLastMousePosition()[0] = 0;
			app->GetLastMousePosition()[1] = 0;

			app->IsTrackingMouse() = false;
		}
	}
	else if (event->type == SDL_EVENT_MOUSE_WHEEL)
	{
		app->GetZoomLevel() = std::max(0.01f, app->GetZoomLevel() + event->wheel.y / 20.f);
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
	App* app = reinterpret_cast<App*>(appstate);

	if (SDL_GetWindowFlags(app->GetWindow()) & SDL_WINDOW_MINIMIZED)
	{
		return SDL_APP_CONTINUE;
	}

	if (app->GetImGUIState().StartRaytracing)
	{
		if (app->GetIsRaytracing()) app->StopRaytracing();

		VkExtent2D& render_target_extent = app->GetRenderTargetExtent();
		ImGUIState& imgui_state = app->GetImGUIState();

		if (render_target_extent.width != imgui_state.TempRenderTargetExtent[0] ||
			render_target_extent.height != imgui_state.TempRenderTargetExtent[1])
		{
			render_target_extent.width = imgui_state.TempRenderTargetExtent[0];
			render_target_extent.height = imgui_state.TempRenderTargetExtent[1];
			app->RecreateRenderTarget();
		}
		
		uint32_t& max_samples = app->GetMaxSamples();
		if (max_samples != imgui_state.TempMaxSamples)
		{
			max_samples = imgui_state.TempMaxSamples;
		}

		app->RunRaytrace();
	}

	app->RunDisplay();

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
}
