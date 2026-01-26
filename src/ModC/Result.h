#ifndef MODC_RESULT_H
#define MODC_RESULT_H

/* Docs

#### Definitions

To define a result type:
`MODC_DEFINE_RESULT_STRUCT(ModC_ResultName, valueType)`
Equivalent to
```c
typedef struct
{
    bool HasError;
    union
    {
        valueType Value;
        ModC_Error* Error;
    } ValueOrError;
} ModC_ResultName;
...
```

Define `MODC_DEFAULT_ALLOC()` to use non `_ALLOC` macro variants
Define `ModC_ResultName` to use `_S` macro variants


#### Functions:

`static inline ModC_ConstStringView ModC_GetFileName(const ModC_ConstStringView path);`

```c
typedef struct
{
    ModC_StringView File;
    ModC_StringView Function;
    int32_t Line;
} ModC_Trace;
static inline void ModC_Trace_Create(   const char* file, 
                                        const char* function, 
                                        int line, 
                                        ModC_Trace* outTrace);
```

```c
typedef struct
{
    ModC_Trace Traces[MODC_MAX_TRACES];
    uint8_t TracesSize;
    ModC_String ErrorMsg;
    int32_t ErrorCode;
} ModC_Error;
```

Macro:
`void MODC_ERROR_APPEND_TRACE(ModC_Error* ModC_ErrorPtr);`

Macro:
    You can use `MODC_LAST_ERROR` which points to the error used to return.
    You can use `MODC_LAST_ERROR_MSG` which points to the error message used to return.
    Use `MODC_RET_ERROR()` inside `failedAction` to return the error.
`valueType* MODC_RESULT_TRY(result, failedAction);`


Macro:
    `allocated` is `true` if `msg` can be freed with `MODC_RESULT_FREE`
```c
ModC_ResultName MODC_ERROR_MSG_EC_ALLOC(ModC_ResultName,
                                        ModC_StringUnion msg,
                                        ModC_Allocator allocator,
                                        int32_t errorCode);
ModC_ResultName MODC_ERROR_MSG_EC(ModC_ResultName, ModC_StringUnion msg,int32_t errorCode);

//
ModC_ResultName MODC_ERROR_MSG_EC_ALLOC_S(  ModC_StringUnion msg,
                                            ModC_Allocator allocator,
                                            int32_t errorCode);
ModC_ResultName MODC_ERROR_MSG_EC_S(ModC_StringUnion msg, int32_t errorCode);
```

Macro:
```c
ModC_ResultName MODC_ERROR_MSG_ALLOC(ModC_ResultName, ModC_StringUnion msg, ModC_Allocator allocator);
ModC_ResultName MODC_ERROR_MSG(ModC_ResultName, ModC_StringUnion msg);

ModC_ResultName MODC_ERROR_MSG_ALLOC_S(ModC_StringUnion msg, ModC_Allocator allocator);
ModC_ResultName MODC_ERROR_MSG_S(ModC_StringUnion msg);
```


Macro:
```c
ModC_ResultName MODC_ERROR_STR_FMT(ModC_ResultName, (format, ...));
ModC_ResultName MODC_ERROR_STR_FMT(ModC_ResultName, (format, ...), int32_t errorCode);

ModC_ResultName MODC_ERROR_STR_FMT_ALLOC(ModC_ResultName, ModC_Allocator allocator, (format, ...));
ModC_ResultName MODC_ERROR_STR_FMT_ALLOC(ModC_ResultName, ModC_Allocator allocator, (format, ...), int32_t errorCode);

ModC_ResultName MODC_ERROR_CSTR(ModC_ResultName, cstr);
ModC_ResultName MODC_ERROR_CSTR(ModC_ResultName, cstr, int32_t errorCode);

ModC_ResultName MODC_ERROR_CSTR_ALLOC(ModC_ResultName, ModC_Allocator allocator, cstr);
ModC_ResultName MODC_ERROR_CSTR_ALLOC(ModC_ResultName, ModC_Allocator allocator, cstr, int32_t errorCode);

ModC_ResultName MODC_ERROR_STR_FMT_S((format, ...));
ModC_ResultName MODC_ERROR_STR_FMT_S((format, ...), int32_t errorCode);

ModC_ResultName MODC_ERROR_STR_FMT_ALLOC_S(ModC_Allocator allocator, (format, ...));
ModC_ResultName MODC_ERROR_STR_FMT_ALLOC_S(ModC_Allocator allocator, (format, ...), int32_t errorCode);

ModC_ResultName MODC_ERROR_CSTR_S(cstr);
ModC_ResultName MODC_ERROR_CSTR_S(cstr, int32_t errorCode);

ModC_ResultName MODC_ERROR_CSTR_ALLOC_S(ModC_Allocator allocator, cstr);
ModC_ResultName MODC_ERROR_CSTR_ALLOC_S(ModC_Allocator allocator, cstr, int32_t errorCode);

```

Macro:
`ModC_ResultName MODC_RESULT_VALUE(ModC_ResultName, valueType val);`
`ModC_ResultName MODC_RESULT_VALUE_S(valueType val);`

Macro:
`void MODC_RESULT_FREE_RESOURCE(ModC_ResultName, ModC_ResultName* result);`
`void MODC_RESULT_FREE_RESOURCE_S(ModC_ResultName* result);`

Macro:
```c
ModC_String MODC_RESULT_TO_STRING_ALLOC(ModC_ResultName, 
                                        ModC_ResultName resultVal, 
                                        ModC_Allocator allocator);
ModC_String MODC_RESULT_TO_STRING(ModC_ResultName, ModC_ResultName resultVal);

ModC_String MODC_RESULT_TO_STRING_ALLOC_S(  ModC_ResultName resultVal, 
                                            ModC_Allocator allocator);
ModC_String MODC_RESULT_TO_STRING_S(ModC_ResultName resultVal);
```


Macros:
    `ModC_ResultName` needs to be defined first
    You can use `MODC_LAST_ERROR` which points to the error used to return.
    You can use `MODC_LAST_ERROR_MSG` which points to the error message used to return.
    You can specify the error code with the `_EC` macro variants.
    Use `MODC_RET_ERROR()` inside `failedAction` to return the error.
```c
MODC_ASSERT_ALLOC(expr, (formatAppend), allocator, failedAction);
MODC_ASSERT(expr, (formatAppend), failedAction);

MODC_ASSERT_EC_ALLOC(expr, errorCode, (formatAppend), allocator, failedAction);
MODC_ASSERT_EC(expr, errorCode, (formatAppend), failedAction);
```
*/


