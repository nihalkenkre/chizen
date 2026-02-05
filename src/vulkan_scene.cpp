#include "vulkan_scene.hpp"
#include "vulkan_objects.hpp"
#include "vulkan_interface.hpp"
#include "resources.hpp"
#include <stb_image.h>

VulkanScene::VulkanScene(const Scene& scene, const VulkanInterface* vulkan_interface, const bool IsCPUShading)
	: mDevice(vulkan_interface->GetVkDevice()), mIndicesSize(scene.GetIndexData().size())
{
	VmaAllocator allocator = vulkan_interface->GetVmaAllocator();

	auto positions_data = scene.GetPositionsData();
	size_t positions_data_size = positions_data.size() * sizeof(positions_data[0]);

	mPositionsData = std::make_unique<DeviceBufferResource>(
		mDevice, allocator,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		positions_data_size, "scene positions data"
	);

	auto staging_positions_data = std::make_unique<HostBufferResource>(
		mDevice, allocator,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		positions_data.data(), positions_data_size, "staging positions data"
	);

	auto vertex_data = scene.GetVertexData();
	size_t vertex_data_size = vertex_data.size() * sizeof(vertex_data[0]);

	mVertexData = std::make_unique<DeviceBufferResource>(
		mDevice, allocator,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		vertex_data_size, "scene vertex data"
	);

	auto staging_vertex_data = std::make_unique<HostBufferResource>(
		mDevice, allocator,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		vertex_data.data(), vertex_data_size, "staging vertex data"
	);

	auto index_data = scene.GetIndexData();
	size_t index_data_size = index_data.size() * sizeof(index_data[0]);

	mIndexData = std::make_unique<DeviceBufferResource>(
		mDevice, allocator,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		index_data_size, "scene index data"
	);

	auto staging_index_data = std::make_unique<HostBufferResource>(
		mDevice, allocator,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		index_data.data(), index_data_size, "staging index data"
	);

	auto camera_matrices_data = scene.GetCameraMatricesData();
	size_t camera_matrices_data_size = camera_matrices_data.size() * sizeof(camera_matrices_data[0]);

	mCameraMatricesData = std::make_unique<DeviceBufferResource>(
		mDevice, allocator,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		camera_matrices_data_size, "scene cam matrices data"
	);

	auto staging_camera_matrices_data = std::make_unique<HostBufferResource>(
		mDevice, allocator,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		camera_matrices_data.data(), camera_matrices_data_size, "staging cam matrices data"
	);


	auto model_matrices_data = scene.GetModelMatricesData();
	size_t model_matrices_data_size = model_matrices_data.size() * sizeof(model_matrices_data[0]);

	mModelMatricesData = std::make_unique<DeviceBufferResource>(
		mDevice, allocator,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		model_matrices_data.size(), "scene model matrices data"
	);

	auto staging_model_matrices_data = std::make_unique<HostBufferResource>(
		mDevice, allocator,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		model_matrices_data.data(), model_matrices_data_size, "staging model matrices data"
	);

	mCameraInstances.reserve(scene.GetCameraInstances().size());
	for (const auto& cam_instance : scene.GetCameraInstances())
	{
		mCameraInstances.push_back(VulkanScene::CameraInstance(cam_instance));
	}

	mCameras.reserve(scene.GetCameras().size());
	for (const auto& cam : scene.GetCameras())
	{
		mCameras.push_back(VulkanScene::Camera(cam));
	}

	mMeshInstances.reserve(scene.GetMeshInstances().size());
	for (const auto& mesh_instance : scene.GetMeshInstances())
	{
		mMeshInstances.push_back(VulkanScene::MeshInstance(mesh_instance));
	}

	mMaterials.reserve(scene.GetMaterials().size());
	for (const auto& material : scene.GetMaterials())
	{
		mMaterials.push_back(VulkanScene::Material(material));
	}

	mLights.reserve(scene.GetLights().size());
	for (const auto& light : scene.GetLights())
	{
		mLights.push_back(VulkanScene::Light(light));
	}

	//std::vector<uint8_t> materials_data(mMaterials.size() * sizeof(Material));
	//std::memcpy(materials_data.data(), mMaterials.data(), materials_data.size());

	size_t materials_data_size = mMaterials.size() * sizeof(mMaterials[0]);

	auto staging_materials_data = std::make_unique<HostBufferResource>(
		mDevice, allocator,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		mMaterials.data(), materials_data_size, "staging materials"
	);

	mMaterialsData = std::make_unique<DeviceBufferResource>(
		mDevice, allocator,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		materials_data_size, "materials"
	);

	//std::vector<uint8_t> lights_data(mLights.size() * sizeof(Light));
	//std::memcpy(lights_data.data(), mLights.data(), lights_data.size());

	size_t lights_data_size = mLights.size() * sizeof(mLights[0]);

	auto staging_lights_data = std::make_unique<HostBufferResource>(
		mDevice, allocator,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		mLights.data(), lights_data_size, "staging lights"
	);

	mLightsData = std::make_unique<DeviceBufferResource>(
		mDevice, allocator,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		lights_data_size, "lights"
	);

	TransferHelpers* transfer_helpers = vulkan_interface->GetTransferHelpers();

	transfer_helpers->RecordBatch();
	transfer_helpers->CopyBufferToBuffer(staging_positions_data->GetVkBuffer(), mPositionsData->GetVkBuffer(), positions_data.size());
	transfer_helpers->CopyBufferToBuffer(staging_vertex_data->GetVkBuffer(), mVertexData->GetVkBuffer(), vertex_data.size());
	transfer_helpers->CopyBufferToBuffer(staging_index_data->GetVkBuffer(), mIndexData->GetVkBuffer(), index_data.size());
	transfer_helpers->CopyBufferToBuffer(staging_camera_matrices_data->GetVkBuffer(), mCameraMatricesData->GetVkBuffer(), camera_matrices_data.size());
	transfer_helpers->CopyBufferToBuffer(staging_model_matrices_data->GetVkBuffer(), mModelMatricesData->GetVkBuffer(), model_matrices_data.size());
	transfer_helpers->CopyBufferToBuffer(staging_materials_data->GetVkBuffer(), mMaterialsData->GetVkBuffer(), mMaterials.size() * sizeof(Material));
	transfer_helpers->CopyBufferToBuffer(staging_lights_data->GetVkBuffer(), mLightsData->GetVkBuffer(), mLights.size() * sizeof(Light));
	transfer_helpers->SubmitBatch();

	if (!IsCPUShading)
	{
		mImages.reserve(scene.GetImages().size());
		for (const auto& image : scene.GetImages())
		{
			mImages.push_back(VulkanScene::Image(std::string(SDL_GetBasePath()).append("/images/one_pix.jpg").c_str(), vulkan_interface, VK_FORMAT_R8G8B8A8_SRGB));
		}

		for (const auto& material : mMaterials)
		{
			auto base_normal_index = material.GetBaseNormalMetalroughIndex();

			if (base_normal_index.x >= 0)
				mImages[base_normal_index.x] = VulkanScene::Image(
					scene.GetImages()[base_normal_index.x], scene.GetImagesData(), vulkan_interface, VK_FORMAT_R8G8B8A8_SRGB
				);

			if (base_normal_index.y >= 0)
				mImages[base_normal_index.y] = VulkanScene::Image(
					scene.GetImages()[base_normal_index.y], scene.GetImagesData(), vulkan_interface, VK_FORMAT_R8G8B8A8_UNORM
				);

			if (base_normal_index.z > 0)
				mImages[base_normal_index.z] = VulkanScene::Image(
					scene.GetImages()[base_normal_index.z], scene.GetImagesData(), vulkan_interface, VK_FORMAT_R8G8B8A8_UNORM
				);
		}
	}

	if (mImages.size() == 0)
	{
		mImages.push_back(VulkanScene::Image(std::string(SDL_GetBasePath()).append("/images/one_pix.jpg").c_str(), vulkan_interface, VK_FORMAT_R8G8B8A8_SRGB));
	}

	mImageDescs.reserve(mImages.size());
	for (const auto& image : mImages)
	{
		mImageDescs.push_back(image.GetImageResource()->GetDescriptorInfo());
	}

	mMeshes.reserve(scene.GetMeshes().size());
	for (const auto& mesh : scene.GetMeshes())
	{
		mMeshes.push_back(VulkanScene::Mesh(mesh));
	}
}

