#include "alloc_arena.h"

#include <stdint.h>
#include <string.h>

void *alloc_arena(Arena *a, ptrdiff_t size, ptrdiff_t align, ptrdiff_t count)
{
    ptrdiff_t padding = -(uintptr_t)a->beg & (align - 1);
    ptrdiff_t available = a->end - a->beg - padding;
    if (available < 0 || count > available/size) {
        return NULL;
    }
    void *p = a->beg + padding;
    a->beg += padding + count*size;
    return memset(p, 0, count*size);
}