#include "ModC/Strings/Strings.h"
#include "ModC/Allocator.h"
#include "ModC/ChainUtil.h"

#include "MacroPowerToys/Miscellaneous.h"
#include "MacroPowerToys/RemoveParenthesisInList.h"
#include "MacroPowerToys/Overload.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

typedef struct
{
    ModC_StringView File;
    ModC_StringView Function;
    int32_t Line;
} ModC_Trace;

static inline ModC_ConstStringView ModC_GetFileName(const ModC_ConstStringView path)
{
    int lastSlash = 0;
    int curr = 0;
    for(int i = 0; i < path.Length; ++i)
    {
        const char curChar = path.Data[curr];
        if(curChar == '/' || curChar == '\\')
            lastSlash = curr + 1;
        ++curr;
    }
    
    return ModC_ConstStringView_Slice(&path, lastSlash, path.Length);
}

static inline void ModC_Trace_Create(   const char* file, 
                                        const char* function, 
                                        int line, 
                                        ModC_Trace* outTrace)
{
    if(!outTrace)
        return;
    
    ModC_ConstStringView constFileView = 
        MODC_CHAIN( ModC_ConstStringView_Create, (file, strlen(file)),
                    ModC_GetFileName, ());
    ModC_ConstStringView constFuncView = ModC_ConstStringView_Create(function, strlen(function));
    *outTrace = (ModC_Trace)
                {
                    .File = ModC_ConstStringView_Unconst(&constFileView),
                    .Function = ModC_ConstStringView_Unconst(&constFuncView),
                    .Line = line
                };
}


#define MODC_MAX_TRACES 64

