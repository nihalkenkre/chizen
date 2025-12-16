#include "embree_raytracer_scene.hpp"
#include "scene.hpp"

void EMBREE_CHECK(RTCDevice device)
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
	mScene = rtcNewScene(mDevice); EMBREE_CHECK(mDevice);
	rtcSetSceneBuildQuality(mScene, RTC_BUILD_QUALITY_HIGH); EMBREE_CHECK(mDevice);
	rtcSetSceneFlags(mScene, RTC_SCENE_FLAG_ROBUST); EMBREE_CHECK(mDevice);

	mMeshInstances.reserve(scene.GetMeshInstances().size());
	for (const auto& mesh_instance : scene.GetMeshInstances())
	{
		RTCGeometry geom = rtcNewGeometry(mDevice, RTC_GEOMETRY_TYPE_INSTANCE); EMBREE_CHECK(mDevice);

		rtcSetGeometryInstancedScene(geom, mScene); EMBREE_CHECK(mDevice);
		rtcSetGeometryTransform(
			geom, 1,
			RTC_FORMAT_FLOAT3X4_COLUMN_MAJOR,
			scene.GetUniformData().data() + mesh_instance.GetModelMatrixOffset()); EMBREE_CHECK(mDevice);

		rtcCommitGeometry(geom); EMBREE_CHECK(mDevice);
		mMeshInstances.push_back(EmbreeRaytracerScene::MeshInstance(rtcAttachGeometry(mScene, geom))); EMBREE_CHECK(mDevice);
		rtcReleaseGeometry(geom);
	}

	mMeshes.reserve(scene.GetMeshes().size());
	for (const auto& mesh : scene.GetMeshes())
	{
		std::vector<EmbreeRaytracerScene::Mesh::Primitive> primitives;
		primitives.reserve(mesh.GetPrimitives().size());

		for (const auto& prim : mesh.GetPrimitives())
		{
			RTCGeometry geom = rtcNewGeometry(mDevice, RTC_GEOMETRY_TYPE_TRIANGLE); EMBREE_CHECK(mDevice);

			void* pos_buff = rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(float) * 3, prim.GetVertexCount()); EMBREE_CHECK(mDevice);
			std::memcpy(pos_buff, scene.GetVertexData().data() + prim.GetPositionsOffset(), prim.GetPositionsSize());

			VkIndexType idx_type = prim.GetIndexType();
			std::vector<uint32_t> idxs(prim.GetIndexCount());
			
			if (idx_type == VK_INDEX_TYPE_UINT16)
			{
				std::vector<uint16_t> idxs_16(prim.GetIndexCount());
				std::memcpy(idxs_16.data(), scene.GetVertexData().data() + prim.GetIndicesOffset(), prim.GetIndicesSize());
				
				idxs.append_range(idxs_16);
			}
			else if (idx_type == VK_INDEX_TYPE_UINT32)
			{
				std::memcpy(idxs.data(), scene.GetVertexData().data() + prim.GetIndicesOffset(), prim.GetIndicesSize());
			}

			void* idx_buff = nullptr;
			idx_buff = rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(uint32_t) * 3, prim.GetIndexCount()); EMBREE_CHECK(mDevice);
			std::memcpy(idx_buff, idxs.data(), prim.GetIndicesSize());

			rtcCommitGeometry(geom); EMBREE_CHECK(mDevice);
			primitives.push_back(EmbreeRaytracerScene::Mesh::Primitive(rtcAttachGeometry(mScene, geom))); EMBREE_CHECK(mDevice);
			rtcReleaseGeometry(geom); EMBREE_CHECK(mDevice);
		}

		mMeshes.push_back(EmbreeRaytracerScene::Mesh(primitives));
	}

	rtcCommitScene(mScene); EMBREE_CHECK(mDevice);
	mTraversable = rtcGetSceneTraversable(mScene); EMBREE_CHECK(mDevice);
}

EmbreeRaytracerScene::~EmbreeRaytracerScene() noexcept
{
	rtcReleaseScene(mScene);
	rtcReleaseDevice(mDevice);
}

RTCTraversable EmbreeRaytracerScene::GetTraversable() const
{
	return mTraversable;
}

RTCScene EmbreeRaytracerScene::GetScene() const
{
	return mScene;
}

RTCDevice EmbreeRaytracerScene::GetDevice() const
{
	return mDevice;
}

EmbreeRaytracerScene::Mesh::Mesh(std::vector<Primitive> primitives)
	: mPrimitives(primitives)
{

}

EmbreeRaytracerScene::Mesh::Primitive::Primitive(unsigned int geom_id)
	: mGeomId(geom_id)
{
}

unsigned int EmbreeRaytracerScene::Mesh::Primitive::GetGeomId() const
{
	return mGeomId;
}

const std::vector<EmbreeRaytracerScene::Mesh::Primitive>& EmbreeRaytracerScene::Mesh::GetPrimitives() const
{
	return mPrimitives;
}

EmbreeRaytracerScene::MeshInstance::MeshInstance(unsigned int geom_id)
	: mGeomId(geom_id)
{
}

unsigned int EmbreeRaytracerScene::MeshInstance::GetGeomId() const
{
	return mGeomId;
}
