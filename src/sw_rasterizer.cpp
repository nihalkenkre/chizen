#include "sw_rasterizer.hpp"
#include "sw_rasterizer_scene.hpp"
#include "events.hpp"

void SWRasterizer::Start(const SWRasterizerScene* scene, const uint32_t width, const uint32_t height, const uint32_t max_samples, const uint32_t cam_index, float* pixels)
{
	glm::mat4 view_matrix = glm::make_mat4(reinterpret_cast<const float*>(scene->GetUniformData().data() + scene->GetCameraInstances()[cam_index].GetViewMatrixOffset()));
	glm::mat4 proj_matrix = glm::make_mat4(reinterpret_cast<const float*>(scene->GetUniformData().data() + scene->GetCameras()[scene->GetCameraInstances()[cam_index].GetCameraIndex()].GetProjectionMatrixOffset()));

	mTriangles.resize(scene->GetMeshInstances().size());

	tbb::blocked_range<size_t> mesh_instance_range(0, scene->GetMeshInstances().size());
	tbb::parallel_for(mesh_instance_range, [this, scene, proj_matrix, view_matrix, width, height](const tbb::blocked_range<size_t>& mi)
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

						size_t internal_index_offset = prim.GetIndexType() == VK_INDEX_TYPE_UINT16 ? index_idx * 2 : index_idx * 4;
						size_t index0 = (scene->GetVertexData().data() + prim.GetIndicesOffset())[internal_index_offset];

						glm::vec4 pos0 = proj_matrix * view_matrix * model_matrix * glm::vec4(positions[index0], 1);
						pos0.x /= pos0.w;
						pos0.y /= pos0.w;
						pos0.z /= pos0.w;

						++index_idx;
						internal_index_offset = prim.GetIndexType() == VK_INDEX_TYPE_UINT16 ? index_idx * 2 : index_idx * 4;
						size_t index1 = (scene->GetVertexData().data() + prim.GetIndicesOffset())[internal_index_offset];

						glm::vec4 pos1 = proj_matrix * view_matrix * model_matrix * glm::vec4(positions[index1], 1);
						pos1.x /= pos1.w;
						pos1.y /= pos1.w;
						pos1.z /= pos1.w;

						++index_idx;
						internal_index_offset = prim.GetIndexType() == VK_INDEX_TYPE_UINT16 ? index_idx * 2 : index_idx * 4;
						size_t index2 = (scene->GetVertexData().data() + prim.GetIndicesOffset())[internal_index_offset];

						glm::vec4 pos2 = proj_matrix * view_matrix * model_matrix * glm::vec4(positions[index2], 1);
						pos2.x /= pos2.w;
						pos2.y /= pos2.w;
						pos2.z /= pos2.w;

						mTriangles[m].push_back(
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
	);

	//for (const auto& mesh_instance : scene->GetMeshInstances())
	//{
	//	glm::mat4 model_matrix = glm::make_mat4(reinterpret_cast<const float*>(scene->GetUniformData().data() + mesh_instance.GetModelMatrixOffset()));
	//	auto mesh = scene->GetMeshes()[mesh_instance.GetMeshIndex()];

	//	for (const auto& prim : mesh.GetPrimitives())
	//	{
	//		const glm::vec3* positions = reinterpret_cast<const glm::vec3*>(scene->GetVertexData().data() + prim.GetPositionsOffset());

	//		for (size_t index_idx = 0; index_idx < prim.GetIndexCount(); ++index_idx)
	//		{
	//			if (mStopRendering) goto shutdown;
	//			size_t internal_index_offset = prim.GetIndexType() == VK_INDEX_TYPE_UINT16 ? index_idx * 2 : index_idx * 4;
	//			size_t index0 = (scene->GetVertexData().data() + prim.GetIndicesOffset())[internal_index_offset];

	//			glm::vec3 pos0 = proj_matrix * view_matrix * model_matrix * glm::vec4(positions[index0], 1);

	//			++index_idx;
	//			internal_index_offset = prim.GetIndexType() == VK_INDEX_TYPE_UINT16 ? index_idx * 2 : index_idx * 4;
	//			size_t index1 = (scene->GetVertexData().data() + prim.GetIndicesOffset())[internal_index_offset];

	//			glm::vec3 pos1 = proj_matrix * view_matrix * model_matrix * glm::vec4(positions[index1], 1);

	//			++index_idx;
	//			internal_index_offset = prim.GetIndexType() == VK_INDEX_TYPE_UINT16 ? index_idx * 2 : index_idx * 4;
	//			size_t index2 = (scene->GetVertexData().data() + prim.GetIndicesOffset())[internal_index_offset];

	//			glm::vec3 pos2 = proj_matrix * view_matrix * model_matrix * glm::vec4(positions[index2], 1);

	//			mTriangles.push_back(Triangle(Point(pos0, glm::vec3(1, 0, 0)), Point(pos1, glm::vec3(0, 1, 0)), Point(pos2, glm::vec3(0, 0, 1))));
	//		}
	//	}
	//}

	for (size_t s = 1; s <= max_samples; ++s)
	{
		tbb::blocked_range<size_t> mesh_instance_range(0, mTriangles.size());
		tbb::parallel_for(mesh_instance_range, [this, pixels, width, height](const tbb::blocked_range<size_t>& mi)
			{
				for (size_t m = mi.begin(); m < mi.end(); ++m)
				{
					tbb::blocked_range<size_t> triangle_range(0, mTriangles[m].size());
					tbb::parallel_for(triangle_range, [this, m, pixels, width, height](const tbb::blocked_range<size_t>& tri)
						{
							for (size_t t = tri.begin(); t < tri.end(); ++t)
							{
								if (this->mStopRendering) tbb::task::current_context()->cancel_group_execution();
								mTriangles[m][t].Draw(pixels, width, height);
							}
						}
					);
				}
			}
		);

		//for (auto const& triangle : mTriangles)
		//{
		//	if (mStopRendering) break;
		//	triangle.Draw(pixels, width);
		//}
		//tbb::blocked_range2d<uint32_t> render_range(0, width, 0, height);
		//tbb::parallel_for(
		//	render_range, [this, pixels, s, max_samples, width](const tbb::blocked_range2d<uint32_t>& xy)
		//	{
		//		for (uint32_t y = xy.cols().begin(); y < xy.cols().end(); ++y)
		//		{
		//			for (uint32_t x = xy.rows().begin(); x < xy.rows().end(); ++x)
		//			{
		//				if (this->mStopRendering)
		//				{
		//					tbb::task::current_context()->cancel_group_execution();
		//				}

		//				uint32_t pixel_idx = (y * width + x) * 4;

		//				pixels[pixel_idx] = static_cast<float>(s) / static_cast<float>(max_samples);
		//				pixels[pixel_idx + 1] = 0;// static_cast<float>(x) / static_cast<float>(width);
		//				pixels[pixel_idx + 2] = 0;// static_cast<float>(x) / static_cast<float>(width);
		//				pixels[pixel_idx + 3] = 1;
		//			}
		//		}
		//	}
		//	);
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