typedef struct
{
    ModC_Trace Traces[MODC_MAX_TRACES];
    ModC_Allocator Allocator;
    ModC_String ErrorMsg;
    uint8_t TracesSize;
    int32_t ErrorCode;
} ModC_Error;

static ModC_Error* ModC_GlobalError = NULL;
static ModC_Error* ModC_GlobalRetError = NULL;
static inline void ModC_VoidGlobalError(void) { (void)ModC_GlobalError; }
static inline void ModC_VoidGlobalRetError(void) { (void)ModC_GlobalRetError; }


#define MODC_DEFINE_RESULT_STRUCT(ModC_DefResultName, valueType) \
    typedef struct \
    { \
        bool HasError; \
        union \
        { \
            valueType Value; \
            ModC_Error* Error; \
        } ValueOrError; \
    } ModC_DefResultName; \
    \
    static inline ModC_DefResultName MPT_CONCAT(ModC_DefResultName, _CreateValue)(valueType val) \
    { \
        return (ModC_DefResultName) { .HasError = false, .ValueOrError.Value = val }; \
    } \
    \
    static inline ModC_DefResultName MPT_CONCAT(ModC_DefResultName, _CreateError)(ModC_Error* err) \
    { \
        return (ModC_DefResultName) { .HasError = true, .ValueOrError.Error = err }; \
    } \
    \
    static inline valueType \
    MPT_CONCAT(ModC_DefResultName, _ValueOrDefault)(const ModC_DefResultName result) \
    { \
        if(result.HasError) \
            ModC_GlobalError = result.ValueOrError.Error; \
        else \
            return result.ValueOrError.Value; \
        return (valueType){0}; \
    } \
    \
    static inline ModC_String \
    MPT_CONCAT(ModC_DefResultName, _ToString)( const ModC_DefResultName result, \
                                            ModC_Allocator allocator) \
    { \
        if(!result.HasError) \
            return (ModC_String){0}; \
        \
        ModC_Error* errorPtr = result.ValueOrError.Error; \
        ModC_String outputString = ModC_String_Create(allocator, 512); \
        \
        /* TODO: snprintf the whole thing? */ \
        ModC_String_AppendFormat(&outputString, "Error:\n  %.*s", MODC_LENGTH_DATA(errorPtr->ErrorMsg)); \
        if(errorPtr->ErrorCode != 0) \
            ModC_String_AppendFormat(&outputString, "\nError Code: %d", errorPtr->ErrorCode); \
        \
        ModC_String_AppendLiteral(&outputString, "\n\nStack trace:"); \
        for(int i = 0; i < errorPtr->TracesSize; ++i) \
        { \
            /* Trace */ \
            ModC_String_AppendFormat(   &outputString, \
                                        "\n  at %.*s:%d in %.*s()", \
                                        MODC_LENGTH_DATA(errorPtr->Traces[i].File), \
                                        errorPtr->Traces[i].Line, \
                                        MODC_LENGTH_DATA(errorPtr->Traces[i].Function)); \
        } \
        return outputString; \
    }
    
static inline ModC_Error* 
ModC_Error_InternCreateErrorMsgEc(  ModC_StringUnion msg,
                                    const char* file,
                                    const char* func,
                                    int32_t line,
                                    int32_t errorCode,
                                    ModC_Allocator allocator)
{
    ModC_Error* modcErrorPtr = ModC_Allocator_Malloc(&allocator, sizeof(ModC_Error));
    if(modcErrorPtr)
    {
        *modcErrorPtr = (ModC_Error){0};
        modcErrorPtr->Allocator = allocator;
        ModC_Trace_Create(file, func, line, &modcErrorPtr->Traces[modcErrorPtr->TracesSize++]);
        
        //ModC_Error.ErrorMsg
        {
            if(msg.Type == MODC_TAGGED_TYPE(ModC_StringUnion, ModC_String))
                modcErrorPtr->ErrorMsg = msg.Data.MODC_TAGGED_FIELD(ModC_StringUnion, ModC_String);
            else
            {
                modcErrorPtr->ErrorMsg = ModC_String_Create(allocator, 256);
                ModC_ConstStringView* msgView = &msg.Data.MODC_TAGGED_FIELD(ModC_StringUnion, 
                                                                            ModC_ConstStringView);
                ModC_String_AddRange(&modcErrorPtr->ErrorMsg, msgView->Data, msgView->Length);
            }
        }
        modcErrorPtr->ErrorCode = errorCode;
    }
    return modcErrorPtr;
}

