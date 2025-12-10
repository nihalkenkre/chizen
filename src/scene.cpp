#include "scene.hpp"
#include "resources.hpp"
#include "vulkan_interface.hpp"
#include "vulkan_objects.hpp"
#include "utils.hpp"

#include <cgltf.h>

WorldScene::WorldScene(const VulkanInterface* vulkan_interface, const VkCommandBuffer cmd_buff, const std::vector<VkDescriptorSetLayout>& desc_set_layouts, const VkQueue queue, const std::string& path)
{
	cgltf_options options = {};
	cgltf_data* gltf = nullptr;

	if (cgltf_parse_file(&options, path.c_str(), &gltf) != cgltf_result_success ||
		cgltf_load_buffers(&options, gltf, path.c_str()) != cgltf_result_success ||
		cgltf_validate(gltf))
	{
		std::println("Could not load GLTF from {}", path);
	}

	VkDevice device = vulkan_interface->GetDevice()->GetDevice();
	VmaAllocator allocator = vulkan_interface->GetAllocator()->GetAllocator();

	for (size_t n = 0; n < gltf->nodes_count; ++n)
	{
		cgltf_node* curr_node = gltf->nodes + n;
		if (curr_node->mesh != nullptr)
		{
			mMeshInstances.push_back(std::make_unique<WorldScene::MeshInstance>(gltf, curr_node, device, allocator, desc_set_layouts[1], cmd_buff, queue));
		}
		else if (curr_node->camera != nullptr)
		{
			mCameraInstances.push_back(std::make_unique<WorldScene::CameraInstance>(gltf, curr_node));
		}
	}

	mMeshes.reserve(gltf->meshes_count);
	for (size_t m = 0; m < gltf->meshes_count; ++m)
	{
		mMeshes.push_back(std::make_unique<WorldScene::Mesh>(gltf, gltf->meshes + m, device, allocator, cmd_buff, queue));
	}

	mCameras.reserve(gltf->cameras_count);
	for (size_t c = 0; c < gltf->cameras_count; ++c)
	{
		mCameras.push_back(std::make_unique<WorldScene::Camera>(gltf, gltf->cameras + c));
	}

	cgltf_free(gltf);

	mViewProjBuffer = std::make_unique<BufferResource>(device, allocator, sizeof(glm::mat4) * 2,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
		VMA_MEMORY_USAGE_AUTO_PREFER_HOST, "view proj buffer");

	if (mCameraInstances.size() == 0)
	{
		mCameraInstances.push_back(std::make_unique<WorldScene::CameraInstance>());
		mCameras.push_back(std::make_unique<WorldScene::Camera>());
	}

	const VkDescriptorPoolSize pool_sizes[] = {
		{
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
		},
	};

	const VkDescriptorPoolCreateInfo dsp_ci = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = 1,
		.poolSizeCount = std::size(pool_sizes),
		.pPoolSizes = pool_sizes,
	};

	VK_CHECK("create view proj desc pool", vkCreateDescriptorPool(device, &dsp_ci, nullptr, &view_proj_desc_pool));

	const VkDescriptorSetAllocateInfo ds_ai = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = view_proj_desc_pool,
		.descriptorSetCount = 1,
		.pSetLayouts = &desc_set_layouts[0],
	};

	VK_CHECK("alloc view proj desc set", vkAllocateDescriptorSets(device, &ds_ai, &view_proj_desc_set));

	VkDescriptorBufferInfo view_proj_buff_info = mViewProjBuffer->GetDescriptorInfo();
	const VkWriteDescriptorSet view_proj_desc_write = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = view_proj_desc_set,
		.dstBinding = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.pBufferInfo = &view_proj_buff_info,
	};

	vkUpdateDescriptorSets(device, 1, &view_proj_desc_write, 0, nullptr);

#ifdef _DEBUG
	Utils_SetObjectName(device, VK_OBJECT_TYPE_DESCRIPTOR_POOL, reinterpret_cast<uint64_t>(view_proj_desc_pool), "view proj desc pool");
	Utils_SetObjectName(device, VK_OBJECT_TYPE_DESCRIPTOR_SET, reinterpret_cast<uint64_t>(view_proj_desc_set), "view proj desc set");
