#ifndef MODC_STRINGS_STRINGS_H
#define MODC_STRINGS_STRINGS_H

/* Docs
Just read the code
*/

#include "ModC/Allocator.h"

#define MODC_LIST_NAME ModC_String
#define MODC_VALUE_TYPE char
#include "ModC/List.h"


#define MODC_VIEW_NAME ModC_StringView
#define MODC_CONST_VIEW_NAME ModC_ConstStringView
#define MODC_VALUE_TYPE char
#include "ModC/View.h"

#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define MODC_TAGGED_UNION_NAME ModC_StringUnion
#define MODC_VALUE_TYPES ModC_String,ModC_StringView,ModC_ConstStringView
#include "ModC/TaggedUnion.h"

#define ModC_StringUnion_ViewFromLiteral(cstr) \
    MODC_TAGGED_INIT(ModC_StringUnion, ModC_ConstStringView, ModC_ConstStringView_FromLiteral(cstr))

#define ModC_StringUnion_StringFromLiteral(allocator, cstr) \
    MODC_TAGGED_INIT(ModC_StringUnion, ModC_String, ModC_String_FromLiteral(allocator, cstr))

#define ModC_String_AppendLiteral(stringObj, cstr) \
    ModC_String_AddRange((stringObj), cstr, sizeof(cstr) - 1)

static inline ModC_String ModC_String_FromData( ModC_Allocator allocator, 
                                                const char* data, 
                                                uint64_t length);

#define ModC_String_FromLiteral(allocator, cstr) \
    ModC_String_FromData(allocator, cstr, sizeof(cstr) - 1)
    
static inline ModC_String ModC_String_FromFormat(   ModC_Allocator allocator, 
                                                    const char* format, 
                                                    ...);
static inline ModC_String* ModC_String_AppendFormat(ModC_String* this, const char* format, ...);

static inline ModC_String ModC_String_FromVFormat(  ModC_Allocator allocator, 
                                                    const char* format, 
                                                    va_list args);
static inline ModC_String* ModC_String_AppendVFormat(   ModC_String* this, 
                                                        const char* format, 
                                                        va_list args);

#define ModC_String_IsEqualLiteral(obj, cstr) \
    ((obj)->Length == sizeof(cstr) - 1 && memcmp((obj)->Data, cstr, sizeof(cstr) - 1) == 0)

#define ModC_StringView_IsEqualLiteral(obj, cstr) ModC_String_IsEqualLiteral(obj, cstr)
#define ModC_ConstStringView_IsEqualLiteral(obj, cstr) ModC_String_IsEqualLiteral(obj, cstr)

#define ModC_ConstStringView_FromLiteral(cstr) ModC_ConstStringView_Create(cstr, sizeof(cstr) - 1)




//=======================================================================================
//Implementations
//=======================================================================================
static inline ModC_String ModC_String_FromData( ModC_Allocator allocator, 
                                                const char* data, 
                                                uint64_t length)
{
    ModC_String retStr = ModC_String_Create(allocator, length);
    ModC_String_AddRange(&retStr, data, length);
    return retStr;
}

static inline ModC_String ModC_String_FromFormat(   ModC_Allocator allocator, 
                                                    const char* format, 
                                                    ...)
{
    va_list args1;
    va_start(args1, format);
    ModC_String retStr = ModC_String_FromVFormat(allocator, format, args1);
    va_end(args1);
    return retStr;
}

static inline ModC_String* ModC_String_AppendFormat(ModC_String* this, const char* format, ...)
{
    va_list args1;
    va_start(args1, format);
    ModC_String_AppendVFormat(this, format, args1);
    va_end(args1);
    return this;
}

static inline ModC_String ModC_String_FromVFormat(  ModC_Allocator allocator, 
                                                    const char* format, 
                                                    va_list args)
{
    ModC_String retStr = ModC_String_Create(allocator, 0);
    ModC_String_AppendVFormat(&retStr, format, args);
    return retStr;
}

static inline ModC_String* ModC_String_AppendVFormat(   ModC_String* this, 
                                                        const char* format, 
                                                        va_list args)
{
    if(!this)
        return this;
    
    va_list args2;
    va_copy(args2, args);
    
    int writeLen = vsnprintf(NULL, 0, format, args);
    if(writeLen <= 0)
        goto exitPoint;
    
    uint32_t oldLen = this->Length;
    ModC_String_Resize(this, this->Length + writeLen + 1);
    if(!this->Data || this->Length == oldLen)
        goto exitPoint;
    
    writeLen = vsnprintf(this->Data + oldLen, writeLen + 1, format, args2);
    ModC_String_Resize(this, this->Length - 1);
    
    exitPoint:;
    va_end(args2);
    
    return this;
}

#endif
