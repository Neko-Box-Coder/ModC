#include "ModC/Allocator.h"
#include "MacroPowerToys/Miscellaneous.h"

/* Docs
Define `LIST_NAME` for the name of the list.
Define `VALUE_TYPE` for the element type stored in the list
Define `VALUE_FREE` optionally which will be called as `VALUE_FREE(VALUE_TYPE* val)`
Define `NO_TYPEDEF` optionally to not use typedef struct

Then include this file

Just read the code for the functions
*/

#ifndef LIST_NAME
    #error "LIST_NAME is not defined"
#endif

#ifndef VALUE_TYPE
    #error "VALUE_TYPE is not defined"
#endif

#include <string.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>


#if NO_TYPEDEF
    struct LIST_NAME
    {
        ModC_Allocator Allocator;
        VALUE_TYPE* Data;
        uint64_t Length;
        uint64_t Cap;
    };
#else
    typedef struct LIST_NAME
    {
        ModC_Allocator Allocator;
        VALUE_TYPE* Data;
        uint64_t Length;
        uint64_t Cap;
    } LIST_NAME;
#endif

static inline LIST_NAME*
MPT_DELAYED_CONCAT(LIST_NAME, _Reserve)(LIST_NAME* this, uint32_t reserveSize)
{
    if(!this || this->Cap >= reserveSize)
        return this;
    
    void* dataPtr = this->Cap == 0 ? 
                    ModC_Allocator_Malloc(&this->Allocator, sizeof(VALUE_TYPE) * reserveSize) :
                    ModC_Allocator_Realloc( &this->Allocator, 
                                            this->Data, 
                                            sizeof(VALUE_TYPE) * reserveSize);
    if(!dataPtr)
        return this;
    this->Data = dataPtr;
    this->Cap = reserveSize;
    return this;
}

static inline LIST_NAME 
MPT_DELAYED_CONCAT(LIST_NAME, _Create)(ModC_Allocator allocator, uint64_t cap)
{
    LIST_NAME retList = { .Allocator = allocator };
    MPT_DELAYED_CONCAT(LIST_NAME, _Reserve)(&retList, cap);
    return retList;
}

static inline void MPT_DELAYED_CONCAT(LIST_NAME, _Free)(LIST_NAME* this)
{
    if(!this)
        return;
    
    #ifdef VALUE_FREE
        for(uint64_t i = 0; i < this->Length; ++i)
        {
            VALUE_FREE(&this->Data[i]);
        }
    #endif
    
    if(!this->Data)
    {
        *this = (LIST_NAME){0};
        return;
    }
    ModC_Allocator_Free(&this->Allocator, this->Data);
    ModC_Allocator_Destroy(&this->Allocator);
    *this = (LIST_NAME){0};
    return;
}

static inline LIST_NAME* 
MPT_DELAYED_CONCAT(LIST_NAME, _Resize)(LIST_NAME* this, uint64_t resizeLength)
{
    if(!this)
        return this;
    
    //Shrinking
    if(this->Length >= resizeLength)
    {
        #ifdef VALUE_FREE
            for(uint64_t i = resizeLength; i < this->Length; ++i)
            {
                VALUE_FREE(&this->Data[i]);
            }
        #endif
        
        this->Length = resizeLength;
        return this;
    }
    
    if(this->Cap >= resizeLength)
    {
        this->Length = resizeLength;
        return this;
    }
    
    bool empty = this->Cap == 0;
    const uint64_t newCap = empty ? 
                            //Use requested count if we are empty
                            resizeLength : 
                            (
                                UINT64_MAX / 2 < this->Cap * sizeof(VALUE_TYPE)?
                                //If doubling our cap reaches max, use max
                                UINT64_MAX / sizeof(VALUE_TYPE):
                                (
                                    //Does doubling the cap sufficient?
                                    this->Cap * 2 < resizeLength ?
                                    resizeLength :      //No? Use request count
                                    this->Cap * 2       //Yes? Just double the cap
                                )
                            );
    void* dataPtr = empty ? 
                    ModC_Allocator_Malloc(&this->Allocator, newCap * sizeof(VALUE_TYPE)) :
                    ModC_Allocator_Realloc(&this->Allocator, this->Data, newCap * sizeof(VALUE_TYPE));
    if(!dataPtr)
        return this;
    this->Data = dataPtr;
    this->Length = resizeLength;
    this->Cap = newCap;
    assert(newCap >= resizeLength);
    return this;
}

static inline LIST_NAME* 
MPT_DELAYED_CONCAT(LIST_NAME, _AddValue)(LIST_NAME* this, const VALUE_TYPE val)
{
    if(!this)
        return this;
    uint64_t oldLength = this->Length;
    MPT_DELAYED_CONCAT(LIST_NAME, _Resize)(this, oldLength + 1);
    if(this->Length != oldLength + 1)
        return this;
    this->Data[oldLength] = val;
    return this;
}