#endif // _DEBUG
}

WorldScene::~WorldScene() noexcept
{
	if (mDevice != VK_NULL_HANDLE)
		vkDestroyDescriptorPool(mDevice, view_proj_desc_pool, nullptr);
}

void WorldScene::Render(const VkDevice device, const VkCommandBuffer cmd_buff, const VkPipelineLayout pipeline_layout, const uint32_t cam_index) const
{
	glm::mat4 view = mCameraInstances[cam_index]->GetViewMatrix();
	std::memcpy(
		mViewProjBuffer->GetAllocationInfo2().allocationInfo.pMappedData,
		&view,
		sizeof(glm::mat4)
	);

	glm::mat4 proj = mCameras[mCameraInstances[cam_index]->GetCameraIndex()]->GetProjectionMatrix();
	proj[1][1] *= -1;
	std::memcpy(
		reinterpret_cast<uint8_t*>(mViewProjBuffer->GetAllocationInfo2().allocationInfo.pMappedData) + sizeof(glm::mat4),
		&proj,
		sizeof(glm::mat4)
	);

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
		const VkDescriptorSet model_desc_set = mesh_instance->GetDescriptorSet();

		const VkBindDescriptorSetsInfoKHR bind_ds_info = {
			.sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO_KHR,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.layout = pipeline_layout,
			.firstSet = 1,
			.descriptorSetCount = 1,
			.pDescriptorSets = &model_desc_set,
		};

		vkCmdBindDescriptorSets2KHR(cmd_buff, &bind_ds_info);

		const std::vector<std::unique_ptr<WorldScene::Mesh::Primitive>>& curr_prims = mMeshes[mesh_instance->GetMeshIndex()]->GetPrimitives();

		for (const auto& curr_prim : curr_prims)
		{
			const VkBuffer buffers[] = {
				curr_prim->GetPositionsBuffer()->GetDescriptorInfo().buffer,
				curr_prim->GetTexCoordsBuffer()->GetDescriptorInfo().buffer,
				curr_prim->GetNormalsBuffer()->GetDescriptorInfo().buffer,
			};

			const VkDeviceSize offsets[] = { 0,0,0 };

			vkCmdBindVertexBuffers2EXT(cmd_buff, 0, 3, buffers, offsets, nullptr, nullptr);
			vkCmdBindIndexBuffer2KHR(cmd_buff,
				curr_prim->GetIndicesBuffer()->GetDescriptorInfo().buffer, 0,
				curr_prim->GetIndicesBuffer()->GetBufferSize(),
				curr_prim->GetIndexType()
			);
			vkCmdDrawIndexed(cmd_buff, curr_prim->GetIndicesCount(), 1, 0, 0, 0);
		}
	}
}

