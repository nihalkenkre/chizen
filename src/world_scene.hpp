#pragma once

#include "scene.hpp"

#include <string>
#include <vector>

class world_scene : public scene
{
public:
	world_scene(const std::string& path, renderer* r);

	void handle_mouse_move(const POINT mouse_pos) override;
	void update() override;
	void render(renderer* r) override;
	~world_scene();

private:
	void import_scene_data(const cgltf_data* data);

	struct mesh
	{
		std::string name;
		uint8_t prims_count;

		bool operator==(const std::string& name)
		{
			return this->name == name;
		}
	};

	struct material
	{
		std::string name;
		std::vector<mesh> meshes;

		bool operator==(const std::string& name)
		{
			return this->name == name;
		}
	};

	struct scene_data
	{
		std::vector<material> mats;
	};

	std::unique_ptr<scene_data> sd;
};