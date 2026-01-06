#ifndef MODC_STRINGS_STRINGS_H
#define MODC_STRINGS_STRINGS_H

/* Docs
Just read the code
*/

#include "ModC/Allocator.h"

#include "MacroPowerToys/MacroPowerToys.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>

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

typedef struct
{
    bool IsString;
    union
    {
        ModC_String String;
        ModC_ConstStringView View;
    } Value;
} ModC_StringOrConstView;

static inline ModC_StringOrConstView ModC_StringOrConstView_String(const ModC_String str);
static inline ModC_StringOrConstView ModC_StringOrConstView_View(const ModC_ConstStringView view);


//`cstr` is already allocated from allocator
static inline ModC_String ModC_String_FromCStr(const ModC_Allocator allocator, char* cstr);
static inline ModC_String ModC_String_FromView( const ModC_Allocator allocator, 
                                                const ModC_StringView view);
static inline ModC_String ModC_String_FromConstView(const ModC_Allocator allocator, 
                                                    const ModC_ConstStringView view);
static inline ModC_String* ModC_String_Reserve(ModC_String* this, uint32_t reserveBytes);
static inline ModC_String* ModC_String_Resize(ModC_String* this, uint32_t resizeBytes);
static inline ModC_String* ModC_String_AppendConstView( ModC_String* this, 
                                                        const ModC_ConstStringView strToAppend);
static inline ModC_String* ModC_String_AppendView(  ModC_String* this, 
                                                    const ModC_StringView strToAppend);
static inline ModC_String* ModC_String_AppendString(ModC_String* this, const ModC_String strToAppend);
#define ModC_AppendString(...) ModC_AppendString(__VA_ARGS__)
static inline ModC_String* ModC_String_AppendFormat(ModC_String* this, const char* format, ...);
#define ModC_AppendFormat(...) ModC_String_AppendFormat(__VA_ARGS__)
static inline ModC_String ModC_String_Create(const ModC_Allocator allocator, uint32_t cap);
static inline void ModC_String_Free(ModC_String* this);
static inline ModC_StringView ModC_String_View(const ModC_String this);
static inline ModC_ConstStringView ModC_String_ConstView(const ModC_String this);


static inline ModC_ConstStringView ModC_StringView_ConstView(const ModC_StringView this);
static inline ModC_StringView ModC_StringView_FromCStr(char* cstr);
static inline ModC_ConstStringView ModC_ConstStringView_FromConstCStr(const char* cstr);
#define ModC_FromConstCStr(...) ModC_ConstStringView_FromConstCStr(__VA_ARGS__)
static inline ModC_StringView ModC_ConstStringView_RemoveConst(const ModC_ConstStringView this);

#define MODC_FULL_STRING UINT32_MAX

//`length` excludes null-terminator. Use `MODC_FULL_STRING` for `length` to reach the end.
static inline ModC_StringView ModC_String_Subview(  const ModC_String this, 
                                                    const uint32_t index, 
                                                    uint32_t length);
//`length` excludes null-terminator. Use `MODC_FULL_STRING` for `length` to reach the end.
static inline ModC_ConstStringView ModC_String_ConstSubview(const ModC_String this, 
                                                            const uint32_t index, 
                                                            uint32_t length);
//`length` excludes null-terminator. Use `MODC_FULL_STRING` for `length` to reach the end.
static inline ModC_StringView ModC_StringView_Subview(  const ModC_StringView this, 
                                                        const uint32_t index, 
                                                        uint32_t length);
//`length` excludes null-terminator. Use `MODC_FULL_STRING` for `length` to reach the end.
static inline ModC_ConstStringView ModC_StringView_ConstSubview(const ModC_StringView this, 
                                                                const uint32_t index, 
                                                                uint32_t length);
//`length` excludes null-terminator. Use `MODC_FULL_STRING` for `length` to reach the end.
static inline ModC_ConstStringView 
ModC_ConstStringView_ConstSubview(  const ModC_ConstStringView this, 
                                    const uint32_t index, 
                                    uint32_t length);






static inline ModC_StringOrConstView ModC_StringOrConstView_String(const ModC_String str)
{
    return (ModC_StringOrConstView){ .IsString = true, .Value.String = str };
}

static inline ModC_StringOrConstView ModC_StringOrConstView_View(const ModC_ConstStringView view)
{
    return (ModC_StringOrConstView){ .IsString = false, .Value.View = view };
}

//`cstr` is already allocated from allocator
static inline ModC_String ModC_String_FromCStr(const ModC_Allocator allocator, char* cstr)
{
    uint32_t len = strlen(cstr);
    return (ModC_String){ .Allocator = allocator, .Data = cstr, .Length = len, .Cap = len + 1 };
}

static inline ModC_String ModC_String_FromView( const ModC_Allocator allocator, 
                                                const ModC_StringView view)
{
    ModC_String retStr = ModC_String_Create(allocator, view.Length + 1);
    ModC_String_Resize(&retStr, view.Length);
    if(!retStr.Data)
    {
        ModC_String_Free(&retStr);
        return (ModC_String){0};
    }
    memcpy(retStr.Data, view.Data, view.Length);
    retStr.Data[view.Length] = '\0';
    return retStr;
}

