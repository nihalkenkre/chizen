#pragma once

class VulkanInterface;
class Viewport;
class Display;
class VulkanRaytracer;
class EmbreeRaytracer;
class SWRasterizer;
class ImGUIState;
class ImageResource;
class HostBufferResource;
class ViewportScene;
class VulkanRaytracerScene;
class EmbreeRaytracerScene;
class SWRasterizerScene;

class App
{
public:
	App() = delete;
	App(SDL_Window* window, const std::string& current_path);

	App(const App& other) = delete;
	App& operator=(const App& other) = delete;

	~App() noexcept;

	void ProcessEvent(SDL_Event* event);
	void Iterate();
	void RunViewport();
	void RunDisplay();
	void RecreateRenderTarget();
	void StopRendering();

	void RecreateViewportSwapchain();

	VulkanInterface* GetVulkanInterface() const;
	Viewport* GetRasterizer() const;
	Display* GetDisplay() const;

	SDL_Window* GetWindow() const;
	bool& IsTrackingMouse();
	float* GetDeltaMousePosition();
	float* GetLastMousePosition();
	float& GetZoomLevel();

	uint32_t& GetMaxSamples();

	ImGUIState* GetImGUIState() const;
	bool& IsRaytracing();

	VkExtent2D& GetFinalRenderTargetExtent();
	ImageResource* GetFinalRenderTarget() const;

private:
	float mDeltaMousePosition[2] = {};
	float mLastMousePosition[2] = {};
	float mZoomLevel = 1.f;
	uint32_t mMaxSamples = 1024;
	std::string mCurrentPath;
	std::unique_ptr<VulkanInterface> mVulkanInterface = nullptr;
	std::unique_ptr<Viewport> mViewport = nullptr;
	std::unique_ptr<Display> mDisplay = nullptr;
	std::unique_ptr<VulkanRaytracer> mVulkanRaytracer = nullptr;
	std::unique_ptr<EmbreeRaytracer> mEmbreeRaytracer = nullptr;
	std::unique_ptr<ImGUIState> mImGUIState = nullptr;
	std::unique_ptr<ViewportScene> mViewportScene = nullptr;
	std::unique_ptr<VulkanRaytracerScene> mVulkanRaytracerScene = nullptr;
	std::unique_ptr<EmbreeRaytracerScene> mEmbreeRaytacerScene = nullptr;
	std::unique_ptr<SWRasterizerScene> mSWRasterizerScene = nullptr;
	std::unique_ptr<SWRasterizer> mSWRasterizer = nullptr;
	std::unique_ptr<ImageResource> mFinalRenderTarget = nullptr;
	std::unique_ptr<HostBufferResource> mStagingRenderTarget = nullptr;
	SDL_Window* mWindow = nullptr;
	VkExtent2D mFinalRenderTargetExtent = { 1280, 720 };
	std::thread mRenderThread = {};
	bool mIsTrackingMouse = false;
	bool mDisplayRender = false;
	uint8_t mRenderType = 0;
};
