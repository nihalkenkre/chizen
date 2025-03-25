#include "default_scene.hpp"

#include "renderer.hpp"

void default_scene::handle_mouse_move(const POINT mouse_pos)
{
    this->mouse_pos = mouse_pos;
}

void default_scene::update()
{
}

void default_scene::render(renderer *r)
{   
    r->render_background(this->mouse_pos);
}

default_scene::~default_scene()
{
}