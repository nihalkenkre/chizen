#pragma once

class VulkanInterface;
class BufferResource;

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
		Mesh() {}

		class Primitive
		{
		public:
			Primitive() {}
			Primitive(
				const size_t positions_size, const size_t positions_offset, 
				const size_t normals_size, const size_t normals_offset, 
				const size_t texcoords_size, const size_t texcoords_offset, 
				const size_t vertex_count, 
				const size_t indices_size, const size_t indices_offset, const size_t index_count, const VkIndexType index_type,
				const int32_t base_img_index, const int32_t normal_img_index
			);

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

			int32_t GetBaseImageIndex() const;
			int32_t GetNormalImageIndex() const;

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

			int32_t mBaseImageIndex = -1;
			int32_t mNormalImageIndex = -1;
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
		Camera() {}
		Camera(const size_t proj_mat_offset);

		size_t GetProjMatOffset() const;

	private:
		size_t mProjMatrixOffset = 0;
	};

	class Image
	{
	public:
		Image() {}
		Image(const size_t offset, const size_t size);

		size_t GetDataOffset() const;
		size_t GetDataSize() const;

	private:
		size_t mDataOffset = 0;
		size_t mDataSize = 0;
	};

	const std::vector<MeshInstance>& GetMeshInstances() const;
	const std::vector<Mesh> GetMeshes() const;
	const std::vector<CameraInstance> GetCameraInstances() const;
	const std::vector<Camera> GetCameras() const;
	const std::vector<Image> GetImages() const;

	const std::vector<std::string> GetCameraNames() const;

	const std::vector<uint8_t> GetVertexData() const;
	const std::vector<uint8_t> GetUniformData() const;
	const std::vector<uint8_t> GetImagesData() const;

private:
	void AddMeshInstance(const cgltf_data* gltf, const cgltf_node* node, const VkDeviceSize uniform_buffer_alignment);
	void AddCameraInstance(const cgltf_data* gltf, const cgltf_node* node, const VkDeviceSize uniform_buffer_alignment);
	void AddMesh(const cgltf_data* gltf, const cgltf_mesh* mesh, const size_t mesh_index);
	void AddCamera(const cgltf_camera* camera, const size_t camera_index, const VkDeviceSize uniform_buffer_alignment);
	void AddImage(const cgltf_image* image);

	std::vector<MeshInstance> mMeshInstances;
	std::vector<Mesh> mMeshes;
	std::vector<CameraInstance> mCameraInstances;
	std::vector<Camera> mCameras;
	std::vector<Image> mImages;

	std::vector<std::string> mCameraNames;

	std::vector<uint8_t> mVertexData;
	std::vector<uint8_t> mUniformData;
	std::vector<uint8_t> mImagesData;
};
