#include "primitive_optix.h"
#include "utils.h"

#include <stdlib.h>
#include <string.h>

primitive_optix primitive_optix_create(cgltf_primitive* curr_prim, mat4 node_xform, const OptixDeviceContext ctx, const cudaStream_t stream)
{
	primitive_optix p = { 0 };

	p.indices_count = curr_prim->indices->count;
	p.indices = malloc(curr_prim->indices->count * sizeof(uint32_t));

	if (curr_prim->indices->component_type == cgltf_component_type_r_32u)
	{
		memcpy(p.indices, (void*)((size_t)curr_prim->indices->buffer_view->buffer->data + curr_prim->indices->buffer_view->offset + curr_prim->indices->offset), p.indices_count * sizeof(uint32_t));
	}
	else if (curr_prim->indices->component_type == cgltf_component_type_r_16u)
	{
		uint16_t* idxs = (uint16_t*)((size_t)curr_prim->indices->buffer_view->buffer->data + curr_prim->indices->buffer_view->offset + curr_prim->indices->offset);

		for (size_t i = 0; i < p.indices_count; ++i)
		{
			p.indices[i] = idxs[i];
		}
	}

	for (size_t a = 0; a < curr_prim->attributes_count; ++a)
	{
		cgltf_attribute* curr_attr = curr_prim->attributes + a;

		if (strcmp(curr_attr->name, "POSITION") == 0)
		{
			p.positions_count = curr_attr->data->count;
			p.positions = malloc(curr_attr->data->buffer_view->size);
			memcpy(p.positions, (void*)((size_t)curr_attr->data->buffer_view->buffer->data + curr_attr->data->buffer_view->offset + curr_attr->data->offset), curr_attr->data->buffer_view->size);
		}
		else if (strcmp(curr_attr->name, "NORMAL") == 0)
		{
			p.normals_count = curr_attr->data->count;
			p.normals = malloc(curr_attr->data->buffer_view->size);
			memcpy(p.normals, (void*)((size_t)curr_attr->data->buffer_view->buffer->data + curr_attr->data->buffer_view->offset + curr_attr->data->offset), curr_attr->data->buffer_view->size);
		}
		else if (strcmp(curr_attr->name, "TEXCOORD_0") == 0)
		{
			p.uvs_count = curr_attr->data->count;
			p.uvs = malloc(curr_attr->data->buffer_view->size);
			memcpy(p.uvs, (void*)((size_t)curr_attr->data->buffer_view->buffer->data + curr_attr->data->buffer_view->offset + curr_attr->data->offset), curr_attr->data->buffer_view->size);
		}
	}

	return p;
}

void primitive_optix_destroy(primitive_optix p)
{
	if (p.positions != NULL)
	{
		free(p.positions);
		p.positions = NULL;
		p.positions_count = 0;
	}

	if (p.normals != NULL)
	{
		free(p.normals);
		p.normals = NULL;
		p.normals_count = 0;
	}
	
	if (p.uvs != NULL)
	{
		free(p.uvs);
		p.uvs = NULL;
		p.uvs_count = 0;
	}

	if (p.indices != NULL)
	{
		free(p.indices);
		p.indices = NULL;
		p.indices_count = 0;
	}
}
