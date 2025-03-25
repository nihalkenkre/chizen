#pragma once

#include <iostream>

#include "renderer.hpp"

class vulkan_renderer : public renderer
{
public:
    void import_scene(cgltf_data *data) override;
    ~vulkan_renderer();
};