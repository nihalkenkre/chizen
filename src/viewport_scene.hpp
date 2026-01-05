#pragma once

#include "scene.hpp"

class DeviceBufferResource;
class ImageResource;
class ViewportScenePipelineData;
class VulkanScene;

class ViewportScene
{
public:
	virtual void Render(const VkCommandBuffer cmd_buff, const uint32_t cam_index, const bool is_cpu_shading) const = 0;
	virtual ~ViewportScene() noexcept {}
};

class ViewportEmptyScene : public ViewportScene
{
public:
	void Render(const VkCommandBuffer cmd_buff, const uint32_t cam_index, const bool is_cpu_shading) const override {}
};

class ViewportWorldScene : public ViewportScene
{
public:
	ViewportWorldScene(const VulkanScene* scene, const VulkanInterface* vulkan_interface);

	void Render(const VkCommandBuffer cmd_buff, const uint32_t cam_index, const bool is_cpu_shading) const override;

	~ViewportWorldScene() noexcept override;

private:
	std::unique_ptr<ViewportScenePipelineData> mPipelineData = nullptr;

	VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
	VkDescriptorSet mMTexturesDescSet = VK_NULL_HANDLE;
	VkDescriptorBufferInfo mCameraDescBuffer = {};
	VkDescriptorSet mCameraMatrixDescSet = VK_NULL_HANDLE;
	VkDescriptorSet mModelMatrixDescSet = VK_NULL_HANDLE;

	const VulkanScene* mScene = nullptr;
	VkDevice mDevice = VK_NULL_HANDLE;
};