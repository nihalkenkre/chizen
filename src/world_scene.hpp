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

// namespace world_scene
// {
//     void init(const std::string path = "")
//     {
//         std::cout << "world_scene init\n";
//     }

//     void set_renderer(const renderer &r)
//     {
//         std::cout << "world_scene set_renderer\n";
//     }

//     void update()
//     {
//         std::cout << "world_scene update\n";
//     }

//     void render()
//     {
//         std::cout << "world_scene render\n";
//     }

//     void shutdown() {}
// }