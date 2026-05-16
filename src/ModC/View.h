#include "MacroPowerToys/Miscellaneous.h"

/* Docs
Define `VIEW_NAME` for the name of the list.
Define `VALUE_TYPE` for the element type stored in the list

Just read the code for the functions
*/

#ifndef VIEW_NAME
    #error "VIEW_NAME is not defined"
#endif

#ifndef CONST_VIEW_NAME
    #error "CONST_VIEW_NAME is not defined"
#endif

#ifndef VALUE_TYPE
    #error "VALUE_TYPE is not defined"
#endif

#include <string.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

typedef struct VIEW_NAME
{
    VALUE_TYPE* Data;
    uint64_t Length;
} VIEW_NAME;

typedef struct CONST_VIEW_NAME
{
    const VALUE_TYPE* Data;
    uint64_t Length;
} CONST_VIEW_NAME;

static inline VIEW_NAME 
MPT_DELAYED_CONCAT(VIEW_NAME, _Create)(VALUE_TYPE* data, uint64_t dataLength)
{
    if(!dataLength || !data)
        return (VIEW_NAME){0};
    else
        return (VIEW_NAME){ .Data = data, .Length = dataLength };
}

static inline CONST_VIEW_NAME 
MPT_DELAYED_CONCAT(CONST_VIEW_NAME, _Create)(const VALUE_TYPE* data, uint64_t dataLength)
{
    if(!dataLength || !data)
        return (CONST_VIEW_NAME){0};
    else
        return (CONST_VIEW_NAME){ .Data = data, .Length = dataLength };
}

static inline VIEW_NAME* MPT_DELAYED_CONCAT(VIEW_NAME, _Remove)(VIEW_NAME* this, uint64_t index)
{
    if(!this || index >= this->Length)
        return this;
    
    #ifdef MODC_VALUE_FREE
        MODC_VALUE_FREE(&this->Data[index]);
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

static inline VIEW_NAME*
MPT_DELAYED_CONCAT(VIEW_NAME, _RemoveRange)(VIEW_NAME* this, 
                                            uint64_t startIndex, 
                                            uint64_t endExclusiveIndex)
{
    if(!this || startIndex >= this->Length || startIndex >= endExclusiveIndex)
        return this;
    
    endExclusiveIndex = endExclusiveIndex > this->Length ? this->Length : endExclusiveIndex;
    
    #ifdef MODC_VALUE_FREE
        for(uint64_t i = startIndex; i < endExclusiveIndex; ++i)
        {
            MODC_VALUE_FREE(&this->Data[i]);
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

static inline VALUE_TYPE* MPT_DELAYED_CONCAT(VIEW_NAME, _At)(VIEW_NAME* this, uint64_t index)
{
    if(!this || index >= this->Length)
        return NULL;
    return this->Data + index;
}

static inline const VALUE_TYPE* 
MPT_DELAYED_CONCAT(CONST_VIEW_NAME, _At)(const CONST_VIEW_NAME* this, uint64_t index)
{
    if(!this || index >= this->Length)
        return NULL;
    return this->Data + index;
}

static inline uint64_t
MPT_DELAYED_CONCAT(VIEW_NAME, _Find)(const VIEW_NAME* this, const VALUE_TYPE* data)
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

//Returns the index of the found element. Otherwise return `this->Length`
static inline uint64_t
MPT_DELAYED_CONCAT(CONST_VIEW_NAME, _Find)(const CONST_VIEW_NAME* this, const VALUE_TYPE* data)
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

static inline VIEW_NAME
MPT_DELAYED_CONCAT(VIEW_NAME, _Slice)(  VIEW_NAME* this, 
                                        uint64_t startIndex, 
                                        uint64_t endExclusiveIndex)
{
    if(!this || startIndex >= this->Length || startIndex >= endExclusiveIndex)
        return (VIEW_NAME){0};
    if(endExclusiveIndex >= this->Length)
        endExclusiveIndex = this->Length;
    if(startIndex == 0)
    {
        VIEW_NAME retView = *this;
        retView.Length = endExclusiveIndex;
        return retView;
    }
    
    return MPT_DELAYED_CONCAT(VIEW_NAME, _Create)(  this->Data + startIndex, 
                                                    endExclusiveIndex - startIndex);
}

static inline CONST_VIEW_NAME
MPT_DELAYED_CONCAT(CONST_VIEW_NAME, _Slice)(const CONST_VIEW_NAME* this, 
                                            uint64_t startIndex, 
                                            uint64_t endExclusiveIndex)
{
    if(!this || startIndex >= this->Length || startIndex >= endExclusiveIndex)
        return (CONST_VIEW_NAME){0};
    if(endExclusiveIndex >= this->Length)
        endExclusiveIndex = this->Length;
    if(startIndex == 0)
        return (CONST_VIEW_NAME){ .Data = this->Data, .Length = endExclusiveIndex };
    
    return MPT_DELAYED_CONCAT(CONST_VIEW_NAME, _Create)(this->Data + startIndex, 
                                                        endExclusiveIndex - startIndex);
}

static inline VIEW_NAME
MPT_DELAYED_CONCAT(CONST_VIEW_NAME, _Unconst)(const CONST_VIEW_NAME* this)
{
    if(!this)
        return (VIEW_NAME){0};
    return (VIEW_NAME){ .Data = (VALUE_TYPE*)this->Data, .Length = this->Length };
}




#undef VIEW_NAME
#undef CONST_VIEW_NAME
#undef VALUE_TYPE
