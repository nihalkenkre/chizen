#pragma once

#include <iostream>

#include "renderer.hpp"

class vulkan_renderer : public renderer
{
public:
    void import_scene(cgltf_data *data) override;
    void resize(const WORD width, const WORD height) override;
    ~vulkan_renderer();
};