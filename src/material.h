#pragma once

#include <optix.h>
#include <cuda_runtime.h>

typedef struct material
{
	CUdeviceptr d_material;
} material;
