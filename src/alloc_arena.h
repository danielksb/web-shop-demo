#ifndef __ALLOC_ARENA_H_
#define __ALLOC_ARENA_H_

#include <stddef.h>

typedef struct {
    char *beg;
    char *end;
} Arena;

#define alloc_arena_new(a, t, n) (t *)alloc_arena(a, sizeof(t), _Alignof(t), n)

void *alloc_arena(Arena *a, ptrdiff_t size, ptrdiff_t align, ptrdiff_t count);

#endif