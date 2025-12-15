#include "embree_raytracer.hpp"
#include "scene.hpp"
#include "events.hpp"
#include "utils.hpp"

void EmbreeRaytracer::Start(const EmbreeRaytracerScene* scene, const uint32_t max_samples, float* pixels)
{
	SDL_CHECK(SDL_PushEvent(&events.RaytraceStopped));

	tbb::blocked_range2d<float> render_range(0, static_cast<float>(mWidth), 0, static_cast<float>(mHeight));
	tbb::parallel_for(render_range, [this, pixels](const tbb::blocked_range2d<float>& xy) {
		for (float x = xy.rows().begin(); x < xy.rows().end(); ++x)
		{
			for (float y = xy.cols().begin(); y < xy.cols().end(); ++y)
			{
				(pixels)[static_cast<uint32_t>((y * mWidth + x) * 4)] = x / mWidth;
				(pixels)[static_cast<uint32_t>((y * mWidth + x) * 4 + 1)] = x/mWidth;
				(pixels)[static_cast<uint32_t>((y * mWidth + x) * 4 + 2)] = x/mWidth;
				(pixels)[static_cast<uint32_t>((y * mWidth + x) * 4 + 3)] = 1.f;
			}
		}
	});
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
