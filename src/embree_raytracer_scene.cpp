#include "embree_raytracer_scene.hpp"
#include "scene.hpp"

#include <stb_image.h>

inline void EMBREE_CHECK(RTCDevice device)
{
#ifdef _DEBUG
	RTCError err = rtcGetDeviceError(device);
	if (err > RTC_ERROR_NONE)
		std::println("{}", rtcGetErrorString(err));
#endif
}

EmbreeRaytracerScene::EmbreeRaytracerScene(const Scene& scene)
{
	mDevice = rtcNewDevice(nullptr); EMBREE_CHECK(mDevice);
	mScene = rtcNewScene(mDevice);

	for (const auto& mesh_instance : scene.GetMeshInstances())
	{
		auto mesh = scene.GetMeshes()[mesh_instance.GetMeshIndex()];

		for (const auto& prim : mesh.GetPrimitives())
		{
			auto geom = rtcNewGeometry(mDevice, RTC_GEOMETRY_TYPE_TRIANGLE); EMBREE_CHECK(mDevice);

			void* indices = rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(uint32_t), prim.GetIndexCount()); EMBREE_CHECK(mDevice);
			std::memcpy(indices, scene.GetVertexData().data() + prim.GetIndicesOffset(), prim.GetIndicesSize()); EMBREE_CHECK(mDevice);

			glm::vec3* vertex_data = reinterpret_cast<glm::vec3*>(rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(glm::vec3), prim.GetVertexCount())); EMBREE_CHECK(mDevice);
			std::memcpy(vertex_data, scene.GetVertexData().data() + prim.GetPositionsOffset(), prim.GetPositionsSize()); EMBREE_CHECK(mDevice);

			for (size_t v = 0; v < prim.GetVertexCount(); ++v)
			{
				vertex_data[v] = glm::make_mat4(reinterpret_cast<const float*>(scene.GetCameraMatricesData().data() + mesh_instance.GetModelMatrixOffset())) * glm::vec4(vertex_data[v], 1);
			}

			rtcCommitGeometry(geom); EMBREE_CHECK(mDevice);

			auto material = scene.GetMaterials()[prim.GetMaterialIndex()];

			auto mat_info = MaterialInfo{
				.mBaseColorFactor = material.GetBaseColorFactor(),
				.mBaseColorTexture = material.GetBaseNormalMetalroughIndex().x,
				.mNormalTexture = material.GetBaseNormalMetalroughIndex().y,
			};

			auto prim_geom = PrimGeom{
				.mId = rtcAttachGeometry(mScene, geom),
				.mMaterialInfo = mat_info,
			};

			mPrimGeoms.push_back(prim_geom); EMBREE_CHECK(mDevice);
			rtcReleaseGeometry(geom); EMBREE_CHECK(mDevice);
		}
	}

	rtcCommitScene(mScene); EMBREE_CHECK(mDevice);
	mTraversable = rtcGetSceneTraversable(mScene); EMBREE_CHECK(mDevice);

	for (const auto& camera_instance : scene.GetCameraInstances())
	{
		mCameraMatrices.push_back(
			CameraMatrices{
				.mViewInverse = glm::make_mat4(reinterpret_cast<const float*>(scene.GetCameraMatricesData().data() + camera_instance.GetViewInverseMatrixOffset())),
				.mProjInverse = glm::make_mat4(reinterpret_cast<const float*>(scene.GetCameraMatricesData().data() + camera_instance.GetProjInverseMatrixOffset())),
			}
			);
	}

	for (const auto& image : scene.GetImages())
	{
		int x, y, c;
		uint8_t* pixels_data = stbi_load_from_memory(scene.GetImagesData().data() + image.GetDataOffset(), static_cast<int>(image.GetDataSize()), &x, &y, &c, 3);

		std::vector<uint8_t> pixels(x * y * 3);
		std::memcpy(pixels.data(), pixels_data, pixels.size());

		mImages.push_back(
			Image{
				.mData = pixels,
			}
			);

		stbi_image_free(pixels_data);
	}
}

EmbreeRaytracerScene::~EmbreeRaytracerScene() noexcept
{
	rtcReleaseScene(mScene);
	rtcReleaseDevice(mDevice);
}

void EmbreeRaytracerScene::Render(const uint32_t x, const uint32_t y, const uint32_t width, const uint32_t height, const uint32_t cam_index, float* pixels) const
{
	glm::vec2 pixel_center = glm::vec2(x, y) + 0.5f;
	glm::vec2 in_uv = pixel_center / glm::vec2(width, height);

	glm::vec4 d = glm::vec4(in_uv * 2.f - 1.f, 1, 1);
	glm::vec4 target = mCameraMatrices[cam_index].mProjInverse * d;

	glm::vec4 origin = mCameraMatrices[cam_index].mViewInverse * glm::vec4(0, 0, 0, 1);
	glm::vec4 direction = mCameraMatrices[cam_index].mViewInverse * glm::normalize(target);

	RTCRayHit rayhit = {
		.ray = {
			.org_x = origin.x,
			.org_y = origin.y,
			.org_z = origin.z,
			.tnear = 0.001f,
			.dir_x = direction.x,
			.dir_y = direction.y,
			.dir_z = direction.z,
			.tfar = std::numeric_limits<float>::infinity(),
			.mask = 0xFF,
		},
		.hit = {
			.geomID = RTC_INVALID_GEOMETRY_ID,
			.instID = RTC_INVALID_GEOMETRY_ID,
		},
	};

	RTCIntersectArguments i_args;
	rtcInitIntersectArguments(&i_args); EMBREE_CHECK(mDevice);

	i_args.feature_mask = (RTCFeatureFlags)(RTC_FEATURE_FLAG_TRIANGLE);

	rtcTraversableIntersect1(mTraversable, &rayhit); EMBREE_CHECK(mDevice);

	if (rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID)
	{
		auto it = std::find_if(mPrimGeoms.begin(), mPrimGeoms.end(), [&](PrimGeom prim_geom) { return prim_geom.mId == rayhit.hit.geomID; });

		glm::vec4 pixel_color = glm::vec4(1.f);

		if (it != mPrimGeoms.end())
		{
			pixel_color = it->mMaterialInfo.mBaseColorFactor;
		}

		(pixels)[static_cast<uint32_t>((y * width + x) * 4)] = pixel_color.x;
		(pixels)[static_cast<uint32_t>((y * width + x) * 4 + 1)] = pixel_color.y;
		(pixels)[static_cast<uint32_t>((y * width + x) * 4 + 2)] = pixel_color.z;
		(pixels)[static_cast<uint32_t>((y * width + x) * 4 + 3)] = 1.f;
	}
	else
	{
		(pixels)[static_cast<uint32_t>((y * width + x) * 4)] = rand() / static_cast<float>(RAND_MAX);
		(pixels)[static_cast<uint32_t>((y * width + x) * 4 + 1)] = rand() / static_cast<float>(RAND_MAX);
		(pixels)[static_cast<uint32_t>((y * width + x) * 4 + 2)] = rand() / static_cast<float>(RAND_MAX);
		(pixels)[static_cast<uint32_t>((y * width + x) * 4 + 3)] = 1.f;
	}
}

RTCTraversable EmbreeRaytracerScene::GetTraversable() const
{
	return mTraversable;
}

RTCDevice EmbreeRaytracerScene::GetDevice() const
{
	return mDevice;
}
