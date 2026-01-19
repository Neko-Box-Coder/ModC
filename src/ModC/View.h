#include "MacroPowerToys/Miscellaneous.h"

/* Docs
Define `MODC_VIEW_NAME` for the name of the list.
Define `MODC_VALUE_TYPE` for the element type stored in the list

Just read the code for the functions
*/

#ifndef MODC_VIEW_NAME
    #error "MODC_VIEW_NAME is not defined"
#endif

#ifndef MODC_CONST_VIEW_NAME
    #error "MODC_CONST_VIEW_NAME is not defined"
#endif

#ifndef MODC_VALUE_TYPE
    #error "MODC_VALUE_TYPE is not defined"
#endif

#include <string.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

typedef struct
{
    MODC_VALUE_TYPE* Data;
    uint64_t Length;
} MODC_VIEW_NAME;

typedef struct
{
    const MODC_VALUE_TYPE* Data;
    uint64_t Length;
} MODC_CONST_VIEW_NAME;

static inline MODC_VIEW_NAME 
MPT_DELAYED_CONCAT(MODC_VIEW_NAME, _Create)(MODC_VALUE_TYPE* data, uint64_t dataLength)
{
    if(!dataLength || !data)
        return (MODC_VIEW_NAME){0};
    else
        return (MODC_VIEW_NAME){ .Data = data, .Length = dataLength };
}

static inline MODC_CONST_VIEW_NAME 
MPT_DELAYED_CONCAT(MODC_CONST_VIEW_NAME, _Create)(const MODC_VALUE_TYPE* data, uint64_t dataLength)
{
    if(!dataLength || !data)
        return (MODC_CONST_VIEW_NAME){0};
    else
        return (MODC_CONST_VIEW_NAME){ .Data = data, .Length = dataLength };
}

static inline MODC_VIEW_NAME* MPT_DELAYED_CONCAT(MODC_VIEW_NAME, _Remove)(  MODC_VIEW_NAME* this, 
                                                                            uint64_t index)
{
    if(!this || index >= this->Length)
        return this;
    if(index == this->Length - 1)
    {
        --(this->Length);
        return this;
    }
    memmove(this->Data + index, 
            this->Data + index + 1, 
            (this->Length - index - 1) * sizeof(MODC_VALUE_TYPE));
    --(this->Length);
    return this;
}

static inline MODC_VIEW_NAME*
MPT_DELAYED_CONCAT(MODC_VIEW_NAME, _RemoveRange)(   MODC_VIEW_NAME* this, 
                                                    uint64_t startIndex, 
                                                    uint64_t endExclusiveIndex)
{
    if(!this || startIndex >= this->Length || startIndex >= endExclusiveIndex)
        return this;
    if(endExclusiveIndex >= this->Length)
    {
        endExclusiveIndex = this->Length;
        this->Length -= endExclusiveIndex - startIndex;
        return this;
    }
    memmove(this->Data + startIndex, 
            this->Data + endExclusiveIndex, 
            (this->Length - endExclusiveIndex) * sizeof(MODC_VALUE_TYPE));
    this->Length -= endExclusiveIndex - startIndex;
    return this;
}

static inline MODC_VALUE_TYPE* MPT_DELAYED_CONCAT(MODC_VIEW_NAME, _At)( MODC_VIEW_NAME* this, 
                                                                        uint64_t index)
{
    if(!this || index >= this->Length)
        return NULL;
    return this->Data + index;
}

static inline const MODC_VALUE_TYPE* 
MPT_DELAYED_CONCAT(MODC_CONST_VIEW_NAME, _At)(const MODC_CONST_VIEW_NAME* this, uint64_t index)
{
    if(!this || index >= this->Length)
        return NULL;
    return this->Data + index;
}

static inline uint64_t
MPT_DELAYED_CONCAT(MODC_VIEW_NAME, _Find)(const MODC_VIEW_NAME* this, const MODC_VALUE_TYPE* data)
{
    if(!this || !data)
        return this->Length;
    
    uint64_t i;
    for(i = 0; i < this->Length; ++i)
    {
        if(memcmp(&this->Data[i], data, sizeof(MODC_VALUE_TYPE)) == 0)
            return i;
    }
    return i;
}

//Returns the index of the found element. Otherwise return `this->Length`
static inline uint64_t
MPT_DELAYED_CONCAT(MODC_CONST_VIEW_NAME, _Find)(const MODC_CONST_VIEW_NAME* this, 
                                                const MODC_VALUE_TYPE* data)
{
    if(!this || !data)
        return this->Length;
    
    uint64_t i;
    for(i = 0; i < this->Length; ++i)
    {
        if(memcmp(&this->Data[i], data, sizeof(MODC_VALUE_TYPE)) == 0)
            return i;
    }
    return i;
}

static inline MODC_VIEW_NAME
MPT_DELAYED_CONCAT(MODC_VIEW_NAME, _Slice)( MODC_VIEW_NAME* this, 
                                            uint64_t startIndex, 
                                            uint64_t endExclusiveIndex)
{
    if(!this || startIndex >= this->Length || startIndex >= endExclusiveIndex)
        return (MODC_VIEW_NAME){0};
    if(endExclusiveIndex >= this->Length)
        endExclusiveIndex = this->Length;
    if(startIndex == 0)
    {
        MODC_VIEW_NAME retView = *this;
        retView.Length = endExclusiveIndex;
        return retView;
    }
    
    return MPT_DELAYED_CONCAT(MODC_VIEW_NAME, _Create)( this->Data + startIndex, 
                                                        endExclusiveIndex - startIndex);
}

static inline MODC_CONST_VIEW_NAME
MPT_DELAYED_CONCAT(MODC_CONST_VIEW_NAME, _Slice)(   const MODC_CONST_VIEW_NAME* this, 
                                                    uint64_t startIndex, 
                                                    uint64_t endExclusiveIndex)
{
    if(!this || startIndex >= this->Length || startIndex >= endExclusiveIndex)
        return (MODC_CONST_VIEW_NAME){0};
    if(endExclusiveIndex >= this->Length)
        endExclusiveIndex = this->Length;
    if(startIndex == 0)
        return (MODC_CONST_VIEW_NAME){ .Data = this->Data, .Length = endExclusiveIndex };
    
    return MPT_DELAYED_CONCAT(MODC_CONST_VIEW_NAME, _Create)(   this->Data + startIndex, 
                                                                endExclusiveIndex - startIndex);
}

static inline MODC_VIEW_NAME
MPT_DELAYED_CONCAT(MODC_CONST_VIEW_NAME, _Unconst)(const MODC_CONST_VIEW_NAME* this)
{
    if(!this)
        return (MODC_VIEW_NAME){0};
    return (MODC_VIEW_NAME){ .Data = (MODC_VALUE_TYPE*)this->Data, .Length = this->Length };
}




#undef MODC_VIEW_NAME
#undef MODC_CONST_VIEW_NAME
#undef MODC_VALUE_TYPE
