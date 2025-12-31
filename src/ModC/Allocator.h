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

#define INTERN_MODC_DEBUG_PRINT_ALLOC 1
#if INTERN_MODC_DEBUG_PRINT_ALLOC
    #include <stdio.h>
    #include <inttypes.h>
    
    #ifndef INTERN_PRINT_MODC_PRINT_TRACE
        #define INTERN_PRINT_MODC_PRINT_TRACE(data) \
            do \
            { \
                printf("%s: %p\n", __func__, (void*)data); \
                fflush(stdout); \
            } while(0)
    #endif
#else
    #ifndef INTERN_PRINT_MODC_PRINT_TRACE
        #define INTERN_PRINT_MODC_PRINT_TRACE(data) do {(void)data;} while(0)
    #endif
#endif

static inline void* ModC_Allocator_Malloc(const ModC_Allocator this, uint64_t size)
{
    if(!this.Malloc)
        return NULL;
    return this.Malloc(this.Allocator, size);
}

static inline void* ModC_Allocator_Realloc(const ModC_Allocator this, void* data, uint64_t size)
{
    if(!this.Realloc)
        return NULL;
    return this.Realloc(this.Allocator, data, size);
}

static inline void ModC_Allocator_Free(const ModC_Allocator this, void* data)
{
    if(!this.Free)
        return;
    this.Free(this.Allocator, data);
}

static inline void ModC_Allocator_Destroy(ModC_Allocator* this)
{
    if(!this || !this->Destroy)
        return;
    if(this->Type == ModC_AllocatorTypeOwnedArena)
        this->Destroy(this->Allocator);
    *this = (ModC_Allocator){0};
}

//Arena
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
    INTERN_PRINT_MODC_PRINT_TRACE(allocator);
    arena_destroy(allocator);
}

static inline ModC_Allocator ModC_CreateOwnedArenaAllocator(uint64_t allocateSize)
{
    ModC_Allocator retAlloc =
            {
                .Type = ModC_AllocatorTypeOwnedArena,
                .Allocator = arena_create(allocateSize),
                .Malloc = &ModC_ArenaMalloc,
                .Realloc = NULL,
                .Free = NULL,
                .Destroy = &ModC_ArenaDestroy,
            };
    INTERN_PRINT_MODC_PRINT_TRACE(retAlloc.Allocator);
    return retAlloc;
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

//Heap
static inline void* ModC_HeapMalloc(void* allocator, uint64_t size) 
{ 
    (void)allocator; 
    void* retPtr = malloc(size); 
    INTERN_PRINT_MODC_PRINT_TRACE(retPtr);
    return retPtr;
}

static inline void* ModC_HeapRealloc(void* allocator, void* data, uint64_t size) 
{
    (void)allocator;
    void* retPtr = realloc(data, size);
    INTERN_PRINT_MODC_PRINT_TRACE(retPtr);
    return retPtr;
}

static inline void ModC_HeapFree(void* allocator, void* data) 
{
    (void)allocator; 
    INTERN_PRINT_MODC_PRINT_TRACE(data);
    free(data); 
}

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
