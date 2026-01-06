#ifndef MODC_LIST_H
#define MODC_LIST_H

#include "ModC/Allocator.h"

#include "MacroPowerToys/MacroPowerToys.h"

#include <string.h>
#include <assert.h>
#include <stddef.h>

//#define MODC_DEFINE_LIST_STRUCT(ModC_ListName, valueType) 

typedef struct
{
    ModC_Allocator Allocator;
    valueType* Data;
    uint64_t Length;
    uint64_t Cap;
} ModC_ListName


static inline ModC_ListName 
MPT_CONCAT(ModC_ListName, _Create)(const ModC_Allocator allocator, uint32_t cap)
{
    ModC_ListName retList = { .Allocator = allocator };
    MPT_CONCAT(ModC_ListName, _Reserve)(&retList, cap);
    return retList;
}

static inline void MPT_CONCAT(ModC_ListName, _Free)(ModC_ListName* this)
{
    if(!this)
        return;
    if(!this->Data)
    {
        *this = (ModC_ListName){0};
        return;
    }
    ModC_Allocator_Free(this->Allocator, this->Data);
    ModC_Allocator_Destroy(&this->Allocator);
    *this = (ModC_ListName){0};
    return;
}

static inline ModC_ListName 
MPT_CONCAT(ModC_ListName, _Free)(const ModC_Allocator allocator, uint32_t cap)
{
    ModC_ListName retList = { .Allocator = allocator };
    MPT_CONCAT(ModC_ListName, _Reserve)(&retList, cap);
    return retList;
}

static inline ModC_ListName* 
MPT_CONCAT(ModC_ListName, _Resize)(ModC_ListName* this, uint64_t resizeLength)
{
    if(!this)
        return this;
    if(this->Length >= resizeLength || this->Cap >= resizeLength)
    {
        this->Length = resizeLength;
        return this;
    }
    
    bool empty = this->Cap == 0;
    const uint64_t newCap = empty ? 
                            //Use requested count if we are empty
                            resizeLength : 
                            (
                                UINT64_MAX / 2 < this->Cap ?
                                //If doubling our cap reaches max, use max
                                UINT64_MAX :
                                (
                                    //Does doubling the cap sufficient?
                                    this->Cap * 2 < resizeLength ?
                                    resizeLength :      //No? Use request count
                                    this->Cap * 2       //Yes? Just double the cap
                                )
                            );
    void* dataPtr = empty ? 
                    ModC_Allocator_Malloc(this->Allocator, newCap) :
                    ModC_Allocator_Realloc(this->Allocator, this->Data, newCap);
    if(!dataPtr)
        return this;
    this->Data = dataPtr;
    this->Length = resizeLength;
    this->Cap = newCap;
    assert(newCap >= resizeLength);
    return this;
}

static inline ModC_ListName* 
MPT_CONCAT(ModC_ListName, _AddValue)(ModC_ListName* this, const valueType val)
{
    if(!this)
        return this;
    uint64_t oldLength = this->Length;
    MPT_CONCAT(ModC_ListName, _Resize)(oldLength + 1);
    if(this->Length != oldLength + 1)
        return this;
    this->Data[oldLength] = val;
    return this;
}

static inline ModC_ListName* 
MPT_CONCAT(ModC_ListName, _Add)(ModC_ListName* this, const valueType* val)
{
    if(!this || !val)
        return this;
    uint64_t oldLength = this->Length;
    MPT_CONCAT(ModC_ListName, _Resize)(oldLength + 1);
    if(this->Length != oldLength + 1)
        return this;
    memcpy(this->Data + oldLength, val, sizeof(valueType));
    return this;
}

static inline ModC_ListName* MPT_CONCAT(ModC_ListName, _AddEmpty)(ModC_ListName* this, bool zeroInit)
{
    if(!this || !val)
        return this;
    uint64_t oldLength = this->Length;
    MPT_CONCAT(ModC_ListName, _Resize)(oldLength + 1);
    if(this->Length != oldLength + 1)
        return this;
    if(zeroInit)
        memset(this->Data + oldLength, 0, sizeof(valueType));
    return this;
}

static inline ModC_ListName* MPT_CONCAT(ModC_ListName, _Remove)(ModC_ListName* this, uint64_t index)
{
    if(!this || index >= this->Length)
        return this;
    if(index == this->Length - 1)
    {
        --(this->Length);
        return this;
    }
    memmove(this->Data + index, this->Data + index + 1, (this->Length - index - 1) * sizeof(valueType));
    --(this->Length);
    return this;
}

static inline ModC_ListName*
MPT_CONCAT(ModC_ListName, _RemoveRange)(ModC_ListName* this, uint64_t startIndex, uint64_t endIndex)
{
    if(!this || startIndex >= this->Length || startIndex >= this->Length || startIndex > endIndex)
        return this;
    if(endIndex == this->Length - 1)
    {
        this->Length -= endIndex - startIndex + 1;
        return this;
    }
    memmove(this->Data + startIndex, 
            this->Data + endIndex + 1, 
            (this->Length - endIndex - 1) * sizeof(valueType));
    this->Length -= endIndex - startIndex + 1;
    return this;
}

static inline valueType* MPT_CONCAT(ModC_ListName, _At)(ModC_ListName* this, uint64_t index)
{
    if(!this || index >= this->Length)
        return NULL;
    return this->data + index;
}


#endif


