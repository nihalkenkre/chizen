#include "viewport_scene.hpp"
#include "utils.hpp"
#include "resources.hpp"
#include "vulkan_objects.hpp"
#include "common.hpp"

#include <stb_image.h>

ViewportWorldScene::ViewportWorldScene(const Scene& scene, const VkDevice device, const VmaAllocator allocator, const std::vector<VkDescriptorSetLayout>& desc_set_layouts, const std::vector<uint32_t>& queue_family_indices, TransferHelpers* transfer_helpers)
	: mDevice(device)
{
	auto wait_and_delete = [device, transfer_helpers](HostBufferResource* hbr) {
		VkSemaphore sem = transfer_helpers->GetSemaphore();
		const uint64_t sem_value = transfer_helpers->GetSemaphoreValueConst();

		const VkSemaphoreWaitInfo wait_info = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
			.semaphoreCount = 1,
			.pSemaphores = &sem,
			.pValues = &sem_value,
		};
		VK_CHECK("wait for sem", vkWaitSemaphoresKHR(device, &wait_info, UINT64_MAX));

		hbr->~HostBufferResource();
		};

	auto vertex_data = scene.GetVertexData();

	mPositionsData = std::make_unique<DeviceBufferResource>(
		device, allocator,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		vertex_data.size(), "scene vertex data");

	std::unique_ptr<HostBufferResource, decltype(wait_and_delete)> staging_vertex_data(new HostBufferResource(
		device, allocator,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		vertex_data, "staging vertex data"), wait_and_delete
	);

	auto uniform_data = scene.GetUniformData();

	mUniformData = std::make_unique<DeviceBufferResource>(
		device, allocator,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		uniform_data.size(), "scene uniform data");

	std::unique_ptr<HostBufferResource, decltype(wait_and_delete)> staging_uniform_data(new HostBufferResource(
		device, allocator,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		uniform_data, "staging uniform data"), wait_and_delete
	);

	transfer_helpers->BeginBatch();
	transfer_helpers->CopyBufferToBuffer(staging_vertex_data->GetVkBuffer(), mPositionsData->GetVkBuffer(), vertex_data.size());
	transfer_helpers->CopyBufferToBuffer(staging_uniform_data->GetVkBuffer(), mUniformData->GetVkBuffer(), uniform_data.size());
	transfer_helpers->EndBatch();

	VkDescriptorPoolSize view_proj_desc_size = {
		.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = static_cast<uint32_t>(scene.GetCameraInstances().size()),
	};
	VkDescriptorPoolSize model_mat_desc_size = {
		.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = static_cast<uint32_t>(scene.GetMeshInstances().size()),
	};

	uint32_t desc_count = 0;
	for (const auto& mesh : scene.GetMeshes())
	{
		for (const auto& prim : mesh.GetPrimitives())
		{
			desc_count += 3; // diffuse_col, diffuse_tex and normal tex
		}
	}
	VkDescriptorPoolSize model_tex_desc_size = {
		.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = desc_count,
	};

	const VkDescriptorPoolSize pool_sizes[] = {
		view_proj_desc_size,
		model_mat_desc_size,
		model_tex_desc_size,
	};

	const VkDescriptorPoolCreateInfo dsp_ci = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = view_proj_desc_size.descriptorCount + model_mat_desc_size.descriptorCount + model_tex_desc_size.descriptorCount,
		.poolSizeCount = std::size(pool_sizes),
		.pPoolSizes = pool_sizes,
	};

	VK_CHECK("create descriptor pool", vkCreateDescriptorPool(mDevice, &dsp_ci, nullptr, &mDescriptorPool));

#ifdef _DEBUG
	Utils_SetObjectName(mDevice, VK_OBJECT_TYPE_DESCRIPTOR_POOL, reinterpret_cast<uint64_t>(mDescriptorPool), "scene descriptor pool");