static inline ModC_String ModC_String_FromConstView(const ModC_Allocator allocator, 
                                                    const ModC_ConstStringView view)
{
    return ModC_String_FromView(allocator, ModC_ConstStringView_RemoveConst(view));
}

static inline ModC_String* ModC_String_Reserve(ModC_String* this, uint32_t reserveBytes)
{
    if(!this || this->Cap >= reserveBytes)
        return this;
    
    bool emptyString = this->Cap == 0;
    void* dataPtr = emptyString ? 
                    ModC_Allocator_Malloc(this->Allocator, reserveBytes) :
                    ModC_Allocator_Realloc(this->Allocator, this->Data, reserveBytes);
    if(!dataPtr)
        return this;
    this->Data = dataPtr;
    this->Cap = reserveBytes;
    return this;
}

//`resizeBytes` excludes null-terminator
static inline ModC_String* ModC_String_Resize(ModC_String* this, uint32_t resizeBytes)
{
    if(!this)
        return this;
    if(this->Length > resizeBytes)
    {
        this->Data[resizeBytes] = '\0';
        this->Length = resizeBytes;
        return this;
    }
    
    if(this->Cap > resizeBytes - 1)
    {
        this->Length = resizeBytes;
        return this;
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
    void* dataPtr = emptyString ? 
                    ModC_Allocator_Malloc(this->Allocator, newCap) :
                    ModC_Allocator_Realloc(this->Allocator, this->Data, newCap);
    if(!dataPtr)
        return this;
    this->Data = dataPtr;
    this->Length = resizeBytes;
    this->Cap = newCap;
    assert(newCap > resizeBytes);
    return this;
}

//TODO: Add version which accepts an array?
static inline ModC_String* ModC_String_AppendConstView( ModC_String* this, 
                                                        const ModC_ConstStringView strToAppend)
{
    if(!this || !strToAppend.Data || strToAppend.Length == 0)
        return this;
    if(!this->Data || this->Cap == 0)
    {
        ModC_String_Resize(this, strToAppend.Length);
        if(!this->Data)
            return this;
        memcpy(this->Data, strToAppend.Data, strToAppend.Length);
    }
    else
    {
        uint32_t oldLen = this->Length;
        ModC_String_Resize(this, this->Length + strToAppend.Length);
        if(!this->Data || this->Length == oldLen)
            return this;
        memcpy(this->Data + oldLen, strToAppend.Data, strToAppend.Length);
    }
    this->Data[this->Length] = '\0';
    return this;
}

static inline ModC_String* ModC_String_AppendView(  ModC_String* this, 
                                                    const ModC_StringView strToAppend)
{
    return ModC_String_AppendConstView(this, ModC_StringView_ConstView(strToAppend));
}
static inline ModC_String* ModC_String_AppendString(ModC_String* this, const ModC_String strToAppend)
{
    return ModC_String_AppendConstView(this, ModC_String_ConstView(strToAppend));
}

static inline ModC_String* ModC_String_AppendFormat(ModC_String* this, const char* format, ...)
{
    if(!this)
        return this;
    
    va_list args1;
    va_list args2;
    va_start(args1, format);
    va_copy(args2, args1);
    
    int writeLen = vsnprintf(NULL, 0, format, args1);
    if(writeLen < 0)
        goto exitPoint;
    
    uint32_t oldLen = this->Length;
    ModC_String_Resize(this, this->Length + writeLen);
    if(!this->Data || this->Length == oldLen)
        goto exitPoint;
    
    writeLen = vsnprintf(this->Data + oldLen, writeLen + 1, format, args2);
    
    exitPoint:;
    va_end(args1);
    va_end(args2);
    
    return this;
}

//`cap` includes null-terminator
static inline ModC_String ModC_String_Create(const ModC_Allocator allocator, uint32_t cap)
{
    ModC_String retStr = { .Allocator = allocator };
    ModC_String_Reserve(&retStr, cap);
    return retStr;
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
    ModC_Allocator_Free(this->Allocator, this->Data);
    ModC_Allocator_Destroy(&this->Allocator);
    *this = (ModC_String){0};
    return;
}

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

static inline ModC_StringView ModC_String_View(const ModC_String this)
{
    return ModC_String_Subview(this, 0, MODC_FULL_STRING);
}

static inline ModC_ConstStringView ModC_String_ConstView(const ModC_String this)
{
    return ModC_String_ConstSubview(this, 0, MODC_FULL_STRING);
}

static inline ModC_ConstStringView ModC_StringView_ConstView(const ModC_StringView this)
{
    return ModC_StringView_ConstSubview(this, 0, MODC_FULL_STRING);
}

static inline ModC_StringView ModC_StringView_FromCStr(char* cstr)
{
    return (ModC_StringView){ .Data = cstr, .Length = strlen(cstr), .NullEnd = true };
}

static inline ModC_ConstStringView ModC_ConstStringView_FromConstCStr(const char* cstr)
{
    return (ModC_ConstStringView){ .Data = cstr, .Length = strlen(cstr), .NullEnd = true };
}

static inline ModC_StringView ModC_ConstStringView_RemoveConst(const ModC_ConstStringView this)
{
    return  (ModC_StringView)
            { 
                .Data = (char*)this.Data, 
                .Length = (uint32_t)this.Length, 
                .NullEnd = (bool)this.NullEnd 
            };
}

#endif
