#include "embree_raytracer.hpp"
#include "scene.hpp"
#include "events.hpp"
#include "utils.hpp"

void EmbreeRaytracer::Start(const EmbreeRaytracerScene* scene, const uint32_t max_samples, float* pixels)
{
	tbb::blocked_range2d<uint32_t> render_range(0, static_cast<uint32_t>(mWidth), 0, static_cast<uint32_t>(mHeight));
	tbb::parallel_for(render_range, [this, pixels](const tbb::blocked_range2d<uint32_t>& xy) {
		for (uint32_t y = xy.cols().begin(); y < xy.cols().end(); ++y)
			for (uint32_t x = xy.rows().begin(); x < xy.rows().end(); ++x)
			{
				{
					pixels[static_cast<uint32_t>((y * mWidth + x) * 4)] = static_cast<float>(x) / mWidth;
					pixels[static_cast<uint32_t>((y * mWidth + x) * 4 + 1)] = static_cast<float>(y) / mHeight;
					pixels[static_cast<uint32_t>((y * mWidth + x) * 4 + 2)] = 0.f;
					pixels[static_cast<uint32_t>((y * mWidth + x) * 4 + 3)] = 1.f;
				}
			}
		});

	SDL_PushEvent(&events.RaytraceStopped);
}

void EmbreeRaytracer::Stop()
{
	mStopRendering = true;
}

EmbreeRaytracer::~EmbreeRaytracer() noexcept
{
}

void EmbreeRaytracer::RecreateRenderResources(const uint32_t width, const uint32_t height)
{
	mWidth = width;
	mHeight = height;
}
