#include "scene.hpp"
#include "resources.hpp"
#include "vulkan_interface.hpp"
#include "vulkan_objects.hpp"
#include "utils.hpp"

#include <cgltf.h>
//
//WorldScene::WorldScene(const VulkanInterface* vulkan_interface, const VkCommandBuffer cmd_buff, const std::vector<VkDescriptorSetLayout>& desc_set_layouts, const VkQueue queue, const std::string& path) : mDevice(vulkan_interface->GetDevice()->GetDevice())
//{
//	cgltf_options options = {};
//	cgltf_data* gltf = nullptr;
//
//	if (cgltf_parse_file(&options, path.c_str(), &gltf) != cgltf_result_success ||
//		cgltf_load_buffers(&options, gltf, path.c_str()) != cgltf_result_success ||
//		cgltf_validate(gltf))
//	{
//		std::println("Could not load GLTF from {}", path);
//	}
//
//	VkDevice device = vulkan_interface->GetDevice()->GetDevice();
//	VmaAllocator allocator = vulkan_interface->GetAllocator()->GetAllocator();
//
//	for (size_t n = 0; n < gltf->nodes_count; ++n)
//	{
//		cgltf_node* curr_node = gltf->nodes + n;
//		if (curr_node->mesh != nullptr)
//		{
//			mMeshInstances.push_back(std::make_unique<WorldScene::MeshInstance>(gltf, curr_node, device, allocator, desc_set_layouts[1], cmd_buff, queue));
//		}
//		else if (curr_node->camera != nullptr)
//		{
//			auto camera = std::make_unique<WorldScene::CameraInstance>(gltf, curr_node);
//			mCameraNames.push_back(camera->GetName().c_str());
//			mCameraInstances.push_back(std::move(camera));
//		}
//	}
//
//	mMeshes.reserve(gltf->meshes_count);
//	for (size_t m = 0; m < gltf->meshes_count; ++m)
//	{
//		mMeshes.push_back(std::make_unique<WorldScene::Mesh>(gltf, gltf->meshes + m, device, allocator, cmd_buff, queue));
//	}
//
//	mCameras.reserve(gltf->cameras_count);
//	for (size_t c = 0; c < gltf->cameras_count; ++c)
//	{
//		mCameras.push_back(std::make_unique<WorldScene::Camera>(gltf, gltf->cameras + c));
//	}
//
//	cgltf_free(gltf);
//
//	mViewProjBuffer = std::make_unique<BufferResource>(device, allocator, sizeof(glm::mat4) * 2,
//		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
//		VMA_MEMORY_USAGE_AUTO_PREFER_HOST, "view proj uniform");
//
//	if (mCameraInstances.size() == 0)
//	{
//		auto camera_instance = std::make_unique<WorldScene::CameraInstance>();
//		mCameraNames.push_back(camera_instance->GetName().c_str());
//		mCameraInstances.push_back(std::move(camera_instance));
//		mCameras.push_back(std::make_unique<WorldScene::Camera>());
//	}
//
//	const VkDescriptorPoolSize pool_sizes[] = {
//		{
//			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
//			.descriptorCount = 1,
//		},
//	};
//
//	const VkDescriptorPoolCreateInfo dsp_ci = {
//		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
//		.maxSets = 1,
//		.poolSizeCount = std::size(pool_sizes),
//		.pPoolSizes = pool_sizes,
//	};
//
//	VK_CHECK("create view proj desc pool", vkCreateDescriptorPool(device, &dsp_ci, nullptr, &mViewProjDescPool));
//
//	const VkDescriptorSetAllocateInfo ds_ai = {
//		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
//		.descriptorPool = mViewProjDescPool,
//		.descriptorSetCount = 1,
//		.pSetLayouts = &desc_set_layouts[0],
//	};
//
//	VK_CHECK("alloc view proj desc set", vkAllocateDescriptorSets(device, &ds_ai, &mViewProjDescSet));
//
//	VkDescriptorBufferInfo view_proj_buff_info = mViewProjBuffer->GetDescriptorInfo();
//	const VkWriteDescriptorSet view_proj_desc_write = {
//		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
//		.dstSet = mViewProjDescSet,
//		.dstBinding = 0,
//		.descriptorCount = 1,
//		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
//		.pBufferInfo = &view_proj_buff_info,
//	};
//
//	vkUpdateDescriptorSets(device, 1, &view_proj_desc_write, 0, nullptr);
//
//#ifdef _DEBUG
//	Utils_SetObjectName(device, VK_OBJECT_TYPE_DESCRIPTOR_POOL, reinterpret_cast<uint64_t>(mViewProjDescPool), "view proj desc pool");
//	Utils_SetObjectName(device, VK_OBJECT_TYPE_DESCRIPTOR_SET, reinterpret_cast<uint64_t>(mViewProjDescSet), "view proj desc set");
//#endif // _DEBUG
//}
//
//WorldScene::~WorldScene() noexcept
//{
//	if (mDevice != VK_NULL_HANDLE)
//	{
//		vkDestroyDescriptorPool(mDevice, mViewProjDescPool, nullptr);
//	}
//}
//
//void WorldScene::Render(const VkDevice device, const VkCommandBuffer cmd_buff, const VkPipelineLayout pipeline_layout, const uint32_t cam_index) const
//{
//	glm::mat4 view = mCameraInstances[cam_index]->GetViewMatrix();
//	std::memcpy(
//		mViewProjBuffer->GetAllocationInfo2().allocationInfo.pMappedData,
//		&view,
//		sizeof(glm::mat4)
//	);
//
//	glm::mat4 proj = mCameras[mCameraInstances[cam_index]->GetCameraIndex()]->GetProjectionMatrix();
//	proj[1][1] *= -1;
//	std::memcpy(
//		reinterpret_cast<uint8_t*>(mViewProjBuffer->GetAllocationInfo2().allocationInfo.pMappedData) + sizeof(glm::mat4),
//		&proj,
//		sizeof(glm::mat4)
//	);
//
//	const VkBindDescriptorSetsInfoKHR bind_ds_info = {
//		.sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO_KHR,
//		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
//		.layout = pipeline_layout,
//		.descriptorSetCount = 1,
//		.pDescriptorSets = &mViewProjDescSet,
//	};
//
//	vkCmdBindDescriptorSets2KHR(cmd_buff, &bind_ds_info);
//
//	for (const auto& mesh_instance : mMeshInstances)
//	{
//		const VkDescriptorSet model_desc_set = mesh_instance->GetDescriptorSet();
//
//		const VkBindDescriptorSetsInfoKHR bind_ds_info = {
//			.sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO_KHR,
//			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
//			.layout = pipeline_layout,
//			.firstSet = 1,
//			.descriptorSetCount = 1,
//			.pDescriptorSets = &model_desc_set,
//		};
//
//		vkCmdBindDescriptorSets2KHR(cmd_buff, &bind_ds_info);
//
//		const auto& curr_prims = mMeshes[mesh_instance->GetMeshIndex()]->GetPrimitives();
//
//		for (const auto& curr_prim : curr_prims)
//		{
//			if (curr_prim->GetTexCoordsBuffer() != nullptr)
//			{
//				const VkBuffer buffers[] = {
//					curr_prim->GetPositionsBuffer()->GetDescriptorInfo().buffer,
//					curr_prim->GetNormalsBuffer()->GetDescriptorInfo().buffer,
//					curr_prim->GetTexCoordsBuffer()->GetDescriptorInfo().buffer,
//				};
//
//				const VkDeviceSize offsets[] = { 0,0,0 };
//
//				vkCmdBindVertexBuffers2EXT(cmd_buff, 0, std::size(buffers), buffers, offsets, nullptr, nullptr);
//			}
//
//			if (curr_prim->GetIndicesBuffer() != nullptr)
//			{
//				vkCmdBindIndexBuffer2KHR(cmd_buff,
//					curr_prim->GetIndicesBuffer()->GetDescriptorInfo().buffer, 0,
//					curr_prim->GetIndicesBuffer()->GetBufferSize(),
//					curr_prim->GetIndexType()
//				);
//
//				vkCmdDrawIndexed(cmd_buff, curr_prim->GetIndicesCount(), 1, 0, 0, 0);
//			}
//			else
//			{
//				vkCmdDraw(cmd_buff, curr_prim->GetVertexCount(), 1, 0, 0);
//			}
//		}
//	}
//}
//
//const std::vector<const char*>& WorldScene::GetCameraNames() const
//{
//	return mCameraNames;
//}
//
//const std::pair<size_t, const std::unique_ptr<WorldScene::CameraInstance>*> WorldScene::GetCameraInstances() const
//{
//	return std::make_pair(mCameraInstances.size(), mCameraInstances.data());
//}
//
//const std::vector<std::unique_ptr<WorldScene::Camera>>& WorldScene::GetCameras() const
//{
//	return mCameras;
//}
//
//const std::vector<std::unique_ptr<WorldScene::MeshInstance>>& WorldScene::GetMeshInstances() const
//{
//	return mMeshInstances;
//}
//
//const std::vector<std::unique_ptr<WorldScene::Mesh>>& WorldScene::GetMeshes() const
//{
//	return mMeshes;
//}
//
//WorldScene::MeshInstance::MeshInstance(const cgltf_data* gltf, const cgltf_node* node, const VkDevice device, const VmaAllocator allocator, const VkDescriptorSetLayout desc_set_layout, const VkCommandBuffer cmd_buff, const VkQueue queue) : mDevice(device)
//{
//	mMeshIndex = static_cast<uint32_t>(cgltf_mesh_index(gltf, node->mesh));
//	mTransformMatrix = Utils_GetTransformForGLTFNode(node);
//
//	mModelMatrixBuffer = std::make_unique<BufferResource>(device, allocator, sizeof(mTransformMatrix),
//		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
//		0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, std::string(node->name == nullptr ? "node" : node->name).append(" xform"));
//	auto staging_buffer = std::make_unique<BufferResource>(
//		device,
//		allocator,
//		sizeof(glm::mat4), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
//		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
//		VMA_MEMORY_USAGE_AUTO_PREFER_HOST, std::string("instance staging"));
//	std::memcpy(
//		staging_buffer->GetAllocationInfo2().allocationInfo.pMappedData,
//		&mTransformMatrix,
//		sizeof(mTransformMatrix)
//	);
//
//	staging_buffer->CopyToBuffer(cmd_buff, queue, mModelMatrixBuffer->GetDescriptorInfo().buffer, sizeof(glm::mat4));
//
//	const VkDescriptorPoolSize pool_sizes[] = {
//		{
//			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
//			.descriptorCount = 1,
//		},
//	};
//
//	const VkDescriptorPoolCreateInfo dsp_ci = {
//		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
//		.maxSets = 1,
//		.poolSizeCount = std::size(pool_sizes),
//		.pPoolSizes = pool_sizes,
//	};
//
//	VK_CHECK("mesh instance desc pool", vkCreateDescriptorPool(device, &dsp_ci, nullptr, &mDescriptorPool));
//
//	const VkDescriptorSetAllocateInfo ds_ai = {
//		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
//		.descriptorPool = mDescriptorPool,
//		.descriptorSetCount = 1,
//		.pSetLayouts = &desc_set_layout,
//	};
//
//	VK_CHECK("mesh instance desc set", vkAllocateDescriptorSets(device, &ds_ai, &mDescriptorSet));
//
//	VkDescriptorBufferInfo model_mat_buff = mModelMatrixBuffer->GetDescriptorInfo();
//	const VkWriteDescriptorSet desc_write = {
//		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
//		.dstSet = mDescriptorSet,
//		.descriptorCount = 1,
//		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
//		.pBufferInfo = &model_mat_buff,
//	};
//
//	vkUpdateDescriptorSets(mDevice, 1, &desc_write, 0, nullptr);
//
//#ifdef _DEBUG
//	Utils_SetObjectName(device, VK_OBJECT_TYPE_DESCRIPTOR_POOL, reinterpret_cast<uint64_t>(mDescriptorPool), std::string(node->name == nullptr ? "node" : node->name).append(" descriptor pool"));
//	Utils_SetObjectName(device, VK_OBJECT_TYPE_DESCRIPTOR_SET, reinterpret_cast<uint64_t>(mDescriptorSet), std::string(node->name == nullptr ? "node" : node->name).append(" descriptor set"));
//#endif // _DEBUG
//}
//
//WorldScene::MeshInstance::~MeshInstance() noexcept
//{
//	if (mDevice != VK_NULL_HANDLE)
//		vkDestroyDescriptorPool(mDevice, mDescriptorPool, nullptr);
//}
//
//uint32_t WorldScene::MeshInstance::GetMeshIndex() const
//{
//	return mMeshIndex;
//}
//
//glm::mat4 WorldScene::MeshInstance::GetTransformMatrix() const
//{
//	return mTransformMatrix;
//}
//
//BufferResource* WorldScene::MeshInstance::GetTransformMatrixBufferResource() const
//{
//	return mModelMatrixBuffer.get();
//}
//
//VkDescriptorSet WorldScene::MeshInstance::GetDescriptorSet() const
//{
//	return mDescriptorSet;
//}
//
//WorldScene::Mesh::Mesh(const cgltf_data* gltf, const cgltf_mesh* mesh, const VkDevice device, const VmaAllocator allocator, const VkCommandBuffer cmd_buff, const VkQueue queue)
//{
//	mPrimitives.reserve(mesh->primitives_count);
//
//	for (size_t p = 0; p < mesh->primitives_count; ++p)
//	{
//		mPrimitives.push_back(std::make_unique<WorldScene::Mesh::Primitive>(gltf, mesh->primitives + p, device, allocator, cmd_buff, queue, std::string(mesh->name == nullptr ? "mesh" : mesh->name).append(" prim ").append(std::to_string(p))));
//	}
//}
//
//WorldScene::Mesh::~Mesh() noexcept
//{
//}
//
//const std::vector<std::unique_ptr<WorldScene::Mesh::Primitive>>& WorldScene::Mesh::GetPrimitives() const
//{
//	return mPrimitives;
//}
//
//WorldScene::Mesh::Primitive::Primitive(const cgltf_data* gltf, const cgltf_primitive* primitive, const VkDevice device, const VmaAllocator allocator, const VkCommandBuffer cmd_buff, const VkQueue queue, const std::string& name)
//{
//	for (size_t a = 0; a < primitive->attributes_count; ++a)
//	{
//		cgltf_attribute* curr_attr = primitive->attributes + a;
//
//		if (std::string(curr_attr->name) == std::string("POSITION"))
//		{
//			mPositionsBuffer = std::make_unique<BufferResource>(device, allocator, curr_attr->data->buffer_view->size,
//				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
//				VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
//				VMA_MEMORY_USAGE_AUTO, std::string(name).append(" positions"));
//			//auto staging_buffer = std::make_unique<BufferResource>(
//			//	device,
//			//	allocator,
//			//	curr_attr->data->buffer_view->size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
//			//	VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
//			//	VMA_MEMORY_USAGE_AUTO, std::string("positions staging"));
//			std::memcpy(
//				mPositionsBuffer->GetAllocationInfo2().allocationInfo.pMappedData,
//				reinterpret_cast<uint8_t*>(curr_attr->data->buffer_view->buffer->data) + curr_attr->data->buffer_view->offset + curr_attr->data->offset,
//				curr_attr->data->buffer_view->size
//			);
//			//staging_buffer->CopyToBuffer(cmd_buff, queue, mPositionsBuffer->GetDescriptorInfo().buffer, curr_attr->data->buffer_view->size);
//			mVertexCount = static_cast<uint32_t>(curr_attr->data->count);
//		}
//		else if (std::string(curr_attr->name) == std::string("TEXCOORD_0"))
//		{
//			mTexCoordsBuffer = std::make_unique<BufferResource>(device, allocator, curr_attr->data->buffer_view->size,
//				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
//				VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
//				VMA_MEMORY_USAGE_AUTO, std::string(name).append(" texcoords"));
//			//auto staging_buffer = std::make_unique<BufferResource>(
//			//	device,
//			//	allocator,
//			//	curr_attr->data->buffer_view->size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
//			//	VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
//			//	VMA_MEMORY_USAGE_AUTO, std::string("texcoords staging"));
//			std::memcpy(
//				mTexCoordsBuffer->GetAllocationInfo2().allocationInfo.pMappedData,
//				reinterpret_cast<uint8_t*>(curr_attr->data->buffer_view->buffer->data) + curr_attr->data->buffer_view->offset + curr_attr->data->offset,
//				curr_attr->data->buffer_view->size
//			);
//			//staging_buffer->CopyToBuffer(cmd_buff, queue, mTexCoordsBuffer->GetDescriptorInfo().buffer, curr_attr->data->buffer_view->size);
//		}
//		else if (std::string(curr_attr->name) == std::string("NORMAL"))
//		{
//			mNormalsBuffer = std::make_unique<BufferResource>(device, allocator, curr_attr->data->buffer_view->size,
//				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
//				VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
//				VMA_MEMORY_USAGE_AUTO, std::string(name).append(" normals"));
//			//auto staging_buffer = std::make_unique<BufferResource>(
//			//	device,
//			//	allocator,
//			//	curr_attr->data->buffer_view->size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
//			//	VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
//			//	VMA_MEMORY_USAGE_AUTO, std::string("normals staging"));
//			std::memcpy(
//				mNormalsBuffer->GetAllocationInfo2().allocationInfo.pMappedData,
//				reinterpret_cast<uint8_t*>(curr_attr->data->buffer_view->buffer->data) + curr_attr->data->buffer_view->offset + curr_attr->data->offset,
//				curr_attr->data->buffer_view->size
//			);
//			//staging_buffer->CopyToBuffer(cmd_buff, queue, mNormalsBuffer->GetDescriptorInfo().buffer, curr_attr->data->buffer_view->size);
//		}
//	}
//
//	if (mTexCoordsBuffer == nullptr)
//	{
//		mTexCoordsBuffer = std::make_unique<BufferResource>(device, allocator, mVertexCount * sizeof(float) * 2,
//			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
//			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
//			VMA_MEMORY_USAGE_AUTO, std::string(name).append(" texcoords"));
//		//TODO: Initialize buffer to zero.
//	}
//
//	if (mNormalsBuffer == nullptr)
//	{
//		mNormalsBuffer = std::make_unique<BufferResource>(device, allocator, mVertexCount * sizeof(float) * 3,
//			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
//			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
//			VMA_MEMORY_USAGE_AUTO, std::string(name).append(" normals"));
//		//TODO: Initialize buffer to zero.
//	}
//
//	if (primitive->indices != nullptr)
//	{
//		mIndicesCount = static_cast<uint32_t>(primitive->indices->count);
//
//		if (primitive->indices->component_type == cgltf_component_type_r_32u)
//		{
//			mIndexType = VK_INDEX_TYPE_UINT32;
//		}
//
//		{
//			mIndicesBuffer = std::make_unique<BufferResource>(device, allocator, primitive->indices->buffer_view->size,
//				VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
//				VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
//				VMA_MEMORY_USAGE_AUTO, std::string(name).append(" indices"));
//			//auto staging_buffer = std::make_unique<BufferResource>(
//			//	device,
//			//	allocator,
//			//	primitive->indices->buffer_view->size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
//			//	VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
//			//	VMA_MEMORY_USAGE_AUTO, std::string("indices staging"));
//			std::memcpy(
//				mIndicesBuffer->GetAllocationInfo2().allocationInfo.pMappedData,
//				reinterpret_cast<uint8_t*>(primitive->indices->buffer_view->buffer->data) + primitive->indices->buffer_view->offset + primitive->indices->offset,
//				primitive->indices->buffer_view->size
//			);
//			//staging_buffer->CopyToBuffer(cmd_buff, queue, mIndicesBuffer->GetDescriptorInfo().buffer, primitive->indices->buffer_view->size);
//		}
//	}
//}
//
//WorldScene::Mesh::Primitive::~Primitive() noexcept
//{
//}
//
//BufferResource* WorldScene::Mesh::Primitive::GetPositionsBuffer() const
//{
//	return mPositionsBuffer.get();
//}
//
//BufferResource* WorldScene::Mesh::Primitive::GetTexCoordsBuffer() const
//{
//	return mTexCoordsBuffer.get();
//}
//
//BufferResource* WorldScene::Mesh::Primitive::GetNormalsBuffer() const
//{
//	return mNormalsBuffer.get();
//}
//
//BufferResource* WorldScene::Mesh::Primitive::GetIndicesBuffer() const
//{
//	return mIndicesBuffer.get();
//}
//
//VkIndexType WorldScene::Mesh::Primitive::GetIndexType() const
//{
//	return mIndexType;
//}
//
//uint32_t WorldScene::Mesh::Primitive::GetIndicesCount() const
//{
//	return mIndicesCount;
//}
//
//uint32_t WorldScene::Mesh::Primitive::GetVertexCount() const
//{
//	return mVertexCount;
//}
//
//size_t SceneData::Mesh::Primitive::GetVertexCount() const
//{
//	return mVertexCount;
//}
//
//size_t SceneData::Mesh::Primitive::GetIndexCount() const
//{
//	return mIndexCount;
//}
//
//VkDescriptorSet WorldScene::Mesh::Primitive::GetDescriptorSet() const
//{
//	return mDescriptorSet;
//}
//
//VkDescriptorPool WorldScene::Mesh::Primitive::GetDescriptorPool() const
//{
//	return mDescriptorPool;
//}
//
//WorldScene::CameraInstance::CameraInstance()
//{
//	mCameraIndex = 0;
//
//	mViewMatrix = glm::lookAt(glm::vec3(10, 10, 10), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
//	mTransformMatrix = glm::inverse(mViewMatrix);
//	mName = "scene cam";
//}
//
//WorldScene::CameraInstance::CameraInstance(const cgltf_data* gltf, const cgltf_node* node)
//{
//	mCameraIndex = static_cast<uint32_t>(cgltf_camera_index(gltf, node->camera));
//	mTransformMatrix = Utils_GetTransformForGLTFNode(node);
//	mViewMatrix = glm::inverse(mTransformMatrix);
//	mName = std::string(node->name == nullptr ? "scene cam" : node->name);
//}
//
//uint32_t WorldScene::CameraInstance::GetCameraIndex() const
//{
//	return mCameraIndex;
//}
//
//glm::mat4 WorldScene::CameraInstance::GetTranformMatrix() const
//{
//	return mTransformMatrix;
//}
//
//glm::mat4 WorldScene::CameraInstance::GetViewMatrix() const
//{
//	return mViewMatrix;
//}
//
//const std::string& WorldScene::CameraInstance::GetName() const
//{
//	return mName;
//}
//
//WorldScene::Camera::Camera()
//{
//	mProjectionMatrix = glm::perspective(glm::radians(60.f), 1.777f, 0.001f, 1000.f);
//}
//
//WorldScene::Camera::Camera(const cgltf_data* gltf, const cgltf_camera* camera)
//{
//	if (camera->type == cgltf_camera_type_perspective)
//	{
//		cgltf_camera_perspective persp_data = camera->data.perspective;
//		mProjectionMatrix = glm::perspective(
//			persp_data.yfov,
//			persp_data.has_aspect_ratio ? persp_data.aspect_ratio : 1.777f,
//			persp_data.znear,
//			persp_data.zfar
//		);
//	}
//	else if (camera->type == cgltf_camera_type_orthographic)
//	{
//		cgltf_camera_orthographic ortho_data = camera->data.orthographic;
//		mProjectionMatrix = glm::ortho(
//			-ortho_data.xmag / 2.f, ortho_data.xmag / 2.f,
//			-ortho_data.ymag / 2.f, ortho_data.ymag / 2.f,
//			ortho_data.znear, ortho_data.zfar);
//	}
//}
//
//glm::mat4 WorldScene::Camera::GetProjectionMatrix() const
//{
//	return mProjectionMatrix;
//}

