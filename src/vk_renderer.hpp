#pragma once

#include "renderer.hpp"
#include "vk_objects.hpp"

#include <memory>

#include <cglm/include/cglm/cglm.h>

class vk_renderer : public renderer
{
public:
	vk_renderer(const HWND h_wnd);

	void import_scene_data(const cgltf_data* data) override;
	void resize(const UINT width, const UINT height) override;
	void begin_frame() override;
	void clear_frame(const float color[]) override;
	void render_world() override;
	void end_frame() override;
	void clear_scene_data() override;

	~vk_renderer();

private:
	std::unique_ptr<vk_instance> instance;
	std::unique_ptr<vk_surface> surface;
	std::unique_ptr<vk_phydev> phy_dev;
	std::unique_ptr<vk_device> device;
	std::unique_ptr<vk_swapchain> swapchain;
	std::unique_ptr<vk_semaphore> acq_sig_sem;
	std::unique_ptr<vk_semaphore> acq_wait_sem;

	struct float3
	{
		float x;
		float y;
		float z;
	};

	struct primitive_data {
		mat4 xform;
		std::vector<VkDeviceSize> geom_offsets;
		size_t meshlets_count;
	};

	struct material_info {
		uint64_t id;
		std::vector<primitive_data> pds;
	};

	struct scene_data
	{
		std::vector<material_info> mis;
		VkBuffer geometry_buffer;
		VkDeviceMemory geometry_memory;
	};

	std::unique_ptr<scene_data> sd;

	uint32_t img_idx;
	uint64_t acq_wait_sem_val;
	VkViewport viewport;

	std::pair<VkBuffer, VkDeviceMemory> CreateBufferAndHeapFromMeshletData();
};