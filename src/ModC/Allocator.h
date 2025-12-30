#ifndef MODC_ALLOCATOR_H
#define MODC_ALLOCATOR_H

#include <arena-allocator/arena.h>

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct
{
    void* Allocator;
    void* (*Malloc)(void* allocator, uint64_t size);
    void* (*Realloc)(void* allocator, void* data, uint64_t size);
    void (*Free)(void* allocator, void* data);
} ModC_Allocator;

#define INTERN_MODC_DEBUG_PRINT_ALLOC 0
#if INTERN_MODC_DEBUG_PRINT_ALLOC
    #include <stdio.h>
    #include <inttypes.h>
#endif

static inline void* ModC_Allocator_Malloc(const ModC_Allocator this, uint64_t size)
{
    if(!this.Malloc)
        return NULL;
    void* retPtr = this.Malloc(this.Allocator, size);
    #if INTERN_MODC_DEBUG_PRINT_ALLOC
        printf("ModC_Allocator_Malloc out: %p\n", retPtr);
        fflush(stdout);
    #endif
    return retPtr;
}

static inline void* ModC_Allocator_Realloc(const ModC_Allocator this, void* data, uint64_t size)
{
    if(!this.Realloc)
        return NULL;
    #if INTERN_MODC_DEBUG_PRINT_ALLOC
        printf("ModC_Allocator_Realloc in: %p with size %" PRIu64 "\n", data, size);
    #endif
    void* retPtr = this.Realloc(this.Allocator, data, size);
    
    #if INTERN_MODC_DEBUG_PRINT_ALLOC
        printf("ModC_Allocator_Realloc out: %p\n", retPtr);
        fflush(stdout);
    #endif
    return retPtr;
}

static inline void ModC_Allocator_Free(const ModC_Allocator this, void* data)
{
    if(!this.Free)
        return;
    #if INTERN_MODC_DEBUG_PRINT_ALLOC
        printf("ModC_Allocator_Free in: %p\n", data);
        fflush(stdout);
    #endif
    this.Free(this.Allocator, data);
}


#if 0
void* ModC_ArenaMalloc(void* allocator, uint64_t size) { return arena_alloc(allocator, size); }
void* ModC_ArenaRealloc(void* allocator, void* data, uint64_t size) 
{
    (void)data;
    
    return arena_alloc(allocator, size);
}
void ModC_ArenaFree(void* allocator, void* data) { (void)allocator; (void)data; }
static inline ModC_Allocator ModC_CreateArenaAllocator(uint64_t allocateSize)
{
    return  (ModC_Allocator)
            {
                .Allocator = arena_create(allocateSize),
                &ModC_ArenaMalloc,
                &ModC_ArenaRealloc,
                &ModC_ArenaFree
            };
}
#endif

//Heap allocator?

void* ModC_HeapMalloc(void* allocator, uint64_t size) { (void)allocator; return malloc(size); }
void* ModC_HeapRealloc(void* allocator, void* data, uint64_t size) 
{
    (void)allocator;
    return realloc(data, size); 
}
void ModC_HeapFree(void* allocator, void* data) { (void)allocator; free(data); }

static inline ModC_Allocator ModC_CreateHeapAllocator(void)
{
    return  (ModC_Allocator)
            {
                .Allocator = NULL,
                &ModC_HeapMalloc,
                &ModC_HeapRealloc,
                &ModC_HeapFree
            };
}


#endif
