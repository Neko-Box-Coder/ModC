#ifndef MODC_ALLOCATOR_H
#define MODC_ALLOCATOR_H

/* Docs
Define `MODC_ALLOCATOR_NO_CANARY` to disable canary checks

Just read the code
*/

#include "ModC/Assert.h"

#include "static_assert.h/assert.h"
#include "arena-allocator/arena.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <stdbool.h>

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

struct ModC_ArenaWrapper;

typedef struct ModC_ArenaWrapper
{
    Arena* CurrentArena;
    struct ModC_ArenaWrapper* NextArena;
    struct ModC_ArenaWrapper* PrevArena;
} ModC_ArenaWrapper;




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

typedef union
{
    void* Ptr;
    uint64_t Size;
} PtrOrSize;

//When `NULL` is passed to `allocPtr`, it returns additional bytes needed.
//Otherwise, returns the pointer for `allocSize`
static inline PtrOrSize ModC_SetupCanariesAndSize(void* allocPtr, uint64_t allocSize)
{
    #if MODC_ALLOCATOR_NO_CANARY
    {
        if(!allocPtr)
        return (PtrOrSize){ .Size = allocSize + sizeof(uint64_t) };
    
        static_assert(sizeof(char) == 1, "");
        memcpy(allocPtr, &allocSize, sizeof(allocSize));
        allocPtr = (char*)allocPtr + 8;
        return (PtrOrSize){ .Ptr = allocPtr };
    }
    #else
    {
        if(!allocPtr)
            return (PtrOrSize){ .Size = allocSize + sizeof(uint64_t) + sizeof("canaries") - 1 };
        
        static_assert(sizeof("canaries") - 1 == 8, "");
        
        //"cana" <allocation size> "ries" <allocation> "cana"
        
        //First 4 bytes are canary
        char* canaryPtr = allocPtr;
        memcpy(canaryPtr, "cana", sizeof("cana") - 1);
        allocPtr = (char*)allocPtr + 4;
        
        //Second 8 bytes are allocation size
        memcpy(allocPtr, &allocSize, sizeof(allocSize));
        allocPtr = (char*)allocPtr + 8;
        
        //Third 4 bytes are canary
        canaryPtr = allocPtr;
        memcpy(canaryPtr, "ries", sizeof("ries") - 1);
        allocPtr = (char*)allocPtr + 4;
        
        
        //Last 4 bytes are canary
        canaryPtr = allocPtr;
        canaryPtr += allocSize;
        memcpy(canaryPtr, "cana", sizeof("cana") - 1);
        
        return (PtrOrSize){ .Ptr = allocPtr };
    }
    #endif
}

static inline bool ModC_CheckFrontCanary(void* allocPtr)
{
    #if MODC_ALLOCATOR_NO_CANARY
        return true;
    #else
        char* canaryPtr = allocPtr;
        canaryPtr -= (sizeof("cana") - 1) + sizeof(uint64_t) + (sizeof("ries") - 1);
        
        return  memcmp(canaryPtr, "cana", sizeof("cana") - 1) == 0 &&
                memcmp( canaryPtr + sizeof("cana") - 1 + sizeof(uint64_t), 
                        "ries", 
                        sizeof("ries") - 1) == 0;
    #endif
}

static inline uint64_t ModC_GetSize(void* allocPtr)
{
    allocPtr = (char*)allocPtr - (sizeof("ries") - 1) - sizeof(uint64_t);
    uint64_t retSize = 0;
    memcpy(&retSize, allocPtr, sizeof(uint64_t));
    return retSize;
}

static inline bool ModC_CheckBackCanary(void* allocPtr, uint64_t allocSize)
{
    #if MODC_ALLOCATOR_NO_CANARY
        return true;
    #else
        char* canaryPtr = allocPtr;
        canaryPtr += allocSize;
        return memcmp(canaryPtr, "cana", sizeof("cana") - 1) == 0;
    #endif
}

