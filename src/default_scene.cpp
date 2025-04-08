#include "default_scene.hpp"
#include "renderer.hpp"

#include <DirectXMath.h>

void default_scene::handle_mouse_move(const POINT mouse_pos)
{
    this->mouse_pos = mouse_pos;
}

void default_scene::update()
{
}

void default_scene::render(renderer *r)
{
    r->begin_frame();
    float color[] = {
        (float)this->mouse_pos.x / (r->wnd_rect.right - r->wnd_rect.left),
        0.0,
        (float)this->mouse_pos.y / (r->wnd_rect.bottom - r->wnd_rect.top),
        1.0,
    };
    r->clear_frame(color);
    r->end_frame();
}

default_scene::~default_scene()
{
}