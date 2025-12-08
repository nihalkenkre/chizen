#include "app.hpp"
#include "utils.hpp"
#include "vulkan_interface.hpp"
#include "imgui_state.hpp"
#include "vulkan_objects.hpp"
#include "display.hpp"

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
	if (SDL_GetWindowFlags(app->GetWindow()) & SDL_WINDOW_MINIMIZED)
	{
		return SDL_APP_CONTINUE;
	}

	ImGui_ImplSDL3_ProcessEvent(event);
	ImGuiIO& io = ImGui::GetIO();

	float* delta_mouse_position = app->GetDeltaMousePosition();
	float* last_mouse_position = app->GetLastMousePosition();
	bool& is_tracking_mouse = app->IsTrackingMouse();
	float& zoom_level = app->GetZoomLevel();

	if (event->type == SDL_EVENT_QUIT)
	{
		app->StopRaytracing();
		return SDL_APP_SUCCESS;
	}
	else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
	{
		if (!io.WantCaptureMouse)
		{
			is_tracking_mouse = true;

			last_mouse_position[0] = event->motion.x;
			last_mouse_position[1] = event->motion.y;
		}
	}
	else if (event->type == SDL_EVENT_MOUSE_MOTION)
	{
		if (!io.WantCaptureMouse)
		{
			if (is_tracking_mouse)
			{
				delta_mouse_position[0] += ((last_mouse_position[0] - event->motion.x) / static_cast<float>(app->GetVulkanInterface()->GetSurfaceKHR()->GetSurfaceCapabilities().surfaceCapabilities.currentExtent.width)) * 2;
				delta_mouse_position[1] += ((last_mouse_position[1] - event->motion.y) / static_cast<float>(app->GetVulkanInterface()->GetSurfaceKHR()->GetSurfaceCapabilities().surfaceCapabilities.currentExtent.height)) * 2;

				last_mouse_position[0] = event->motion.x;
				last_mouse_position[1] = event->motion.y;
			}
		}
	}
	else if (event->type == SDL_EVENT_MOUSE_BUTTON_UP)
	{
		if (!io.WantCaptureMouse)
		{
			last_mouse_position[0] = 0;
			last_mouse_position[1] = 0;

			is_tracking_mouse = false;
		}
	}
	else if (event->type == SDL_EVENT_MOUSE_WHEEL)
	{
		zoom_level = std::max(0.01f, zoom_level + event->wheel.y / 20.f);
	}
	else if (event->type == SDL_EVENT_WINDOW_RESIZED)
	{
		app->RecreateSwapchain();
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

	if (app->GetImGUIState()->GetShouldStartRaytracing())
	{
		if (app->IsRaytracing()) app->StopRaytracing();

		VkExtent2D& render_target_extent = app->GetRenderTargetExtent();
		int* tmp_render_target_extent = app->GetImGUIState()->GetRenderTargetExtent();

		if (render_target_extent.width != tmp_render_target_extent[0] ||
			render_target_extent.height != tmp_render_target_extent[1])
		{
			render_target_extent.width = tmp_render_target_extent[0];
			render_target_extent.height = tmp_render_target_extent[1];
			app->RecreateRenderTarget();
		}

		int& tmp_max_samples = app->GetImGUIState()->GetMaxSamples();
		uint32_t& max_samples = app->GetMaxSamples();
		if (max_samples != tmp_max_samples)
		{
			max_samples = tmp_max_samples;
		}

		app->RunRaytrace();
	}

	app->RunDisplay();

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
}
