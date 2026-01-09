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

	void Render(const uint32_t x, const uint32_t y, const uint32_t width, const uint32_t height, const uint32_t cam_index, float* pixels) const;

	RTCTraversable GetTraversable() const;
	RTCDevice GetDevice() const;

private:
	struct CameraMatrices
	{
		glm::mat4 mViewInverse;
		glm::mat4 mProjInverse;
	};
	
	struct Image
	{
		std::vector<uint8_t> mData;
	};

	struct MaterialInfo
	{
		glm::vec4 mBaseColorFactor;
		int32_t mBaseColorTexture;
		int32_t mNormalTexture;
	};

	struct PrimGeom
	{
		unsigned int mId;
		MaterialInfo mMaterialInfo;
	};

	RTCTraversable mTraversable = nullptr;
	RTCScene mScene = nullptr;
	RTCDevice mDevice = nullptr;

	std::vector<PrimGeom> mPrimGeoms;
	std::vector<CameraMatrices> mCameraMatrices;
	std::vector<Image> mImages;
};

