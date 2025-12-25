#include "sw_rasterizer.hpp"
#include "sw_rasterizer_scene.hpp"
#include "events.hpp"

void SWRasterizer::Start(const SWRasterizerScene* scene, const uint32_t width, const uint32_t height, const uint32_t max_samples, const uint32_t cam_index, float* pixels)
{
	mMeshInstanceTriangles.clear();

	glm::mat4 view_proj_matrix = glm::make_mat4(reinterpret_cast<const float*>(scene->GetUniformData().data() + scene->GetCameraInstances()[cam_index].GetViewProjMatrixOffset()));
	mMeshInstanceTriangles.resize(scene->GetMeshInstances().size());

	tbb::blocked_range<size_t> mesh_instance_range(0, scene->GetMeshInstances().size());
	tbb::parallel_for(mesh_instance_range, [this, scene, view_proj_matrix, width, height](const tbb::blocked_range<size_t>& mi)
		{
			for (size_t m = mi.begin(); m < mi.end(); ++m)
			{
				auto mesh_instance = scene->GetMeshInstances()[m];

				glm::mat4 model_matrix = glm::make_mat4(reinterpret_cast<const float*>(scene->GetUniformData().data() + mesh_instance.GetModelMatrixOffset()));
				auto mesh = scene->GetMeshes()[mesh_instance.GetMeshIndex()];
				for (const auto& prim : mesh.GetPrimitives())
				{
					const glm::vec3* positions = reinterpret_cast<const glm::vec3*>(scene->GetVertexData().data() + prim.GetPositionsOffset());
					for (size_t index_idx = 0; index_idx < prim.GetIndexCount(); ++index_idx)
					{
						if (mStopRendering) tbb::task::current_context()->cancel_group_execution();

						size_t index0 = 0;
						if (prim.GetIndexType() == VK_INDEX_TYPE_UINT16)
						{
							index0 = static_cast<size_t>(reinterpret_cast<const uint16_t*>(scene->GetVertexData().data() + prim.GetIndicesOffset())[index_idx]);
						}
						else if (prim.GetIndexType() == VK_INDEX_TYPE_UINT32)
						{
							index0 = static_cast<size_t>(reinterpret_cast<const uint32_t*>(scene->GetVertexData().data() + prim.GetIndicesOffset())[index_idx]);
						}

						glm::vec4 pos0 = view_proj_matrix * model_matrix * glm::vec4(positions[index0], 1);

						++index_idx;
						size_t index1 = 0;
						if (prim.GetIndexType() == VK_INDEX_TYPE_UINT16)
						{
							index1 = static_cast<size_t>(reinterpret_cast<const uint16_t*>(scene->GetVertexData().data() + prim.GetIndicesOffset())[index_idx]);
						}
						else if (prim.GetIndexType() == VK_INDEX_TYPE_UINT32)
						{
							index1 = static_cast<size_t>(reinterpret_cast<const uint32_t*>(scene->GetVertexData().data() + prim.GetIndicesOffset())[index_idx]);
						}

						glm::vec4 pos1 = view_proj_matrix * model_matrix * glm::vec4(positions[index1], 1);

						++index_idx;
						size_t index2 = 0;
						if (prim.GetIndexType() == VK_INDEX_TYPE_UINT16)
						{
							index2 = static_cast<size_t>(reinterpret_cast<const uint16_t*>(scene->GetVertexData().data() + prim.GetIndicesOffset())[index_idx]);
						}
						else if (prim.GetIndexType() == VK_INDEX_TYPE_UINT32)
						{
							index2 = static_cast<size_t>(reinterpret_cast<const uint32_t*>(scene->GetVertexData().data() + prim.GetIndicesOffset())[index_idx]);
						}

						glm::vec4 pos2 = view_proj_matrix * model_matrix * glm::vec4(positions[index2], 1);

						if (!ArePointsToBeClipped(pos0, pos1, pos2))
						{
							pos0 /= pos0.w;
							pos1 /= pos1.w;
							pos2 /= pos2.w;

							mMeshInstanceTriangles[m].push_back(
								Triangle(
									Point(glm::vec3(pos0), glm::vec3(1, 0, 0)),
									Point(glm::vec3(pos1), glm::vec3(0, 1, 0)),
									Point(glm::vec3(pos2), glm::vec3(0, 0, 1)),
									width,
									height
								)
							);
						}
					}
				}
			}
		}
	);

	for (size_t s = 1; s <= max_samples; ++s)
	{
		tbb::blocked_range<size_t> mesh_instance_range(0, mMeshInstanceTriangles.size());
		tbb::parallel_for(mesh_instance_range, [this, pixels, width, height](const tbb::blocked_range<size_t>& mi)
			{
				for (size_t m = mi.begin(); m < mi.end(); ++m)
				{
					tbb::blocked_range<size_t> triangle_range(0, mMeshInstanceTriangles[m].size());
					tbb::parallel_for(triangle_range, [this, m, pixels, width, height](const tbb::blocked_range<size_t>& tri)
						{
							for (size_t t = tri.begin(); t < tri.end(); ++t)
							{
								if (this->mStopRendering) tbb::task::current_context()->cancel_group_execution();
								mMeshInstanceTriangles[m][t].Draw(pixels, width, height);
							}
						}
					);
				}
			}
		);

		Sleep(2); // Need this to see the updates happening on the viewport. maybe because of very less work happening in the threads.. !?!?
		SDL_PushEvent(&events.RenderSampleDone);

		if (mStopRendering) goto shutdown;
	}

shutdown:

	mStopRendering = false;
	SDL_PushEvent(&events.RenderStopped);
}

void SWRasterizer::Stop()
{
	mStopRendering = true;
}

SWRasterizer::~SWRasterizer() noexcept
{
}

bool SWRasterizer::ArePointsToBeClipped(const glm::vec4 p0, const glm::vec4 p1, const glm::vec4 p2) const
{
	return (
		p0.x > p0.w || p0.x < -p0.w || p0.y > p0.w || p0.y < -p0.w || p0.z > p0.w || p0.z < -p0.w || p0.w <= 0 &&
		p1.x > p1.w || p1.x < -p1.w || p1.y > p1.w || p1.y < -p1.w || p1.z > p1.w || p1.z < -p1.w || p1.w <= 0 &&
		p2.x > p2.w || p2.x < -p2.w || p2.y > p2.w || p2.y < -p2.w || p2.z > p2.w || p2.z < -p2.w || p2.w <= 0
		);
}
