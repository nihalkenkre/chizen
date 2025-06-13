#include "utils.h"

size_t ALIGNED_SIZE(size_t value, size_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}