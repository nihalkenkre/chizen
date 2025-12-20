#pragma once

#include "scene.hpp"

class DeviceBufferResource;
class ImageResource;
class TransferHelpers;

class RasterizerScene
{
public:
	virtual void Render(const VkCommandBuffer cmd_buff, const VkPipelineLayout pipeline_layout, const uint32_t cam_index) const = 0;
	virtual ~RasterizerScene() noexcept {}
};

class RasterizeEmptyScene : public RasterizerScene
{
public:
	void Render(const VkCommandBuffer cmd_buff, const VkPipelineLayout pipeline_layout, const uint32_t cam_index) const override {}
};

class RasterizerWorldScene : public RasterizerScene
{
public:
	RasterizerWorldScene(const Scene& scene, const VkDevice device, const VmaAllocator allocator, const std::vector<VkDescriptorSetLayout>& desc_set_layouts, const std::vector<uint32_t>& queue_family_indices, TransferHelpers* transfer_objects);

	void Render(const VkCommandBuffer cmd_buff, const VkPipelineLayout pipeline_layout, const uint32_t cam_index) const override;

	class MeshInstance : public Scene::MeshInstance
	{
	public:
		MeshInstance(const Scene::MeshInstance& mesh_instance, const DeviceBufferResource* scene_data, const VkDevice device, const VkDescriptorPool desc_pool, const VkDescriptorSetLayout desc_set_layout);

		~MeshInstance() noexcept;

		VkDescriptorSet GetModelMatDescSet() const;

	private:
		VkDescriptorSet mModelMatDescSet = VK_NULL_HANDLE;
	};

	class Image : public Scene::Image
	{
	public:
		Image(const Scene::Image& image, const std::vector<uint8_t>& images_data, const VkDevice device, const VmaAllocator allocator, const std::vector<uint32_t>& queue_family_indices, TransferHelpers* transfer_helpers);

		ImageResource* GetImageResource() const;

	private:
		std::unique_ptr<ImageResource> mImageResource;
	};

	class Mesh : public Scene::Mesh
	{
	public:
		Mesh(const Scene::Mesh& mesh, const std::vector<RasterizerWorldScene::Image>& images,
			const VkDevice device, const VmaAllocator allocator, const VkDescriptorPool desc_pool, const VkDescriptorSetLayout desc_set_layout,
			const std::vector<uint32_t>& queue_family_indices, const VkSampler null_sampler,
			TransferHelpers* transfer_helpers);

		class Primitive : public Scene::Mesh::Primitive
		{
		public:
			Primitive(const Scene::Mesh::Primitive& primitive, const std::vector<RasterizerWorldScene::Image>& images,
				const VkDevice device, const VmaAllocator allocator,
				const VkDescriptorPool desc_pool, const VkDescriptorSetLayout desc_set_layout,
				const std::vector<uint32_t>& queue_family_indices,
				const VkSampler null_sampler, TransferHelpers* transfer_helpers
			);

			VkDescriptorSet GetTexDescSet() const;

		private:
			VkDescriptorSet mTexsDescSet = VK_NULL_HANDLE;

			std::unique_ptr<ImageResource> mBaseColorImage = nullptr;
		};

		const std::vector<Primitive>& GetPrimitives() const;

	private:
		std::vector<Primitive> mPrimitives;
	};

	class CameraInstance : public Scene::CameraInstance
	{
	public:
		CameraInstance(const Scene::CameraInstance& camera_instance, const DeviceBufferResource* scene_data, const VkDevice device, const VkDescriptorPool desc_pool, const VkDescriptorSetLayout desc_set_layout);

		VkDescriptorSet GetViewProjDescSet() const;

	private:
		VkDescriptorSet mViewProjDescSet = VK_NULL_HANDLE;
	};

	class Camera : public Scene::Camera
	{
	public:
		Camera(const Scene::Camera& camera, const DeviceBufferResource* scene_data);
	};

	~RasterizerWorldScene() noexcept override;

private:
	std::vector<RasterizerWorldScene::MeshInstance> mMeshInstances;
	std::vector<RasterizerWorldScene::Mesh> mMeshes;
	std::vector<RasterizerWorldScene::CameraInstance> mCameraInstances;
	std::vector<RasterizerWorldScene::Camera> mCameras;
	std::vector<RasterizerWorldScene::Image> mImages;

	std::unique_ptr<DeviceBufferResource> mPositionsData = nullptr;
	std::unique_ptr<DeviceBufferResource> mUniformData = nullptr;

	// All the descs in the scene
	VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
	VkSampler mNullSampler = VK_NULL_HANDLE;
	VkDevice mDevice = VK_NULL_HANDLE;
};