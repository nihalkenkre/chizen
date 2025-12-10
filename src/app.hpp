#pragma once

class VulkanInterface;
class Rasterizer;
class Display;
class Raytracer;
class ImGUIState;
class ImageResource;
class Scene;

class App
{
public:
	App() = delete;
	App(SDL_Window* window, const std::string& current_path);

	App(const App& other) = delete;
	App& operator=(const App& other) = delete;

	~App() noexcept;

	void ProcessEvent(SDL_Event* event);
	void RunRasterizer();
	void RunDisplay();
	void StartRaytracer();
	void RecreateRenderTarget();
	void StopRaytracing();

	void RecreateSwapchain();

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
	std::unique_ptr<Raytracer> mRaytracer = nullptr;
	std::unique_ptr<ImageResource> mFinalRenderTarget = nullptr;
	std::unique_ptr<ImGUIState> mImGUIState = nullptr;
	std::unique_ptr<Scene> mScene = nullptr;
	SDL_Window* mWindow = nullptr;
	VkExtent2D mRenderTargetExtent = { 1280, 720 };
	std::thread mRaytraceThread = {};
	bool mIsTrackingMouse = false;
	bool mIsRaytracing = false;
};