WorldScene::MeshInstance::MeshInstance(const cgltf_data* gltf, const cgltf_node* node, const VkDevice device, const VmaAllocator allocator, const VkDescriptorSetLayout desc_set_layout, const VkCommandBuffer cmd_buff, const VkQueue queue) : mDevice(device)
{
	mMeshIndex = static_cast<uint32_t>(cgltf_mesh_index(gltf, node->mesh));

	if (node->has_matrix)
	{
		mTransformMatrix = glm::make_mat4(node->matrix);
	}
	else
	{
		if (node->has_translation)
		{
			mTransformMatrix = glm::translate(mTransformMatrix, glm::make_vec3(node->translation));
		}

		if (node->has_rotation)
		{
			auto rot_quat = glm::make_quat(node->rotation);
			auto axis = glm::axis(rot_quat);
			mTransformMatrix = glm::rotate(mTransformMatrix, glm::angle(rot_quat), axis);
		}

		if (node->has_scale)
		{
			mTransformMatrix = glm::scale(mTransformMatrix, glm::make_vec3(node->scale));
		}
	}

	mModelMatrixBuffer = std::make_unique<BufferResource>(device, allocator, sizeof(mTransformMatrix),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, std::string(node->name == nullptr ? "node" : node->name).append(" xform buffer"));
	auto staging_buffer = std::make_unique<BufferResource>(
		device,
		allocator,
		sizeof(glm::mat4), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
		VMA_MEMORY_USAGE_AUTO_PREFER_HOST, std::string("instance staging buffer"));
	std::memcpy(
		staging_buffer->GetAllocationInfo2().allocationInfo.pMappedData,
		&mTransformMatrix,
		sizeof(mTransformMatrix)
	);

	staging_buffer->CopyToBuffer(cmd_buff, queue, mModelMatrixBuffer->GetDescriptorInfo().buffer, sizeof(glm::mat4));

	const VkDescriptorPoolSize pool_sizes[] = {
		{
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
		},
	};

	const VkDescriptorPoolCreateInfo dsp_ci = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = 1,
		.poolSizeCount = std::size(pool_sizes),
		.pPoolSizes = pool_sizes,
	};

	VK_CHECK("mesh instance desc pool", vkCreateDescriptorPool(device, &dsp_ci, nullptr, &mDescriptorPool));

	const VkDescriptorSetAllocateInfo ds_ai = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = mDescriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = &desc_set_layout,
	};

	VK_CHECK("mesh instance desc set", vkAllocateDescriptorSets(device, &ds_ai, &mDescriptorSet));

	VkDescriptorBufferInfo model_mat_buff = mModelMatrixBuffer->GetDescriptorInfo();
	const VkWriteDescriptorSet desc_write = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = mDescriptorSet,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.pBufferInfo = &model_mat_buff,
	};

	vkUpdateDescriptorSets(mDevice, 1, &desc_write, 0, nullptr);

#ifdef _DEBUG
	Utils_SetObjectName(device, VK_OBJECT_TYPE_DESCRIPTOR_POOL, reinterpret_cast<uint64_t>(mDescriptorPool), std::string(node->name == nullptr ? "node" : node->name).append(" descriptor pool"));
	Utils_SetObjectName(device, VK_OBJECT_TYPE_DESCRIPTOR_SET, reinterpret_cast<uint64_t>(mDescriptorSet), std::string(node->name == nullptr ? "node" : node->name).append(" descriptor set"));
#endif // _DEBUG
}

WorldScene::MeshInstance::~MeshInstance() noexcept
{
	if (mDevice != VK_NULL_HANDLE)
		vkDestroyDescriptorPool(mDevice, mDescriptorPool, nullptr);
}

uint32_t WorldScene::MeshInstance::GetMeshIndex() const
{
	return mMeshIndex;
}

glm::mat4 WorldScene::MeshInstance::GetTransformMatrix() const
{
	return mTransformMatrix;
}

BufferResource* WorldScene::MeshInstance::GetTransformMatrixBufferResource() const
{
	return mModelMatrixBuffer.get();
}

VkDescriptorSet WorldScene::MeshInstance::GetDescriptorSet() const
{
	return mDescriptorSet;
}

WorldScene::Mesh::Mesh(const cgltf_data* gltf, const cgltf_mesh* mesh, const VkDevice device, const VmaAllocator allocator, const VkCommandBuffer cmd_buff, const VkQueue queue)
{
	mPrimitives.reserve(mesh->primitives_count);

	for (size_t p = 0; p < mesh->primitives_count; ++p)
	{
		mPrimitives.push_back(std::make_unique<WorldScene::Mesh::Primitive>(gltf, mesh->primitives + p, device, allocator, cmd_buff, queue, std::string(mesh->name == nullptr ? "mesh" : mesh->name).append(" prim ").append(std::to_string(p))));
	}
}

WorldScene::Mesh::~Mesh() noexcept
{
}

const std::vector<std::unique_ptr<WorldScene::Mesh::Primitive>>& WorldScene::Mesh::GetPrimitives() const
{
	return mPrimitives;
}