static inline ModC_Allocator ModC_CreateArenaAllocator(uint64_t allocateSize);

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
            break;
        case ModC_AllocatorType_SharedArena:
        case ModC_AllocatorType_OwnedArena:
        {
            if(!this->Allocator)
            {
                INTERN_PRINT_MODC_PRINT_TRACE("Trying to allocate with NULL arena wrapper\n");
                goto ret;
            }
            
            ModC_ArenaWrapper* arenaWrapper = this->Allocator;
            if(!arenaWrapper->CurrentArena)
            {
                INTERN_PRINT_MODC_PRINT_TRACE("Trying to allocate with NULL arena\n");
                goto ret;
            }
            
            uint64_t minRequiredSize = ModC_SetupCanariesAndSize(NULL, size).Size;
            retPtr = arena_alloc(arenaWrapper->CurrentArena, minRequiredSize);
            
            //If we failed to allocate in the current arena, use/create next arena
            while(!retPtr)
            {
                //Go to next arena
                if(arenaWrapper->NextArena)
                    arenaWrapper = arenaWrapper->NextArena;
                //Try create next arena
                else
                {
                    INTERN_PRINT_MODC_PRINT_TRACE("Creating child arena...\n");
                    
                    //If doubling overflows or doubling is not enough
                    if( SIZE_MAX / 2 < arenaWrapper->CurrentArena->size ||
                        arenaWrapper->CurrentArena->size * 2 < minRequiredSize)
                    {
                        arenaWrapper->NextArena = 
                            ModC_CreateArenaAllocator(minRequiredSize).Allocator;
                    }
                    //Otherwise just double the current size
                    else
                    {
                        arenaWrapper->NextArena = 
                            ModC_CreateArenaAllocator(arenaWrapper->CurrentArena->size * 2).Allocator;
                    }
                    
                    //Exit if we fail to create the next arena
                    if(!arenaWrapper->NextArena)
                        goto ret;
                    
                    //Set the previous arena entry for the next arena to this, and go to it
                    arenaWrapper->NextArena->PrevArena = arenaWrapper;
                    arenaWrapper = arenaWrapper->NextArena;
                }
                
                retPtr = arena_alloc(arenaWrapper->CurrentArena, minRequiredSize);
            }
            
            retPtr = ModC_SetupCanariesAndSize(retPtr, size).Ptr;
        } //case ModC_AllocatorType_OwnedArena:
        default:
            break;
    } //switch(this->Type)
    
    ret:;
    if(!retPtr)
    {
        INTERN_PRINT_MODC_PRINT_TRACE(  "Allocation failed for allocator: %p, "
                                        "with size %" PRIu64 "\n", this->Allocator, size);
    }
    else
        INTERN_PRINT_MODC_PRINT_TRACE("retPtr: %p, %" PRIu64 "\n", retPtr, size);
    return retPtr;
}

#define ModC_Allocator_Malloc(...) INTERN_PRINT_CALL(ModC_Allocator_Malloc, __VA_ARGS__)

static inline void* ModC_Allocator_Realloc(const ModC_Allocator* this, void* data, uint64_t size)
{
    void* retPtr = NULL;
    if(!this)
        goto ret;
    
    static_assert((int)ModC_AllocatorType_Count == 3, "");
    switch(this->Type)
    {
        case ModC_AllocatorType_Heap:
            retPtr = realloc(data, size);
            break;
        case ModC_AllocatorType_SharedArena:
        case ModC_AllocatorType_OwnedArena:
        {
            MODC_ASSERT(ModC_CheckFrontCanary(data) && ModC_CheckBackCanary(data, ModC_GetSize(data)) &&
                        "Canary check broken, out-of-bound write detected");
            
            uint64_t origSize = ModC_GetSize(data);
            retPtr = ModC_Allocator_Malloc(this, size);
            memcpy(retPtr, data, origSize);
            break;
        }
        default:
            break;
    }
    
    ret:;
    if(!retPtr)
    {
        INTERN_PRINT_MODC_PRINT_TRACE(  "Reallocation failed for allocator: %p, "
                                        "with size %" PRIu64 "\n", this->Allocator, size);
    }
    else
        INTERN_PRINT_MODC_PRINT_TRACE("retPtr: %p, %" PRIu64 "\n", retPtr, size);
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
            MODC_ASSERT(ModC_CheckFrontCanary(data) && ModC_CheckBackCanary(data, ModC_GetSize(data)) &&
                        "Canary check broken, out-of-bound write detected");
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
        {
            if(!this->Allocator)
                return;
            
            //Find the last linked arena wrapper
            ModC_ArenaWrapper* currentNode = this->Allocator;
            while(currentNode->NextArena)
            {
                ModC_ArenaWrapper* prevArena = currentNode;
                currentNode = currentNode->NextArena;
                MODC_ASSERT(prevArena == currentNode->PrevArena);
            }
            
            //Free from back to front
            while(currentNode->PrevArena)
            {
                ModC_ArenaWrapper* prevArena = currentNode->PrevArena;
                INTERN_PRINT_MODC_PRINT_TRACE("Destroying child: %p\n", (void*)currentNode);
                MODC_ASSERT(currentNode->CurrentArena);
                arena_destroy(currentNode->CurrentArena);
                currentNode = prevArena;
            }
            
            //Free first wrapper
            INTERN_PRINT_MODC_PRINT_TRACE("Destroying: %p\n", this->Allocator);
            MODC_ASSERT(currentNode->CurrentArena);
            arena_destroy(currentNode->CurrentArena);
            break;
        }
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
        .Allocator = NULL
    };
    
    Arena* arena = arena_create(allocateSize + sizeof(ModC_ArenaWrapper) + 64);
    if(!arena)
        return retAlloc;
    
    ModC_ArenaWrapper* arenaWrapper = arena_alloc(arena, sizeof(ModC_ArenaWrapper));
    retAlloc.Allocator = arenaWrapper;
    if(!retAlloc.Allocator)
    {
        arena_destroy(arena);
        return retAlloc;
    }
    
    *arenaWrapper = (ModC_ArenaWrapper){0};
    arenaWrapper->CurrentArena = arena;
    
    INTERN_PRINT_MODC_PRINT_TRACE(  "retAlloc.Allocator: %p with size %"PRIu64"\n", 
                                    retAlloc.Allocator, allocateSize + sizeof(ModC_ArenaWrapper) + 64);
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