#define MODC_ERROR_APPEND_TRACE(ModC_ErrorPtr) \
    do \
    { \
        if((ModC_ErrorPtr) && ModC_ErrorPtr->TracesSize < MODC_MAX_TRACES) \
        { \
            ModC_Trace_Create(  __FILE__, \
                                __func__, \
                                __LINE__, \
                                &ModC_ErrorPtr->Traces[ModC_ErrorPtr->TracesSize++]); \
        } \
    } while(false)

//Failed reuslt name
#define MODC_LAST_ERROR ModC_GlobalError
#define MODC_LAST_ERROR_MSG ModC_GlobalError->ErrorMsg
#define MODC_RET_ERROR() \
    do \
    { \
        ModC_GlobalRetError = ModC_GlobalError; \
        ModC_GlobalError = NULL; \
        return MPT_DELAYED_CONCAT(ModC_ResultName, _CreateError) (ModC_GlobalRetError); \
    } while(0)

#if 0
    #define INTERN_MODC_RESULT_TRY(ModC_ExprResultName, expr, failedAction, counter) \
        MPT_DELAYED_CONCAT(ModC_ExprResultName, _ValueOrDefault)(expr); \
        do \
        { \
            if(ModC_GlobalError) \
            { \
                MODC_ERROR_APPEND_TRACE(ModC_GlobalError); \
                MPT_REMOVE_PARENTHESIS(failedAction); \
            } \
        } while(0)


    #define MODC_RESULT_TRY(ModC_ExprResultName, expr, failedAction) \
        INTERN_MODC_RESULT_TRY(ModC_ExprResultName, expr, failedAction, __COUNTER__)
#else
    #define MODC_RESULT_TRY(result, ... /* failedAction */) \
        &result.ValueOrError.Value; \
        do \
        { \
            if(result.HasError) \
            { \
                ModC_GlobalError = result.ValueOrError.Error; \
                MODC_ERROR_APPEND_TRACE(result.ValueOrError.Error); \
                __VA_ARGS__; \
            } \
        } while(0)
#endif


#define MODC_ERROR_MSG_EC_ALLOC(ModC_ResultName, msg, allocator, errorCode) \
    MPT_DELAYED_CONCAT2(ModC_ResultName, _CreateError) \
    ( \
        ModC_Error_InternCreateErrorMsgEc(  (msg), \
                                            __FILE__, \
                                            __func__, \
                                            __LINE__, \
                                            (errorCode), \
                                            (allocator)) \
    )

#define MODC_ERROR_MSG_EC_ALLOC_S(msg, allocator, errorCode) \
    MODC_ERROR_MSG_EC_ALLOC(ModC_ResultName, msg, allocator, errorCode)

#define MODC_ERROR_MSG_ALLOC(ModC_ResultName, msg, allocator) \
    MODC_ERROR_MSG_EC_ALLOC(ModC_ResultName, msg, allocator, 0)

#define MODC_ERROR_MSG_ALLOC_S(msg, allocator) MODC_ERROR_MSG_ALLOC(ModC_ResultName, msg, allocator)

#define MODC_ERROR_MSG(ModC_ResultName, msg) \
    MODC_ERROR_MSG_EC_ALLOC(ModC_ResultName, msg, MODC_DEFAULT_ALLOC(), 0)

#define MODC_ERROR_MSG_S(msg) MODC_ERROR_MSG(ModC_ResultName, msg)

#define MODC_ERROR_MSG_EC(ModC_ResultName, msg, errorCode) \
    MODC_ERROR_MSG_EC_ALLOC(ModC_ResultName, msg, MODC_DEFAULT_ALLOC(), errorCode)

#define MODC_ERROR_MSG_EC_S(msg, errorCode) MODC_ERROR_MSG_EC(ModC_ResultName, msg, errorCode)



