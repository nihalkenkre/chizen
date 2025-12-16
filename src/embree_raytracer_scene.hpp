#pragma once

class Scene;

class EmbreeRaytracerScene
{
public:
	EmbreeRaytracerScene() = delete;

	EmbreeRaytracerScene(const Scene& scene);

	EmbreeRaytracerScene(const EmbreeRaytracerScene& other) = delete;
	EmbreeRaytracerScene& operator=(const EmbreeRaytracerScene& other) = delete;

	~EmbreeRaytracerScene() noexcept;

	class MeshInstance
	{
	public:
		MeshInstance(unsigned int geom_id);

		unsigned int GetGeomId() const;

	private:
		unsigned int mGeomId = 0;
	};

	class Mesh
	{
	public:
		class Primitive
		{
		public:
			Primitive(unsigned int geom_id);

			unsigned int GetGeomId() const;

		private:
			unsigned int mGeomId = 0;
		};

		Mesh(std::vector<EmbreeRaytracerScene::Mesh::Primitive> primitives);

		const std::vector<Primitive>& GetPrimitives() const;

	private:
		std::vector<Primitive> mPrimitives;
	};

	class CameraInstance
	{
	public:
	};

	class Camera
	{

	};

	RTCTraversable GetTraversable() const;
	RTCScene GetScene() const;
	RTCDevice GetDevice() const;

private:
	RTCTraversable mTraversable = nullptr;
	RTCDevice mDevice = nullptr;
	RTCScene mScene = nullptr;

	std::vector<MeshInstance> mMeshInstances;
	std::vector<Mesh> mMeshes;
};

