#pragma once

class EmbreeRaytracerScene;

class EmbreeRaytracer
{
public:
   EmbreeRaytracer() {}

   EmbreeRaytracer(const EmbreeRaytracer& other) = delete;
   EmbreeRaytracer& operator=(const EmbreeRaytracer& other) = delete;

   void Start(EmbreeRaytracerScene* scene, const uint32_t width, const uint32_t height, const uint32_t max_samples, const uint32_t cam_index, float* pixels);
   void Stop();

   ~EmbreeRaytracer() noexcept;

private:

   bool mStopRendering = false;
};