#define INTERN_MODC_STR_VIEW_FROM_STR_FMT(allocator, strfmts) \
    MODC_TAGGED_INIT(   ModC_StringUnion, \
                        ModC_String, \
                        ModC_String_FromFormat(allocator, MPT_REMOVE_PARENTHESIS(strfmts)))

#define INTERN_MODC_ERROR_STR_FMT_ALLOC_4(ModC_ResultName, allocator, strfmts, errorCode) \
    MPT_DELAYED_CONCAT2(ModC_ResultName, _CreateError) \
    ( \
        ModC_Error_InternCreateErrorMsgEc(  INTERN_MODC_STR_VIEW_FROM_STR_FMT(allocator, strfmts), \
                                            __FILE__, \
                                            __func__, \
                                            __LINE__, \
                                            (errorCode), \
                                            (allocator)) \
    )

#define INTERN_MODC_ERROR_STR_FMT_ALLOC_3(ModC_ResultName, allocator, strfmts) \
    INTERN_MODC_ERROR_STR_FMT_ALLOC_4(ModC_ResultName, allocator, strfmts, 0)

//ModC_ResultName MODC_ERROR_STR_FMT_ALLOC(ModC_ResultName, ModC_Allocator allocator, (format, ...));
//ModC_ResultName MODC_ERROR_STR_FMT_ALLOC(ModC_ResultName, ModC_Allocator allocator, (format, ...), int32_t errorCode);
#define MODC_ERROR_STR_FMT_ALLOC(...) \
    MPT_OVERLOAD_MACRO(INTERN_MODC_ERROR_STR_FMT_ALLOC, __VA_ARGS__)

#define MODC_ERROR_STR_FMT_ALLOC_S(...) MODC_ERROR_STR_FMT_ALLOC(ModC_ResultName, __VA_ARGS__)

//ModC_ResultName MODC_ERROR_STR_FMT(ModC_ResultName, (format, ...));
//ModC_ResultName MODC_ERROR_STR_FMT(ModC_ResultName, (format, ...), int32_t errorCode);
#define MODC_ERROR_STR_FMT(ModC_ResultName, ...) \
    MODC_ERROR_STR_FMT_ALLOC(ModC_ResultName, MODC_DEFAULT_ALLOC(), __VA_ARGS__)

#define MODC_ERROR_STR_FMT_S(...) MODC_ERROR_STR_FMT(ModC_ResultName, __VA_ARGS__)

#define INTERN_MODC_ERROR_CSTR_ALLOC_4(ModC_ResultName, allocator, cstr, errorCode) \
    MPT_DELAYED_CONCAT2(ModC_ResultName, _CreateError) \
    ( \
        ModC_Error_InternCreateErrorMsgEc(  ModC_StringUnion_ViewFromLiteral(cstr), \
                                            __FILE__, \
                                            __func__, \
                                            __LINE__, \
                                            (errorCode), \
                                            (allocator)) \
    )

#define INTERN_MODC_ERROR_CSTR_ALLOC_3(ModC_ResultName, allocator, cstr) \
    INTERN_MODC_ERROR_CSTR_ALLOC_4(ModC_ResultName, allocator, cstr, 0)

//ModC_ResultName MODC_ERROR_CSTR_ALLOC(ModC_ResultName, ModC_Allocator allocator, cstr);
//ModC_ResultName MODC_ERROR_CSTR_ALLOC(ModC_ResultName, ModC_Allocator allocator, cstr, int32_t errorCode);
#define MODC_ERROR_CSTR_ALLOC(...) MPT_OVERLOAD_MACRO(INTERN_MODC_ERROR_CSTR_ALLOC, __VA_ARGS__)

#define MODC_ERROR_CSTR_ALLOC_S(...) MODC_ERROR_CSTR_ALLOC(ModC_ResultName, __VA_ARGS__)

//ModC_ResultName MODC_ERROR_CSTR(ModC_ResultName, cstr);
//ModC_ResultName MODC_ERROR_CSTR(ModC_ResultName, cstr, int32_t errorCode);
#define MODC_ERROR_CSTR(ModC_ResultName, ...) \
    MODC_ERROR_CSTR_ALLOC(ModC_ResultName, MODC_DEFAULT_ALLOC(), __VA_ARGS__)

