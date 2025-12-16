#pragma once

class Scene;

class VulkanRaytracerScene
{
public:
	VulkanRaytracerScene() = delete;

	VulkanRaytracerScene(const Scene& scene, const VkCommandBuffer cmd_buff, const VkQueue queue);
	
	VulkanRaytracerScene(const VulkanRaytracerScene& other) = delete;
	VulkanRaytracerScene& operator=(const VulkanRaytracerScene& other) = delete;

	~VulkanRaytracerScene() noexcept;

private:

};
