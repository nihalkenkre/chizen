#include "embree_raytracer.hpp"
#include "embree_raytracer_scene.hpp"
#include "events.hpp"
#include "utils.hpp"

void EmbreeRaytracer::Start(const EmbreeRaytracerScene* scene, const uint32_t width, const uint32_t height, const uint32_t max_samples, float* pixels)
{
	for (uint32_t s = 1; s <= max_samples; ++s)
	{
		tbb::blocked_range2d<uint32_t> render_range(0, static_cast<uint32_t>(mWidth), 0, static_cast<uint32_t>(mHeight));
		tbb::parallel_for(render_range, [this, pixels, scene](const tbb::blocked_range2d<uint32_t>& xy) {
			for (uint32_t y = xy.cols().begin(); y < xy.cols().end(); ++y)
				for (uint32_t x = xy.rows().begin(); x < xy.rows().end(); ++x)
				{
					if (mStopRendering) tbb::task::current_context()->cancel_group_execution();

					/*glm::vec2 pixel_center = glm::vec2(x, y) + 0.5f;
					glm::vec2 in_uv = pixel_center / glm::vec2(mWidth, mHeight);

					glm::vec4 d = glm::vec4(in_uv * 2.f - 1.f, 1, 1);
					glm::vec4 target = scene->GetProjInverse() * d;

					glm::vec4 origin = scene->GetViewInverse() * glm::vec4(0, 0, 0, 1);
					glm::vec4 direction = scene->GetViewInverse() * glm::normalize(target);

					RTCRayHit rayhit = {
						.ray = {
							.org_x = origin.x,
							.org_y = origin.y,
							.org_z = origin.z,
							.tnear = 0.1f,
							.dir_x = direction.x,
							.dir_y = direction.y,
							.dir_z = direction.z,
							.tfar = std::numeric_limits<float>::infinity(),
							.mask = 0xFF,
						},
						.hit = {
							.geomID = RTC_INVALID_GEOMETRY_ID,
							.instID = RTC_INVALID_GEOMETRY_ID,
						},
					};

					RTCIntersectArguments i_args;
					rtcInitIntersectArguments(&i_args);

					i_args.feature_mask = (RTCFeatureFlags)(RTC_FEATURE_FLAG_TRIANGLE);

					rtcTraversableIntersect1(scene->GetTraversable(), &rayhit);

					if (rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID)
					{
						(pixels)[static_cast<uint32_t>((y * mWidth + x) * 4)] = 1.f;
						(pixels)[static_cast<uint32_t>((y * mWidth + x) * 4 + 1)] = 0.f;
						(pixels)[static_cast<uint32_t>((y * mWidth + x) * 4 + 2)] = 0.f;
						(pixels)[static_cast<uint32_t>((y * mWidth + x) * 4 + 3)] = 1.f;
					}
					else
					{
						(pixels)[static_cast<uint32_t>((y * mWidth + x) * 4)] = rand() / static_cast<float>(RAND_MAX);
						(pixels)[static_cast<uint32_t>((y * mWidth + x) * 4 + 1)] = rand() / static_cast<float>(RAND_MAX);
						(pixels)[static_cast<uint32_t>((y * mWidth + x) * 4 + 2)] = rand() / static_cast<float>(RAND_MAX);;
						(pixels)[static_cast<uint32_t>((y * mWidth + x) * 4 + 3)] = 1.f;
					}*/
				}
			});

		SDL_PushEvent(&events.RenderSampleDone);
		if (mStopRendering) break;
	}

	mStopRendering = false;
	SDL_PushEvent(&events.RaytraceStopped);
}

void EmbreeRaytracer::Stop()
{
	mStopRendering = true;
}

EmbreeRaytracer::~EmbreeRaytracer() noexcept
{
}
