#include "scene.hpp"
#include "resources.hpp"
#include "vulkan_interface.hpp"
#include "vulkan_objects.hpp"
#include "utils.hpp"

Scene::Scene(const std::string& path, const VkDeviceSize uniform_buffer_alignment)
{
	cgltf_options options = {};
	cgltf_data* gltf = nullptr;

	if (cgltf_parse_file(&options, path.c_str(), &gltf) != cgltf_result_success ||
		cgltf_load_buffers(&options, gltf, path.c_str()) != cgltf_result_success ||
		cgltf_validate(gltf))
	{
		std::println("Could not load GLTF from {}", path);
	}

	mMaterials.reserve(gltf->materials_count);
	for (size_t m = 0; m < gltf->materials_count; ++m)
	{
		AddMaterial(gltf, gltf->materials + m);
	}

	// Default material
	mMaterials.push_back(Scene::Material(-1, -1, -1, glm::vec4(1, 0, 0, 1), 1.f, 1.f));

	for (size_t n = 0; n < gltf->nodes_count; ++n)
	{
		cgltf_node* curr_node = gltf->nodes + n;
		if (curr_node->mesh != nullptr)
		{
			AddMeshInstance(gltf, curr_node, uniform_buffer_alignment);
		}
		else if (curr_node->camera != nullptr)
		{
			AddCameraInstance(gltf, curr_node, uniform_buffer_alignment);
		}
		else if (curr_node->light != nullptr)
		{
			AddLight(gltf, curr_node);
		}
	}

	mMeshes.reserve(gltf->meshes_count);
	for (size_t m = 0; m < gltf->meshes_count; ++m)
	{
		AddMesh(gltf, gltf->meshes + m);
	}

	mCameras.reserve(gltf->cameras_count);
	for (size_t c = 0; c < gltf->cameras_count; ++c)
	{
		AddCamera(gltf->cameras + c, uniform_buffer_alignment);
	}

	if (mCameraInstances.size() == 0)
	{
		size_t view_mat_offset = mUniformData.size();
		auto view_matrix = glm::lookAt(glm::vec3(10, 10, 10), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
		auto proj_matrix = glm::perspective(glm::radians(35.f), 1.7777f, 0.001f, 1000.f);
		proj_matrix[1][1] *= -1;
		auto view_proj_matrix = proj_matrix * view_matrix;

		mCameraInstances.push_back(CameraInstance(0, mUniformData.size()));

		std::vector<uint8_t> view_proj_data(ALIGNED_SIZE(sizeof(glm::mat4), uniform_buffer_alignment));
		std::memcpy(view_proj_data.data(), &view_proj_matrix, sizeof(glm::mat4));
		mUniformData.append_range(view_proj_data);

		// for raygen shader
		auto view_inverse = glm::inverse(view_matrix);
		auto proj_inverse = glm::inverse(proj_matrix);

		std::vector<uint8_t> view_data(ALIGNED_SIZE(sizeof(glm::mat4), uniform_buffer_alignment));
		std::memcpy(view_data.data(), &view_inverse, sizeof(glm::mat4));
		mUniformData.append_range(view_data);

		std::vector<uint8_t> proj_data(ALIGNED_SIZE(sizeof(glm::mat4), uniform_buffer_alignment));
		std::memcpy(proj_data.data(), &proj_inverse, sizeof(glm::mat4));
		mUniformData.append_range(proj_data);

		mCameraNames.push_back("scene cam");
	}

	if (mLights.size() == 0)
	{
		mLights.reserve(mCameraInstances.size());

		for (const auto& camera_instance : mCameraInstances)
		{
			auto cam_xform = (glm::make_mat4(reinterpret_cast<const float*>(mUniformData.data() + camera_instance.GetViewInverseMatrixOffset())));

			glm::vec3 position = cam_xform[3];
			glm::vec3 direction = glm::mat3(cam_xform) * glm::vec3(0, 0, -1);

			mLights.push_back(Scene::Light(position, direction, glm::vec3(1.f), 1.f));
		}
	}

	mImages.reserve(gltf->images_count);
	for (size_t i = 0; i < gltf->images_count; ++i)
	{
		size_t last_slash_pos = std::string(path).find_last_of("/\\");

		if (std::string::npos != last_slash_pos)
		{
			AddImage(gltf->images + i, path.substr(0, last_slash_pos + 1));
		}
	}

	cgltf_free(gltf);
}

const std::vector<Scene::MeshInstance>& Scene::GetMeshInstances() const
{
	return mMeshInstances;
}

const std::vector<Scene::Mesh>& Scene::GetMeshes() const
{
	return mMeshes;
}

const std::vector<Scene::CameraInstance>& Scene::GetCameraInstances() const
{
	return mCameraInstances;
}

const std::vector<Scene::Camera>& Scene::GetCameras() const
{
	return mCameras;
}

const std::vector<Scene::Image>& Scene::GetImages() const
{
	return mImages;
}

const std::vector<Scene::Material>& Scene::GetMaterials() const
{
	return mMaterials;
}

const std::vector<Scene::Light>& Scene::GetLights() const
{
	return mLights;
}

const std::vector<std::string>& Scene::GetCameraNames() const
{
	return mCameraNames;
}

const std::vector<uint8_t>& Scene::GetVertexData() const
{
	return mVertexData;
}

const std::vector<uint8_t>& Scene::GetUniformData() const
{
	return mUniformData;
}

const std::vector<uint8_t>& Scene::GetImagesData() const
{
	return mImagesData;
}

Scene::Image::Image(const size_t offset, const size_t size, const std::string& name)
	: mDataOffset(offset), mDataSize(size), mName(name)
{
}

size_t Scene::Image::GetDataOffset() const
{
	return mDataOffset;
}

size_t Scene::Image::GetDataSize() const
{
	return mDataSize;
}

const std::string& Scene::Image::GetName() const
{
	return mName;
}

void Scene::AddMeshInstance(const cgltf_data* gltf, const cgltf_node* node, const VkDeviceSize uniform_buffer_alignment)
{
	glm::mat4 xform_matrix = Utils_GetTransformForGLTFNode(node);
	size_t mesh_index = cgltf_mesh_index(gltf, node->mesh);

	mMeshInstances.push_back(MeshInstance(mesh_index, mUniformData.size()));

	std::vector<uint8_t> xform_data(ALIGNED_SIZE(sizeof(glm::mat4), uniform_buffer_alignment));
	std::memcpy(xform_data.data(), &xform_matrix, sizeof(glm::mat4));

	mUniformData.append_range(xform_data);
}

void Scene::AddCameraInstance(const cgltf_data* gltf, const cgltf_node* node, const VkDeviceSize uniform_buffer_alignment)
{
	glm::mat4 view_matrix = glm::inverse(Utils_GetTransformForGLTFNode(node));
	glm::mat4 proj_matrix = glm::mat4(1.f);
	glm::mat4 view_proj_matrix = glm::mat4(1.f);

	size_t camera_index = cgltf_camera_index(gltf, node->camera);

	cgltf_camera* camera = node->camera;

	if (camera->type == cgltf_camera_type_perspective)
	{
		cgltf_camera_perspective persp_data = camera->data.perspective;
		proj_matrix = glm::perspective(
			persp_data.yfov,
			persp_data.has_aspect_ratio ? persp_data.aspect_ratio : 1.777f,
			persp_data.znear,
			persp_data.zfar
		);
		proj_matrix[1][1] *= -1;
		view_proj_matrix = proj_matrix * view_matrix;
	}
	else if (camera->type == cgltf_camera_type_orthographic)
	{
		cgltf_camera_orthographic ortho_data = camera->data.orthographic;
		proj_matrix = glm::ortho(
			-ortho_data.xmag / 2.f, ortho_data.xmag / 2.f,
			-ortho_data.ymag / 2.f, ortho_data.ymag / 2.f,
			ortho_data.znear,
			ortho_data.zfar
		);
		proj_matrix[1][1] *= -1;
		view_proj_matrix = proj_matrix * view_matrix;
	}

	mCameraInstances.push_back(CameraInstance(camera_index, mUniformData.size(), node->name == nullptr ? "scene cam" : node->name));

	std::vector<uint8_t> view_proj_data(ALIGNED_SIZE(sizeof(glm::mat4), uniform_buffer_alignment));
	std::memcpy(view_proj_data.data(), &view_proj_matrix, sizeof(glm::mat4));
	mUniformData.append_range(view_proj_data);

	// for raygen shader
	auto view_inverse = glm::inverse(view_matrix);
	auto proj_inverse = glm::inverse(proj_matrix);

	std::vector<uint8_t> view_data(ALIGNED_SIZE(sizeof(glm::mat4), uniform_buffer_alignment));
	std::memcpy(view_data.data(), &view_inverse, sizeof(glm::mat4));
	mUniformData.append_range(view_data);

	std::vector<uint8_t> proj_data(ALIGNED_SIZE(sizeof(glm::mat4), uniform_buffer_alignment));
	std::memcpy(proj_data.data(), &proj_inverse, sizeof(glm::mat4));
	mUniformData.append_range(proj_data);

	mCameraNames.push_back(node->name == nullptr ? "scene cam" : node->name);
}

void Scene::AddMesh(const cgltf_data* gltf, const cgltf_mesh* mesh)
{
	std::vector<Scene::Mesh::Primitive> primitives;
	primitives.reserve(mesh->primitives_count);

	for (size_t p = 0; p < mesh->primitives_count; ++p)
	{
		cgltf_primitive* curr_prim = mesh->primitives + p;

		size_t positions_size = 0;
		size_t positions_offset = 0;
		size_t indices_size = 0;
		size_t indices_offset = 0;
		size_t vertex_count = 0;
		size_t index_count = 0;
		size_t material_index = mMaterials.size() - 1; // default material

		VkIndexType index_type = VK_INDEX_TYPE_UINT32;
		mVertexData.resize(ALIGNED_SIZE(mVertexData.size(), sizeof(uint32_t)));

		if (curr_prim->indices->component_type == cgltf_component_type_r_32u)
		{
			indices_size = curr_prim->indices->buffer_view->size;
			indices_offset = mVertexData.size();
			index_count = curr_prim->indices->count;

			std::vector<uint8_t> index_data(indices_size);
			std::memcpy(
				index_data.data(),
				reinterpret_cast<uint8_t*>(curr_prim->indices->buffer_view->buffer->data) + curr_prim->indices->buffer_view->offset + curr_prim->indices->offset,
				curr_prim->indices->buffer_view->size
			);
			mVertexData.append_range(index_data);
		}
		else if (curr_prim->indices->component_type == cgltf_component_type_r_16u)
		{
			std::vector<uint16_t> indices_16(curr_prim->indices->count);
			std::memcpy(
				indices_16.data(),
				reinterpret_cast<uint8_t*>(curr_prim->indices->buffer_view->buffer->data) + curr_prim->indices->buffer_view->offset + curr_prim->indices->offset,
				curr_prim->indices->buffer_view->size
			);

			std::vector<uint32_t> indices_32(curr_prim->indices->count);
			std::copy(indices_16.begin(), indices_16.end(), indices_32.begin());

			indices_size = indices_32.size() * sizeof(uint32_t);
			indices_offset = mVertexData.size();
			index_count = curr_prim->indices->count;

			std::vector<uint8_t> index_data(indices_size);
			std::memcpy(
				index_data.data(),
				indices_32.data(),
				indices_size
			);
			mVertexData.append_range(index_data);
		}

		std::vector<VertexData> vertices_data;
		std::vector<glm::vec4> tangents;
		std::vector<glm::vec3> normals;
		std::vector<glm::vec2> uvs;

		for (size_t a = 0; a < curr_prim->attributes_count; ++a)
		{
			cgltf_attribute* curr_attr = curr_prim->attributes + a;

			if (std::string(curr_attr->name) == std::string("POSITION"))
			{
				std::vector<uint8_t>attr_data(curr_attr->data->buffer_view->size);
				std::memcpy(attr_data.data(),
					reinterpret_cast<uint8_t*>(curr_attr->data->buffer_view->buffer->data) + curr_attr->data->buffer_view->offset + curr_attr->data->offset,
					curr_attr->data->buffer_view->size
				);
				positions_offset = mVertexData.size();
				mVertexData.append_range(attr_data);

				vertex_count = curr_attr->data->count;
				positions_size = curr_attr->data->buffer_view->size;
				vertices_data.reserve(vertex_count);
			}
			else if (std::string(curr_attr->name) == std::string("TANGENT"))
			{
				tangents.resize(curr_attr->data->count);
				std::memcpy(
					tangents.data(),
					reinterpret_cast<uint8_t*>(curr_attr->data->buffer_view->buffer->data) + curr_attr->data->buffer_view->offset + curr_attr->data->offset,
					curr_attr->data->buffer_view->size
				);
			}
			else if (std::string(curr_attr->name) == std::string("NORMAL"))
			{
				normals.resize(curr_attr->data->count);
				std::memcpy(
					normals.data(),
					reinterpret_cast<uint8_t*>(curr_attr->data->buffer_view->buffer->data) + curr_attr->data->buffer_view->offset + curr_attr->data->offset,
					curr_attr->data->buffer_view->size
				);
			}
			else if (std::string(curr_attr->name) == std::string("TEXCOORD_0"))
			{
				uvs.resize(curr_attr->data->count);
				std::memcpy(
					uvs.data(),
					reinterpret_cast<uint8_t*>(curr_attr->data->buffer_view->buffer->data) + curr_attr->data->buffer_view->offset + curr_attr->data->offset,
					curr_attr->data->buffer_view->size
				);
			}
		}

		if (uvs.size() == 0)
		{
			uvs.resize(vertex_count, glm::vec2(0.f));
		}

		if (tangents.size() == 0)
		{
			std::println("tangents for {} not found...", mesh->name);
			continue;
		}

		for (size_t v = 0; v < vertex_count; ++v)
		{
			vertices_data.push_back(
				VertexData{ .tangent = tangents[v], .normal = normals[v], .uv = uvs[v] }
			);
		}

		size_t vertices_data_offset = mVertexData.size();
		size_t vertices_data_size = vertices_data.size() * sizeof(VertexData);

		std::vector<uint8_t> vertices_data_data(vertices_data_size);
		std::memcpy(
			vertices_data_data.data(),
			vertices_data.data(),
			vertices_data_size
		);
		mVertexData.append_range(vertices_data_data);

		if (curr_prim->material != nullptr)
		{
			material_index = static_cast<int32_t>(cgltf_material_index(gltf, curr_prim->material));
		}

		primitives.push_back(
			Scene::Mesh::Primitive(
				positions_size, positions_offset,
				vertices_data_size, vertices_data_offset, vertex_count,
				indices_size, indices_offset, index_count, index_type,
				material_index
			)
		);
	}

	mMeshes.push_back(Mesh(primitives));
}

void Scene::AddCamera(const cgltf_camera* camera, const VkDeviceSize uniform_buffer_alignment)
{
	auto proj_matrix = glm::mat4(1.f);
	float z_near = 0.001f;
	float z_far = 1000.f;

	if (camera->type == cgltf_camera_type_perspective)
	{
		cgltf_camera_perspective persp_data = camera->data.perspective;
		z_near = persp_data.znear;
		z_far = persp_data.zfar;
	}
	else if (camera->type == cgltf_camera_type_orthographic)
	{
		cgltf_camera_orthographic ortho_data = camera->data.orthographic;
		z_near = ortho_data.znear;
		z_far = ortho_data.zfar;
	}

	mCameras.push_back(Camera(mUniformData.size(), z_near, z_far));
}

void Scene::AddMaterial(const cgltf_data* gltf, const cgltf_material* material)
{
	int32_t base_color_index = -1;
	int32_t normal_color_index = -1;
	int32_t metalrough_index = -1;
	glm::vec4 base_color_factor = glm::vec4(1.f);
	float metal_factor = 1.f;
	float rough_factor = 1.f;

	if (material->has_pbr_metallic_roughness)
	{
		if (material->pbr_metallic_roughness.base_color_texture.texture != nullptr)
		{
			base_color_index = static_cast<int32_t>(cgltf_image_index(gltf, material->pbr_metallic_roughness.base_color_texture.texture->image));
		}
		std::memcpy(&base_color_factor, material->pbr_metallic_roughness.base_color_factor, sizeof(glm::vec4));

		if (material->normal_texture.texture != nullptr)
		{
			normal_color_index = static_cast<int32_t>(cgltf_image_index(gltf, material->normal_texture.texture->image));
		}

		if (material->pbr_metallic_roughness.metallic_roughness_texture.texture != nullptr)
		{
			metalrough_index = static_cast<int32_t>(cgltf_image_index(gltf, material->pbr_metallic_roughness.metallic_roughness_texture.texture->image));
		}

		metal_factor = material->pbr_metallic_roughness.metallic_factor;
		rough_factor = material->pbr_metallic_roughness.roughness_factor;
	}

	mMaterials.push_back(Scene::Material(base_color_index, normal_color_index, metalrough_index, base_color_factor, metal_factor, rough_factor));
}

void Scene::AddImage(const cgltf_image* image, const std::string& path)
{
	if (image->buffer_view != nullptr)
	{
		mImages.push_back(Scene::Image(mImagesData.size(), image->buffer_view->size, image->name == nullptr ? "image" : image->name));

		std::vector<uint8_t> image_data(image->buffer_view->size);
		std::memcpy(
			image_data.data(),
			reinterpret_cast<uint8_t*>(image->buffer_view->buffer->data) + image->buffer_view->offset,
			image_data.size()
		);
		mImagesData.append_range(image_data);
	}
	else if (image->uri != nullptr)
	{
		size_t image_data_size = 0;
		uint8_t* data = reinterpret_cast<uint8_t*>(SDL_LoadFile(std::string(path).append(image->uri).c_str(), &image_data_size));

		if (data == nullptr)
		{
			std::println("Could not load {}, {}", std::string(path).append(image->uri).c_str(), SDL_GetError());
		}
		else
		{
			mImages.push_back(Scene::Image(mImagesData.size(), image_data_size, image->name == nullptr ? "image" : image->name));

			std::vector<uint8_t> image_data(image_data_size);
			std::memcpy(
				image_data.data(),
				data,
				image_data_size
			);
			mImagesData.append_range(image_data);

			SDL_free(data);
		}
	}
}

void Scene::AddLight(const cgltf_data* gltf, const cgltf_node* node)
{
	glm::vec3 position = glm::vec3(0.f);
	glm::vec3 direction = glm::vec3(0, 0, -1);

	if (node->has_matrix)
	{
		auto xform = Utils_GetTransformForGLTFNode(node);
		position = xform[3];
		direction = glm::mat3(xform) * direction;
	}
	else
	{
		if (node->has_translation)
		{
			position = glm::make_vec3(node->translation);
		}

		if (node->has_rotation)
		{
			direction = glm::make_quat(node->rotation) * direction;
		}
	}

	cgltf_light* curr_light = node->light;
	mLights.push_back(Scene::Light(position, direction, glm::make_vec3(curr_light->color), curr_light->intensity, static_cast<LightType>(curr_light->type), curr_light->range, curr_light->spot_outer_cone_angle, curr_light->spot_inner_cone_angle));
}

Scene::MeshInstance::MeshInstance(const size_t mesh_index, const size_t model_matrix_offset)
	:mMeshIndex(mesh_index),
	mModelMatrixOffset(model_matrix_offset)
{
}

size_t Scene::MeshInstance::GetMeshIndex() const
{
	return mMeshIndex;
}

size_t Scene::MeshInstance::GetModelMatrixOffset() const
{
	return mModelMatrixOffset;
}

Scene::CameraInstance::CameraInstance(const size_t camera_index, const size_t view_matrix_offset, const char* name)
	:mCameraIndex(camera_index),
	mViewProjMatrixOffset(view_matrix_offset),
	mViewInverseMatrixOffset(view_matrix_offset + sizeof(glm::mat4)),
	mProjInverseMatrixOffset(view_matrix_offset + sizeof(glm::mat4) * 2),
	mName(name)
{
}

size_t Scene::CameraInstance::GetCameraIndex() const
{
	return mCameraIndex;
}

size_t Scene::CameraInstance::GetViewProjMatrixOffset() const
{
	return mViewProjMatrixOffset;
}

size_t Scene::CameraInstance::GetViewInverseMatrixOffset() const
{
	return mViewInverseMatrixOffset;
}

size_t Scene::CameraInstance::GetProjInverseMatrixOffset() const
{
	return mProjInverseMatrixOffset;
}

const std::string& Scene::CameraInstance::GetName() const
{
	return mName;
}

Scene::Camera::Camera(const size_t proj_mat_offset, const float z_near, const float z_far)
	:mProjInverseMatrixOffset(proj_mat_offset), mZNear(z_near), mZFar(z_far)
{
}

size_t Scene::Camera::GetProjectionMatrixOffset() const
{
	return mProjInverseMatrixOffset;
}

float Scene::Camera::GetZNear() const
{
	return mZNear;
}

float Scene::Camera::GetZFar() const
{
	return mZFar;
}

Scene::Mesh::Mesh(std::vector<Scene::Mesh::Primitive> primitives)
	:mPrimitives(primitives)
{
}

const std::vector<Scene::Mesh::Primitive>& Scene::Mesh::GetPrimitives() const
{
	return mPrimitives;
}

Scene::Mesh::Primitive::Primitive(
	const size_t positions_size, const size_t positions_offset,
	const size_t vertices_data_size, const size_t vertices_data_offset, const size_t vertex_count,
	const size_t indices_size, const size_t indices_offset, const size_t index_count, const VkIndexType index_type,
	const size_t material_index
)
	: mPositionsSize(positions_size),
	mPositionsOffset(positions_offset),
	mVerticesDataSize(vertices_data_size),
	mVerticesDataOffset(vertices_data_offset),
	mVertexCount(vertex_count),
	mIndicesSize(indices_size),
	mIndicesOffset(indices_offset),
	mIndexCount(index_count),
	mIndexType(index_type),
	mMaterialIndex(material_index)
{
}

size_t Scene::Mesh::Primitive::GetPositionsSize() const
{
	return mPositionsSize;
}

size_t Scene::Mesh::Primitive::GetPositionsOffset() const
{
	return mPositionsOffset;
}

size_t Scene::Mesh::Primitive::GetVerticesDataSize() const
{
	return mVerticesDataSize;
}

size_t Scene::Mesh::Primitive::GetVerticesDataOffset() const
{
	return mVerticesDataOffset;
}

size_t Scene::Mesh::Primitive::GetIndicesSize() const
{
	return mIndicesSize;
}

size_t Scene::Mesh::Primitive::GetIndicesOffset() const
{
	return mIndicesOffset;
}

VkIndexType Scene::Mesh::Primitive::GetIndexType() const
{
	return mIndexType;
}

size_t Scene::Mesh::Primitive::GetVertexCount() const
{
	return mVertexCount;
}

size_t Scene::Mesh::Primitive::GetIndexCount() const
{
	return mIndexCount;
}

size_t Scene::Mesh::Primitive::GetMaterialIndex() const
{
	return mMaterialIndex;
}

glm::vec4 Scene::Material::GetBaseColorFactor() const
{
	return mBaseColorFactor;
}

glm::ivec4 Scene::Material::GetBaseNormalMetalroughIndex() const
{
	return mBaseNormalMetalroughIndex;
}
