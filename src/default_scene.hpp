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