#endif // _DEBUG

	mCameraInstances.reserve(scene.GetCameraInstances().size());
	for (const auto& cam_instance : scene.GetCameraInstances())
	{
		mCameraInstances.push_back(ViewportWorldScene::CameraInstance(cam_instance, mUniformData.get(), device, mDescriptorPool, desc_set_layouts[0]));
	}

	mCameras.reserve(scene.GetCameras().size());
	for (const auto& cam : scene.GetCameras())
	{
		mCameras.push_back(ViewportWorldScene::Camera(cam, mUniformData.get()));
	}

	mMeshInstances.reserve(scene.GetMeshInstances().size());
	for (const auto& mesh_instance : scene.GetMeshInstances())
	{
		mMeshInstances.push_back(ViewportWorldScene::MeshInstance(mesh_instance, mUniformData.get(), device, mDescriptorPool, desc_set_layouts[1]));
	}

	mImages.reserve(scene.GetImages().size());
	for (const auto& image : scene.GetImages())
	{
		mImages.push_back(ViewportWorldScene::Image(image, scene.GetImagesData(), device, allocator, queue_family_indices, transfer_helpers));
	}

	const VkSamplerCreateInfo s_ci = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
	};

	VK_CHECK("create null sampler", vkCreateSampler(device, &s_ci, nullptr, &mNullSampler));

	mMeshes.reserve(scene.GetMeshes().size());
	for (const auto& mesh : scene.GetMeshes())
	{
		mMeshes.push_back(ViewportWorldScene::Mesh(mesh, mImages, device, allocator, mDescriptorPool, desc_set_layouts[2],
			queue_family_indices, mNullSampler, transfer_helpers
		));
	}
}

void ViewportWorldScene::Render(const VkCommandBuffer cmd_buff, const VkPipelineLayout pipeline_layout, const uint32_t cam_index) const
{
	const VkDescriptorSet view_proj_desc_set = mCameraInstances[cam_index].GetViewProjDescSet();
	const VkBindDescriptorSetsInfoKHR bind_ds_info = {
		.sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO_KHR,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.layout = pipeline_layout,
		.descriptorSetCount = 1,
		.pDescriptorSets = &view_proj_desc_set,
	};

	vkCmdBindDescriptorSets2KHR(cmd_buff, &bind_ds_info);

	for (const auto& mesh_instance : mMeshInstances)
	{
		const VkDescriptorSet mesh_desc_set = mesh_instance.GetModelMatDescSet();

		const VkBindDescriptorSetsInfoKHR bind_ds_info = {
			.sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO_KHR,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.layout = pipeline_layout,
			.firstSet = 1,
			.descriptorSetCount = 1,
			.pDescriptorSets = &mesh_desc_set,
		};

		vkCmdBindDescriptorSets2KHR(cmd_buff, &bind_ds_info);

		const auto& curr_prims = mMeshes[mesh_instance.GetMeshIndex()].GetPrimitives();

		for (const auto& curr_prim : curr_prims)
		{
			const VkDescriptorSet tex_desc_set = curr_prim.GetTexDescSet();
			const VkBindDescriptorSetsInfoKHR bind_ds_info = {
				.sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO_KHR,
				.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
				.layout = pipeline_layout,
				.firstSet = 2,
				.descriptorSetCount = 1,
				.pDescriptorSets = &tex_desc_set,
			};

			vkCmdBindDescriptorSets2KHR(cmd_buff, &bind_ds_info);

			const VkBuffer buffers[] = {
				mPositionsData->GetVkBuffer(),
				mPositionsData->GetVkBuffer(),
				mPositionsData->GetVkBuffer(),
			};

			const VkDeviceSize offsets[] = {
				curr_prim.GetPositionsOffset(),
				curr_prim.GetNormalsOffset(),
				curr_prim.GetTexcoordsOffset(),
			};

			vkCmdBindVertexBuffers2EXT(cmd_buff, 0, std::size(buffers), buffers, offsets, nullptr, nullptr);

			if (curr_prim.GetIndexCount() != 0)
			{
				vkCmdBindIndexBuffer2KHR(cmd_buff,
					mPositionsData->GetVkBuffer(),
					curr_prim.GetIndicesOffset(),
					curr_prim.GetIndicesSize(),
					curr_prim.GetIndexType()
				);

				vkCmdDrawIndexed(cmd_buff, static_cast<uint32_t>(curr_prim.GetIndexCount()), 1, 0, 0, 0);
			}
			else
			{
				vkCmdDraw(cmd_buff, static_cast<uint32_t>(curr_prim.GetVertexCount()), 1, 0, 0);
			}
		}
	}
}