#define MODC_ERROR_CSTR_S(...) MODC_ERROR_CSTR(ModC_ResultName, __VA_ARGS__)


#define MODC_RESULT_VALUE(ModC_ResultName, val) \
    MPT_DELAYED_CONCAT(ModC_ResultName, _CreateValue)(val)

#define MODC_RESULT_VALUE_S(val) MODC_RESULT_VALUE(ModC_ResultName, val)

#define MODC_RESULT_FREE_RESOURCE(ModC_ResultName, resultPtr) \
    do \
    { \
        if((resultPtr) == NULL) \
            break; \
         \
        if(!(resultPtr)->HasError) \
        { \
            *(resultPtr) = (ModC_ResultName){0}; \
            break; \
        } \
        ModC_String_Free(&(resultPtr)->ValueOrError.Error->ErrorMsg); \
        ModC_Allocator errorAllocator = (resultPtr)->ValueOrError.Error->Allocator; \
        ModC_Allocator_Free(&errorAllocator, (resultPtr)->ValueOrError.Error); \
        ModC_Allocator_Destroy(&errorAllocator); \
        *(resultPtr) = (ModC_ResultName){0}; \
        break; \
    } while(0)

#define MODC_RESULT_FREE_RESOURCE_S(resultPtr) MODC_RESULT_FREE_RESOURCE(ModC_ResultName, resultPtr)


#define MODC_RESULT_TO_STRING_ALLOC(ModC_ResultName, resultVal, allocator) \
    MPT_DELAYED_CONCAT(ModC_ResultName, _ToString)(resultVal, allocator)

#define MODC_RESULT_TO_STRING_ALLOC_S(resultVal, allocator) \
    MODC_RESULT_TO_STRING_ALLOC(ModC_ResultName, resultVal, allocator)

#define MODC_RESULT_TO_STRING(ModC_ResultName, resultVal) \
    MPT_DELAYED_CONCAT(ModC_ResultName, _ToString)(resultVal, MODC_DEFAULT_ALLOC())

#define MODC_RESULT_TO_STRING_S(resultVal) MODC_RESULT_TO_STRING(ModC_ResultName, resultVal)

#define INTERNAL_MODC_ASSERT_EC_ALLOC(expr, errorCode, formatAppend, allocator, ... /* failedAction */) \
    do \
    { \
        if(!(expr)) \
        { \
            ModC_ConstStringView exprView = \
                ModC_ConstStringView_FromLiteral("Expression \"" #expr "\" has failed."); \
            ModC_StringUnion assertView = MODC_TAGGED_INIT( ModC_StringUnion, \
                                                            ModC_ConstStringView, \
                                                            exprView); \
            ModC_GlobalError = ModC_Error_InternCreateErrorMsgEc(   assertView, \
                                                                    __FILE__, \
                                                                    __func__, \
                                                                    __LINE__, \
                                                                    (errorCode), \
                                                                    (allocator)); \
            ModC_String_AppendFormat(   &ModC_GlobalError->ErrorMsg, \
                                        MPT_REMOVE_PARENTHESIS(formatAppend)); \
            __VA_ARGS__; \
        } \
    } \
    while(false)

#define MODC_ASSERT(expr, formatAppend, ... /* failedAction */) \
    INTERNAL_MODC_ASSERT_EC_ALLOC(expr, 0, formatAppend, (MODC_DEFAULT_ALLOC()), __VA_ARGS__)

#define MODC_ASSERT_ALLOC(expr, formatAppend, allocator, ... /* failedAction */) \
    INTERNAL_MODC_ASSERT_EC_ALLOC(expr, 0, formatAppend, allocator, __VA_ARGS__)

#define MODC_ASSERT_EC_ALLOC(expr, errorCode, formatAppend, allocator, ... /* failedAction */) \
    INTERNAL_MODC_ASSERT_EC_ALLOC(expr, errorCode, formatAppend, allocator, __VA_ARGS__)

#define MODC_ASSERT_EC(expr, errorCode, formatAppend, ... /* failedAction */) \
    INTERNAL_MODC_ASSERT_EC_ALLOC(expr, errorCode, formatAppend, (MODC_DEFAULT_ALLOC()), __VA_ARGS__)

#endif
