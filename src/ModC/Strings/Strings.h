#ifndef MODC_STRINGS_STRINGS_H
#define MODC_STRINGS_STRINGS_H

/* Docs
Creates String using List.h and StringView/ConstStringView using View.h

Also creates a tagged union StringUnion which is an union of all of the types above using 
TaggedUnion.h

Other than that, this also contains various helper functions. Just read the code to see what they are
*/

#include "ModC/Allocator.h"

#define LIST_NAME String
#define VALUE_TYPE char
#include "ModC/List.h"


#define VIEW_NAME StringView
#define CONST_VIEW_NAME ConstStringView
#define VALUE_TYPE char
#include "ModC/View.h"

#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define TU_NAME StringUnion
#define VALUE_TYPES String,StringView,ConstStringView
#include "ModC/TaggedUnion.h"

#define StringUnion_ViewFromLiteral(cstr) \
    TU_INIT(StringUnion, ConstStringView, ConstStringView_FromLiteral(cstr))

#define StringUnion_StringFromLiteral(allocator, cstr) \
    TU_INIT(StringUnion, String, String_FromLiteral(allocator, cstr))

static inline ConstStringView StringUnion_GetConstView(const StringUnion* this);

#define String_AppendLiteral(stringObj, cstr) String_AddRange((stringObj), cstr, sizeof(cstr) - 1)

static inline String String_FromData(Allocator allocator, const char* data, uint64_t length);

#define String_FromLiteral(allocator, cstr) String_FromData(allocator, cstr, sizeof(cstr) - 1)
    
static inline String String_FromFormat(Allocator allocator, const char* format, ...);
static inline String* String_AppendFormat(String* this, const char* format, ...);

static inline String String_FromVFormat(Allocator allocator, const char* format, va_list args);
static inline String* String_AppendVFormat(String* this, const char* format, va_list args);

#define String_IsEqualLiteral(this, cstr) \
    ((this)->Length == sizeof(cstr) - 1 && memcmp((this)->Data, cstr, sizeof(cstr) - 1) == 0)

#define StringView_IsEqualLiteral(this, cstr) String_IsEqualLiteral(this, cstr)
#define ConstStringView_IsEqualLiteral(this, cstr) String_IsEqualLiteral(this, cstr)

#define ConstStringView_FromLiteral(cstr) ConstStringView_Create(cstr, sizeof(cstr) - 1)

#define StringLikeEqual(strA, strB) \
    ((strA).Length == (strB).Length && memcmp((strA).Data, (strB).Data, (strA).Length) == 0)


//=======================================================================================
//Implementations
//=======================================================================================
static inline ConstStringView StringUnion_GetConstView(const StringUnion* this)
{
    #undef TaggedUnionNameState
    #define TaggedUnionNameState StringUnion
    if(!this)
        return (ConstStringView){0};
    
    return  this->Type == TU_TYPE_S(String) ?
            ConstStringView_Create(this->TU_DATA_S(String).Data, this->TU_DATA_S(String).Length) :
            this->TU_DATA_S(ConstStringView);
}

static inline String String_FromData(Allocator allocator, const char* data, uint64_t length)
{
    String retStr = String_Create(allocator, length);
    String_AddRange(&retStr, data, length);
    return retStr;
}

static inline String String_FromFormat(Allocator allocator, const char* format, ...)
{
    va_list args1;
    va_start(args1, format);
    String retStr = String_FromVFormat(allocator, format, args1);
    va_end(args1);
    return retStr;
}

static inline String* String_AppendFormat(String* this, const char* format, ...)
{
    va_list args1;
    va_start(args1, format);
    String_AppendVFormat(this, format, args1);
    va_end(args1);
    return this;
}

static inline String String_FromVFormat(Allocator allocator, const char* format, va_list args)
{
    String retStr = String_Create(allocator, 0);
    String_AppendVFormat(&retStr, format, args);
    return retStr;
}

static inline String* String_AppendVFormat(String* this, const char* format, va_list args)
{
    if(!this)
        return this;
    
    va_list args2;
    va_copy(args2, args);
    
    int writeLen = vsnprintf(NULL, 0, format, args);
    if(writeLen <= 0)
        goto exitPoint;
    
    uint32_t oldLen = this->Length;
    String_Resize(this, this->Length + writeLen + 1);
    if(!this->Data || this->Length == oldLen)
        goto exitPoint;
    
    writeLen = vsnprintf(this->Data + oldLen, writeLen + 1, format, args2);
    String_Resize(this, this->Length - 1);
    
    exitPoint:;
    va_end(args2);
    
    return this;
}

#endif
