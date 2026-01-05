#pragma once

#include "scene.hpp"

class VulkanInterface;
class ImageResource;
class DeviceBufferResource;

class VulkanScene
{
public:
   VulkanScene() = delete;
	VulkanScene(const Scene& scene, const VulkanInterface* vulkan_interface, const bool is_cpu_shading);

   VulkanScene(const VulkanScene& scene) = delete;
   VulkanScene& operator=(const VulkanScene& scene) = delete;

   ~VulkanScene() noexcept;

   class MeshInstance : public Scene::MeshInstance
   {
   public:
      MeshInstance(const Scene::MeshInstance& mesh_instance) : Scene::MeshInstance(mesh_instance) {};
   };

   class Material : public Scene::Material
   {
	public:
      Material() {}
      Material(const Scene::Material& material) : Scene::Material(material) {};
   };

   class Image : public Scene::Image
   {
	public:
		Image(const char* image_path, const VulkanInterface* vulkan_interface);
		Image(const Scene::Image& image, const std::vector<uint8_t>& images_data, const VulkanInterface* vulkan_interface);

		const ImageResource* GetImageResource() const;

	private:
      std::unique_ptr<ImageResource> mImageResource;
   };

	class Mesh : public Scene::Mesh
	{
	public:
		Mesh(const Scene::Mesh& mesh);

		class Primitive : public Scene::Mesh::Primitive
		{
		public:
			Primitive(const Scene::Mesh::Primitive& primitive) : Scene::Mesh::Primitive(primitive) {}
		};

		const std::vector<Primitive>& GetPrimitives() const;

	private:
		std::vector<Primitive> mPrimitives;
	};

	class CameraInstance : public Scene::CameraInstance
	{
	public:
		CameraInstance(const Scene::CameraInstance& camera_instance) : Scene::CameraInstance(camera_instance) {};
	};

	class Camera : public Scene::Camera
	{
	public:
		Camera(const Scene::Camera& camera) : Scene::Camera(camera) {};
	};

	const std::vector<VulkanScene::MeshInstance>& GetMeshInstances() const;
	const std::vector<VulkanScene::Mesh>& GetMeshes() const;
	const std::vector<VulkanScene::CameraInstance>& GetCameraInstances() const;
	const std::vector<VulkanScene::Camera>& GetCameras() const;
	const std::vector<VulkanScene::Image>& GetImages() const;
	const std::vector<VulkanScene::Material>& GetMaterials() const;

	const DeviceBufferResource* GetVertexData() const;
	const DeviceBufferResource* GetUniformData() const;
	const DeviceBufferResource* GetMaterialsData() const;

private:
	std::vector<VulkanScene::MeshInstance> mMeshInstances;
	std::vector<VulkanScene::Mesh> mMeshes;
	std::vector<VulkanScene::CameraInstance> mCameraInstances;
	std::vector<VulkanScene::Camera> mCameras;
	std::vector<VulkanScene::Image> mImages;
	std::vector<VulkanScene::Material> mMaterials;

	std::unique_ptr<DeviceBufferResource> mVertexData;
	std::unique_ptr<DeviceBufferResource> mUniformData;
	std::unique_ptr<DeviceBufferResource> mMaterialsData;

	VkDevice mDevice;
};