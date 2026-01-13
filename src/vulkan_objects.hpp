#pragma once

#include "scene.hpp"

class DeviceBufferResource;

struct PhysicalDeviceData
{
	VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
	VkPhysicalDeviceProperties2KHR Properties = {};
	VkPhysicalDeviceMemoryProperties2KHR MemoryProperties = {};
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR RayTracingProperties = {};
	VkPhysicalDeviceAccelerationStructurePropertiesKHR AccelerationStructureProperties = {};
	uint32_t GraphicsQueueFamilyIndex = 0;
	uint32_t ComputeQueueFamilyIndex = 0;
	uint32_t TransferQueueFamilyIndex = 0;
};

class Instance
{
public:
	Instance() = delete;
	Instance(const char* const* extensions, const uint32_t extensions_count);

	Instance(const Instance& other) = delete;
	Instance& operator=(const Instance& other) = delete;

	~Instance() noexcept;

	VkInstance GetInstance() const;

	PhysicalDeviceData GetPhysicalDeviceData(const VkSurfaceKHR& surface) const;

private:
	VkInstance mInstance;
};

class Surface
{
public:
	Surface() = delete;
	Surface(SDL_Window* window, const VkInstance& instance);

	Surface(const Surface& other) = delete;
	Surface& operator=(const Surface& oher) = delete;

	~Surface() noexcept;

	VkSurfaceKHR GetSurfaceKHR() const;
	VkPresentModeKHR GetPresentMode() const;
	VkSurfaceFormatKHR GetSurfaceFormat() const;
	VkSurfaceCapabilities2KHR GetSurfaceCapabilities() const;

	void PopulateSurfaceData(const VkPhysicalDevice& physical_device);

private:
	VkSurfaceKHR mSurface = VK_NULL_HANDLE;
	VkPresentModeKHR mPresentMode = VK_PRESENT_MODE_FIFO_KHR;
	VkSurfaceFormatKHR mSurfaceFormat = {};
	VkSurfaceCapabilities2KHR mSurfaceCapabilities = { .sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR };

	VkInstance mInstance = VK_NULL_HANDLE;
};

class Device
{
public:
	Device() = delete;
	Device(const PhysicalDeviceData* physical_device_data);

	Device(const Device& other) = delete;
	Device& operator=(const Device& other) = delete;

	~Device() noexcept;

	VkDevice GetDevice() const;
	VkQueue GetGraphicsQueue() const;
	VkQueue GetComputeQueue() const;
	VkQueue GetTransferQueue() const;

private:
	VkDevice mDevice = VK_NULL_HANDLE;
	VkQueue mGraphicsQueue = VK_NULL_HANDLE;
	VkQueue mComputeQueue = VK_NULL_HANDLE;
	VkQueue mTransferQueue = VK_NULL_HANDLE;
};

class Allocator
{
public:
	Allocator() = delete;
	Allocator(const VkInstance& instance, const VkPhysicalDevice& physical_device, const VkDevice& device);

	Allocator(const Allocator& other) = delete;
	Allocator& operator= (const Allocator& other) = delete;

	~Allocator() noexcept;

	VmaAllocator GetAllocator() const;

private:
	VmaAllocator mAllocator;
};

class Pool
{
public:
	Pool() = delete;
	Pool(const VmaAllocator& allocator, const VkDeviceSize min_alignment, const uint32_t mem_type_id);

	Pool(const Pool& other) = delete;
	Pool& operator=(const Pool& other) = delete;

	~Pool() noexcept;

	VmaPool GetPool() const;

private:
	VmaPool mPool = VK_NULL_HANDLE;

	VmaAllocator mAllocator = VK_NULL_HANDLE;
};

class Swapchain
{
public:
	Swapchain() = delete;
	Swapchain(const VkDevice device, const Surface* surface, const uint32_t graphics_queue_family_index, const std::string& name);

