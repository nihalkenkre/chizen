#pragma once

#include "scene.hpp"
#include "dx12_renderer.hpp"

#include <string>
#include <iostream>

class default_scene : public scene
{
public:
    void handle_mouse_move(const POINT mouse_pos) override;
    void update() override;
    void render(renderer *r) override;
    ~default_scene();
};

// namespace default_scene
// {
//     void init(const std::string path = "")
//     {
//         std::cout << "default_scene init\n";
//     }

//     void set_renderer(const renderer &r)
//     {
//         std::cout << "default_scene set_renderer\n";
//     }

//     void update()
//     {
//         std::cout << "default_scene update\n";
//     }

//     void render()
//     {
//         std::cout << "default_scene render\n";
//     }

//     void shutdown() {}
// }