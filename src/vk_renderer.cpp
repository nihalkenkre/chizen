#include "vk_renderer.hpp"

#include <iostream>
#include <meshoptimizer/src/meshoptimizer.h>
#include <cglm/include/cglm/cglm.h>

#define MAX_VERTICES 64
#define MAX_TRIANGLES 124

inline static VkViewport RECT_TO_VIEWPORT(const RECT& rect)
{
	const VkViewport v = {
		.x = 0,
		.y = 0,
		.width = static_cast<float>(rect.right - rect.left),
		.height = static_cast<float>(rect.bottom - rect.top),
		.minDepth = 0,
		.maxDepth = 1,
	};

	return v;
}

inline static RECT VIEWPORT_TO_RECT(const VkViewport& viewport)
{
	const RECT r = {
		.left = static_cast<LONG>(viewport.x),
		.top = static_cast<LONG>(viewport.y),
		.right = static_cast<LONG>(viewport.width),
		.bottom = static_cast<LONG>(viewport.height),
	};

	return r;
}

inline static RECT SANITIZE_RECT_FOR_RENDER(const RECT& rect)
{
	const RECT r = {
		.left = 0,
		.top = 0,
		.right = rect.right - rect.left,
		.bottom = rect.bottom - rect.top,
	};

	return r;
}

vk_renderer::vk_renderer(const HWND h_wnd) : img_idx(0), acq_wait_sem_val(0)
{
	VkResult result = volkInitialize();
	instance = std::make_unique<vk_instance>();
	volkLoadInstance(instance->instance);
	surface = std::make_unique<vk_surface>(instance->instance, GetModuleHandleA(nullptr), h_wnd);
	phy_dev = std::make_unique<vk_phydev>(instance->instance, surface.get());
	device = std::make_unique<vk_device>(phy_dev->phy_dev, phy_dev->q_fly_idx, phy_dev->q_count);
	swapchain = std::make_unique<vk_swapchain>(device->device, surface.get(), phy_dev.get());
	acq_sig_sem = std::make_unique<vk_semaphore>(device->device, false);
	acq_wait_sem = std::make_unique<vk_semaphore>(device->device, true);

	const VkSemaphoreSignalInfo sem_sig_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
		.semaphore = acq_wait_sem->semaphore,
		.value = ++acq_wait_sem_val,
	};
	vkSignalSemaphore(device->device, &sem_sig_info);

	GetWindowRect(h_wnd, &wnd_rect);
	wnd_rect = SANITIZE_RECT_FOR_RENDER(wnd_rect);
	viewport = RECT_TO_VIEWPORT(wnd_rect);

	sd = std::make_unique<scene_data>();
}

