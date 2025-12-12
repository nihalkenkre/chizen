#pragma once

class VulkanInterface;
class BufferResource;

class Scene
{
public:
	virtual void Render(const VkDevice device, const VkCommandBuffer cmd_buff, const VkPipelineLayout pipeline_layout, const uint32_t cam_index) const = 0;
	//const virtual std::vector<const char*>& GetCameraNames() const = 0;
	virtual ~Scene() noexcept {}
};

class EmptyScene : public Scene
{
public:
	void Render(const VkDevice device, const VkCommandBuffer cmd_buff, const VkPipelineLayout pipeline_layout, const uint32_t cam_index) const override {}
	//const virtual std::vector<const char*>& GetCameraNames() const override { return {}; }
};

class WorldScene : public Scene
{
public:
	WorldScene() = delete;
	WorldScene(const VulkanInterface* vulkan_interface, const VkCommandBuffer cmd_buff, const std::vector<VkDescriptorSetLayout>& desc_set_layouts, const VkQueue queue, const std::string& path);

	WorldScene(const WorldScene& other) = delete;
	WorldScene& operator=(const WorldScene& other) = delete;

	~WorldScene() noexcept override;

	class MeshInstance
	{
	public:
		MeshInstance() = delete;

		MeshInstance(const cgltf_data* gltf, const cgltf_node* node, const VkDevice device, const VmaAllocator allocator, const VkDescriptorSetLayout desc_set_layout, const VkCommandBuffer cmd_buff, const VkQueue queue);

		MeshInstance(const MeshInstance& other) = delete;
		MeshInstance& operator=(const MeshInstance& other) = delete;

		~MeshInstance() noexcept;

		uint32_t GetMeshIndex() const;
		glm::mat4 GetTransformMatrix() const;
		BufferResource* GetTransformMatrixBufferResource() const;
		VkDescriptorSet GetDescriptorSet() const;

	private:
		uint32_t mMeshIndex = -1;
		glm::mat4 mTransformMatrix = glm::mat4(1.f);
		std::unique_ptr<BufferResource> mModelMatrixBuffer = nullptr;
		VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
		VkDescriptorSet mDescriptorSet = VK_NULL_HANDLE;

		VkDevice mDevice = VK_NULL_HANDLE;
	};

	class Mesh
	{
	public:
		Mesh() = delete;
		Mesh(const cgltf_data* gltf, const cgltf_mesh* mesh, const VkDevice device, const VmaAllocator allocator, const VkCommandBuffer cmd_buff, const VkQueue queue);

		~Mesh() noexcept;

		class Primitive
		{
		public:
			Primitive() = delete;

			Primitive(const cgltf_data* gltf, const cgltf_primitive* primitive, const VkDevice device, const VmaAllocator allocator, const VkCommandBuffer cmd_buff, const VkQueue queue, const std::string& name);

			~Primitive() noexcept;

			BufferResource* GetPositionsBuffer() const;
			BufferResource* GetTexCoordsBuffer() const;
			BufferResource* GetNormalsBuffer() const;
			BufferResource* GetIndicesBuffer() const;
			VkIndexType GetIndexType() const;
			uint32_t GetIndicesCount() const;
			uint32_t GetVertexCount() const;
			VkDescriptorSet GetDescriptorSet() const;
			VkDescriptorPool GetDescriptorPool() const;

		private:
			std::unique_ptr<BufferResource> mPositionsBuffer = nullptr;
			std::unique_ptr<BufferResource> mTexCoordsBuffer = nullptr;
			std::unique_ptr<BufferResource> mNormalsBuffer = nullptr;
			std::unique_ptr<BufferResource> mIndicesBuffer = nullptr;
			VkIndexType mIndexType = VK_INDEX_TYPE_UINT16;
			uint32_t mIndicesCount = 0;
			uint32_t mVertexCount = 0;
			VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
			VkDescriptorSet mDescriptorSet = VK_NULL_HANDLE;
		};

		const std::vector<std::unique_ptr<WorldScene::Mesh::Primitive>>& GetPrimitives() const;

	private:
		std::vector<std::unique_ptr<WorldScene::Mesh::Primitive>> mPrimitives;
	};

	class CameraInstance
	{
	public:
		CameraInstance();

		CameraInstance(const cgltf_data* gltf, const cgltf_node* node);

		uint32_t GetCameraIndex() const;
		glm::mat4 GetTranformMatrix() const;
		glm::mat4 GetViewMatrix() const;
		const std::string& GetName() const;

	private:
		uint32_t mCameraIndex = -1;
		glm::mat4 mTransformMatrix = glm::mat4(1.f);
		glm::mat4 mViewMatrix = glm::mat4(1.f);
		std::string mName;
	};

	class Camera
	{
	public:
		Camera();

		Camera(const cgltf_data* gltf, const cgltf_camera* camera);

		Camera(const Camera& other) = delete;
		Camera& operator=(const Camera& other) = delete;

		glm::mat4 GetProjectionMatrix() const;

	private:
		glm::mat4 mProjectionMatrix = glm::mat4(1.f);
	};

	void Render(const VkDevice device, const VkCommandBuffer cmd_buff, const VkPipelineLayout pipeline_layout, const uint32_t cam_index) const override;
	const std::vector<const char*>& GetCameraNames() const;

private:
	std::vector<std::unique_ptr<WorldScene::CameraInstance>> mCameraInstances;
	std::vector<std::unique_ptr<WorldScene::Camera>> mCameras;
	std::vector<std::unique_ptr<WorldScene::MeshInstance>> mMeshInstances;
	std::vector<std::unique_ptr<WorldScene::Mesh>> mMeshes;
	std::vector<const char*> mCameraNames;
	std::unique_ptr<BufferResource> mViewProjBuffer = nullptr;
	VkDescriptorSet view_proj_desc_set = VK_NULL_HANDLE;
	VkDescriptorPool mViewProjDescPool = VK_NULL_HANDLE;

	VkDevice mDevice = VK_NULL_HANDLE;
};