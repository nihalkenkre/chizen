#pragma once

class VulkanInterface;
class BufferResource;

//class Scene
//{
//public:
//	virtual void Render(const VkDevice device, const VkCommandBuffer cmd_buff, const VkPipelineLayout pipeline_layout, const uint32_t cam_index) const = 0;
//	virtual ~Scene() noexcept {}
//};
//
//class EmptyScene : public Scene
//{
//public:
//	void Render(const VkDevice device, const VkCommandBuffer cmd_buff, const VkPipelineLayout pipeline_layout, const uint32_t cam_index) const override {}
//};
//
//class WorldScene : public Scene
//{
//public:
//	WorldScene() = delete;
//	WorldScene(const VulkanInterface* vulkan_interface, const VkCommandBuffer cmd_buff, const std::vector<VkDescriptorSetLayout>& desc_set_layouts, const VkQueue queue, const std::string& path);
//
//	WorldScene(const WorldScene& other) = delete;
//	WorldScene& operator=(const WorldScene& other) = delete;
//
//	~WorldScene() noexcept override;
//
//	class MeshInstance
//	{
//	public:
//		MeshInstance() = delete;
//
//		MeshInstance(const cgltf_data* gltf, const cgltf_node* node, const VkDevice device, const VmaAllocator allocator, const VkDescriptorSetLayout desc_set_layout, const VkCommandBuffer cmd_buff, const VkQueue queue);
//
//		MeshInstance(const MeshInstance& other) = delete;
//		MeshInstance& operator=(const MeshInstance& other) = delete;
//
//		~MeshInstance() noexcept;
//
//		uint32_t GetMeshIndex() const;
//		glm::mat4 GetTransformMatrix() const;
//		BufferResource* GetTransformMatrixBufferResource() const;
//		VkDescriptorSet GetDescriptorSet() const;
//
//	private:
//		uint32_t mMeshIndex = -1;
//		glm::mat4 mTransformMatrix = glm::mat4(1.f);
//		std::unique_ptr<BufferResource> mModelMatrixBuffer = nullptr;
//		VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
//		VkDescriptorSet mDescriptorSet = VK_NULL_HANDLE;
//
//		VkDevice mDevice = VK_NULL_HANDLE;
//	};
//
//	class Mesh
//	{
//	public:
//		Mesh() = delete;
//		Mesh(const cgltf_data* gltf, const cgltf_mesh* mesh, const VkDevice device, const VmaAllocator allocator, const VkCommandBuffer cmd_buff, const VkQueue queue);
//
//		~Mesh() noexcept;
//
//		class Primitive
//		{
//		public:
//			Primitive() = delete;
//
//			Primitive(const cgltf_data* gltf, const cgltf_primitive* primitive, const VkDevice device, const VmaAllocator allocator, const VkCommandBuffer cmd_buff, const VkQueue queue, const std::string& name);
//
//			~Primitive() noexcept;
//
//			BufferResource* GetPositionsBuffer() const;
//			BufferResource* GetTexCoordsBuffer() const;
//			BufferResource* GetNormalsBuffer() const;
//			BufferResource* GetIndicesBuffer() const;
//			VkIndexType GetIndexType() const;
//			uint32_t GetIndicesCount() const;
//			uint32_t GetVertexCount() const;
//			VkDescriptorSet GetDescriptorSet() const;
//			VkDescriptorPool GetDescriptorPool() const;
//
//		private:
//			std::unique_ptr<BufferResource> mPositionsBuffer = nullptr;
//			std::unique_ptr<BufferResource> mTexCoordsBuffer = nullptr;
//			std::unique_ptr<BufferResource> mNormalsBuffer = nullptr;
//			std::unique_ptr<BufferResource> mIndicesBuffer = nullptr;
//			VkIndexType mIndexType = VK_INDEX_TYPE_UINT16;
//			uint32_t mIndicesCount = 0;
//			uint32_t mVertexCount = 0;
//			VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
//			VkDescriptorSet mDescriptorSet = VK_NULL_HANDLE;
//		};
//
//		const std::vector<std::unique_ptr<WorldScene::Mesh::Primitive>>& GetPrimitives() const;
//
//	private:
//		std::vector<std::unique_ptr<WorldScene::Mesh::Primitive>> mPrimitives;
//	};
//
//	class CameraInstance
//	{
//	public:
//		CameraInstance();
//
//		CameraInstance(const cgltf_data* gltf, const cgltf_node* node);
//
//		uint32_t GetCameraIndex() const;
//		glm::mat4 GetTranformMatrix() const;
//		glm::mat4 GetViewMatrix() const;
//		const std::string& GetName() const;
//
//	private:
//		uint32_t mCameraIndex = -1;
//		glm::mat4 mTransformMatrix = glm::mat4(1.f);
//		glm::mat4 mViewMatrix = glm::mat4(1.f);
//		std::string mName;
//	};
//
//	class Camera
//	{
//	public:
//		Camera();
//
//		Camera(const cgltf_data* gltf, const cgltf_camera* camera);
//
//		Camera(const Camera& other) = delete;
//		Camera& operator=(const Camera& other) = delete;
//
//		glm::mat4 GetProjectionMatrix() const;
//
//	private:
//		glm::mat4 mProjectionMatrix = glm::mat4(1.f);
//	};
//
//	void Render(const VkDevice device, const VkCommandBuffer cmd_buff, const VkPipelineLayout pipeline_layout, const uint32_t cam_index) const override;
//	const std::vector<const char*>& GetCameraNames() const;
//
//	const std::pair<size_t, const std::unique_ptr<WorldScene::CameraInstance>*> GetCameraInstances() const;
//	const std::vector<std::unique_ptr<WorldScene::Camera>>& GetCameras() const;
//	const std::vector<std::unique_ptr<WorldScene::MeshInstance>>& GetMeshInstances() const;
//	const std::vector<std::unique_ptr<WorldScene::Mesh>>& GetMeshes() const;
//
//private:
//	std::vector<std::unique_ptr<WorldScene::CameraInstance>> mCameraInstances;
//	std::vector<std::unique_ptr<WorldScene::Camera>> mCameras;
//	std::vector<std::unique_ptr<WorldScene::MeshInstance>> mMeshInstances;
//	std::vector<std::unique_ptr<WorldScene::Mesh>> mMeshes;
//	std::unique_ptr<BufferResource> mViewProjBuffer = nullptr;
//	VkDescriptorSet mViewProjDescSet = VK_NULL_HANDLE;
//	VkDescriptorPool mViewProjDescPool = VK_NULL_HANDLE;
//	std::vector<const char*> mCameraNames;
//
//	VkDevice mDevice = VK_NULL_HANDLE;
//};