void vk_renderer::import_scene_data(const cgltf_data* data)
{
	for (cgltf_size n = 0; n < data->nodes_count; ++n)
	{
		cgltf_node* curr_node = data->nodes + n;

		if (curr_node->mesh == nullptr)
			continue;

		cgltf_mesh* curr_mesh = curr_node->mesh;

		for (cgltf_size p = 0; p < curr_mesh->primitives_count; ++p)
		{
			primitive_data pd;
			glm_mat4_identity(pd.xform);

			if (curr_node->has_matrix)
			{
				std::memcpy(pd.xform, curr_node->matrix, sizeof(pd.xform));
			}
			else {
				if (curr_node->has_scale)
				{
					glm_scale(pd.xform, curr_node->scale);
				}

				if (curr_node->has_rotation)
				{
					glm_quat_rotate(pd.xform, curr_node->rotation, pd.xform);
				}

				if (curr_node->has_translation)
				{
					glm_translate(pd.xform, curr_node->translation);
				}
			}

			std::vector<uint32_t>indices;
			std::vector<float3> positions;

			cgltf_primitive* curr_prim = curr_mesh->primitives + p;
			if (curr_prim->material == nullptr)
				continue;

			if (curr_prim->indices->component_type == cgltf_component_type_r_32u)
			{
				size_t curr_ind_size = indices.size();
				indices.resize(indices.size() + curr_prim->indices->count);
				std::memcpy(indices.data() + curr_ind_size, (void*)((ULONG_PTR)curr_prim->indices->buffer_view->buffer->data + curr_prim->indices->offset + curr_prim->indices->buffer_view->offset), curr_prim->indices->buffer_view->size);
			}
			else if (curr_prim->indices->component_type == cgltf_component_type_r_16u)
			{
				indices.reserve(indices.size() + curr_prim->indices->count);
				uint16_t* idx_ptr = (uint16_t*)((ULONG_PTR)curr_prim->indices->buffer_view->buffer->data + curr_prim->indices->offset + curr_prim->indices->buffer_view->offset);

				for (cgltf_size i = 0; i < curr_prim->indices->count; ++i)
				{
					indices.push_back(idx_ptr[i]);
				}
			}

			for (cgltf_size a = 0; a < curr_prim->attributes_count; ++a)
			{
				cgltf_attribute* curr_attr = curr_prim->attributes + a;

				if (std::strcmp(curr_attr->name, "POSITION") == 0)
				{
					size_t curr_geom_size = positions.size();
					positions.resize(positions.size() + curr_attr->data->count);

					std::memcpy(&positions[curr_geom_size], (void*)((ULONG_PTR)curr_attr->data->buffer_view->buffer->data + curr_attr->data->buffer_view->offset + curr_attr->data->offset), curr_attr->data->count * sizeof(float3));
				}
			}

			size_t max_meshlets = meshopt_buildMeshletsBound(indices.size(), MAX_VERTICES, MAX_TRIANGLES);

			std::vector<meshopt_Meshlet> meshlets(max_meshlets);
			std::vector<uint32_t>meshlets_vertices(max_meshlets * MAX_VERTICES);
			std::vector<uint8_t>meshlets_triangles(max_meshlets * MAX_TRIANGLES);

			pd.meshlets_count = meshopt_buildMeshlets(
				meshlets.data(),
				meshlets_vertices.data(),
				meshlets_triangles.data(),
				indices.data(),
				indices.size(),
				reinterpret_cast<float*>(positions.data()),
				positions.size(),
				sizeof(float3),
				MAX_VERTICES,
				MAX_TRIANGLES,
				0.0
			);

			meshopt_Meshlet last_meshlet = meshlets[pd.meshlets_count - 1];
			meshlets_vertices.resize(last_meshlet.vertex_offset + last_meshlet.vertex_count);
			meshlets_triangles.resize(last_meshlet.triangle_offset + ((last_meshlet.triangle_count * 3 + 3) & ~3));
			meshlets.resize(pd.meshlets_count);

			std::vector<uint32_t> meshlet_triangles_32;
			size_t t_32_idx = 0;

			for (auto& meshlet : meshlets)
			{
				uint32_t triangle_offset = static_cast<uint32_t>(meshlet_triangles_32.size());

				for (size_t t = 0; t < meshlet.triangle_count; ++t)
				{
					uint32_t curr_tri = (static_cast<uint32_t>(meshlets_triangles[meshlet.triangle_offset + (t * 3)]) << 0) |
						(static_cast<uint32_t>(meshlets_triangles[meshlet.triangle_offset + (t * 3 + 1)]) << 8) |
						(static_cast<uint32_t>(meshlets_triangles[meshlet.triangle_offset + (t * 3 + 2)]) << 16);

					meshlet_triangles_32.push_back(curr_tri);
				};

				meshlet.triangle_offset = triangle_offset;
			}

			size_t meshlets_data_size = meshlets.size() * sizeof(meshlets[0]);
			std::vector<uint8_t> meshlets_data(meshlets_data_size);
			std::memcpy(meshlets_data.data(), meshlets.data(), meshlets_data_size);

			size_t positions_data_size = positions.size() * sizeof(positions[0]);
			std::vector<uint8_t> positions_data(positions_data_size);
			std::memcpy(positions_data.data(), positions.data(), positions_data_size);

			size_t meshlets_vertices_data_size = meshlets_vertices.size() * sizeof(meshlets_vertices[0]);
			std::vector<uint8_t> meshlets_vertices_data(meshlets_vertices_data_size);
			std::memcpy(meshlets_vertices_data.data(), meshlets_vertices.data(), meshlets_vertices_data_size);

			size_t meshlets_triangles_data_size = meshlet_triangles_32.size() * sizeof(meshlet_triangles_32[0]);
			std::vector<uint8_t> meshlets_triangles_data(meshlets_triangles_data_size);
			std::memcpy(meshlets_triangles_data.data(), meshlet_triangles_32.data(), meshlets_triangles_data_size);

			uint64_t mat_id = std::hash<std::string>{}(curr_prim->material->name);
			auto it = std::find_if(sd->mis.begin(), sd->mis.end(), [&mat_id](const material_info& mi) { return mi.id == mat_id;});

			if (it == sd->mis.end())
			{
				material_info mi = {
					.id = mat_id,
					.pds = {pd},
				};

				sd->mis.push_back(mi);
			}
			else
			{
				it->pds.push_back(pd);
			}
		}
	}
}

void vk_renderer::resize(const UINT width, const UINT height)
{
	VK_CHECK("wait for present fence", vkWaitForFences(device->device, 1, &swapchain->present_fences[img_idx], VK_TRUE, UINT64_MAX));
	VK_CHECK("reset present fence", vkResetFences(device->device, 1, &swapchain->present_fences[img_idx]));

	VK_CHECK("get surface capabilities", vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phy_dev->phy_dev, surface->surface, &surface->surf_caps));

	swapchain.reset();
	swapchain = std::make_unique<vk_swapchain>(device->device, surface.get(), phy_dev.get());
}

