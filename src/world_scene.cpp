#include "world_scene.hpp"
#include "dx12_renderer.hpp"

#include <commctrl.h>

#include <cgltf/cgltf.h>

world_scene::world_scene(const std::string& file_path, renderer* r)
{
	cgltf_options options = {};
	cgltf_data* data = nullptr;

	if (cgltf_parse_file(&options, file_path.c_str(), &data) != cgltf_result_success ||
		cgltf_validate(data) != cgltf_result_success ||
		cgltf_load_buffers(&options, data, file_path.c_str()) != cgltf_result_success)
	{
		std::cerr << "ERR Could not parse gltf file\n";
	}
	else
	{
		this->import_scene_data(data);
		r->clear_scene_data();
		r->import_scene_data(file_path);
		cgltf_free(data);
	}
}

void world_scene::import_scene_data(const cgltf_data* data)
{
	sd = std::make_unique<scene_data>();

	for (cgltf_size n = 0; n < data->nodes_count; ++n)
	{
		cgltf_node* curr_node = data->nodes + n;

		if (curr_node->mesh == nullptr)
			continue;

		cgltf_mesh* curr_mesh = curr_node->mesh;

		for (cgltf_size p = 0; p < curr_mesh->primitives_count; ++p)
		{
			cgltf_primitive* curr_prim = curr_mesh->primitives + p;

			if (curr_prim->material == nullptr)
			{
				continue;
			}

			auto mat_it = std::find(sd->mats.begin(), sd->mats.end(), std::string(curr_prim->material->name));

			if (mat_it != sd->mats.end())
			{
				auto msh_it = std::find(mat_it->meshes.begin(), mat_it->meshes.end(), std::string(curr_prim->material->name));

				if (msh_it != mat_it->meshes.end())
				{
					++msh_it->prims_count;
				}
				else
				{
					mesh msh = {
						.name = std::string(curr_mesh->name),
						.prims_count = 1,
					};

					mat_it->meshes.push_back(msh);
				}
			}
			else
			{
				mesh msh = {
					.name = std::string(curr_mesh->name),
					.prims_count = 1,
				};

				material mat = {
					.name = std::string(curr_prim->material->name),
					.meshes = {msh},
				};

				sd->mats.push_back(mat);
			}
		}
	}
}

void world_scene::handle_mouse_move(const POINT mouse_pos)
{
	this->mouse_pos = mouse_pos;
}

void world_scene::update()
{
}

void world_scene::render(renderer* r)
{
	r->begin_frame();
	float color[] = { 0.2, 0.2, 0.2, 1 };
	r->clear_frame(color);
	//r->render_world();
	r->end_frame();
}

world_scene::~world_scene()
{
}