#include "rasterizer_scene.hpp"
#include "utils.hpp"
#include "resources.hpp"
#include "vulkan_objects.hpp"

RasterizerWorldScene::RasterizerWorldScene(const Scene& scene, const VkDevice device, const VmaAllocator allocator, const std::vector<VkDescriptorSetLayout>& desc_set_layouts, TransferHelpers* transfer_objects)
	: mDevice(device)
{
	auto wait_and_delete = [device, transfer_objects](HostBufferResource* hbr) {
		VkSemaphore sem = transfer_objects->GetSemaphore();
		const uint64_t sem_value = transfer_objects->GetSemaphoreValueConst();

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

	mVertexData = std::make_unique<DeviceBufferResource>(
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

	transfer_objects->BeginBatch();
	transfer_objects->CopyBufferToBuffer(staging_vertex_data->GetVkBuffer(), mVertexData->GetVkBuffer(), vertex_data.size());
	transfer_objects->CopyBufferToBuffer(staging_uniform_data->GetVkBuffer(), mUniformData->GetVkBuffer(), uniform_data.size());
	transfer_objects->EndBatch();

	VkDescriptorPoolSize view_proj_desc_size = {
		.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = static_cast<uint32_t>(scene.GetCameraInstances().size()),
	};
	VkDescriptorPoolSize model_mat_desc_size = {
		.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = static_cast<uint32_t>(scene.GetMeshInstances().size()),
	};

	VkDescriptorPoolSize model_tex_desc_size = {
		.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = static_cast<uint32_t>(scene.GetMeshes().size()), // (number of textures expected for the mesh)
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
		mCameraInstances.push_back(RasterizerWorldScene::CameraInstance(cam_instance, mUniformData.get(), device, mDescriptorPool, desc_set_layouts[0]));
	}

	mCameras.reserve(scene.GetCameras().size());
	for (const auto& cam : scene.GetCameras())
	{
		mCameras.push_back(RasterizerWorldScene::Camera(cam, mUniformData.get()));
	}

	mMeshInstances.reserve(scene.GetMeshInstances().size());
	for (const auto& mesh_instance : scene.GetMeshInstances())
	{
		mMeshInstances.push_back(RasterizerWorldScene::MeshInstance(mesh_instance, mUniformData.get(), device, mDescriptorPool, desc_set_layouts[1]));
	}

	mMeshes.reserve(scene.GetMeshes().size());
	for (const auto& mesh : scene.GetMeshes())
	{
		mMeshes.push_back(RasterizerWorldScene::Mesh(mesh, mVertexData.get()));
	}
}

void RasterizerWorldScene::Render(const VkCommandBuffer cmd_buff, const VkPipelineLayout pipeline_layout, const uint32_t cam_index) const
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
		const VkDescriptorSet model_desc_set = mesh_instance.GetModelMatDescSet();

		const VkBindDescriptorSetsInfoKHR bind_ds_info = {
			.sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO_KHR,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.layout = pipeline_layout,
			.firstSet = 1,
			.descriptorSetCount = 1,
			.pDescriptorSets = &model_desc_set,
		};

		vkCmdBindDescriptorSets2KHR(cmd_buff, &bind_ds_info);

		const auto& curr_prims = mMeshes[mesh_instance.GetMeshIndex()].GetPrimitives();

		for (const auto& curr_prim : curr_prims)
		{
			const VkBuffer buffers[] = {
				mVertexData->GetDescriptorInfo().buffer,
				mVertexData->GetDescriptorInfo().buffer,
				mVertexData->GetDescriptorInfo().buffer,
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
					mVertexData->GetDescriptorInfo().buffer,
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

RasterizerWorldScene::~RasterizerWorldScene() noexcept
{
	if (mDevice != VK_NULL_HANDLE)
		vkDestroyDescriptorPool(mDevice, mDescriptorPool, nullptr);
}

RasterizerWorldScene::MeshInstance::MeshInstance(const Scene::MeshInstance& mesh_instance, const DeviceBufferResource* scene_data, const VkDevice device, const VkDescriptorPool desc_pool, const VkDescriptorSetLayout desc_set_layout)
	: Scene::MeshInstance(mesh_instance)
{
	const VkDescriptorBufferInfo desc_info = {
		.buffer = scene_data->GetDescriptorInfo().buffer,
		.offset = mesh_instance.GetModelMatrixOffset(),
		.range = sizeof(glm::mat4),
	};

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

	const VkWriteDescriptorSet write_desc_set = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = mModelMatDescSet,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.pBufferInfo = &desc_info,
	};

	vkUpdateDescriptorSets(device, 1, &write_desc_set, 0, nullptr);
}

RasterizerWorldScene::MeshInstance::~MeshInstance() noexcept
{
}

VkDescriptorSet RasterizerWorldScene::MeshInstance::GetModelMatDescSet() const
{
	return mModelMatDescSet;
}

RasterizerWorldScene::Mesh::Mesh(const Scene::Mesh& mesh, const DeviceBufferResource* scene_data)
	: Scene::Mesh(mesh)
{
}

RasterizerWorldScene::Mesh::Primitive::Primitive(const Scene::Mesh::Primitive& primitive, const DeviceBufferResource* scene_data)
	: Scene::Mesh::Primitive(primitive)
{
}

RasterizerWorldScene::CameraInstance::CameraInstance(const Scene::CameraInstance& camera_instance, const DeviceBufferResource* scene_data, const VkDevice device, const VkDescriptorPool desc_pool, const VkDescriptorSetLayout desc_set_layout)
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

VkDescriptorSet RasterizerWorldScene::CameraInstance::GetViewProjDescSet() const
{
	return mViewProjDescSet;
}

RasterizerWorldScene::Camera::Camera(const Scene::Camera& camera, const DeviceBufferResource* scene_data)
	: Scene::Camera(camera)
{
}