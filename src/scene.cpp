#include "scene.hpp"
#include "resources.hpp"
#include "vulkan_interface.hpp"
#include "vulkan_objects.hpp"
#include "utils.hpp"

#include <cgltf.h>

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

	mMeshes.resize(gltf->meshes_count);
	mCameras.resize(gltf->cameras_count);
	for (size_t n = 0; n < gltf->nodes_count; ++n)
	{
		cgltf_node* curr_node = gltf->nodes + n;
		if (curr_node->mesh != nullptr)
		{
			AddMeshInstance(gltf, curr_node, uniform_buffer_alignment);
			AddMesh(gltf, curr_node->mesh, cgltf_mesh_index(gltf, curr_node->mesh));
		}
		else if (curr_node->camera != nullptr)
		{
			AddCameraInstance(gltf, curr_node, uniform_buffer_alignment);
			AddCamera(curr_node->camera, cgltf_camera_index(gltf, curr_node->camera), uniform_buffer_alignment);
		}
	}

	if (mCameraInstances.size() == 0)
	{
		size_t view_mat_offset = mUniformData.size();
		auto view_mat = glm::lookAt(glm::vec3(10, 10, 10), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
		std::vector<uint8_t> view_mat_data(ALIGNED_SIZE(sizeof(glm::mat4), uniform_buffer_alignment));
		std::memcpy(view_mat_data.data(), &view_mat, sizeof(glm::mat4));

		mUniformData.append_range(view_mat_data);
		mCameraInstances.push_back(Scene::CameraInstance(0, view_mat_offset));
		mCameraNames.push_back("scene cam");

		size_t proj_mat_offset = mUniformData.size();
		auto proj_mat = glm::perspective(glm::radians(35.f), 1.7777f, 0.001f, 1000.f);
		proj_mat[1][1] *= -1;
		std::vector<uint8_t> proj_mat_data(ALIGNED_SIZE(sizeof(glm::mat4), uniform_buffer_alignment));
		std::memcpy(proj_mat_data.data(), &proj_mat, sizeof(glm::mat4));

		mUniformData.append_range(proj_mat_data);
		mCameras.push_back(Scene::Camera(proj_mat_offset));
	}

	std::vector<float> uniform_debug(mUniformData.size() / sizeof(float));
	std::memcpy(uniform_debug.data(), mUniformData.data(), mUniformData.size());

	mMaterials.reserve(gltf->materials_count);
	for (size_t m = 0; m < gltf->materials_count; ++m)
	{
		AddMaterial(gltf, gltf->materials + m);
	}

	mImages.reserve(gltf->images_count);
	for (size_t i = 0; i < gltf->images_count; ++i)
	{
		AddImage(gltf->images + i);
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

const std::vector<std::string>& Scene::GetCameraNames() const
{
	return mCameraNames;
}

const std::vector<uint8_t>& Scene::GetVertexData() const
{
	return mPositionsData;
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

void Scene::AddCameraInstance(const cgltf_data* gltf, const cgltf_node* node, const VkDeviceSize uniform_buffer_alignments)
{
	glm::mat4 xform_matrix = glm::inverse(Utils_GetTransformForGLTFNode(node));
	size_t camera_index = cgltf_camera_index(gltf, node->camera);

	mCameraInstances.push_back(CameraInstance(camera_index, mUniformData.size(), node->name == nullptr ? "scene cam" : node->name));

	std::vector<uint8_t> xform_data(ALIGNED_SIZE(sizeof(glm::mat4), uniform_buffer_alignments));
	std::memcpy(xform_data.data(), &xform_matrix, sizeof(glm::mat4));

	mUniformData.append_range(xform_data);

	mCameraNames.push_back(node->name == nullptr ? "scene cam" : node->name);
}

void Scene::AddMesh(const cgltf_data* gltf, const cgltf_mesh* mesh, const size_t mesh_index)
{
	std::vector<Scene::Mesh::Primitive> primitives;
	primitives.reserve(mesh->primitives_count);

	for (size_t p = 0; p < mesh->primitives_count; ++p)
	{
		cgltf_primitive* curr_prim = mesh->primitives + p;

		size_t positions_size = 0;
		size_t positions_offset = 0;
		size_t normals_size = 0;
		size_t normals_offset = 0;
		size_t texcoords_size = 0;
		size_t texcoords_offset = 0;
		size_t indices_size = 0;
		size_t indices_offset = 0;
		size_t vertex_count = 0;
		size_t index_count = 0;
		int32_t material_index = -1;

		for (size_t a = 0; a < curr_prim->attributes_count; ++a)
		{
			cgltf_attribute* curr_attr = curr_prim->attributes + a;

			std::vector<uint8_t>attr_data(curr_attr->data->buffer_view->size);
			std::memcpy(attr_data.data(),
				reinterpret_cast<uint8_t*>(curr_attr->data->buffer_view->buffer->data) + curr_attr->data->buffer_view->offset + curr_attr->data->offset,
				curr_attr->data->buffer_view->size);

			if (std::string(curr_attr->name) == std::string("POSITION"))
			{
				positions_offset = mPositionsData.size();
				mPositionsData.append_range(attr_data);

				vertex_count = curr_attr->data->count;
				positions_size = curr_attr->data->buffer_view->size;
			}
			else if (std::string(curr_attr->name) == std::string("NORMAL"))
			{
				normals_offset = mPositionsData.size();
				mPositionsData.append_range(attr_data);

				normals_size = curr_attr->data->buffer_view->size;
			}
			else if (std::string(curr_attr->name) == std::string("TEXCOORD_0"))
			{
				texcoords_offset = mPositionsData.size();
				mPositionsData.append_range(attr_data);

				texcoords_size = curr_attr->data->buffer_view->size;
			}
		}

		VkIndexType index_type = VK_INDEX_TYPE_UINT16;
		if (curr_prim->indices->component_type == cgltf_component_type_r_32u)
		{
			index_type = VK_INDEX_TYPE_UINT32;

			mPositionsData.resize(ALIGNED_SIZE(mPositionsData.size(), sizeof(uint32_t)));
		}

		indices_size = curr_prim->indices->buffer_view->size;
		indices_offset = mPositionsData.size();
		index_count = curr_prim->indices->count;

		std::vector<uint8_t> index_data(indices_size);
		std::memcpy(
			index_data.data(),
			reinterpret_cast<uint8_t*>(curr_prim->indices->buffer_view->buffer->data) + curr_prim->indices->buffer_view->offset + curr_prim->indices->offset,
			curr_prim->indices->buffer_view->size
		);
		mPositionsData.append_range(index_data);

		if (curr_prim->material != nullptr)
		{
			material_index = static_cast<int32_t>(cgltf_material_index(gltf, curr_prim->material));
		}

		primitives.push_back(
			Scene::Mesh::Primitive(
				positions_size, positions_offset,
				normals_size, normals_offset,
				texcoords_size, texcoords_offset,
				vertex_count,
				indices_size, indices_offset, index_count, index_type,
				material_index
			)
		);
	}

	mMeshes[mesh_index] = Mesh(primitives);
}

void Scene::AddCamera(const cgltf_camera* camera, const size_t camera_index, const VkDeviceSize uniform_buffer_alignment)
{
	if (camera->type == cgltf_camera_type_perspective)
	{
		cgltf_camera_perspective persp_data = camera->data.perspective;
		auto proj_mat = glm::perspective(
			persp_data.yfov,
			persp_data.has_aspect_ratio ? persp_data.aspect_ratio : 1.777f,
			persp_data.znear,
			persp_data.zfar
		);
		proj_mat[1][1] *= -1;
		mCameras[camera_index] = Camera(mUniformData.size());

		std::vector<uint8_t> mat_data(ALIGNED_SIZE(sizeof(glm::mat4), uniform_buffer_alignment));
		std::memcpy(mat_data.data(), &proj_mat, sizeof(glm::mat4));

		mUniformData.append_range(mat_data);
	}
	else if (camera->type == cgltf_camera_type_orthographic)
	{
		cgltf_camera_orthographic ortho_data = camera->data.orthographic;
		auto proj_mat = glm::ortho(
			-ortho_data.xmag / 2.f, ortho_data.xmag / 2.f,
			-ortho_data.ymag / 2.f, ortho_data.ymag / 2.f,
			ortho_data.znear, ortho_data.zfar);
		proj_mat[1][1] *= -1;
		mCameras[camera_index] = Camera(mUniformData.size());

		std::vector<uint8_t> mat_data(ALIGNED_SIZE(sizeof(glm::mat4), uniform_buffer_alignment));
		std::memcpy(mat_data.data(), &proj_mat, sizeof(glm::mat4));

		mUniformData.append_range(mat_data);
	}
}

void Scene::AddMaterial(const cgltf_data* gltf, const cgltf_material* material)
{
	int32_t base_color_index = -1;
	int32_t normal_color_index = -1;
	glm::vec4 base_color_factor = glm::vec4(1.f);

	if (material->has_pbr_metallic_roughness)
	{
		if (material->pbr_metallic_roughness.base_color_texture.texture != nullptr)
		{
			base_color_index = static_cast<int32_t>(cgltf_image_index(gltf, material->pbr_metallic_roughness.base_color_texture.texture->image));
		}
		std::memcpy(&base_color_factor, material->pbr_metallic_roughness.base_color_factor, sizeof(glm::vec4));
	}
	if (material->normal_texture.texture != nullptr)
	{
		normal_color_index = static_cast<int32_t>(cgltf_image_index(gltf, material->normal_texture.texture->image));
	}

	mMaterials.push_back(Scene::Material(base_color_index, normal_color_index, base_color_factor));
}

void Scene::AddImage(const cgltf_image* image)
{
	mImages.push_back(Scene::Image(mImagesData.size(), image->buffer_view->size, image->name));

	std::vector<uint8_t> image_data(image->buffer_view->size);
	std::memcpy(image_data.data(),
		reinterpret_cast<uint8_t*>(image->buffer_view->buffer->data) + image->buffer_view->offset,
		image_data.size()
	);

	mImagesData.append_range(image_data);
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
	mViewMatrixOffset(view_matrix_offset),
	mName(name)
{
}

size_t Scene::CameraInstance::GetCameraIndex() const
{
	return mCameraIndex;
}

size_t Scene::CameraInstance::GetViewMatrixOffset() const
{
	return mViewMatrixOffset;
}

const std::string& Scene::CameraInstance::GetName() const
{
	return mName;
}

Scene::Camera::Camera(const size_t proj_mat_offset)
	:mProjMatrixOffset(proj_mat_offset)
{
}

size_t Scene::Camera::GetProjectionMatrixOffset() const
{
	return mProjMatrixOffset;
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
	const size_t positions_size, 	const size_t positions_offset, 
	const size_t normals_size, const size_t normals_offset, 
	const size_t texcoords_size, const size_t texcoords_offset, 
	const size_t vertex_count, 
	const size_t indices_size, const size_t indices_offset, const size_t index_count, const VkIndexType index_type,
	const int32_t material_index
)
	: mPositionsSize(positions_size),
	mPositionsOffset(positions_offset),
	mNormalsSize(normals_size),
	mNormalsOffset(normals_offset),
	mTexCoordsSize(texcoords_size),
	mTexCoordsOffset(texcoords_offset),
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

size_t Scene::Mesh::Primitive::GetNormalsSize() const
{
	return mNormalsSize;
}

size_t Scene::Mesh::Primitive::GetNormalsOffset() const
{
	return mNormalsOffset;
}

size_t Scene::Mesh::Primitive::GetTexCoordsSize() const
{
	return mTexCoordsSize;
}

size_t Scene::Mesh::Primitive::GetTexcoordsOffset() const
{
	return mTexCoordsOffset;
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

int32_t Scene::Mesh::Primitive::GetMaterialIndex() const
{
	return mMaterialIndex;
}