void vk_renderer::begin_frame()
{
	uint64_t wait_values = acq_wait_sem_val;

	const VkSemaphoreWaitInfo wait_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
		.semaphoreCount = 1,
		.pSemaphores = &acq_wait_sem->semaphore,
		.pValues = &wait_values,
	};
	vkWaitSemaphores(device->device, &wait_info, UINT64_MAX);

	VK_CHECK("acquire image index", vkAcquireNextImageKHR(device->device, swapchain->swapchain, UINT64_MAX, acq_sig_sem->semaphore, VK_NULL_HANDLE, &img_idx));
	VK_CHECK("reset command buffer", vkResetCommandBuffer(swapchain->cmd_buffs[img_idx], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));

	const VkImageMemoryBarrier2 img_mem_barr2 = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = 0,
		.srcAccessMask = 0,
		.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = swapchain->images[img_idx],
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
		},
	};

	const VkDependencyInfo dep_info = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &img_mem_barr2,
	};

	const VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};

	vkBeginCommandBuffer(swapchain->cmd_buffs[img_idx], &begin_info);
	vkCmdPipelineBarrier2(swapchain->cmd_buffs[img_idx], &dep_info);
}

void vk_renderer::clear_frame(const float color[])
{
	const VkRenderingAttachmentInfoKHR color_attachment_infos[] = {
	{
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView = swapchain->image_views[img_idx],
		.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue = {
			.color = {
				.float32 = {
					color[0], color[1], color[2], color[3]
				},
			},
		},
	},
	};

	const VkRenderingInfoKHR rendering_info = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.renderArea = {
			.extent = surface->surf_caps.currentExtent,
		},
		.layerCount = 1,
		.colorAttachmentCount = _countof(color_attachment_infos),
		.pColorAttachments = color_attachment_infos,
	};

	vkCmdBeginRendering(swapchain->cmd_buffs[img_idx], &rendering_info);
}

void vk_renderer::render_world()
{
}

void vk_renderer::end_frame()
{
	vkCmdEndRendering(swapchain->cmd_buffs[img_idx]);

	const VkImageMemoryBarrier2 img_mem_barr = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
		.dstAccessMask = 0,
		.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = swapchain->images[img_idx],
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
		},
	};

	const VkDependencyInfo dep_info = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &img_mem_barr,
	};

	vkCmdPipelineBarrier2(swapchain->cmd_buffs[img_idx], &dep_info);

	vkEndCommandBuffer(swapchain->cmd_buffs[img_idx]);

	const VkDeviceQueueInfo2 q_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
		.queueFamilyIndex = phy_dev->q_fly_idx,
		.queueIndex = 0,
	};
	VkQueue q = VK_NULL_HANDLE;

	vkGetDeviceQueue2(device->device, &q_info, &q);

	const VkSemaphoreSubmitInfo wait_sem_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = acq_sig_sem->semaphore,
			.stageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
		}
	};

	const VkSemaphoreSubmitInfo sig_sem_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = swapchain->rndr_semaphores[img_idx],
			.stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
		},
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			.semaphore = acq_wait_sem->semaphore,
			.value = ++acq_wait_sem_val,
			.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		}
	};

	const VkCommandBufferSubmitInfo cmd_buff_infos[] = {
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
			.commandBuffer = swapchain->cmd_buffs[img_idx],
		}
	};

	const VkSubmitInfo2 submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.waitSemaphoreInfoCount = _countof(wait_sem_infos),
		.pWaitSemaphoreInfos = wait_sem_infos,
		.commandBufferInfoCount = _countof(cmd_buff_infos),
		.pCommandBufferInfos = cmd_buff_infos,
		.signalSemaphoreInfoCount = _countof(sig_sem_infos),
		.pSignalSemaphoreInfos = sig_sem_infos,
	};

	vkQueueSubmit2(q, 1, &submit_info, VK_NULL_HANDLE);

	const VkSwapchainPresentFenceInfoEXT present_fence_info = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_EXT,
		.swapchainCount = 1,
		.pFences = &swapchain->present_fences[img_idx],
	};

	const VkPresentInfoKHR present_info = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = &present_fence_info,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &swapchain->rndr_semaphores[img_idx],
		.swapchainCount = 1,
		.pSwapchains = &swapchain->swapchain,
		.pImageIndices = &img_idx,
	};

	VK_CHECK("reset present fence", vkResetFences(device->device, 1, &swapchain->present_fences[img_idx]));
	vkQueuePresentKHR(q, &present_info);
}

void vk_renderer::clear_scene_data()
{
	sd = std::make_unique<scene_data>();
}

vk_renderer::~vk_renderer()
{
	VK_CHECK("wait for present fence", vkWaitForFences(device->device, 1, &swapchain->present_fences[img_idx], VK_TRUE, UINT64_MAX));
	VK_CHECK("reset present fence", vkResetFences(device->device, 1, &swapchain->present_fences[img_idx]));
}

std::pair<VkBuffer, VkDeviceMemory> vk_renderer::CreateBufferAndHeapFromMeshletData()
{
	return std::pair<VkBuffer, VkDeviceMemory>();
}
