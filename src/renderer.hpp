#pragma once

#include <Windows.h>
#include <iostream>

#include <cgltf/cgltf.h>

enum RENDERING_API
{
    DX12,
    VULKAN,
};

class renderer
{
public:
    renderer() {}
    virtual void import_scene(cgltf_data *data) = 0;
    virtual void resize(const WORD width, const WORD height) = 0;
    virtual void render_background(const POINT pt) { std::cout << __FUNCTION__ << '\n'; }
    virtual ~renderer() {}

    bool is_inited = false;
};