Scene::Scene(const std::string& path, const VkDeviceSize uniform_buffer_alignment)
{
	cgltf_options options = {};
	cgltf_data* gltf = nullptr;

	if (cgltf_parse_file(&options, path.c_str(), &gltf) != cgltf_result_success ||
		cgltf_load_buffers(&options, gltf, path.c_str()) != cgltf_result_success ||
		cgltf_validate(gltf))
	{
		std::println("Could not load GLTF from {}", path);
	}

	mCameras.reserve(gltf->cameras_count);
	for (size_t n = 0; n < gltf->nodes_count; ++n)
	{
		cgltf_node* curr_node = gltf->nodes + n;
		if (curr_node->mesh != nullptr)
		{
			AddMeshInstance(gltf, curr_node, uniform_buffer_alignment);
		}
		else if (curr_node->camera != nullptr)
		{
			AddCameraInstance(gltf, curr_node, uniform_buffer_alignment);
			AddCamera(gltf->cameras + cgltf_camera_index(gltf, curr_node->camera), uniform_buffer_alignment);
		}
	}

	mMeshes.reserve(gltf->meshes_count);
	for (size_t m = 0; m < gltf->meshes_count; ++m)
	{
		AddMesh(gltf->meshes + m);
	}

	if (mCameraInstances.size() == 0)
	{
		size_t view_mat_offset = mUniformData.size();
		auto view_mat = glm::lookAt(glm::vec3(10, 10, 10), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
		std::vector<uint8_t> view_mat_data(ALIGNED_SIZE(sizeof(glm::mat4), uniform_buffer_alignment));
		std::memcpy(view_mat_data.data(), &view_mat, sizeof(glm::mat4));

		mUniformData.append_range(view_mat_data);
		mCameraInstances.push_back(Scene::CameraInstance(0, view_mat_offset));

		size_t proj_mat_offset = mUniformData.size();
		auto proj_mat = glm::perspective(glm::radians(35.f), 1.7777f, 0.001f, 1000.f);
		proj_mat[1][1] *= -1;
		std::vector<uint8_t> proj_mat_data(ALIGNED_SIZE(sizeof(glm::mat4), uniform_buffer_alignment));
		std::memcpy(proj_mat_data.data(), &proj_mat, sizeof(glm::mat4));

		mUniformData.append_range(proj_mat_data);
		mCameras.push_back(Scene::Camera(proj_mat_offset));
	}

	std::vector<float> uniform_debug(mUniformData.size() / sizeof(float));
	std::memcpy(uniform_debug.data(), mUniformData.data(), mUniformData.size());

	cgltf_free(gltf);
}

const std::vector<Scene::MeshInstance>& Scene::GetMeshInstances() const
{
	return mMeshInstances;
}

const std::vector<Scene::Mesh> Scene::GetMeshes() const
{
	return mMeshes;
}

const std::vector<Scene::CameraInstance> Scene::GetCameraInstances() const
{
	return mCameraInstances;
}

const std::vector<Scene::Camera> Scene::GetCameras() const
{
	return mCameras;
}

const std::vector<std::string> Scene::GetCameraNames() const
{
	return mCameraNames;
}

const std::vector<uint8_t> Scene::GetVertexData() const
{
	return mVertexData;
}

const std::vector<uint8_t> Scene::GetUniformData() const
{
	return mUniformData;
}

void Scene::AddMeshInstance(const cgltf_data* gltf, const cgltf_node* node, const VkDeviceSize uniform_buffer_alignment)
{
	glm::mat4 xform_matrix = Utils_GetTransformForGLTFNode(node);
	size_t mesh_index = cgltf_mesh_index(gltf, node->mesh);

	mMeshInstances.push_back(MeshInstance(mesh_index, mUniformData.size()));

	std::vector<uint8_t> xform_data(ALIGNED_SIZE(sizeof(glm::mat4), uniform_buffer_alignment));
	std::memcpy(xform_data.data(), &xform_matrix, sizeof(glm::mat4));

	mUniformData.append_range(xform_data);
}

void Scene::AddCameraInstance(const cgltf_data* gltf, const cgltf_node* node, const VkDeviceSize uniform_buffer_alignments)
{
	glm::mat4 xform_matrix = glm::inverse(Utils_GetTransformForGLTFNode(node));
	size_t camera_index = cgltf_camera_index(gltf, node->camera);

	mCameraInstances.push_back(CameraInstance(camera_index, mUniformData.size(), node->name));

	std::vector<uint8_t> xform_data(ALIGNED_SIZE(sizeof(glm::mat4), uniform_buffer_alignments));
	std::memcpy(xform_data.data(), &xform_matrix, sizeof(glm::mat4));

	mUniformData.append_range(xform_data);

	mCameraNames.push_back(node->name == nullptr ? "scene cam" : node->name);
}

void Scene::AddMesh(const cgltf_mesh* mesh)
{
	std::vector<Scene::Mesh::Primitive> primitives;
	primitives.reserve(mesh->primitives_count);

	for (size_t p = 0; p < mesh->primitives_count; ++p)
	{
		cgltf_primitive* curr_prim = mesh->primitives + p;

		size_t positions_size = 0;
		size_t positions_offset = 0;
		size_t normals_size = 0;
		size_t normals_offset = 0;
		size_t texcoords_size = 0;
		size_t texcoords_offset = 0;
		size_t indices_size = 0;
		size_t indices_offset = 0;
		size_t vertex_count = 0;
		size_t index_count = 0;

		for (size_t a = 0; a < curr_prim->attributes_count; ++a)
		{
			cgltf_attribute* curr_attr = curr_prim->attributes + a;

			if (std::string(curr_attr->name) == std::string("POSITION"))
			{
				std::vector<uint8_t> pos_data(curr_attr->data->buffer_view->size);
				std::memcpy(pos_data.data(),
					reinterpret_cast<uint8_t*>(curr_attr->data->buffer_view->buffer->data) + curr_attr->data->buffer_view->offset + curr_attr->data->offset,
					curr_attr->data->buffer_view->size);

				positions_offset = mVertexData.size();
				mVertexData.append_range(pos_data);

				vertex_count = curr_attr->data->count;
				positions_size = curr_attr->data->buffer_view->size;
			}
			else if (std::string(curr_attr->name) == std::string("NORMAL"))
			{
				std::vector<uint8_t> nrm_data(curr_attr->data->buffer_view->size);
				std::memcpy(nrm_data.data(),
					reinterpret_cast<uint8_t*>(curr_attr->data->buffer_view->buffer->data) + curr_attr->data->buffer_view->offset + curr_attr->data->offset,
					curr_attr->data->buffer_view->size);

				normals_offset = mVertexData.size();
				mVertexData.append_range(nrm_data);

				normals_size = curr_attr->data->buffer_view->size;
			}
			else if (std::string(curr_attr->name) == std::string("TEXCOORD_0"))
			{
				std::vector<uint8_t> tex_data(curr_attr->data->buffer_view->size);
				std::memcpy(tex_data.data(),
					reinterpret_cast<uint8_t*>(curr_attr->data->buffer_view->buffer->data) + curr_attr->data->buffer_view->offset + curr_attr->data->offset,
					curr_attr->data->buffer_view->size);

				texcoords_offset = mVertexData.size();
				mVertexData.append_range(tex_data);

				texcoords_size = curr_attr->data->buffer_view->size;
			}
		}

		indices_size = curr_prim->indices->buffer_view->size;
		indices_offset = mVertexData.size();
		index_count = curr_prim->indices->count;

		std::vector<uint8_t> index_data(indices_size);
		std::memcpy(
			index_data.data(),
			reinterpret_cast<uint8_t*>(curr_prim->indices->buffer_view->buffer->data) + curr_prim->indices->buffer_view->offset + curr_prim->indices->offset,
			curr_prim->indices->buffer_view->size
		);
		mVertexData.append_range(index_data);

		VkIndexType index_type = curr_prim->indices->component_type == cgltf_component_type_r_16u ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;

		primitives.push_back(Scene::Mesh::Primitive(positions_size, positions_offset, normals_size, normals_offset, texcoords_size, texcoords_offset, vertex_count, indices_size, indices_offset, index_count, index_type));
	}

	mMeshes.push_back(Mesh(primitives));
}

void Scene::AddCamera(const cgltf_camera* camera, const VkDeviceSize uniform_buffer_alignment)
{
	if (camera->type == cgltf_camera_type_perspective)
	{
		cgltf_camera_perspective persp_data = camera->data.perspective;
		auto proj_mat = glm::perspective(
			persp_data.yfov,
			persp_data.has_aspect_ratio ? persp_data.aspect_ratio : 1.777f,
			persp_data.znear,
			persp_data.zfar
		);
		proj_mat[1][1] *= -1;
		mCameras.push_back(Camera(mUniformData.size()));

		std::vector<uint8_t> mat_data(ALIGNED_SIZE(sizeof(glm::mat4), uniform_buffer_alignment));
		std::memcpy(mat_data.data(), &proj_mat, sizeof(glm::mat4));

		mUniformData.append_range(mat_data);
	}
	else if (camera->type == cgltf_camera_type_orthographic)
	{
		cgltf_camera_orthographic ortho_data = camera->data.orthographic;
		auto proj_mat = glm::ortho(
			-ortho_data.xmag / 2.f, ortho_data.xmag / 2.f,
			-ortho_data.ymag / 2.f, ortho_data.ymag / 2.f,
			ortho_data.znear, ortho_data.zfar);
		proj_mat[1][1] *= -1;
		mCameras.push_back(Camera(mUniformData.size()));

		std::vector<uint8_t> mat_data(ALIGNED_SIZE(sizeof(glm::mat4), uniform_buffer_alignment));
		std::memcpy(mat_data.data(), &proj_mat, sizeof(glm::mat4));

		mUniformData.append_range(mat_data);
	}
}

Scene::MeshInstance::MeshInstance(const size_t mesh_index, const size_t model_matrix_offset)
	:mMeshIndex(mesh_index),
	mModelMatrixOffset(model_matrix_offset)
{
}

size_t Scene::MeshInstance::GetMeshIndex() const
{
	return mMeshIndex;
}

size_t Scene::MeshInstance::GetModelMatrixOffset() const
{
	return mModelMatrixOffset;
}

Scene::CameraInstance::CameraInstance(const size_t camera_index, const size_t view_matrix_offset, const char* name)
	:mCameraIndex(camera_index),
	mViewMatrixOffset(view_matrix_offset),
	mName(name)
{
}

size_t Scene::CameraInstance::GetCameraIndex() const
{
	return mCameraIndex;
}

size_t Scene::CameraInstance::GetViewMatrixOffset() const
{
	return mViewMatrixOffset;
}

const std::string& Scene::CameraInstance::GetName() const
{
	return mName;
}

Scene::Camera::Camera(const size_t proj_mat_offset)
	:mProjMatrixOffset(proj_mat_offset)
{
}

size_t Scene::Camera::GetProjMatOffset() const
{
	return mProjMatrixOffset;
}

Scene::Mesh::Mesh(std::vector<Scene::Mesh::Primitive> primitives)
	:mPrimitives(primitives)
{
}

const std::vector<Scene::Mesh::Primitive>& Scene::Mesh::GetPrimitives() const
{
	return mPrimitives;
}

Scene::Mesh::Primitive::Primitive(const size_t positions_size, const size_t positions_offset, const size_t normals_size, const size_t normals_offset, const size_t texcoords_size, const size_t texcoords_offset, const size_t vertex_count, const size_t indices_size, const size_t indices_offset, const size_t index_count, const VkIndexType index_type)
	: mPositionsSize(positions_size),
	mPositionsOffset(positions_offset),
	mNormalsSize(normals_size),
	mNormalsOffset(normals_offset),
	mTexCoordsSize(texcoords_size),
	mTexCoordsOffset(texcoords_offset),
	mVertexCount(vertex_count),
	mIndicesSize(indices_size),
	mIndicesOffset(indices_offset),
	mIndexCount(index_count),
	mIndexType(index_type)
{
}

size_t Scene::Mesh::Primitive::GetPositionsSize() const
{
	return mPositionsSize;
}

size_t Scene::Mesh::Primitive::GetPositionsOffset() const
{
	return mPositionsOffset;
}

size_t Scene::Mesh::Primitive::GetNormalsSize() const
{
	return mNormalsSize;
}

size_t Scene::Mesh::Primitive::GetNormalsOffset() const
{
	return mNormalsOffset;
}

size_t Scene::Mesh::Primitive::GetTexCoordsSize() const
{
	return mTexCoordsSize;
}

size_t Scene::Mesh::Primitive::GetTexcoordsOffset() const
{
	return mTexCoordsOffset;
}

size_t Scene::Mesh::Primitive::GetIndicesSize() const
{
	return mIndicesSize;
}

size_t Scene::Mesh::Primitive::GetIndicesOffset() const
{
	return mIndicesOffset;
}

VkIndexType Scene::Mesh::Primitive::GetIndexType() const
{
	return mIndexType;
}

size_t Scene::Mesh::Primitive::GetVertexCount() const
{
	return mVertexCount;
}

size_t Scene::Mesh::Primitive::GetIndexCount() const
{
	return mIndexCount;
}

RasterizerWorldScene::RasterizerWorldScene(const Scene& scene, const VkDevice device, const VmaAllocator allocator, const std::vector<VkDescriptorSetLayout>& desc_set_layouts, const VkCommandBuffer cmd_buff, const VkQueue queue)
	: mDevice(device)
{
	auto vertex_data = scene.GetVertexData();

	mVertexData = std::make_unique<BufferResource>(device, allocator, vertex_data,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		0, VMA_MEMORY_USAGE_AUTO, "scene vertex data", cmd_buff, queue);

	auto uniform_data = scene.GetUniformData();

	mUniformData = std::make_unique<BufferResource>(device, allocator, uniform_data,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		0, VMA_MEMORY_USAGE_AUTO, "scene uniform data", cmd_buff, queue);

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

RasterizerWorldScene::MeshInstance::MeshInstance(const Scene::MeshInstance& mesh_instance, const BufferResource* scene_data, const VkDevice device, const VkDescriptorPool desc_pool, const VkDescriptorSetLayout desc_set_layout)
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

RasterizerWorldScene::Mesh::Mesh(const Scene::Mesh& mesh, const BufferResource* scene_data)
	: Scene::Mesh(mesh)
{
}

RasterizerWorldScene::Mesh::Primitive::Primitive(const Scene::Mesh::Primitive& primitive, const BufferResource* scene_data)
	: Scene::Mesh::Primitive(primitive)
{
}

RasterizerWorldScene::CameraInstance::CameraInstance(const Scene::CameraInstance& camera_instance, const BufferResource* scene_data, const VkDevice device, const VkDescriptorPool desc_pool, const VkDescriptorSetLayout desc_set_layout)
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

RasterizerWorldScene::Camera::Camera(const Scene::Camera& camera, const BufferResource* scene_data)
	: Scene::Camera(camera)
{
}
