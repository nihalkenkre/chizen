#include "app.hpp"
#include "utils.hpp"

//App::App(SDL_Window* window)
//{
//	vulkan_interface = VulkanInterface(window);
//}

App App_Create(SDL_Window* window, const std::string& current_path)
{
	App app = {
		.VulkanInterface = VulkanInterface_Create(window),
		.Display = Display_Create(app.VulkanInterface, current_path),
		.Window = window,
	};
	app.Raytrace = Raytrace_Create(&app.VulkanInterface, app.RenderTargetExtent, current_path);

	const VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	const	VkDescriptorPoolCreateInfo pool_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		.maxSets = 1000,
		.poolSizeCount = std::size(pool_sizes),
		.pPoolSizes = pool_sizes,
	};

	VK_CHECK("create imgui desc pool", vkCreateDescriptorPool(app.VulkanInterface.DeviceData.Device, &pool_info, nullptr, &app.ImGUIPool));

	ImGui::CreateContext();
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ImGui_ImplVulkan_InitInfo imgui_init_info = {
		.Instance = app.VulkanInterface.Instance,
		.PhysicalDevice = app.VulkanInterface.PhysicalDeviceData.PhysicalDevice,
		.Device = app.VulkanInterface.DeviceData.Device,
		.Queue = app.VulkanInterface.DeviceData.GraphicsQueue,
		.DescriptorPool = app.ImGUIPool,
		.MinImageCount = app.VulkanInterface.SwapchainData.ImagesCount,
		.ImageCount = app.VulkanInterface.SwapchainData.ImagesCount,
		.PipelineInfoMain = {
			.PipelineRenderingCreateInfo = {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
				.colorAttachmentCount = 1,
				.pColorAttachmentFormats = &app.VulkanInterface.SurfaceData.SurfaceFormat.format,
			},
		},
		.UseDynamicRendering = true,
	};

	SDL_CHECK(ImGui_ImplVulkan_Init(&imgui_init_info));

	return app;
}

void App_Display(App* app)
{
	Display_Render(app->Display, app->DeltaMousePosition, app->ZoomLevel, &app->ImGUIState);
}

void App_Raytrace(App* app)
{
	if (!app->IsRaytracing)
	{
		app->RaytraceThread = std::thread(Raytrace_Render, app->Raytrace, &app->IsRaytracing, app->MaxSamples);
		app->RaytraceThread.detach();
		app->IsRaytracing = true;
		app->ImGUIState.StartRaytracing = false;
	}
}

void App_RecreateRenderTarget(App* app)
{
	VK_CHECK("device wait idle", vkDeviceWaitIdle(app->VulkanInterface.DeviceData.Device));
	VulkanInterface_RecreateFinalRenderTarget(&app->VulkanInterface, app->RenderTargetExtent);
	Display_UpdateFinalRenderTarget(app->Display, app->VulkanInterface.FinalRenderTarget);
	Raytrace_RecreateRenderResources(app->Raytrace, app->RenderTargetExtent);
	Raytrace_UpdateFinalRenderTarget(app->Raytrace, app->VulkanInterface.FinalRenderTarget);
}

void App_StopRaytracing(App* app)
{
	Raytrace_StopRendering(app->Raytrace);
	while (app->IsRaytracing) {}
}

void App_Destroy(App* app)
{
	Raytrace_StopRendering(app->Raytrace);

	while (app->IsRaytracing) {}

	VK_CHECK("device wait idle", vkDeviceWaitIdle(app->VulkanInterface.DeviceData.Device));

	ImGui_ImplSDL3_Shutdown();
	ImGui_ImplVulkan_Shutdown();
	ImGui::DestroyContext();

	vkDestroyDescriptorPool(app->VulkanInterface.DeviceData.Device, app->ImGUIPool, nullptr);
	
	Raytrace_Destroy(app->Raytrace);
	Display_Destroy(app->Display);
	VulkanInterface_Destroy(&app->VulkanInterface);
}
