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
    ModC_AllocatorType_Heap,
    ModC_AllocatorType_SharedArena,
    ModC_AllocatorType_OwnedArena,
    ModC_AllocatorType_Count,
} ModC_AllocatorType;

typedef struct
{
    ModC_AllocatorType Type;
    void* Allocator;
} ModC_Allocator;

#define INTERN_MODC_DEBUG_PRINT_ALLOC 0
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

static inline void* ModC_Allocator_Malloc(const ModC_Allocator* this, uint64_t size)
{
    void* retPtr = NULL;
    if(!this)
        goto ret;
    
    //TODO: Assert count
    switch(this->Type)
    {
        case ModC_AllocatorType_Heap:
            retPtr = malloc(size);
            break;
        case ModC_AllocatorType_SharedArena:
        case ModC_AllocatorType_OwnedArena:
            if(!this->Allocator)
                goto ret;
            retPtr = arena_alloc(this->Allocator, size);
            break;
        default:
            break;
    }
    
    ret:;
    INTERN_PRINT_MODC_PRINT_TRACE(retPtr);
    return retPtr;
}

static inline void* ModC_Allocator_Realloc(   const ModC_Allocator* this, 
                                                    void* data, 
                                                    uint64_t size)
{
    void* retPtr = NULL;
    if(!this)
        goto ret;
    
    //TODO: Assert count
    switch(this->Type)
    {
        case ModC_AllocatorType_Heap:
            retPtr = realloc(data, size);
            break;
        case ModC_AllocatorType_SharedArena:
        case ModC_AllocatorType_OwnedArena:
            break;
        default:
            break;
    }
    
    ret:;
    INTERN_PRINT_MODC_PRINT_TRACE(retPtr);
    return retPtr;
}

static inline void ModC_Allocator_Free(const ModC_Allocator* this, void* data)
{
    if(!this)
        return;
    
    INTERN_PRINT_MODC_PRINT_TRACE(data);
    
    //TODO: Assert count
    switch(this->Type)
    {
        case ModC_AllocatorType_Heap:
            free(data);
            break;
        case ModC_AllocatorType_SharedArena:
        case ModC_AllocatorType_OwnedArena:
            break;
        default:
            break;
    }
}

static inline void ModC_Allocator_Destroy(ModC_Allocator* this)
{
    if(!this)
        return;
    
    INTERN_PRINT_MODC_PRINT_TRACE(this);
    
    *this = (ModC_Allocator){0};
    
    //TODO: Assert count
    switch(this->Type)
    {
        case ModC_AllocatorType_Heap:
        case ModC_AllocatorType_SharedArena:
            break;
        case ModC_AllocatorType_OwnedArena:
            if(!this->Allocator)
                return;
            arena_destroy(this->Allocator);
            break;
        default:
            break;
    }
}

static inline ModC_Allocator ModC_Allocator_Share(ModC_Allocator* this)
{
    if(!this)
        return (ModC_Allocator){0};
    
    INTERN_PRINT_MODC_PRINT_TRACE(this);
    
    //TODO: Assert count
    switch(this->Type)
    {
        case ModC_AllocatorType_Heap:
            return *this;
        case ModC_AllocatorType_SharedArena:
            return *this;
        case ModC_AllocatorType_OwnedArena:
        {
            ModC_Allocator retAlloc = *this;
            retAlloc.Type = ModC_AllocatorType_SharedArena;
            return retAlloc;
        }
        default:
            return (ModC_Allocator){0};
    }
}

static inline ModC_Allocator ModC_CreateArenaAllocator(uint64_t allocateSize)
{
    ModC_Allocator retAlloc =
    {
        .Type = ModC_AllocatorType_OwnedArena,
        .Allocator = arena_create(allocateSize),
    };
    INTERN_PRINT_MODC_PRINT_TRACE(retAlloc.Allocator);
    return retAlloc;
}

static inline ModC_Allocator ModC_ShareArenaAllocator(Arena* arena)
{
    INTERN_PRINT_MODC_PRINT_TRACE(arena);
    return  (ModC_Allocator)
            {
                .Type = ModC_AllocatorType_SharedArena,
                .Allocator = arena,
            };
}

static inline ModC_Allocator ModC_CreateHeapAllocator(void)
{
    return  (ModC_Allocator)
            {
                .Type = ModC_AllocatorType_Heap,
                .Allocator = NULL,
            };
}

#endif
