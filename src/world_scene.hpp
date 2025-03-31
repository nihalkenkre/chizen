#pragma once

#include "scene.hpp"

#include <string>
#include <iostream>

class world_scene : public scene
{
public:
    world_scene(const std::string &path);

    void handle_mouse_move(const POINT mouse_pos) override;
    void update() override;
    void render(renderer* r) override;
    ~world_scene();

private:
    void import_scene(const cgltf_data* data);
};