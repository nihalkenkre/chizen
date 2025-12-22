#pragma once

typedef struct Material
{
	float base_color_factor[4];
	int base_color_tex_idx = -1;
} Material;