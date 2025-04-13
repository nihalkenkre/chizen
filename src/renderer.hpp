#pragma once

#include <Windows.h>

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
	virtual void import_scene_data(const cgltf_data* data) = 0;
	virtual void resize(const UINT width, const UINT height) = 0;
	virtual void begin_frame() = 0;
	virtual void clear_frame(const float color[]) = 0;
	virtual void render_world() = 0;
	virtual void end_frame() = 0;
	virtual void clear_scene_data() = 0;
	virtual ~renderer() {}

	bool is_inited = false;
	RECT wnd_rect = { 0,0,0,0 };
};