ViewportWorldScene::~ViewportWorldScene() noexcept
{
	if (mDevice != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(mDevice, mDescriptorPool, nullptr);
		vkDestroySampler(mDevice, mNullSampler, nullptr);
	}
}

ViewportWorldScene::MeshInstance::MeshInstance(const Scene::MeshInstance& mesh_instance, const DeviceBufferResource* scene_data, const VkDevice device, const VkDescriptorPool desc_pool, const VkDescriptorSetLayout desc_set_layout)
	: Scene::MeshInstance(mesh_instance)
{
	const VkDescriptorSetAllocateInfo mesh_ds_ai = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = desc_pool,
		.descriptorSetCount = 1,
		.pSetLayouts = &desc_set_layout,
	};

	VK_CHECK("allocate desc set", vkAllocateDescriptorSets(device, &mesh_ds_ai, &mModelMatDescSet));

#ifdef _DEBUG
	Utils_SetObjectName(device, VK_OBJECT_TYPE_DESCRIPTOR_SET, reinterpret_cast<uint64_t>(mModelMatDescSet), "mesh desc set");
#endif // _DEBUG

	const VkDescriptorBufferInfo desc_info = {
		.buffer = scene_data->GetDescriptorInfo().buffer,
		.offset = mesh_instance.GetModelMatrixOffset(),
		.range = sizeof(glm::mat4),
	};

	const VkWriteDescriptorSet write_desc_set = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = mModelMatDescSet,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.pBufferInfo = &desc_info,
	};

	vkUpdateDescriptorSets(device, 1, &write_desc_set, 0, nullptr);
}

ViewportWorldScene::MeshInstance::~MeshInstance() noexcept
{
}

VkDescriptorSet ViewportWorldScene::MeshInstance::GetModelMatDescSet() const
{
	return mModelMatDescSet;
}

ViewportWorldScene::Mesh::Mesh(const Scene::Mesh& mesh, const std::vector<ViewportWorldScene::Image>& images,
	const VkDevice device, const VmaAllocator allocator, const VkDescriptorPool desc_pool, const VkDescriptorSetLayout desc_set_layout,
	const std::vector<uint32_t>& queue_family_indices, const VkSampler null_sampler,
	TransferHelpers* transfer_helpers)
	: Scene::Mesh(mesh)
{
	mPrimitives.reserve(mesh.GetPrimitives().size());
	for (const auto& prim : mesh.GetPrimitives())
	{
		mPrimitives.push_back(
			ViewportWorldScene::Mesh::Primitive(
				prim, images, device, allocator, desc_pool, desc_set_layout,
				queue_family_indices, null_sampler, transfer_helpers
			)
		);
	}
}

const std::vector<ViewportWorldScene::Mesh::Primitive>& ViewportWorldScene::Mesh::GetPrimitives() const
{
	return mPrimitives;
}

ViewportWorldScene::Mesh::Primitive::Primitive(
	const Scene::Mesh::Primitive& primitive, const std::vector<ViewportWorldScene::Image>& images,
	const VkDevice device, const VmaAllocator allocator,
	const VkDescriptorPool desc_pool, const VkDescriptorSetLayout desc_set_layout,
	const std::vector<uint32_t>& queue_family_indices,
	const VkSampler null_sampler, TransferHelpers* transfer_helpers
)
	: Scene::Mesh::Primitive(primitive)
{
	const VkDescriptorSetAllocateInfo ds_ai = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = desc_pool,
		.descriptorSetCount = 1,
		.pSetLayouts = &desc_set_layout,
	};

	VK_CHECK("allocate ds", vkAllocateDescriptorSets(device, &ds_ai, &mTexsDescSet));