VulkanScene::~VulkanScene() noexcept
{
}

const std::vector<VulkanScene::MeshInstance>& VulkanScene::GetMeshInstances() const
{
	return mMeshInstances;
}

const std::vector<VulkanScene::Mesh>& VulkanScene::GetMeshes() const
{
	return mMeshes;
}

const std::vector<VulkanScene::CameraInstance>& VulkanScene::GetCameraInstances() const
{
	return mCameraInstances;
}

const std::vector<VulkanScene::Camera>& VulkanScene::GetCameras() const
{
	return mCameras;
}

const std::vector<VulkanScene::Material>& VulkanScene::GetMaterials() const
{
	return mMaterials;
}

const std::vector<VulkanScene::Light>& VulkanScene::GetLights() const
{
	return mLights;
}

const std::vector<VkDescriptorImageInfo>& VulkanScene::GetImageDescs() const
{
	return mImageDescs;
}

const std::vector<VulkanScene::Image>& VulkanScene::GetImages() const
{
	return mImages;
}

const DeviceBufferResource* VulkanScene::GetPositionsData() const
{
	return mPositionsData.get();
}

const DeviceBufferResource* VulkanScene::GetVertexData() const
{
	return mVertexData.get();
}

const DeviceBufferResource* VulkanScene::GetIndexData() const
{
	return mIndexData.get();
}

const DeviceBufferResource* VulkanScene::GetCameraMatricesData() const
{
	return mCameraMatricesData.get();
}

const DeviceBufferResource* VulkanScene::GetModelMatricesData() const
{
	return mModelMatricesData.get();
}

