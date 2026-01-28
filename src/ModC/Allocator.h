#ifndef MODC_ALLOCATOR_H
#define MODC_ALLOCATOR_H

/* Docs
Just read the code
*/

#include "static_assert.h/assert.h"
#include "arena-allocator/arena.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>

typedef enum
{
    ModC_AllocatorType_Heap,
    ModC_AllocatorType_SharedArena,
    ModC_AllocatorType_OwnedArena,
    ModC_AllocatorType_Count,   //3
} ModC_AllocatorType;

typedef struct
{
    ModC_AllocatorType Type;
    void* Allocator;
} ModC_Allocator;

#define INTERN_MODC_DEBUG_PRINT_ALLOC 1
#if INTERN_MODC_DEBUG_PRINT_ALLOC
    #include <stdio.h>
    #include <inttypes.h>
    
    #ifndef INTERN_PRINT_MODC_PRINT_TRACE
        #define INTERN_PRINT_MODC_PRINT_TRACE(...) \
            do \
            { \
                printf("%s: \n", __func__); \
                printf("    " __VA_ARGS__); \
                fflush(stdout); \
            } while(0)
    #endif
    
    #define INTERN_PRINT_CALL(callFunc, ...) \
        (printf("Calling " #callFunc " from %s()\n", __func__), fflush(stdout), callFunc(__VA_ARGS__))
#else
    #ifndef INTERN_PRINT_MODC_PRINT_TRACE
        #define INTERN_PRINT_MODC_PRINT_TRACE(...) do {} while(0)
    #endif
    
    #define INTERN_PRINT_CALL(callFunc, ...) callFunc(__VA_ARGS__)
#endif

static inline void* ModC_Allocator_Malloc(const ModC_Allocator* this, uint64_t size)
{
    void* retPtr = NULL;
    if(!this)
        goto ret;
    
    static_assert((int)ModC_AllocatorType_Count == 3, "");
    switch(this->Type)
    {
        case ModC_AllocatorType_Heap:
            retPtr = malloc(size);
            INTERN_PRINT_MODC_PRINT_TRACE("retPtr: %p, %" PRIu64 "\n", retPtr, size);
            break;
        case ModC_AllocatorType_SharedArena:
        case ModC_AllocatorType_OwnedArena:
            if(!this->Allocator)
            {
                INTERN_PRINT_MODC_PRINT_TRACE("Trying to allocate with NULL arena");
                goto ret;
            }
            retPtr = arena_alloc(this->Allocator, size);
            break;
        default:
            break;
    }
    
    ret:;
    if(!retPtr)
    {
        INTERN_PRINT_MODC_PRINT_TRACE(  "Allocation failed for allocator: %p, "
                                        "with size %" PRIu64 "\n", this->Allocator, size);
    }
    return retPtr;
}

#define ModC_Allocator_Malloc(...) INTERN_PRINT_CALL(ModC_Allocator_Malloc, __VA_ARGS__)

static inline void* ModC_Allocator_Realloc( const ModC_Allocator* this, 
                                            void* data, 
                                            uint64_t size)
{
    void* retPtr = NULL;
    if(!this)
        goto ret;
    
    static_assert((int)ModC_AllocatorType_Count == 3, "");
    switch(this->Type)
    {
        case ModC_AllocatorType_Heap:
            retPtr = realloc(data, size);
            INTERN_PRINT_MODC_PRINT_TRACE("retPtr: %p, %" PRIu64 "\n", retPtr, size);
            break;
        case ModC_AllocatorType_SharedArena:
        case ModC_AllocatorType_OwnedArena:
            INTERN_PRINT_MODC_PRINT_TRACE("Arena realloc: %p, %" PRIu64 "\n", this->Allocator, size);
            break;
        default:
            break;
    }
    
    ret:;
    if(!retPtr)
    {
        INTERN_PRINT_MODC_PRINT_TRACE(  "Reallocation failed for allocator: %p, "
                                        "with size %" PRIu64 "\n", this->Allocator, size);
    }
    return retPtr;
}

#define ModC_Allocator_Realloc(...) INTERN_PRINT_CALL(ModC_Allocator_Realloc, __VA_ARGS__)

static inline void ModC_Allocator_Free(const ModC_Allocator* this, void* data)
{
    if(!this)
        return;
    
    static_assert((int)ModC_AllocatorType_Count == 3, "");
    switch(this->Type)
    {
        case ModC_AllocatorType_Heap:
            INTERN_PRINT_MODC_PRINT_TRACE("Free: %p\n", data);
            free(data);
            break;
        case ModC_AllocatorType_SharedArena:
        case ModC_AllocatorType_OwnedArena:
            break;
        default:
            break;
    }
}

#define ModC_Allocator_Free(...) INTERN_PRINT_CALL(ModC_Allocator_Free, __VA_ARGS__)

static inline void ModC_Allocator_Destroy(ModC_Allocator* this)
{
    if(!this)
        return;
    
    static_assert((int)ModC_AllocatorType_Count == 3, "");
    switch(this->Type)
    {
        case ModC_AllocatorType_Heap:
        case ModC_AllocatorType_SharedArena:
            break;
        case ModC_AllocatorType_OwnedArena:
            if(!this->Allocator)
                return;
            INTERN_PRINT_MODC_PRINT_TRACE("this->Allocator: %p\n", this->Allocator);
            arena_destroy(this->Allocator);
            break;
        default:
            break;
    }
    *this = (ModC_Allocator){0};
}

#define ModC_Allocator_Destroy(...) INTERN_PRINT_CALL(ModC_Allocator_Destroy, __VA_ARGS__)

static inline ModC_Allocator ModC_Allocator_Share(ModC_Allocator* this)
{
    if(!this)
        return (ModC_Allocator){0};
    
    //INTERN_PRINT_MODC_PRINT_TRACE(this);
    
    static_assert((int)ModC_AllocatorType_Count == 3, "");
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

#define ModC_Allocator_Share(...) INTERN_PRINT_CALL(ModC_Allocator_Share, __VA_ARGS__)

static inline ModC_Allocator ModC_CreateArenaAllocator(uint64_t allocateSize)
{
    ModC_Allocator retAlloc =
    {
        .Type = ModC_AllocatorType_OwnedArena,
        .Allocator = arena_create(allocateSize),
    };
    INTERN_PRINT_MODC_PRINT_TRACE("retAlloc.Allocator: %p\n", retAlloc.Allocator);
    return retAlloc;
}

#define ModC_CreateArenaAllocator(...) INTERN_PRINT_CALL(ModC_CreateArenaAllocator, __VA_ARGS__)

static inline ModC_Allocator ModC_CreateHeapAllocator(void)
{
    return  (ModC_Allocator)
            {
                .Type = ModC_AllocatorType_Heap,
                .Allocator = NULL,
            };
}

#define ModC_CreateHeapAllocator(...) INTERN_PRINT_CALL(ModC_CreateHeapAllocator, __VA_ARGS__)

#endif
