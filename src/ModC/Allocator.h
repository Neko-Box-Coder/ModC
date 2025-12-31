#ifndef MODC_ALLOCATOR_H
#define MODC_ALLOCATOR_H

/* Docs
Just read the code
*/

#include "arena-allocator/arena.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

typedef enum
{
    ModC_AllocatorTypeHeap,
    ModC_AllocatorTypeSharedArena,
    ModC_AllocatorTypeOwnedArena,
} ModC_AllocatorType;


typedef struct
{
    ModC_AllocatorType Type;
    void* Allocator;
    void* (*Malloc)(void* allocator, uint64_t size);
    void* (*Realloc)(void* allocator, void* data, uint64_t size);
    void (*Free)(void* allocator, void* data);
    void (*Destroy)(void* allocator);
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

static inline void ModC_Allocator_Destroy(ModC_Allocator* this)
{
    if(!this || !this->Destroy)
        return;
    #if INTERN_MODC_DEBUG_PRINT_ALLOC
        printf("ModC_Allocator_Destroy in: %p\n", this);
        fflush(stdout);
    #endif
    if(this->Type == ModC_AllocatorTypeOwnedArena)
        this->Destroy(this->Allocator);
    *this = (ModC_Allocator){0};
}

static inline void* ModC_ArenaMalloc(void* allocator, uint64_t size) 
{
    if(!allocator)
        return NULL;
    return arena_alloc(allocator, size); 
}

static inline void ModC_ArenaDestroy(void* allocator) 
{
    if(!allocator)
        return;
    arena_destroy(allocator);
}

static inline ModC_Allocator ModC_CreateOwnedArenaAllocator(uint64_t allocateSize)
{
    return  (ModC_Allocator)
            {
                .Type = ModC_AllocatorTypeOwnedArena,
                .Allocator = arena_create(allocateSize),
                .Malloc = &ModC_ArenaMalloc,
                .Realloc = NULL,
                .Free = NULL,
                .Destroy = &ModC_ArenaDestroy,
            };
}

static inline ModC_Allocator ModC_CreateSharedArenaAllocator(Arena* arena)
{
    return  (ModC_Allocator)
            {
                .Type = ModC_AllocatorTypeSharedArena,
                .Allocator = arena,
                .Malloc = &ModC_ArenaMalloc,
                .Realloc = NULL,
                .Free = NULL,
                .Destroy = NULL,
            };
}


static inline void* ModC_HeapMalloc(void* allocator, uint64_t size) 
{ 
    (void)allocator; 
    return malloc(size); 
}

static inline void* ModC_HeapRealloc(void* allocator, void* data, uint64_t size) 
{
    (void)allocator;
    return realloc(data, size); 
}

static inline void ModC_HeapFree(void* allocator, void* data) { (void)allocator; free(data); }

static inline ModC_Allocator ModC_CreateHeapAllocator(void)
{
    return  (ModC_Allocator)
            {
                .Type = ModC_AllocatorTypeHeap,
                .Allocator = NULL,
                .Malloc = &ModC_HeapMalloc,
                .Realloc = &ModC_HeapRealloc,
                .Free = &ModC_HeapFree,
                .Destroy = NULL
            };
}


#endif
