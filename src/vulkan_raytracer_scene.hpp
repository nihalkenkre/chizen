#pragma once

#include "scene.hpp"

class ComputeHelpers;
class DeviceBufferResource;
class BLAccelerationStructure;
class TLAccelerationStructure;
class VulkanRaytracerScenePipelineData;
class ImageResource;
class ComputeHelpers;
class Pool;

class VulkanRaytracerScene
{
public:
	VulkanRaytracerScene() = delete;

	VulkanRaytracerScene(const Scene& scene, const VkDevice device, const VmaAllocator allocator, const std::vector<uint32_t>& queue_family_indices, const std::string& current_path, const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& raytracing_properties, const size_t scratch_buffer_alignment, const uint32_t mem_type_id, ComputeHelpers* compute_helpers);
	
	VulkanRaytracerScene(const VulkanRaytracerScene& other) = delete;
	VulkanRaytracerScene& operator=(const VulkanRaytracerScene& other) = delete;

	~VulkanRaytracerScene() noexcept;

	class CameraInstance : public Scene::CameraInstance
	{
	public:
		CameraInstance(const Scene::CameraInstance& camera_instance) : Scene::CameraInstance(camera_instance) {}
	};

	class Camera : public Scene::Camera
	{
	public:
		Camera(const Scene::Camera& camera) : Scene::Camera(camera) {}
	};

	class Image : public Scene::Image
	{
	public:
		Image(const char* image_path, const VkDevice device, const VmaAllocator allocator, const std::vector<uint32_t>& queue_family_indices, ComputeHelpers* compute_helpers);
		Image(const Scene::Image& image, const std::vector<uint8_t>& images_data, const VkDevice device, const VmaAllocator allocator, const std::vector<uint32_t>& queue_family_indices, ComputeHelpers* compute_helpers);

		ImageResource* GetImageResource() const
		{
			return mImageResource.get();
		}

	private:
		std::unique_ptr<ImageResource> mImageResource = nullptr;
	};

	class Material : public Scene::Material
	{
	public:
		Material(const Scene::Material& material) : Scene::Material(material) {}
	};

	void Render(const VkCommandBuffer cmd_buff, const DeviceBufferResource* rand_states, const ImageResource* accum_target, const ImageResource* final_render_target, const uint32_t current_sample, const uint32_t width, const uint32_t height, const uint32_t cam_index) const;

	VkAccelerationStructureKHR GetTLAS() const;
	VulkanRaytracerScenePipelineData* GetPipelineData() const;

private:
	std::unique_ptr<Pool> mScratchBufferPool = nullptr;
	std::unique_ptr<Pool> mSBTBufferPool = nullptr;

	std::unique_ptr<TLAccelerationStructure> mTLAS;
	std::vector<std::unique_ptr<BLAccelerationStructure>> mBLASes;

	std::vector<VulkanRaytracerScene::CameraInstance> mCameraInstances;

	std::unique_ptr<DeviceBufferResource> mVertexData = nullptr;
	std::unique_ptr<DeviceBufferResource> mUniformData = nullptr;
	std::unique_ptr<DeviceBufferResource> mMaterialsBuffer = nullptr;	
	std::unique_ptr<VulkanRaytracerScenePipelineData> mPipelineData = nullptr;
	
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR mRaytracingProperties = {};
	std::unique_ptr<DeviceBufferResource> mRGSbt = nullptr;
	std::unique_ptr<DeviceBufferResource> mMSSbt = nullptr;
	std::unique_ptr<DeviceBufferResource> mCHSbt = nullptr;

	VkDeviceSize mCHSbtRecordAlignedSize = 0;
	VkDeviceSize mCHSbtAlignedSize = 0;

	std::vector<Material> mMaterials;
	std::vector<Image> mImages;

	VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
	VkDescriptorSet mCameraDescSet = VK_NULL_HANDLE;
	VkDescriptorSet mSceneDescSet = VK_NULL_HANDLE;
	VkDescriptorBufferInfo mCameraDescBuffer = {};

	VkDevice mDevice = VK_NULL_HANDLE;
};