class Scene
{
public:
	Scene() {};
	Scene(const std::string& path, const VkDeviceSize uniform_buffer_alignment);

	class MeshInstance
	{
	public:
		MeshInstance() {}
		MeshInstance(const size_t mesh_index, const size_t model_matrix_offset);

		size_t GetMeshIndex() const;
		size_t GetModelMatrixOffset() const;

	private:
		size_t mMeshIndex = 0;
		size_t mModelMatrixOffset = 0;
	};

	class Mesh
	{
	public:

		class Primitive
		{
		public:
			Primitive(const size_t positions_size, const size_t positions_offset, const size_t normals_size, const size_t normals_offset, const size_t texcoords_size, const size_t texcoords_offset, const size_t vertex_count, const size_t indices_size, const size_t indices_offset, const size_t index_count, const VkIndexType index_type);

			size_t GetPositionsSize() const;
			size_t GetPositionsOffset() const;
			size_t GetNormalsSize() const;
			size_t GetNormalsOffset() const;
			size_t GetTexCoordsSize() const;
			size_t GetTexcoordsOffset() const;
			size_t GetIndicesSize() const;
			size_t GetIndicesOffset() const;

			VkIndexType GetIndexType() const;
			size_t GetVertexCount() const;
			size_t GetIndexCount() const;

		private:
			size_t mPositionsSize = 0;
			size_t mPositionsOffset = 0;
			size_t mNormalsSize = 0;
			size_t mNormalsOffset = 0;
			size_t mTexCoordsSize = 0;
			size_t mTexCoordsOffset = 0;
			size_t mIndicesSize = 0;
			size_t mIndicesOffset = 0;

			VkIndexType mIndexType = VK_INDEX_TYPE_UINT16;
			size_t mVertexCount = 0;
			size_t mIndexCount = 0;
		};

		Mesh(std::vector<Scene::Mesh::Primitive> primitives);

		const std::vector<Primitive>& GetPrimitives() const;

	private:
		std::vector<Primitive> mPrimitives;
	};

