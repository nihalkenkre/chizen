#pragma once

class SWRasterizerScene;

class SWRasterizer
{
public:
   SWRasterizer() {}

   SWRasterizer(const SWRasterizer& other) = delete;
   SWRasterizer& operator=(const SWRasterizer& other) = delete;

   void Start(const SWRasterizerScene* scene,const uint32_t width, const uint32_t height, const uint32_t max_samples, float* pixels);
   void Stop();

   ~SWRasterizer() noexcept;

private:
   bool mStopRendering = false;
};
