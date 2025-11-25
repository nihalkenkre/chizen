#pragma once

//class vk_instance
//{
//public:
//	vk_instance() : instance(VK_NULL_HANDLE) { std::println("vk_instance ctor def"); }
//	vk_instance(const char* const* extensions, const uint32_t extensions_count);
//
//	vk_instance(const vk_instance& other) = delete;
//	vk_instance& operator=(const vk_instance& other) = delete;
//
//	vk_instance(vk_instance&& other) noexcept;
//	vk_instance& operator=(vk_instance&& other) noexcept;
//
//	~vk_instance() noexcept;
//
//	VkInstance instance;
//};
//
//class vk_surface
//{
//public:
//	vk_surface() : surface(VK_NULL_HANDLE), instance(VK_NULL_HANDLE) { std::println("vk_surface ctor def"); }
//	vk_surface(SDL_Window* window, const VkInstance instance, const VkAllocationCallbacks* allocator);
//
//	vk_surface(const vk_surface& other) = delete;
//	vk_surface& operator=(const vk_surface& other) = delete;
//
//	vk_surface(vk_surface&& other) noexcept;
//	vk_surface& operator=(vk_surface&& other) noexcept;
//
//	~vk_surface() noexcept;
//
//	VkSurfaceKHR surface;
//
//private:
//	VkInstance instance;
//};

VkInstance VkInstance_Create(const char* const* extensions, const uint32_t extensions_count);
void VkInstance_Destroy(VkInstance instance);

struct PhysicalDeviceData
{
	VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
	VkPhysicalDeviceProperties2 Properties = {};
	VkPhysicalDeviceMemoryProperties2 MemoryProperties = {};
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR RayTracingProperties = {};
	uint32_t GraphicsQueueFamilyIndex = 0;
	uint32_t ComputeQueueFamilyIndex = 0;
	uint32_t TransferQueueFamilyIndex = 0;
};

PhysicalDeviceData VkInstance_GetPhysicalDeviceData(const VkInstance instance, const VkSurfaceKHR surface);

struct SurfaceData
{
	VkSurfaceKHR Surface = VK_NULL_HANDLE;
	VkPresentModeKHR PresentMode = VK_PRESENT_MODE_FIFO_KHR;
	VkSurfaceFormatKHR SurfaceFormat = {};
	VkSurfaceCapabilities2KHR SurfaceCapabilities = {};
};

SurfaceData VkPhysicalDevice_GetSurfaceData(const VkPhysicalDevice physical_device, const VkSurfaceKHR surface);

struct DeviceData
{
	VkDevice Device = VK_NULL_HANDLE;
	VkQueue GraphicsQueue = VK_NULL_HANDLE;
	VkQueue ComputeQueue = VK_NULL_HANDLE;
	VkQueue TransferQueue = VK_NULL_HANDLE;
};

DeviceData DeviceData_Create(const PhysicalDeviceData& physical_device_data);
void DeviceData_Destroy(DeviceData device_data);

VmaAllocator Allocator_Create(const VkInstance instance, const VkPhysicalDevice physical_device, const VkDevice device);
void Allocator_Destroy(VmaAllocator allocator);

struct SwapchainData
{
	VkSwapchainKHR Swapchain = VK_NULL_HANDLE;
	std::vector<VkImage> Images;
	std::vector<VkImageView> ImageViews;
	uint32_t ImagesCount = 0;

	VkDevice Device = VK_NULL_HANDLE;
};

SwapchainData SwapchainData_Create(const VkDevice device, const SurfaceData& surface_data, const uint32_t graphics_queue_family_index, const std::string& name);
void SwapchainData_Destroy(SwapchainData swapchain_data);

struct TransferObjects
{
	VkCommandPool CommandPool = VK_NULL_HANDLE;
	VkQueue Queue = VK_NULL_HANDLE;
	VkCommandBuffer CommandBuffer = VK_NULL_HANDLE;
	uint32_t QueueFamilyIndex = 0;

	VkDevice Device = VK_NULL_HANDLE;
};

TransferObjects TransferObjects_Create(const VkDevice device, const VkQueue transfer_queue, const uint32_t transfer_queue_family_index);
void TransferObjects_Destroy(TransferObjects to);

void TransferObjects_PrepareImage(TransferObjects to, const VkImage image);
