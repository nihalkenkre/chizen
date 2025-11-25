#pragma once

#include "vulkan_interface.hpp"
#include "display.hpp"
#include "raytrace.hpp"
#include "utils.hpp"

//class App
//{
//public:
//   App(SDL_Window* window);
//
//   VulkanInterface vulkan_interface;
//};

struct App
{
	VulkanInterface VulkanInterface = {};
	Display* Display = nullptr;
	Raytrace* Raytrace = nullptr;
	SDL_Window* Window = nullptr;
	VkExtent2D RenderTargetExtent = { 1280, 720 };
	VkDescriptorPool ImGUIPool = VK_NULL_HANDLE;
	std::thread RaytraceThread = {};
	ImGUIState ImGUIState = {};
	float DeltaMousePosition[2];
	float LastMousePosition[2];
	float ZoomLevel = 1.f;
	uint32_t MaxSamples = 1024;
	bool IsTrackingMouse = false;
	bool IsRaytracing = false;
};

App App_Create(SDL_Window* window, const std::string& current_path);
void App_Destroy(App* app);

void App_Display(App* app);
void App_Raytrace(App* app);
void App_RecreateRenderTarget(App* app);
void App_StopRaytracing(App* app);
