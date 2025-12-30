#ifndef MODC_STRINGS_STRINGS_H
#define MODC_STRINGS_STRINGS_H

/* Docs
```c
typedef struct
{
    ModC_Allocator Allocator;
    char* Data;
    uint32_t Length;             //Excluding end null terminator
    uint32_t Cap;
} ModC_String;

typedef struct
{
    [const] char* Data;
    [const] int32_t Length;            //Excluding end null terminator
    [const] bool NullEnd;
} ModC_[Const]StringView;

static inline ModC_String ModC_String_CreateFromCStr(char* cstr, ModC_Allocator allocator);
static inline void ModC_String_Resize(ModC_String* this, uint32_t resizeBytes);

static inline ModC_String ModC_String_Create(   ModC_Allocator allocator, 
                                                char* data, 
                                                uint32_t length, 
                                                uint32_t cap);

static inline ModC_[Const]StringView ModC_String_[Const]Subview(const ModC_String* src, 
                                                                const int32_t index,
                                                                int32_t length);

static inline ModC_[Const]StringView 
ModC_[Const]StringView_[Const]Subview(  const ModC_[Const]StringView* src, 
                                        const int32_t index,
                                        int32_t length);

static inline ModC_StringView ModC_StringView_CreateFromCStr(char* cstr);
static inline ModC_ConstStringView ModC_ConstStringView_CreateFromConstCStr(const char* cstr);

static inline ModC_StringView ModC_ConstStringView_RemoveConst(ModC_ConstStringView src);
```
*/

#include "ModC/Allocator.h"

#include "MacroPowerToys/MacroPowerToys.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

typedef struct
{
    ModC_Allocator Allocator;
    char* Data;
    uint32_t Length;             //Excluding end null terminator
    uint32_t Cap;
} ModC_String;

typedef struct
{
    char* Data;
    uint32_t Length;             //Excluding end null terminator
    bool NullEnd;
} ModC_StringView;

typedef struct
{
    const char* Data;
    const uint32_t Length;       //Excluding end null terminator
    const bool NullEnd;
} ModC_ConstStringView;

static inline ModC_String ModC_String_CreateFromCStr(char* cstr, ModC_Allocator allocator)
{
    uint32_t len = strlen(cstr);
    return (ModC_String){ .Allocator = allocator, .Data = cstr, .Length = len, .Cap = len + 1 };
}

static inline void ModC_String_Resize(ModC_String* this, uint32_t resizeBytes)
{
    if(!this)
        return;
    if(this->Length > resizeBytes)
    {
        this->Data[resizeBytes] = '\0';
        this->Length = resizeBytes;
        return;
    }
    
    if(this->Cap > resizeBytes - 1)
    {
        this->Length = resizeBytes;
        return;
    }
    
    bool emptyString = this->Cap == 0;
    const uint32_t newCap = emptyString ? 
                            //Use requested size + 1 if we have empty string
                            resizeBytes + 1 : 
                            (
                                UINT32_MAX / 2 < this->Cap ?
                                //If doubling our cap reaches max, use max
                                UINT32_MAX :
                                (
                                    //Does doubling the cap sufficient?
                                    this->Cap * 2 < resizeBytes + 1 ?
                                    resizeBytes + 1 :   //No? Use request size + 1
                                    this->Cap * 2       //Yes? Just double the cap
                                )
                            );
    
    if(emptyString && !this->Allocator.Malloc)
        return;
    else if(!emptyString && !this->Allocator.Realloc)
        return;
    void* dataPtr = emptyString ? 
                    ModC_Allocator_Malloc(this->Allocator, newCap) :
                    ModC_Allocator_Realloc(this->Allocator, this->Data, newCap);
    if(!dataPtr)
        return;
    this->Data = dataPtr;
    this->Length = resizeBytes;
    this->Cap = newCap;
    assert(newCap > resizeBytes);
}

//TODO: Add version which accepts an array?
//TODO: Return this to allow chaining?
static inline void ModC_String_Append(ModC_String* this, ModC_ConstStringView strToAppend)
{
    if(!this || !strToAppend.Data || strToAppend.Length == 0)
        return;
    if(!this->Data || this->Cap == 0)
    {
        ModC_String_Resize(this, strToAppend.Length);
        memcpy(this->Data, strToAppend.Data, strToAppend.Length);
    }
    else
    {
        uint32_t oldLen = this->Length;
        ModC_String_Resize(this, this->Length + strToAppend.Length);
        memcpy(this->Data + oldLen, strToAppend.Data, strToAppend.Length);
    }
    this->Data[this->Length] = '\0';
}

static inline ModC_String ModC_String_Create(   ModC_Allocator allocator, 
                                                char* data, 
                                                uint32_t length, 
                                                uint32_t cap)
{
    if(cap != 0)
        assert(cap > length);
    return  (ModC_String)
            {
                .Allocator = allocator,
                .Data = data,
                .Length = length,
                .Cap = cap
            };
}

static inline void ModC_String_Free(ModC_String* this)
{
    if(!this)
        return;
    if(!this->Data)
    {
        *this = (ModC_String){0};
        return;
    }
    this->Allocator.Free(this->Allocator.Allocator, this->Data);
    *this = (ModC_String){0};
    return;
}

#define MODC_FULL_STRING UINT32_MAX

//Returns `ModC_StringView` from `src`, starting from `index` that spans `length` long.
#define INTERN_MODC_DEFINE_STRING_VIEW_TO_VIEW(ModC_StringViewDst, funcName, ModC_StringViewSrc) \
    static inline ModC_StringViewDst funcName(  const ModC_StringViewSrc this, \
                                                const uint32_t index, \
                                                uint32_t length) \
    { \
        if(length == UINT32_MAX) \
            length = this.Length - index; \
        if(UINT32_MAX - length < index) \
            return (ModC_StringViewDst){0}; \
        return  (ModC_StringViewDst) \
                { \
                    .Data = this.Data + index, \
                    .Length = length, \
                    .NullEnd = false \
                }; \
    }

INTERN_MODC_DEFINE_STRING_VIEW_TO_VIEW(ModC_StringView, ModC_String_Subview, ModC_String)
INTERN_MODC_DEFINE_STRING_VIEW_TO_VIEW(ModC_ConstStringView, ModC_String_ConstSubview, ModC_String)
INTERN_MODC_DEFINE_STRING_VIEW_TO_VIEW(ModC_StringView, ModC_StringView_Subview, ModC_StringView)
INTERN_MODC_DEFINE_STRING_VIEW_TO_VIEW( ModC_ConstStringView, 
                                        ModC_StringView_ConstSubview, 
                                        ModC_StringView)
INTERN_MODC_DEFINE_STRING_VIEW_TO_VIEW( ModC_ConstStringView, 
                                        ModC_ConstStringView_ConstSubview,
                                        ModC_ConstStringView)

static inline ModC_StringView ModC_StringView_CreateFromCStr(char* cstr)
{
    return (ModC_StringView){ .Data = cstr, .Length = strlen(cstr), .NullEnd = true };
}

static inline ModC_ConstStringView ModC_ConstStringView_CreateFromConstCStr(const char* cstr)
{
    return (ModC_ConstStringView){ .Data = cstr, .Length = strlen(cstr), .NullEnd = true };
}

static inline ModC_StringView ModC_ConstStringView_RemoveConst(ModC_ConstStringView this)
{
    return  (ModC_StringView)
            { 
                .Data = (char*)this.Data, 
                .Length = (uint32_t)this.Length, 
                .NullEnd = (bool)this.NullEnd 
            };
}

#endif