WorldScene::Mesh::Primitive::Primitive(const cgltf_data* gltf, const cgltf_primitive* primitive, const VkDevice device, const VmaAllocator allocator, const VkCommandBuffer cmd_buff, const VkQueue queue, const std::string& name)
{
	for (size_t a = 0; a < primitive->attributes_count; ++a)
	{
		cgltf_attribute* curr_attr = primitive->attributes + a;

		if (std::string(curr_attr->name) == std::string("POSITION"))
		{
			mPositionsBuffer = std::make_unique<BufferResource>(device, allocator, curr_attr->data->buffer_view->size,
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 0,
				VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, std::string(name).append(" positions buffer"));
			auto staging_buffer = std::make_unique<BufferResource>(
				device,
				allocator,
				curr_attr->data->buffer_view->size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
				VMA_MEMORY_USAGE_AUTO_PREFER_HOST, std::string("positions staging buffer"));
			std::memcpy(
				staging_buffer->GetAllocationInfo2().allocationInfo.pMappedData,
				reinterpret_cast<uint8_t*>(curr_attr->data->buffer_view->buffer->data) + curr_attr->data->buffer_view->offset + curr_attr->data->offset,
				curr_attr->data->buffer_view->size
			);
			staging_buffer->CopyToBuffer(cmd_buff, queue, mPositionsBuffer->GetDescriptorInfo().buffer, curr_attr->data->buffer_view->size);
		}
		else if (std::string(curr_attr->name) == std::string("TEXCOORD_0"))
		{
			mTexCoordsBuffer = std::make_unique<BufferResource>(device, allocator, curr_attr->data->buffer_view->size,
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 0,
				VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, std::string(name).append(" texcoords buffer"));
			auto staging_buffer = std::make_unique<BufferResource>(
				device,
				allocator,
				curr_attr->data->buffer_view->size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
				VMA_MEMORY_USAGE_AUTO_PREFER_HOST, std::string("texcoords staging buffer"));
			std::memcpy(
				staging_buffer->GetAllocationInfo2().allocationInfo.pMappedData,
				reinterpret_cast<uint8_t*>(curr_attr->data->buffer_view->buffer->data) + curr_attr->data->buffer_view->offset + curr_attr->data->offset,
				curr_attr->data->buffer_view->size
			);
			staging_buffer->CopyToBuffer(cmd_buff, queue, mTexCoordsBuffer->GetDescriptorInfo().buffer, curr_attr->data->buffer_view->size);
		}
		else if (std::string(curr_attr->name) == std::string("NORMAL"))
		{
			mNormalsBuffer = std::make_unique<BufferResource>(device, allocator, curr_attr->data->buffer_view->size,
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 0,
				VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, std::string(name).append(" normals buffer"));
			auto staging_buffer = std::make_unique<BufferResource>(
				device,
				allocator,
				curr_attr->data->buffer_view->size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
				VMA_MEMORY_USAGE_AUTO_PREFER_HOST, std::string("normals staging buffer"));
			std::memcpy(
				staging_buffer->GetAllocationInfo2().allocationInfo.pMappedData,
				reinterpret_cast<uint8_t*>(curr_attr->data->buffer_view->buffer->data) + curr_attr->data->buffer_view->offset + curr_attr->data->offset,
				curr_attr->data->buffer_view->size
			);
			staging_buffer->CopyToBuffer(cmd_buff, queue, mNormalsBuffer->GetDescriptorInfo().buffer, curr_attr->data->buffer_view->size);
		}
	}

	mIndicesCount = static_cast<uint32_t>(primitive->indices->count);

	if (primitive->indices->component_type == cgltf_component_type_r_32u)
	{
		mIndexType = VK_INDEX_TYPE_UINT32;
	}

	{
		mIndicesBuffer = std::make_unique<BufferResource>(device, allocator, primitive->indices->buffer_view->size,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 0,
			VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, std::string(name).append(" indices buffer"));
		auto staging_buffer = std::make_unique<BufferResource>(
			device,
			allocator,
			primitive->indices->buffer_view->size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
			VMA_MEMORY_USAGE_AUTO_PREFER_HOST, std::string("indices staging buffer"));
		std::memcpy(
			staging_buffer->GetAllocationInfo2().allocationInfo.pMappedData,
			reinterpret_cast<uint8_t*>(primitive->indices->buffer_view->buffer->data) + primitive->indices->buffer_view->offset + primitive->indices->offset,
			primitive->indices->buffer_view->size
		);
		staging_buffer->CopyToBuffer(cmd_buff, queue, mIndicesBuffer->GetDescriptorInfo().buffer, primitive->indices->buffer_view->size);
	}
}

