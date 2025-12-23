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