#ifdef _DEBUG
	Utils_SetObjectName(device, VK_OBJECT_TYPE_DESCRIPTOR_SET, reinterpret_cast<uint64_t>(mTexsDescSet), "prim desc set");
#endif // _DEBUG

	Material material = primitive.GetMaterial();
	int32_t base_img_index = material.GetBaseImageIndex();

	auto wait_and_delete = [device, transfer_helpers](HostBufferResource* hbr) {
		VkSemaphore sem = transfer_helpers->GetSemaphore();
		const uint64_t sem_value = transfer_helpers->GetSemaphoreValueConst();

		const VkSemaphoreWaitInfo wait_info = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
			.semaphoreCount = 1,
			.pSemaphores = &sem,
			.pValues = &sem_value,
		};
		VK_CHECK("wait for sem", vkWaitSemaphoresKHR(device, &wait_info, UINT64_MAX));

		hbr->~HostBufferResource();
	};

	float base_color[4] = {
		material.GetBaseColorFactor().r,
		material.GetBaseColorFactor().g,
		material.GetBaseColorFactor().b,
		material.GetBaseColorFactor().a,
	};
	std::vector<uint8_t> base_color_data(sizeof(float) * 4);
	std::memcpy(base_color_data.data(), base_color, base_color_data.size());

	mBaseColorImage = std::make_unique<ImageResource>(
		device, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		allocator, queue_family_indices, "base color"
	);

	std::unique_ptr<HostBufferResource, decltype(wait_and_delete)> staging_base_color(new HostBufferResource(
		device, allocator, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		base_color_data, "base color staging"), wait_and_delete
	);

	transfer_helpers->BeginBatch();
	transfer_helpers->ChangeImageLayout(
		VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0,
		VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
		VK_IMAGE_ASPECT_COLOR_BIT, mBaseColorImage->GetImage()
	);
	transfer_helpers->CopyBufferToImage(staging_base_color->GetVkBuffer(), mBaseColorImage->GetImage(), VkExtent2D{ 1,1 });
	transfer_helpers->EndBatch();

	const VkDescriptorImageInfo desc_img_info = mBaseColorImage->GetDescriptorInfo();
	const VkWriteDescriptorSet write_desc_set = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = mTexsDescSet,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = &desc_img_info,
	};

	vkUpdateDescriptorSets(device, 1, &write_desc_set, 0, nullptr);

	if (base_img_index >= 0)
	{
		const VkDescriptorImageInfo desc_img_info = images[base_img_index].GetImageResource()->GetDescriptorInfo();
		const VkWriteDescriptorSet write_desc_set = {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = mTexsDescSet,
				.dstBinding = 1,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo = &desc_img_info
		};
		vkUpdateDescriptorSets(device, 1, &write_desc_set, 0, nullptr);
	}
	else
	{
		const VkDescriptorImageInfo desc_img_info = {
			.sampler = null_sampler,
		};
		const VkWriteDescriptorSet write_desc_set = {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = mTexsDescSet,
				.dstBinding = 1,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo = &desc_img_info,
		};
		vkUpdateDescriptorSets(device, 1, &write_desc_set, 0, nullptr);
	}

	int32_t norm_img_index = material.GetNormalImageIndex();
	if (norm_img_index >= 0)
	{
		const VkDescriptorImageInfo desc_img_info = images[norm_img_index].GetImageResource()->GetDescriptorInfo();
		const VkWriteDescriptorSet write_desc_set = {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = mTexsDescSet,
				.dstBinding = 2,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo = &desc_img_info
		};
		vkUpdateDescriptorSets(device, 1, &write_desc_set, 0, nullptr);
	}
	else
	{
		const VkDescriptorImageInfo desc_img_info = {
			.sampler = null_sampler,
		};
		const VkWriteDescriptorSet write_desc_set = {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = mTexsDescSet,
				.dstBinding = 2,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo = &desc_img_info,
		};
		vkUpdateDescriptorSets(device, 1, &write_desc_set, 0, nullptr);
	}
}

