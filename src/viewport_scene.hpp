#pragma once

#include "scene.hpp"

class DeviceBufferResource;
class ImageResource;
class ViewportScenePipelineData;
class VulkanScene;
class TransferHelpers;

class ViewportScene
{
public:
	virtual void Render(const VkCommandBuffer cmd_buff, const uint32_t cam_index, const bool IsCPUShading) const = 0;
	virtual void ReloadShaders() = 0;
	virtual ~ViewportScene() noexcept {}
};

class ViewportEmptyScene : public ViewportScene
{
public:
	void Render(const VkCommandBuffer cmd_buff, const uint32_t cam_index, const bool IsCPUShading) const override {}
	void ReloadShaders() override {}
};

class ViewportWorldScene : public ViewportScene
{
public:
	ViewportWorldScene(const VulkanScene* scene, const VulkanInterface* vulkan_interface);

	void Render(const VkCommandBuffer cmd_buff, const uint32_t cam_index, const bool is_cpu_shading) const override;
	void ReloadShaders() override;

	~ViewportWorldScene() noexcept override;

private:
	void CreateIndirectBufferAndDrawElementsBuffer(TransferHelpers* transfer_helpers);
	void CreateDescriptorPool();
	void CreateDescriptorSets();

	std::unique_ptr<ViewportScenePipelineData> mPipelineData = nullptr;
	std::unique_ptr<DeviceBufferResource> mIndirectCommandsBuffer = nullptr;
	std::unique_ptr<DeviceBufferResource> mDrawElements = nullptr;

	VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
	VkDescriptorSet mMTexturesDescSet = VK_NULL_HANDLE;
	//VkDescriptorBufferInfo mCameraDescBuffer = {};
	//VkDescriptorSet mCameraMatrixDescSet = VK_NULL_HANDLE;

	struct DrawElement
	{
		uint32_t model_matrix_index = 0;
		uint32_t material_index = 0;
	};

	uint32_t mDrawElementsCount = 0;
	const VulkanScene* mScene = nullptr;
	VkDevice mDevice = VK_NULL_HANDLE;
	VmaAllocator mAllocator = VK_NULL_HANDLE;
};