static inline LIST_NAME* 
MPT_DELAYED_CONCAT(LIST_NAME, _AddRange)(LIST_NAME* this, const VALUE_TYPE* data, uint64_t dataLength)
{
    if(!this || !dataLength || !data)
        return this;
    uint64_t oldLength = this->Length;
    MPT_DELAYED_CONCAT(LIST_NAME, _Resize)(this, oldLength + dataLength);
    if(this->Length != oldLength + dataLength)
        return this;
    memcpy(this->Data + oldLength, data, sizeof(VALUE_TYPE) * dataLength);
    return this;
}

static inline LIST_NAME* 
MPT_DELAYED_CONCAT(LIST_NAME, _InsertValue)(LIST_NAME* this, uint64_t index, const VALUE_TYPE val)
{
    if(!this || index > this->Length)
        return NULL;
    
    if(index == this->Length)
        return MPT_DELAYED_CONCAT(LIST_NAME, _AddValue)(this, val);
    
    uint64_t oldLength = this->Length;
    MPT_DELAYED_CONCAT(LIST_NAME, _Resize)(this, oldLength + 1);
    if(this->Length != oldLength + 1)
        return this;
    
    memmove(this->Data + index + 1, this->Data + index, (oldLength - index) * sizeof(VALUE_TYPE));
    this->Data[index] = val;
    return this;
}

static inline LIST_NAME* 
MPT_DELAYED_CONCAT(LIST_NAME, _InsertRange)(LIST_NAME* this, 
                                            uint64_t index, 
                                            const VALUE_TYPE* data, 
                                            uint64_t dataLength)
{
    if(!this || !dataLength || !data || index > this->Length)
        return this;
    
    if(index == this->Length)
        return MPT_DELAYED_CONCAT(LIST_NAME, _AddRange)(this, data, dataLength);
    
    uint64_t oldLength = this->Length;
    MPT_DELAYED_CONCAT(LIST_NAME, _Resize)(this, oldLength + dataLength);
    if(this->Length != oldLength + dataLength)
        return this;
    
    memmove(this->Data + index + dataLength, 
            this->Data + index, 
            (oldLength - index) * sizeof(VALUE_TYPE));
    memcpy(this->Data + index, data, sizeof(VALUE_TYPE) * dataLength);
    return this;
}

static inline LIST_NAME* MPT_DELAYED_CONCAT(LIST_NAME, _AddEmpty)(LIST_NAME* this, bool zeroInit)
{
    if(!this)
        return this;
    uint64_t oldLength = this->Length;
    MPT_DELAYED_CONCAT(LIST_NAME, _Resize)(this, oldLength + 1);
    if(this->Length != oldLength + 1)
        return this;
    if(zeroInit)
        memset(this->Data + oldLength, 0, sizeof(VALUE_TYPE));
    return this;
}

static inline LIST_NAME* MPT_DELAYED_CONCAT(LIST_NAME, _Remove)(LIST_NAME* this, uint64_t index)
{
    if(!this || index >= this->Length)
        return this;
    
    #ifdef VALUE_FREE
        VALUE_FREE(&this->Data[index]);
    #endif
    
    if(index == this->Length - 1)
    {
        --(this->Length);
        return this;
    }
    memmove(this->Data + index, 
            this->Data + index + 1, 
            (this->Length - index - 1) * sizeof(VALUE_TYPE));
    --(this->Length);
    return this;
}

static inline LIST_NAME*
MPT_DELAYED_CONCAT(LIST_NAME, _RemoveRange)(LIST_NAME* this, 
                                            uint64_t startIndex, 
                                            uint64_t endExclusiveIndex)
{
    if(!this || startIndex >= this->Length || startIndex >= endExclusiveIndex)
        return this;
    
    endExclusiveIndex = endExclusiveIndex > this->Length ? this->Length : endExclusiveIndex;
    
    #ifdef VALUE_FREE
        for(uint64_t i = startIndex; i < endExclusiveIndex; ++i)
        {
            VALUE_FREE(&this->Data[i]);
        }
    #endif
    
    if(endExclusiveIndex == this->Length)
    {
        endExclusiveIndex = this->Length;
        this->Length -= endExclusiveIndex - startIndex;
        return this;
    }
    memmove(this->Data + startIndex, 
            this->Data + endExclusiveIndex, 
            (this->Length - endExclusiveIndex) * sizeof(VALUE_TYPE));
    this->Length -= endExclusiveIndex - startIndex;
    return this;
}

static inline VALUE_TYPE* MPT_DELAYED_CONCAT(LIST_NAME, _At)(LIST_NAME* this, uint64_t index)
{
    if(!this || index >= this->Length)
        return NULL;
    return this->Data + index;
}

//Returns the index of the found element. Otherwise return `this->Length`
static inline uint64_t
MPT_DELAYED_CONCAT(LIST_NAME, _Find)(const LIST_NAME* this, const VALUE_TYPE* data)
{
    if(!this || !data)
        return this->Length;
    
    uint64_t i;
    for(i = 0; i < this->Length; ++i)
    {
        if(memcmp(&this->Data[i], data, sizeof(VALUE_TYPE)) == 0)
            return i;
    }
    return i;
}

#define MODC_DATA_LENGTH(obj) (obj).Data, (obj).Length
#define MODC_LENGTH_DATA(obj) (obj).Length, (obj).Data

#undef LIST_NAME
#undef VALUE_TYPE
#undef VALUE_FREE
#undef NO_TYPEDEF