const DeviceBufferResource* VulkanScene::GetMaterialsData() const
{
	return mMaterialsData.get();
}

const DeviceBufferResource* VulkanScene::GetLightsData() const
{
	return mLightsData.get();
}

size_t VulkanScene::GetIndicesSize() const
{
	return mIndicesSize;
}

VulkanScene::Mesh::Mesh(const Scene::Mesh& mesh)
	: Scene::Mesh(mesh)
{
	mPrimitives.reserve(mesh.GetPrimitives().size());
	for (const auto& prim : mesh.GetPrimitives())
	{
		mPrimitives.push_back(
			VulkanScene::Mesh::Primitive(prim)
		);
	}
}

const std::vector<VulkanScene::Mesh::Primitive>& VulkanScene::Mesh::GetPrimitives() const
{
	return mPrimitives;
}

VulkanScene::Image::Image(const char* image_path, const VulkanInterface* vulkan_interface, const VkFormat format)
{
	VkDevice device = vulkan_interface->GetVkDevice();
	VmaAllocator allocator = vulkan_interface->GetVmaAllocator();
	auto queue_family_indices = std::vector<uint32_t>{
		vulkan_interface->GetPhysicalDeviceData()->GraphicsQueueFamilyIndex,
		vulkan_interface->GetPhysicalDeviceData()->ComputeQueueFamilyIndex,
		vulkan_interface->GetPhysicalDeviceData()->TransferQueueFamilyIndex,
	};

	uint32_t w, h, c;
	uint8_t* pixels = stbi_load(image_path, reinterpret_cast<int*>(&w), reinterpret_cast<int*>(&h), reinterpret_cast<int*>(&c), 4);

	mImageResource = std::make_unique<ImageResource>(
		device, VkExtent3D{ w, h, 1 }, format, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		allocator,
		queue_family_indices,
		"texture"
	);

	auto staging_buffer = std::make_unique<HostBufferResource>(
		device, allocator, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		mImageResource->GetAllocationInfo2().allocationInfo.size, "texture staging"
	);

	std::memcpy(
		staging_buffer->GetAllocationInfo2().allocationInfo.pMappedData,
		pixels,
		w * h * 4
	);
	stbi_image_free(pixels);

	TransferHelpers* transfer_helpers = vulkan_interface->GetTransferHelpers();
	transfer_helpers->RecordBatch();
	transfer_helpers->ChangeImageLayout(
		VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0,
		VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
		VK_IMAGE_ASPECT_COLOR_BIT, mImageResource->GetVkImage()
	);
	transfer_helpers->CopyBufferToImage(staging_buffer->GetVkBuffer(), mImageResource->GetVkImage(), VkExtent3D{ w,h,1 });
	transfer_helpers->SubmitBatch();
}

VulkanScene::Image::Image(const Scene::Image& image, const std::vector<uint8_t>& images_data, const VulkanInterface* vulkan_interface, const VkFormat format)
{
	VkDevice device = vulkan_interface->GetVkDevice();
	VmaAllocator allocator = vulkan_interface->GetVmaAllocator();

	auto queue_family_indices = std::vector<uint32_t>{
		vulkan_interface->GetPhysicalDeviceData()->GraphicsQueueFamilyIndex,
		vulkan_interface->GetPhysicalDeviceData()->ComputeQueueFamilyIndex,
		vulkan_interface->GetPhysicalDeviceData()->TransferQueueFamilyIndex,
	};

	int w, h, c;
	uint8_t* pixels = stbi_load_from_memory(
		images_data.data() + image.GetDataOffset(), static_cast<int>(image.GetDataSize()),
		&w, &h, &c, 4
	);

	mImageResource = std::make_unique<ImageResource>(
		device, VkExtent3D{ static_cast<uint32_t>(w), static_cast<uint32_t>(h), 1 },
		format,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		allocator,
		queue_family_indices,
		"texture"
	);

	auto staging_buffer = std::make_unique<HostBufferResource>(
		device, allocator, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		mImageResource->GetAllocationInfo2().allocationInfo.size, "texture staging"
	);

	std::memcpy(
		staging_buffer->GetAllocationInfo2().allocationInfo.pMappedData,
		pixels,
		w * h * 4
	);
	stbi_image_free(pixels);

	TransferHelpers* transfer_helpers = vulkan_interface->GetTransferHelpers();
	transfer_helpers->RecordBatch();
	transfer_helpers->ChangeImageLayout(
		VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0,
		VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
		VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
		VK_IMAGE_ASPECT_COLOR_BIT, mImageResource->GetVkImage()
	);
	transfer_helpers->CopyBufferToImage(staging_buffer->GetVkBuffer(), mImageResource->GetVkImage(), VkExtent3D{ static_cast<uint32_t>(w), static_cast<uint32_t>(h),1 });
	transfer_helpers->SubmitBatch();
}

const ImageResource* VulkanScene::Image::GetImageResource() const
{
	return mImageResource.get();
}
