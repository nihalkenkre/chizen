#pragma once

class VulkanInterface;
class BufferResource;
struct LightData;

class Scene
{
public:
	Scene() {};
	Scene(const std::string& path, const VkDeviceSize uniform_buffer_alignment, const VkDeviceSize storage_buffer_alignment);

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
			Primitive(
				const size_t positions_size, const size_t positions_offset,
				const size_t vertices_data_size, const size_t vertices_data_offset, const size_t vertex_count,
				const size_t indices_size, const size_t indices_offset, const size_t index_count, const VkIndexType index_type,
				const size_t material_index
			);

			size_t GetPositionsSize() const;
			size_t GetPositionsOffset() const;
			size_t GetVerticesDataSize() const;
			size_t GetVerticesDataOffset() const;
			size_t GetIndicesSize() const;
			size_t GetIndicesOffset() const;

			VkIndexType GetIndexType() const;
			size_t GetVertexCount() const;
			size_t GetIndexCount() const;

			size_t GetMaterialIndex() const;

		private:
			size_t mPositionsSize = 0;
			size_t mPositionsOffset = 0;
			size_t mVerticesDataSize = 0;
			size_t mVerticesDataOffset = 0;
			size_t mIndicesSize = 0;
			size_t mIndicesOffset = 0;

			VkIndexType mIndexType = VK_INDEX_TYPE_UINT16;
			size_t mVertexCount = 0;
			size_t mIndexCount = 0;
			size_t mMaterialIndex = 0;
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
		size_t GetViewProjMatrixOffset() const;
		size_t GetViewInverseMatrixOffset() const;
		size_t GetProjInverseMatrixOffset() const;
		const std::string& GetName() const;

	private:
		size_t mCameraIndex = 0;
		size_t mViewProjMatrixOffset = 0;
		size_t mViewInverseMatrixOffset = 0;
		size_t mProjInverseMatrixOffset = 0;
		std::string mName = "scene cam";
	};

	class Camera
	{
	public:
		Camera() {}
		Camera(const size_t proj_mat_offset, const float z_near, const float z_far);

		size_t GetProjectionMatrixOffset() const;
		float GetZNear() const;
		float GetZFar() const;

	private:
		size_t mProjInverseMatrixOffset = 0;
		float mZNear = 0.1f;
		float mZFar = 100.f;
	};

	class Material
	{
	public:
		Material() {}
		Material(
			const int32_t base_index,
			const int32_t normal_index,
			const int32_t metalrough_index,
			const glm::vec4 base_color_factor,
			const float metal_factor,
			const float rough_factor
		) :
			mBaseNormalMetalroughIndex(base_index, normal_index, metalrough_index, -1),
			mBaseColorFactor(base_color_factor),
			mMetalRoughFactor(metal_factor, rough_factor, -1, -1)
		{
		}

		glm::vec4 GetBaseColorFactor() const;
		glm::ivec4 GetBaseNormalMetalroughIndex() const;

	private:
		glm::vec4 mBaseColorFactor = glm::vec4(1.f);
		glm::ivec4 mBaseNormalMetalroughIndex = glm::ivec4(-1, -1, -1, -1);
		glm::vec4 mMetalRoughFactor = glm::vec4(1.f);
	};

	class Image
	{
	public:
		Image() {}
		Image(const size_t offset, const size_t size, const std::string& name);

		size_t GetDataOffset() const;
		size_t GetDataSize() const;
		const std::string& GetName() const;

	private:
		size_t mDataOffset = 0;
		size_t mDataSize = 0;
		std::string mName;
	};

	enum LightType
	{
		Direction,
		Point,
		Spot
	};

	class Light
	{
	public:
		Light() {};

		Light(const glm::vec3 position, const glm::vec3 direction, const glm::vec3 color, const float intensity, const LightType type = LightType::Direction, const float range = -1, const float outer_cone_angle = 0, const float inner_cone_angle = 0) :
			mPosition(position), mDirection(direction), mColor(color), mIntensity(intensity), mType(type), mRange(range), mOuterConeAngle(outer_cone_angle), mInnerConeAngle(inner_cone_angle) {
		}

	private:
		glm::vec3 mPosition = glm::vec3(0.f);
		glm::vec3 mDirection = glm::vec3(0, 0, -1);
		glm::vec3 mColor = glm::vec3(0.f);
		float mIntensity = 1.f;
		float mRange = 1.f;
		float mOuterConeAngle = 1.f;
		float mInnerConeAngle = 0.f;
		LightType mType = LightType::Direction;
	};

	const std::vector<MeshInstance>& GetMeshInstances() const;
	const std::vector<Scene::Mesh>& GetMeshes() const;
	const std::vector<Scene::CameraInstance>& GetCameraInstances() const;
	const std::vector<Scene::Camera>& GetCameras() const;
	const std::vector<Image>& GetImages() const;
	const std::vector<Material>& GetMaterials() const;
	const std::vector<Light>& GetLights() const;

	const std::vector<std::string>& GetCameraNames() const;

	const std::vector<uint8_t>& GetVertexData() const;
	const std::vector<uint8_t>& GetIndexData() const;
	const std::vector<uint8_t>& GetCameraMatricesData() const;
	const std::vector<uint8_t>& GetModelMatricesData() const;
	const std::vector<uint8_t>& GetImagesData() const;

private:
	void AddMeshInstance(const cgltf_data* gltf, const cgltf_node* node, const VkDeviceSize uniform_buffer_alignment);
	void AddCameraInstance(const cgltf_data* gltf, const cgltf_node* node, const VkDeviceSize uniform_buffer_alignment);
	void AddMesh(const cgltf_data* gltf, const cgltf_mesh* mesh);
	void AddCamera(const cgltf_camera* camera, const VkDeviceSize uniform_buffer_alignment);
	void AddMaterial(const cgltf_data* gltf, const cgltf_material* material);
	void AddImage(const cgltf_image* image, const std::string& path);
	void AddLight(const cgltf_data* gltf, const cgltf_node* node);

	std::vector<MeshInstance> mMeshInstances;
	std::vector<Mesh> mMeshes;
	std::vector<CameraInstance> mCameraInstances;
	std::vector<Camera> mCameras;
	std::vector<Material> mMaterials;
	std::vector<Image> mImages;
	std::vector<Light> mLights;

	std::vector<std::string> mCameraNames;

	std::vector<uint8_t> mVertexData;
	std::vector<uint8_t> mIndexData;
	std::vector<uint8_t> mCameraMatricesData;
	std::vector<uint8_t> mModelMatricesData;
	std::vector<uint8_t> mImagesData;
};
