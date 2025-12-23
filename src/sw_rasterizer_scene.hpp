#pragma once

#include "scene.hpp"

class SWRasterizerScene : public Scene
{
public:
   SWRasterizerScene() = delete;

   SWRasterizerScene(const Scene& scene);

   SWRasterizerScene(const SWRasterizerScene& other) = delete;
   SWRasterizerScene& operator=(const SWRasterizerScene& other) = delete;

   ~SWRasterizerScene() noexcept;
};