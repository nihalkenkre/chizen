#pragma once

#include "scene.hpp"

class DeviceBufferResource;
class ImageResource;
class ViewportScenePipelineData;
class VulkanScene;

class ViewportScene
{
public:
	virtual void Render(const VkCommandBuffer cmd_buff, const uint32_t cam_index) const = 0;
	virtual ~ViewportScene() noexcept {}
};

class ViewportEmptyScene : public ViewportScene
{
public:
	void Render(const VkCommandBuffer cmd_buff, const uint32_t cam_index) const override {}
};

class ViewportWorldScene : public ViewportScene
{
public:
	ViewportWorldScene(const VulkanScene* scene, const VulkanInterface* vulkan_interface, const std::string& current_path);

	void Render(const VkCommandBuffer cmd_buff, const uint32_t cam_index) const override;

	//class MeshInstance : public Scene::MeshInstance
	//{
	//public:
	//	MeshInstance(const Scene::MeshInstance& mesh_instance) : Scene::MeshInstance(mesh_instance) {};
	//};

	//class Material : public Scene::Material
	//{
	//public:
	//	Material() {}
	//	Material(const Scene::Material& material) : Scene::Material(material) {}
	//};

	//class Image : public Scene::Image
	//{
	//public:
	//	Image(const char* image_path, const VulkanInterface* vulkan_interface);
	//	Image(const Scene::Image& image, const std::vector<uint8_t>& images_data, const VulkanInterface* vulkan_interface);

	//	ImageResource* GetImageResource() const;

	//private:
	//	std::unique_ptr<ImageResource> mImageResource;
	//};

	//class Mesh : public Scene::Mesh
	//{
	//public:
	//	Mesh(const Scene::Mesh& mesh);

	//	class Primitive : public Scene::Mesh::Primitive
	//	{
	//	public:
	//		Primitive(const Scene::Mesh::Primitive& primitive) : Scene::Mesh::Primitive(primitive) {}
	//	};

	//	const std::vector<Primitive>& GetPrimitives() const;

	//private:
	//	std::vector<Primitive> mPrimitives;
	//};

	//class CameraInstance : public Scene::CameraInstance
	//{
	//public:
	//	CameraInstance(const Scene::CameraInstance& camera_instance) : Scene::CameraInstance(camera_instance) {};
	//};

	//class Camera : public Scene::Camera
	//{
	//public:
	//	Camera(const Scene::Camera& camera) : Scene::Camera(camera) {};
	//};

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