	Swapchain(const Swapchain& other) = delete;
	Swapchain& operator= (const Swapchain& other) = delete;

	~Swapchain() noexcept;

	VkSwapchainKHR GetSwapchain() const;
	std::vector<VkImage> GetImages() const;
	std::vector<VkImageView> GetImageViews() const;
	uint32_t GetImagesCount() const;

private:
	VkSwapchainKHR mSwapchain = VK_NULL_HANDLE;
	std::vector<VkImage> mImages;
	std::vector<VkImageView> mImageViews;
	uint32_t mImagesCount = 0;

	VkDevice mDevice = VK_NULL_HANDLE;
};

class TransferHelpers
{
public:
	TransferHelpers() = delete;
	TransferHelpers(const VkDevice device, const VkQueue transfer_queue, const uint32_t transfer_queue_family_index, const std::string& name);

	TransferHelpers(const TransferHelpers& other) = delete;
	TransferHelpers& operator=(const TransferHelpers& other) = delete;

	~TransferHelpers() noexcept;

	void RecordBatch();
	void ChangeImageLayout(
		const VkPipelineStageFlags2 src_stage_mask, const VkAccessFlags2 src_access_mask,
		const VkPipelineStageFlags2 dst_stage_mask, const VkAccessFlags2 dst_access_mask,
		const VkImageLayout old_layout, const VkImageLayout new_layout,
		const uint32_t src_q_fly_idx, const uint32_t dst_q_fly_idx,
		const VkImageAspectFlags aspect_mask,
		const VkImage& image) const;
	void CopyBufferToBuffer(const VkBuffer src_buffer, const VkBuffer dst_buffer, const VkDeviceSize size) const;
	void CopyBufferToImage(const VkBuffer src_buffer, const VkImage dst_image, const VkExtent3D& extent) const;
	void InsertMemoryBarrier(const VkPipelineStageFlags2 src_stage_mask, const VkAccessFlags2 src_access_mask,
		const VkPipelineStageFlags2 dst_stage_mask, const VkAccessFlags2 dst_access_mask) const;
	void SubmitBatch(const VkSemaphore wait_semaphore = VK_NULL_HANDLE, const uint64_t wait_semaphore_value = 0, const VkPipelineStageFlags2 wait_stage_mask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT);

	VkCommandPool GetCommandPool() const;
	VkCommandBuffer GetCommandBuffer() const;
	VkQueue GetQueue() const;
	VkSemaphore GetSemaphore() const;
	uint64_t& GetSemaphoreValue();
	const uint64_t GetSemaphoreValueConst() const;
	uint32_t GetQueueFamilyIndex() const;

private:
	VkCommandPool mCommandPool = VK_NULL_HANDLE;
	VkCommandBuffer mCommandBuffer = VK_NULL_HANDLE;
	VkQueue mQueue = VK_NULL_HANDLE;
	VkSemaphore mSemaphore = VK_NULL_HANDLE;
	uint64_t mSemaphoreValue = 0;
	uint32_t mQueueFamilyIndex = 0;

	VkDevice mDevice = VK_NULL_HANDLE;
};

class ComputeHelpers
{
public:
	ComputeHelpers() = delete;
	ComputeHelpers(const VkDevice device, const VkQueue compute_queue, const uint32_t compute_queue_family_index, const std::string& name);

	ComputeHelpers(const ComputeHelpers& other) = delete;
	ComputeHelpers& operator=(const ComputeHelpers& other) = delete;

	~ComputeHelpers() noexcept;