WorldScene::Mesh::Primitive::~Primitive() noexcept
{
}

BufferResource* WorldScene::Mesh::Primitive::GetPositionsBuffer() const
{
	return mPositionsBuffer.get();
}

BufferResource* WorldScene::Mesh::Primitive::GetTexCoordsBuffer() const
{
	return mTexCoordsBuffer.get();
}

BufferResource* WorldScene::Mesh::Primitive::GetNormalsBuffer() const
{
	return mNormalsBuffer.get();
}

BufferResource* WorldScene::Mesh::Primitive::GetIndicesBuffer() const
{
	return mIndicesBuffer.get();
}

VkIndexType WorldScene::Mesh::Primitive::GetIndexType() const
{
	return mIndexType;
}

uint32_t WorldScene::Mesh::Primitive::GetIndicesCount() const
{
	return mIndicesCount;
}

VkDescriptorSet WorldScene::Mesh::Primitive::GetDescriptorSet() const
{
	return mDescriptorSet;
}

VkDescriptorPool WorldScene::Mesh::Primitive::GetDescriptorPool() const
{
	return mDescriptorPool;
}

WorldScene::CameraInstance::CameraInstance()
{
	mViewMatrix = glm::lookAt(glm::vec3(-1, 2, 15), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	mCameraIndex = 0;
}

WorldScene::CameraInstance::CameraInstance(const cgltf_data* gltf, const cgltf_node* node)
{
	mCameraIndex = static_cast<uint32_t>(cgltf_camera_index(gltf, node->camera));

	if (node->has_matrix)
	{
		mViewMatrix = glm::make_mat4(node->matrix);
	}
	else
	{
		if (node->has_translation)
		{
			mViewMatrix = glm::translate(mViewMatrix, glm::make_vec3(node->translation));
		}

		if (node->has_rotation)
		{
			auto rot_quat = glm::make_quat(node->rotation);
			auto axis = glm::axis(rot_quat);
			mViewMatrix = glm::rotate(mViewMatrix, glm::angle(rot_quat), axis);
		}

		if (node->has_scale)
		{
			mViewMatrix = glm::scale(mViewMatrix, glm::make_vec3(node->scale));
		}
	}
}

uint32_t WorldScene::CameraInstance::GetCameraIndex() const
{
	return mCameraIndex;
}

glm::mat4 WorldScene::CameraInstance::GetViewMatrix() const
{
	return mViewMatrix;
}

WorldScene::Camera::Camera()
{
	mProjectionMatrix = glm::perspective(glm::radians(60.f), 1.777f, 0.001f, 1000.f);
}

WorldScene::Camera::Camera(const cgltf_data* gltf, const cgltf_camera* camera)
{
	if (camera->type == cgltf_camera_type_perspective)
	{
		cgltf_camera_perspective persp_data = camera->data.perspective;
		mProjectionMatrix = glm::perspective(
			persp_data.yfov,
			persp_data.has_aspect_ratio ? persp_data.aspect_ratio : 1.777f,
			persp_data.znear,
			persp_data.zfar
		);
	}
	else if (camera->type == cgltf_camera_type_orthographic)
	{
		cgltf_camera_orthographic ortho_data = camera->data.orthographic;
		mProjectionMatrix = glm::ortho(
			-ortho_data.xmag / 2.f, ortho_data.xmag / 2.f,
			-ortho_data.ymag / 2.f, ortho_data.ymag / 2.f,
			ortho_data.znear, ortho_data.zfar);
	}
}

glm::mat4 WorldScene::Camera::GetProjectionMatrix() const
{
	return mProjectionMatrix;
}