VkDescriptorSet ViewportWorldScene::Mesh::Primitive::GetTexDescSet() const
{
	return mTexsDescSet;
}

ViewportWorldScene::CameraInstance::CameraInstance(const Scene::CameraInstance& camera_instance, const DeviceBufferResource* scene_data, const VkDevice device, const VkDescriptorPool desc_pool, const VkDescriptorSetLayout desc_set_layout)
	: Scene::CameraInstance(camera_instance)
{
	const VkDescriptorSetAllocateInfo view_proj_ds_ai = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = desc_pool,
		.descriptorSetCount = 1,
		.pSetLayouts = &desc_set_layout,
	};

	VK_CHECK("allocate desc set", vkAllocateDescriptorSets(device, &view_proj_ds_ai, &mViewProjDescSet));

	const VkDescriptorBufferInfo desc_info = {
		.buffer = scene_data->GetDescriptorInfo().buffer,
		.offset = camera_instance.GetViewMatrixOffset(),
		.range = sizeof(glm::mat4) * 2,
	};

	const VkWriteDescriptorSet write_desc_set = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = mViewProjDescSet,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.pBufferInfo = &desc_info,
	};

	vkUpdateDescriptorSets(device, 1, &write_desc_set, 0, nullptr);
}

VkDescriptorSet ViewportWorldScene::CameraInstance::GetViewProjDescSet() const
{
	return mViewProjDescSet;
}

ViewportWorldScene::Camera::Camera(const Scene::Camera& camera, const DeviceBufferResource* scene_data)
	: Scene::Camera(camera)
{
}

ViewportWorldScene::Image::Image(const Scene::Image& image, const std::vector<uint8_t>& images_data, const VkDevice device, const VmaAllocator allocator, const std::vector<uint32_t>& queue_family_indices, TransferHelpers* transfer_helpers)
{
	uint32_t w, h, c;
	uint8_t* pixels = stbi_load_from_memory(images_data.data() + image.GetDataOffset(), static_cast<int>(image.GetDataSize()),
		reinterpret_cast<int*>(&w), reinterpret_cast<int*>(&h), reinterpret_cast<int*>(&c), 4);

	VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
	if (image.GetName().contains("normal") || image.GetName().contains("NRM") || image.GetName().contains("nrm"))
	{
		format = VK_FORMAT_R8G8B8A8_SNORM;
	}

	mImageResource = std::make_unique<ImageResource>(
		device, VkExtent3D{ w, h, 1 }, format, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		allocator,
		queue_family_indices,
		"texture"
	);

	auto wait_and_delete = [device, transfer_helpers](HostBufferResource* hbr) {
		VkSemaphore sem = transfer_helpers->GetSemaphore();
		const uint64_t sem_value = transfer_helpers->GetSemaphoreValueConst();

		const VkSemaphoreWaitInfo wait_info = {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
			.semaphoreCount = 1,
			.pSemaphores = &sem,
			.pValues = &sem_value,
		};
		VK_CHECK("wait for sem", vkWaitSemaphoresKHR(device, &wait_info, UINT64_MAX));

		hbr->~HostBufferResource();
		};

	std::unique_ptr<HostBufferResource, decltype(wait_and_delete)> staging_buffer(new HostBufferResource(
		device, allocator, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		mImageResource->GetAllocationInfo2().allocationInfo.size, "texture staging"), wait_and_delete);
	std::memcpy(
		staging_buffer->GetAllocationInfo2().allocationInfo.pMappedData,
		pixels,
		w * h * 4
	);

	transfer_helpers->BeginBatch();
	transfer_helpers->ChangeImageLayout(
		VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0,
		VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
		VK_IMAGE_ASPECT_COLOR_BIT, mImageResource->GetImage()
	);
	transfer_helpers->CopyBufferToImage(staging_buffer->GetVkBuffer(), mImageResource->GetImage(), VkExtent2D{ w,h });
	transfer_helpers->EndBatch();
}

ImageResource* ViewportWorldScene::Image::GetImageResource() const
{
	return mImageResource.get();
}