	class CameraInstance
	{
	public:
		CameraInstance(const size_t camera_index, const size_t view_matrix_offset, const char* name = "scene cam");

		size_t GetCameraIndex() const;
		size_t GetViewMatrixOffset() const;
		const std::string& GetName() const;

	private:
		size_t mCameraIndex = 0;
		size_t mViewMatrixOffset = 0;
		std::string mName = "scene cam";
	};

	class Camera
	{
	public:
		Camera(const size_t proj_mat_offset);

		size_t GetProjMatOffset() const;

	private:
		size_t mProjMatrixOffset = 0;
	};

	const std::vector<MeshInstance>& GetMeshInstances() const;
	const std::vector<Mesh> GetMeshes() const;
	const std::vector<CameraInstance> GetCameraInstances() const;
	const std::vector<Camera> GetCameras() const;

	const std::vector<std::string> GetCameraNames() const;

	const std::vector<uint8_t> GetVertexData() const;
	const std::vector<uint8_t> GetUniformData() const;

private:
	void AddMeshInstance(const cgltf_data* gltf, const cgltf_node* node, const VkDeviceSize uniform_buffer_alignment);
	void AddCameraInstance(const cgltf_data* gltf, const cgltf_node* node, const VkDeviceSize uniform_buffer_alignment);
	void AddMesh(const cgltf_mesh* mesh);
	void AddCamera(const cgltf_camera* camera, const VkDeviceSize uniform_buffer_alignment);

	std::vector<MeshInstance> mMeshInstances;
	std::vector<Mesh> mMeshes;
	std::vector<CameraInstance> mCameraInstances;
	std::vector<Camera> mCameras;

	std::vector<std::string> mCameraNames;

	std::vector<uint8_t> mVertexData;
	std::vector<uint8_t> mUniformData;
};

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
	RasterizerWorldScene(const Scene &scene, const VkDevice device, const VmaAllocator allocator, const std::vector<VkDescriptorSetLayout>& desc_set_layouts, const VkCommandBuffer cmd_buff, const VkQueue queue);

	void Render(const VkCommandBuffer cmd_buff, const VkPipelineLayout pipeline_layout, const uint32_t cam_index) const override;

	class MeshInstance : public Scene::MeshInstance
	{
	public:
		MeshInstance(const Scene::MeshInstance& mesh_instance, const BufferResource* scene_data, const VkDevice device, const VkDescriptorPool desc_pool, const VkDescriptorSetLayout desc_set_layout);

		~MeshInstance() noexcept;

		VkDescriptorSet GetModelMatDescSet() const;

	private:
		VkDescriptorSet mModelMatDescSet = VK_NULL_HANDLE;
	};

	class Mesh : public Scene::Mesh
	{
	public:
		Mesh(const Scene::Mesh& mesh, const BufferResource* scene_data);

		class Primitive : public Scene::Mesh::Primitive
		{
		public:
			Primitive(const Scene::Mesh::Primitive& primitive, const BufferResource* scene_data);
		};

		//const std::vector<Primitive> GetPrimitives() const;

	private:
		//std::vector<Primitive> mPrimitives;
	};

	class CameraInstance : public Scene::CameraInstance
	{
	public:
		CameraInstance(const Scene::CameraInstance& camera_instance, const BufferResource* scene_data, const VkDevice device, const VkDescriptorPool desc_pool, const VkDescriptorSetLayout desc_set_layout);

		VkDescriptorSet GetViewProjDescSet() const;

	private:
		VkDescriptorSet mViewProjDescSet = VK_NULL_HANDLE;
	};

	class Camera : public Scene::Camera 
	{
	public:
		Camera(const Scene::Camera& camera, const BufferResource* scene_data);
	};

	~RasterizerWorldScene() noexcept override;

private:
	std::vector<RasterizerWorldScene::MeshInstance> mMeshInstances;
	std::vector<RasterizerWorldScene::Mesh> mMeshes;
	std::vector<RasterizerWorldScene::CameraInstance> mCameraInstances;
	std::vector<RasterizerWorldScene::Camera> mCameras;

	std::unique_ptr<BufferResource> mVertexData = nullptr;
	std::unique_ptr<BufferResource> mUniformData = nullptr;

	// All the descs in the scene
	VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;

	VkDevice mDevice = VK_NULL_HANDLE;
};

class VulkanRaytracerScene
{

};

class EmbreeRaytracerScene
{

};

