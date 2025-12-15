#pragma once

class EmbreeRaytracerScene;

class EmbreeRaytracer
{
public:
   EmbreeRaytracer() {}

   EmbreeRaytracer(const EmbreeRaytracer& other) = delete;
   EmbreeRaytracer& operator=(const EmbreeRaytracer& other) = delete;

   void Start(const EmbreeRaytracerScene* scene, const uint32_t max_samples, float* pixels);
   void Stop();

   void RecreateRenderResources(const uint32_t width, const uint32_t height);

   ~EmbreeRaytracer() noexcept;

private:
   uint32_t mWidth = 1280;
   uint32_t mHeight = 720;

   bool mStopRendering = false;
};