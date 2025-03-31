#pragma once

#include <Windows.h>

#include "renderer.hpp"

#include <string>
#include <memory>

class scene
{
public:
    virtual void handle_mouse_move(const POINT mouse_pos) = 0;
    virtual void update() = 0;
    virtual void render(renderer *r) = 0;
    virtual ~scene() {}

protected:
    POINT mouse_pos;
};