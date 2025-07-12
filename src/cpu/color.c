#include "color.h"

void color_blend(vec4 src, vec4 dst, vec4 result)
{
    result[0] = (src[0] * src[3]) + (dst[0] * (1 - src[3]));
    result[1] = (src[1] * src[3]) + (dst[1] * (1 - src[3]));
    result[2] = (src[2] * src[3]) + (dst[2] * (1 - src[3]));
    result[3] = src[3];
}
