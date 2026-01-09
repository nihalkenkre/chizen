#include "embree_raytracer.hpp"
#include "embree_raytracer_scene.hpp"
#include "events.hpp"
#include "utils.hpp"

void EmbreeRaytracer::Start(EmbreeRaytracerScene* scene, const uint32_t width, const uint32_t height, const uint32_t max_samples, const uint32_t cam_index, float* pixels)
{
	for (uint32_t s = 1; s <= max_samples; ++s)
	{
		tbb::blocked_range2d<uint32_t> render_range(0, static_cast<uint32_t>(width), 0, static_cast<uint32_t>(height));
		tbb::parallel_for(render_range, [this, scene, width, height, cam_index, pixels](const tbb::blocked_range2d<uint32_t>& xy)
			{
				for (uint32_t y = xy.cols().begin(); y < xy.cols().end(); ++y)
				{
					for (uint32_t x = xy.rows().begin(); x < xy.rows().end(); ++x)
					{
						if (mStopRendering) tbb::task::current_context()->cancel_group_execution();

						scene->Render(x, y, width, height, cam_index, pixels);
					}
				}
			}
		);

		SDL_PushEvent(&events.RenderSampleDone);
		if (mStopRendering) break;
	}

	mStopRendering = false;
	SDL_PushEvent(&events.RenderStopped);
}

void EmbreeRaytracer::Stop()
{
	mStopRendering = true;
}

EmbreeRaytracer::~EmbreeRaytracer() noexcept
{
}