	void RecordBatch();
	void ChangeImageLayout(
		const VkPipelineStageFlags2 src_stage_mask, const VkAccessFlags2 src_access_mask,
		const VkPipelineStageFlags2 dst_stage_mask, const VkAccessFlags2 dst_access_mask,
		const VkImageLayout old_layout, const VkImageLayout new_layout,
		const uint32_t src_q_fly_idx, const uint32_t dst_q_fly_idx,
		const VkImageAspectFlags aspect_mask,
		const VkImage& image) const;
	void CopyBufferToBuffer(const VkBuffer src_buffer, const VkBuffer dst_buffer, const VkDeviceSize size, const VkDeviceSize src_offset = 0, const VkDeviceSize dst_offset = 0) const;
	void CopyBufferToImage(const VkBuffer src_buffer, const VkImage dst_image, const VkExtent3D& extent) const;
	void InsertMemoryBarrier(const VkPipelineStageFlags2 src_stage_mask, const VkAccessFlags2 src_access_mask,
		const VkPipelineStageFlags2 dst_stage_mask, const VkAccessFlags2 dst_access_mask) const;
	void ClearImage(const VkImage image, const VkClearColorValue clear_color) const;
	void BuildAccelerationStructure(const std::vector<VkAccelerationStructureBuildGeometryInfoKHR>& build_geom_infos, const std::vector<VkAccelerationStructureBuildRangeInfoKHR*>& range_infos) const;
	void SubmitBatch(const VkSemaphore wait_semaphore = VK_NULL_HANDLE, const uint64_t wait_semaphore_value = 0, const VkPipelineStageFlags2 wait_stage_mask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT);

	VkCommandPool GetCommandPool() const;
	VkCommandBuffer GetCommandBuffer() const;
	VkQueue GetQueue() const;
	VkSemaphore GetSemaphore() const;
	uint64_t& GetSemaphoreValue();
	uint64_t GetSemaphoreValueConst() const;
	uint32_t GetQueueFamilyIndex() const;

private:
	VkCommandPool mCommandPool = VK_NULL_HANDLE;
	VkCommandBuffer mCommandBuffer = VK_NULL_HANDLE;
	VkQueue mQueue = VK_NULL_HANDLE;
	VkSemaphore mSemaphore = VK_NULL_HANDLE;
	uint64_t mSemaphoreValue = 0;
	uint32_t mQueueFamilyIndex = 0;

	VkDevice mDevice = VK_NULL_HANDLE;
};

class BLAccelerationStructure
{
public:
	BLAccelerationStructure() = delete;
	BLAccelerationStructure(const VkDevice device, const VmaAllocator allocator, const Scene::Mesh::Primitive& primitive, const std::vector<uint8_t>& vertex_data, const VmaPool mem_pool, const size_t scratch_buffer_alignment, ComputeHelpers* compute_helpers, const std::string& name);

	BLAccelerationStructure(const BLAccelerationStructure& other) = delete;
	BLAccelerationStructure& operator=(const BLAccelerationStructure& other) = delete;

	~BLAccelerationStructure() noexcept;

	const VkAccelerationStructureKHR GetAS() const;
	DeviceBufferResource* GetASBuffer() const;

private:
	VkAccelerationStructureKHR mAS = VK_NULL_HANDLE;
	std::unique_ptr<DeviceBufferResource> mASBuffer = nullptr;

	VkDevice mDevice = VK_NULL_HANDLE;
};

class TLAccelerationStructure
{
public:
	TLAccelerationStructure() = delete;
	TLAccelerationStructure(const VkDevice device, const VmaAllocator allocator, const std::vector<VkAccelerationStructureInstanceKHR>& instances, const VmaPool mem_pool, const size_t scratch_buffer_alignment, ComputeHelpers* compute_helpers, const std::string& name);

	TLAccelerationStructure(const TLAccelerationStructure& other) = delete;
	TLAccelerationStructure& operator=(const TLAccelerationStructure& other) = delete;

	~TLAccelerationStructure() noexcept;

	VkAccelerationStructureKHR GetAS() const;
	DeviceBufferResource* GetASBuffer() const;

private:
	VkAccelerationStructureKHR mAS = VK_NULL_HANDLE;
	std::unique_ptr<DeviceBufferResource> mASBuffer = nullptr;

	VkDevice mDevice = VK_NULL_HANDLE;
};

