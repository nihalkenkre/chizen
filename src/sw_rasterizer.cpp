#include "sw_rasterizer.hpp"
#include "sw_rasterizer_scene.hpp"
#include "events.hpp"
#include "utils.hpp"

void SWRasterizer::Start(const SWRasterizerScene* scene, const uint32_t width, const uint32_t height, const uint32_t max_samples, const uint32_t cam_index, float* color)
{
	mMeshInstanceTriangles.clear();
	mMeshInstanceTriangles.resize(scene->GetMeshInstances().size());

	glm::mat4 view_proj_matrix = glm::make_mat4(reinterpret_cast<const float*>(scene->GetCameraMatricesData().data() + scene->GetCameraInstances()[cam_index].GetViewProjMatrixOffset()));
	glm::mat4 cam_xform_matrix = glm::inverse(glm::make_mat4(reinterpret_cast<const float*>(scene->GetCameraMatricesData().data() + scene->GetCameraInstances()[cam_index].GetViewInverseMatrixOffset())));

	float z_near = scene->GetCameras()[scene->GetCameraInstances()[cam_index].GetCameraIndex()].GetZNear();
	float z_far = scene->GetCameras()[scene->GetCameraInstances()[cam_index].GetCameraIndex()].GetZFar();

	float* depth = new float[width * height];
	std::fill_n(depth, width * height, 1.f);

	srand(static_cast<unsigned int>(time(NULL)));
	tbb::blocked_range<size_t> mesh_instance_range(0, scene->GetMeshInstances().size());
	tbb::parallel_for(mesh_instance_range, [&](const tbb::blocked_range<size_t>& mi)
		{
			for (size_t m = mi.begin(); m < mi.end(); ++m)
			{
				auto mesh_instance = scene->GetMeshInstances()[m];

				glm::mat4 model_matrix = glm::make_mat4(reinterpret_cast<const float*>(scene->GetCameraMatricesData().data() + mesh_instance.GetModelMatrixOffset()));

				auto mesh = scene->GetMeshes()[mesh_instance.GetMeshIndex()];
				for (const auto& prim : mesh.GetPrimitives())
				{
					mMeshInstanceTriangles[m].reserve(prim.GetIndexCount() / 3);

					const glm::vec3* positions = reinterpret_cast<const glm::vec3*>(scene->GetVertexData().data() + prim.GetPositionsOffset());
					for (size_t index_idx = 0; index_idx < prim.GetIndexCount(); ++index_idx)
					{
						if (mStopRendering) tbb::task::current_context()->cancel_group_execution();

						const uint32_t* indices = reinterpret_cast<const uint32_t*>(scene->GetVertexData().data() + prim.GetIndicesOffset());

						size_t index0 = static_cast<size_t>(indices[index_idx]);
						glm::vec4 pos0 = view_proj_matrix * model_matrix * glm::vec4(positions[index0], 1);

						++index_idx;
						size_t index1 = static_cast<size_t>(indices[index_idx]);
						glm::vec4 pos1 = view_proj_matrix * model_matrix * glm::vec4(positions[index1], 1);

						++index_idx;
						size_t index2 = static_cast<size_t>(indices[index_idx]);
						glm::vec4 pos2 = view_proj_matrix * model_matrix * glm::vec4(positions[index2], 1);

						if (!ArePointsToBeClipped(pos0, pos1, pos2))
						{
							glm::vec4 p0 = pos0 / pos0.w;
							glm::vec4 p1 = pos1 / pos1.w;
							glm::vec4 p2 = pos2 / pos2.w;

							p0.x = (p0.x + 1) * 0.5f * (width - 1);
							p0.y = (p0.y + 1) * 0.5f * (height - 1);
							p0.z = (pos0.w - z_near) / (z_far - z_near);

							p1.x = (p1.x + 1) * 0.5f * (width - 1);
							p1.y = (p1.y + 1) * 0.5f * (height - 1);
							p1.z = (pos1.w - z_near) / (z_far - z_near);

							p2.x = (p2.x + 1) * 0.5f * (width - 1);
							p2.y = (p2.y + 1) * 0.5f * (height - 1);
							p2.z = (pos2.w - z_near) / (z_far - z_near);

							//std::vector<VertexData>vertex_data(prim.GetVerticesDataSize() / sizeof(VertexData));
							//std::memcpy(
							//	vertex_data.data(), 
							//	scene->GetVertexData().data() + prim.GetVerticesDataOffset(), 
							//	prim.GetVerticesDataSize()
							//);

							const VertexData* vertex_data = reinterpret_cast<const VertexData*>(
								scene->GetVertexData().data() + prim.GetVerticesDataOffset()
								);

							//if (!Triangle::IsBackFacing(
							//	vertex_data[index0].normal,
							//	vertex_data[index1].normal,
							//	vertex_data[index2].normal,
							//	model_matrix,
							//	cam_xform_matrix
							//))
							//{
								mMeshInstanceTriangles[m].push_back(
									Triangle(
										Point(glm::vec3(p0), glm::vec3(1, 0, 0)),
										Point(glm::vec3(p1), glm::vec3(0, 1, 0)),
										Point(glm::vec3(p2), glm::vec3(0, 0, 1)),
										width,
										height
									)
								);
							//}
						}
					}
				}
			}
		}
	);

	for (size_t s = 1; s <= max_samples; ++s)
	{
		tbb::blocked_range<size_t> mesh_instance_range(0, mMeshInstanceTriangles.size());
		tbb::parallel_for(mesh_instance_range, [this, color, depth, width, height](const tbb::blocked_range<size_t>& mi)
			{
				for (size_t m = mi.begin(); m < mi.end(); ++m)
				{
					tbb::blocked_range<size_t> triangle_range(0, mMeshInstanceTriangles[m].size());
					tbb::parallel_for(triangle_range, [this, m, color, depth, width, height](const tbb::blocked_range<size_t>& tri)
						{
							for (size_t t = tri.begin(); t < tri.end(); ++t)
							{
								if (this->mStopRendering) tbb::task::current_context()->cancel_group_execution();
								mMeshInstanceTriangles[m][t].Draw(color, depth, width, height);
							}
						}
					);
				}
			}
		);

		Sleep(2); // Need this to see the updates happening on the viewport. Maybe the TBB threads choking the main thread.
		SDL_PushEvent(&events.RenderSampleDone);

		if (mStopRendering) goto shutdown;
	}

shutdown:

	delete[] depth;

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
		(p0.x > p0.w && p1.x > p1.w && p2.x > p0.w) || (p0.x < -p0.w && p1.x < -p1.w && p2.x < p2.w) ||
		(p0.y > p0.w && p1.y > p1.w && p2.y > p0.w) || (p0.y < -p0.w && p1.y < -p1.w && p2.y < p2.w) ||
		(p0.z > p0.w && p1.z > p1.w && p2.z > p0.w) || (p0.z < -p0.w && p1.z < -p1.w && p2.z < p2.w) ||
		(p0.w < 0 && p1.w < 0 && p2.w)
		);
}
