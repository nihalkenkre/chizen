#pragma once

class VulkanInterface;
class Rasterizer;
class Display;
class VulkanRaytracer;
class EmbreeRaytracer;
class ImGUIState;
class ImageResource;
class BufferResource;
class RasterizerScene;
class VulkanRaytracerScene;
class EmbreeRaytracerScene;

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
	void RunRasterizer();
	void RunDisplay();
	void StartRaytracing();
	void RecreateRenderTarget();
	void StopRaytracing();

	void RecreateRasterSwapchain();

	VulkanInterface* GetVulkanInterface() const;
	Rasterizer* GetRasterizer() const;
	Display* GetDisplay() const;

	SDL_Window* GetWindow() const;
	bool& IsTrackingMouse();
	float* GetDeltaMousePosition();
	float* GetLastMousePosition();
	float& GetZoomLevel();

	uint32_t& GetMaxSamples();

	ImGUIState* GetImGUIState();
	bool& IsRaytracing();

	VkExtent2D& GetRenderTargetExtent();
	ImageResource* GetFinalRenderTarget() const;

private:
	float mDeltaMousePosition[2] = {};
	float mLastMousePosition[2] = {};
	float mZoomLevel = 1.f;
	uint32_t mMaxSamples = 1024;
	std::unique_ptr<VulkanInterface> mVulkanInterface = nullptr;
	std::unique_ptr<Rasterizer> mRasterizer = nullptr;
	std::unique_ptr<Display> mDisplay = nullptr;
	std::unique_ptr<VulkanRaytracer> mVulkanRaytracer = nullptr;
	std::unique_ptr<EmbreeRaytracer> mEmbreeRaytracer = nullptr;
	std::unique_ptr<ImGUIState> mImGUIState = nullptr;
	std::unique_ptr<RasterizerScene> mRasterizerScene = nullptr;
	std::unique_ptr<VulkanRaytracerScene> mVulkanRaytracerScene = nullptr;
	std::unique_ptr<EmbreeRaytracerScene> mEmbreeRaytacerScene = nullptr;
	std::unique_ptr<ImageResource> mFinalRenderTarget = nullptr;
	std::unique_ptr<BufferResource> mEmbreeRenderTarget = nullptr;
	SDL_Window* mWindow = nullptr;
	VkExtent2D mRenderTargetExtent = { 1280, 720 };
	std::thread mRaytraceThread = {};
	bool mIsTrackingMouse = false;
	bool mDisplayRender = false;
	uint8_t mRaytracerType = 0;
};
