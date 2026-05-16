#ifndef MODC_ALLOCATOR_H
#define MODC_ALLOCATOR_H

/* Docs
Define `ALLOCATOR_NO_CANARY` to disable canary checks

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

typedef enum AllocatorType
{
    AllocatorType_Invalid,
    AllocatorType_Heap,
    AllocatorType_SharedArena,
    AllocatorType_OwnedArena,
    AllocatorType_Count,   //4
} AllocatorType;

typedef struct Allocator
{
    AllocatorType Type;
    void* Allocator;
} Allocator;

struct ArenaWrapper;

typedef struct ArenaWrapper
{
    Arena* CurrentArena;
    struct ArenaWrapper* NextArena;
    struct ArenaWrapper* PrevArena;
} ArenaWrapper;




#define INTERN_DEBUG_PRINT_ALLOC 0
#if INTERN_DEBUG_PRINT_ALLOC
    #include <stdio.h>
    #include <inttypes.h>
    
    #ifndef INTERN_PRINT_PRINT_TRACE
        #define INTERN_PRINT_PRINT_TRACE(...) \
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
    #ifndef INTERN_PRINT_PRINT_TRACE
        #define INTERN_PRINT_PRINT_TRACE(...) do {} while(0)
    #endif
    
    #define INTERN_PRINT_CALL(callFunc, ...) callFunc(__VA_ARGS__)
#endif

typedef union
{
    void* Ptr;
    uint64_t Size;
} PtrOrSize;

#if ALLOCATOR_NO_CANARY
    #define FrontCanarySize() 0
    #define BackCanarySize() 0
#else
    #define FrontCanarySize() ((sizeof("cana") - 1) + sizeof(uint64_t) + (sizeof("ries") - 1))
    #define BackCanarySize() (sizeof("cana") - 1)
#endif

//When `NULL` is passed to `allocPtr`, it returns additional bytes needed.
//Otherwise, returns the pointer for `allocSize`
static inline PtrOrSize SetupCanariesAndSize(void* allocPtr, uint64_t allocSize)
{
    #if ALLOCATOR_NO_CANARY
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
            return (PtrOrSize){ .Size = FrontCanarySize() + allocSize + BackCanarySize() };
        
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

static inline bool CheckFrontCanary(void* allocPtr)
{
    #if ALLOCATOR_NO_CANARY
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

static inline uint64_t GetAllocSize(void* allocPtr)
{
    allocPtr = (char*)allocPtr - (sizeof("ries") - 1) - sizeof(uint64_t);
    uint64_t retSize = 0;
    memcpy(&retSize, allocPtr, sizeof(uint64_t));
    return retSize;
}

static inline bool CheckBackCanary(void* allocPtr, uint64_t allocSize)
{
    #if ALLOCATOR_NO_CANARY
        return true;
    #else
        char* canaryPtr = allocPtr;
        canaryPtr += allocSize;
        return memcmp(canaryPtr, "cana", sizeof("cana") - 1) == 0;
    #endif
}

static inline Allocator CreateArenaAllocator(uint64_t allocateSize);

static inline void* Allocator_Malloc(const Allocator* this, uint64_t size)
{
    void* retPtr = NULL;
    if(!this)
        goto ret;
    
    static_assert((int)AllocatorType_Count == 4, "");
    switch(this->Type)
    {
        case AllocatorType_Heap:
            retPtr = malloc(size);
            break;
        case AllocatorType_SharedArena:
        case AllocatorType_OwnedArena:
        {
            if(!this->Allocator)
            {
                INTERN_PRINT_PRINT_TRACE("Trying to allocate with NULL arena wrapper\n");
                goto ret;
            }
            
            ArenaWrapper* arenaWrapper = this->Allocator;
            if(!arenaWrapper->CurrentArena)
            {
                INTERN_PRINT_PRINT_TRACE("Trying to allocate with NULL arena\n");
                goto ret;
            }
            
            uint64_t minRequiredSize = SetupCanariesAndSize(NULL, size).Size;
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
                    INTERN_PRINT_PRINT_TRACE("Creating child arena from %p...\n", (void*)arenaWrapper);
                    
                    //If doubling overflows or doubling is not enough
                    if( SIZE_MAX / 2 < arenaWrapper->CurrentArena->size ||
                        arenaWrapper->CurrentArena->size * 2 < minRequiredSize)
                    {
                        arenaWrapper->NextArena = CreateArenaAllocator(minRequiredSize).Allocator;
                    }
                    //Otherwise just double the current size
                    else
                    {
                        arenaWrapper->NextArena = 
                            CreateArenaAllocator(arenaWrapper->CurrentArena->size * 2).Allocator;
                    }
                    
                    //Exit if we fail to create the next arena
                    if(!arenaWrapper->NextArena)
                        goto ret;
                    
                    //Set the previous arena entry for the next arena to this, and go to it
                    arenaWrapper->NextArena->PrevArena = arenaWrapper;
                    arenaWrapper = arenaWrapper->NextArena;
                    //INTERN_PRINT_PRINT_TRACE(   "Child arena prev arena: %p\n", 
                    //                            (void*)arenaWrapper->PrevArena);
                }
                
                ASSERT(arenaWrapper->PrevArena);
                ASSERT(arenaWrapper->PrevArena->NextArena == arenaWrapper);
                retPtr = arena_alloc(arenaWrapper->CurrentArena, minRequiredSize);
            }
            
            retPtr = SetupCanariesAndSize(retPtr, size).Ptr;
        } //case AllocatorType_OwnedArena:
        default:
            break;
    } //switch(this->Type)
    
    ret:;
    if(!retPtr)
    {
        INTERN_PRINT_PRINT_TRACE(   "Allocation failed for allocator: %p, "
                                    "with size %" PRIu64 "\n", this->Allocator, size);
    }
    else
        INTERN_PRINT_PRINT_TRACE("retPtr: %p, %" PRIu64 "\n", retPtr, size);
    return retPtr;
}

#define Allocator_Malloc(...) INTERN_PRINT_CALL(Allocator_Malloc, __VA_ARGS__)

static inline void* Allocator_Realloc(const Allocator* this, void* data, uint64_t size)
{
    void* retPtr = NULL;
    if(!this)
        goto ret;
    
    static_assert((int)AllocatorType_Count == 4, "");
    switch(this->Type)
    {
        case AllocatorType_Heap:
            retPtr = realloc(data, size);
            break;
        case AllocatorType_SharedArena:
        case AllocatorType_OwnedArena:
        {
            ASSERT( CheckFrontCanary(data) && 
                    CheckBackCanary(data, GetAllocSize(data)) &&
                    "Canary check broken, out-of-bound write detected");
            
            uint64_t origSize = GetAllocSize(data);
            retPtr = Allocator_Malloc(this, size);
            memcpy(retPtr, data, origSize);
            break;
        }
        default:
            break;
    }
    
    ret:;
    if(!retPtr)
    {
        INTERN_PRINT_PRINT_TRACE(   "Reallocation failed for allocator: %p, "
                                    "with size %" PRIu64 "\n", this->Allocator, size);
    }
    else
        INTERN_PRINT_PRINT_TRACE("retPtr: %p, %" PRIu64 "\n", retPtr, size);
    return retPtr;
}

#define Allocator_Realloc(...) INTERN_PRINT_CALL(Allocator_Realloc, __VA_ARGS__)

static inline void Allocator_Free(const Allocator* this, void* data)
{
    if(!this)
        return;
    
    static_assert((int)AllocatorType_Count == 4, "");
    switch(this->Type)
    {
        case AllocatorType_Heap:
            INTERN_PRINT_PRINT_TRACE("Free: %p\n", data);
            free(data);
            break;
        case AllocatorType_SharedArena:
        case AllocatorType_OwnedArena:
        {
            ASSERT( CheckFrontCanary(data) && 
                    CheckBackCanary(data, GetAllocSize(data)) &&
                    "Canary check broken, out-of-bound write detected");
            
            ArenaWrapper* currentNode = this->Allocator;
            char* byteDataPtr = data;
            while(currentNode->NextArena)
            {
                Arena* currentArena = currentNode->CurrentArena;
                ASSERT(currentArena);
                if( byteDataPtr >= currentArena->region && 
                    byteDataPtr < currentArena->region + currentArena->index)
                {
                    break;
                }
                
                ArenaWrapper* prevArena = currentNode;
                currentNode = currentNode->NextArena;
                ASSERT(prevArena == currentNode->PrevArena);
            }
            ASSERT(currentNode->CurrentArena);
            
            //TODO: Use walkable allocation list
            if( currentNode->CurrentArena->region + currentNode->CurrentArena->index ==
                byteDataPtr + GetAllocSize(data) + BackCanarySize())
            {
                currentNode->CurrentArena->index -= (FrontCanarySize() + 
                                                    GetAllocSize(data) + 
                                                    BackCanarySize());
                ASSERT( currentNode->CurrentArena->region + 
                        currentNode->CurrentArena->index + 
                        FrontCanarySize() == byteDataPtr);
            }
            
            break;
        }
        default:
            break;
    }
}

#define Allocator_Free(...) INTERN_PRINT_CALL(Allocator_Free, __VA_ARGS__)

//TODO: Allocator_Reset

static inline void Allocator_Destroy(Allocator* this)
{
    if(!this)
        return;
    
    static_assert((int)AllocatorType_Count == 4, "");
    switch(this->Type)
    {
        case AllocatorType_Heap:
        case AllocatorType_SharedArena:
            break;
        case AllocatorType_OwnedArena:
        {
            if(!this->Allocator)
                return;
            
            //Find the last linked arena wrapper
            ArenaWrapper* currentNode = this->Allocator;
            while(currentNode->NextArena)
            {
                ArenaWrapper* prevArena = currentNode;
                currentNode = currentNode->NextArena;
                ASSERT(prevArena == currentNode->PrevArena);
            }
            
            //Free from back to front
            while(currentNode->PrevArena)
            {
                ArenaWrapper* prevArena = currentNode->PrevArena;
                INTERN_PRINT_PRINT_TRACE("Destroying child: %p\n", (void*)currentNode);
                ASSERT(currentNode->CurrentArena);
                arena_destroy(currentNode->CurrentArena);
                currentNode = prevArena;
            }
            
            //Free first wrapper
            INTERN_PRINT_PRINT_TRACE("Destroying: %p\n", this->Allocator);
            ASSERT(currentNode->CurrentArena);
            arena_destroy(currentNode->CurrentArena);
            break;
        }
        default:
            break;
    }
    *this = (Allocator){0};
}

#define Allocator_Destroy(...) INTERN_PRINT_CALL(Allocator_Destroy, __VA_ARGS__)

static inline Allocator Allocator_Share(Allocator* this)
{
    if(!this)
        return (Allocator){0};
    
    //INTERN_PRINT_PRINT_TRACE(this);
    
    static_assert((int)AllocatorType_Count == 4, "");
    switch(this->Type)
    {
        case AllocatorType_Heap:
            return *this;
        case AllocatorType_SharedArena:
            return *this;
        case AllocatorType_OwnedArena:
        {
            Allocator retAlloc = *this;
            retAlloc.Type = AllocatorType_SharedArena;
            return retAlloc;
        }
        default:
            return (Allocator){0};
    }
}

#define Allocator_Share(...) INTERN_PRINT_CALL(Allocator_Share, __VA_ARGS__)

static inline Allocator CreateArenaAllocator(uint64_t allocateSize)
{
    Allocator retAlloc =
    {
        .Type = AllocatorType_OwnedArena,
        .Allocator = NULL
    };
    
    Arena* arena = arena_create(allocateSize + sizeof(ArenaWrapper) + 64);
    if(!arena)
        return retAlloc;
    
    ArenaWrapper* arenaWrapper = arena_alloc(arena, sizeof(ArenaWrapper));
    retAlloc.Allocator = arenaWrapper;
    if(!retAlloc.Allocator)
    {
        arena_destroy(arena);
        return retAlloc;
    }
    
    *arenaWrapper = (ArenaWrapper){0};
    arenaWrapper->CurrentArena = arena;
    
    INTERN_PRINT_PRINT_TRACE(   "retAlloc.Allocator: %p with size %"PRIu64"\n", 
                                retAlloc.Allocator, allocateSize + sizeof(ArenaWrapper) + 64);
    return retAlloc;
}

#define CreateArenaAllocator(...) INTERN_PRINT_CALL(CreateArenaAllocator, __VA_ARGS__)

static inline Allocator CreateArenaAllocator_PreAllocated(  void* preAllocatedData, 
                                                            uint64_t allocatedSize)
{
    #if defined(__clang__)
        uint64_t arenaOffset = __extension__ offsetof(struct { char c; Arena d; }, d);
    #else
        uint64_t arenaOffset = offsetof(struct { char c; Arena d; }, d);
    #endif
    char* byteDataPtr = preAllocatedData;
    uint64_t alignPadding = arenaOffset - ((uint64_t)byteDataPtr % arenaOffset);
    uint64_t minDataNeeded = alignPadding + sizeof(Arena);
    if(allocatedSize <= minDataNeeded)
        return (Allocator){0};
    
    byteDataPtr += alignPadding;
    allocatedSize -= alignPadding;
    ASSERT((uint64_t)byteDataPtr % arenaOffset == 0);
    
    Arena* retArena = (void*)byteDataPtr;
    byteDataPtr += sizeof(Arena);
    allocatedSize -= sizeof(Arena);
    
    arena_init(retArena, byteDataPtr, allocatedSize);
    Allocator retAlloc =
    {
        .Type = AllocatorType_OwnedArena,
        .Allocator = NULL
    };
    
    ArenaWrapper* arenaWrapper = arena_alloc(retArena, sizeof(ArenaWrapper));
    retAlloc.Allocator = arenaWrapper;
    if(!retAlloc.Allocator)
    {
        arena_destroy(retArena);
        return (Allocator){0};
    }
    
    *arenaWrapper = (ArenaWrapper){0};
    arenaWrapper->CurrentArena = retArena;
    
    INTERN_PRINT_PRINT_TRACE(   "retAlloc.Allocator: %p with size %"PRIu64"\n", 
                                retAlloc.Allocator, 
                                allocatedSize + sizeof(ArenaWrapper) + 64);
    return retAlloc;
}

#define CreateArenaAllocator_PreAllocated(...) \
    INTERN_PRINT_CALL(CreateArenaAllocator_PreAllocated, __VA_ARGS__)

static inline Allocator CreateHeapAllocator(void)
{
    return  (Allocator)
            {
                .Type = AllocatorType_Heap,
                .Allocator = NULL,
            };
}

#define CreateHeapAllocator(...) INTERN_PRINT_CALL(CreateHeapAllocator, __VA_ARGS__)

#endif
