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

Define `MODC_DEFAULT_ALLOC_FUNC` and `MODC_DEFAULT_ALLOC_ARGS` to use non `_ALLOC` macro variants


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
    `MODC_LAST_RESULT` needs to be defined with `MDOC_DECLARE_LAST_RESULT(ModC_ResultName)`.
    You can use `MODC_LAST_RESULT` to get the result that contains the error in `failedAction`.
    You can use `MODC_LAST_ERROR` which expands to `MODC_LAST_RESULT.ValueOrError.Error`
    You can use `MODC_LAST_ERROR_MSG` which expands to `MODC_LAST_RESULT.ValueOrError.Error->ErrorMsg`
`valueType MODC_RESULT_TRY(ModC_ResultName, expr, failedAction);`


Macro:
    `allocated` is `true` if `msg` can be freed with `MODC_RESULT_FREE`
```c
ModC_ResultName MODC_ERROR_MSG_EC_ALLOC(ModC_ResultName, 
                                        ModC_StringOrConstView msg,
                                        const ModC_Allocator allocator,
                                        int32_t errorCode);

ModC_ResultName MODC_ERROR_MSG_EC(  ModC_ResultName, 
                                    ModC_StringOrConstView msg,
                                    int32_t errorCode);
```

Macro:
```c
ModC_ResultName MODC_ERROR_MSG_ALLOC(   ModC_ResultName, 
                                        ModC_StringOrConstView msg, 
                                        const ModC_Allocator allocator);

ModC_ResultName MODC_ERROR_MSG( ModC_ResultName, 
                                ModC_StringOrConstView msg);
```

Macro:
`ModC_ResultName MODC_RESULT_VALUE(ModC_ResultName, valueType val);`

Macro:
`void MODC_RESULT_FREE_RESOURCE(ModC_ResultName, ModC_ResultName* result);`

Macro:
```c
ModC_String MODC_RESULT_TO_STRING_ALLOC(ModC_ResultName, 
                                        ModC_ResultName resultVal, 
                                        ModC_Allocator allocator);
ModC_String MODC_RESULT_TO_STRING(  ModC_ResultName, 
                                    ModC_ResultName resultVal);
```

Macros:
    `MODC_LAST_RESULT` needs to be defined with `MDOC_DECLARE_LAST_RESULT(ModC_ResultName)`.
    You can use `MODC_LAST_RESULT` to get the result that contains the error in `failedAction`.
    You can use `MODC_LAST_ERROR` which expands to `MODC_LAST_RESULT.ValueOrError.Error`
    You can use `MODC_LAST_ERROR_MSG` which expands to `MODC_LAST_RESULT.ValueOrError.Error->ErrorMsg`
    You can specify the error code with the `_EC` macro variants
```c
MODC_ASSERT_ALLOC(expr, failedAction, ModC_ResultName, allocator);
MODC_ASSERT(expr, failedAction, ModC_ResultName);

MODC_ASSERT_EC_ALLOC(expr, errorCode, failedAction, ModC_ResultName, allocator);
MODC_ASSERT_EC(expr, errorCode, failedAction, ModC_ResultName);
```
*/


#include "ModC/Strings/Strings.h"
#include "ModC/Allocator.h"
#include "ModC/ChainUtil.h"

#include "MacroPowerToys/MacroPowerToys.h"

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
    
    return ModC_ConstStringView_ConstSubview(path, lastSlash, MODC_FULL_STRING);
}

static inline void ModC_Trace_Create(   const char* file, 
                                        const char* function, 
                                        int line, 
                                        ModC_Trace* outTrace)
{
    if(!outTrace)
        return;
    
    ModC_ConstStringView constFileView = 
        MODC_CHAIN( ModC_ConstStringView_FromConstCStr, (file),
                    ModC_GetFileName, ());
    ModC_ConstStringView constFuncView = ModC_ConstStringView_FromConstCStr(function);
    *outTrace = (ModC_Trace)
                {
                    .File = ModC_ConstStringView_RemoveConst(constFileView),
                    .Function = ModC_ConstStringView_RemoveConst(constFuncView),
                    .Line = line
                };
}


#define MODC_MAX_TRACES 64

#ifndef MODC_PERFORM_ALLOC
    #define MODC_PERFORM_ALLOC() MODC_DEFAULT_ALLOC_FUNC(MODC_DEFAULT_ALLOC_ARGS)
#endif

typedef struct
{
    ModC_Trace Traces[MODC_MAX_TRACES];
    ModC_Allocator Allocator;
    ModC_String ErrorMsg;
    uint8_t TracesSize;
    int32_t ErrorCode;
} ModC_Error;

static ModC_Error* ModC_GlobalError = NULL;
static inline void ModC_VoidGlobalError(void) { (void)ModC_GlobalError; }


#define MODC_DEFINE_RESULT_STRUCT(ModC_ResultName, valueType) \
    typedef struct \
    { \
        bool HasError; \
        union \
        { \
            valueType Value; \
            ModC_Error* Error; \
        } ValueOrError; \
    } ModC_ResultName; \
    \
    static inline ModC_ResultName \
    MPT_CONCAT(ModC_ResultName, _InternCreateErrorMsgEc)(   ModC_StringOrConstView msg, \
                                                            const char* file, \
                                                            const char* func, \
                                                            int32_t line, \
                                                            int32_t errorCode, \
                                                            const ModC_Allocator allocator) \
    { \
        ModC_Error* modcErrorPtr = ModC_Allocator_Malloc(allocator, sizeof(ModC_Error)); \
        if(modcErrorPtr) \
        { \
            *modcErrorPtr = (ModC_Error){0}; \
            modcErrorPtr->Allocator = allocator; \
            /* ModC_Error.Traces and ModC_Error.TracesSize */ \
            ModC_Trace_Create(file, func, line, &modcErrorPtr->Traces[modcErrorPtr->TracesSize++]); \
            \
            /* ModC_Error.ErrorMsg */ \
            { \
                if(msg.IsString) \
                    modcErrorPtr->ErrorMsg = msg.Value.String; \
                else \
                { \
                    modcErrorPtr->ErrorMsg = ModC_String_Create(allocator, 256); \
                    ModC_String_AppendConstView(&modcErrorPtr->ErrorMsg, msg.Value.View); \
                } \
            } \
            \
            /* ModC_Error.ErrorCode */ \
            modcErrorPtr->ErrorCode = errorCode; \
        } \
        \
        return  (ModC_ResultName) \
                { \
                    .HasError = true,  \
                    .ValueOrError.Error = modcErrorPtr \
                }; \
    } \
    \
    static inline valueType MPT_CONCAT(ModC_ResultName, _ValueOrDefault)(const ModC_ResultName result) \
    { \
        if(result.HasError) \
            ModC_GlobalError = result.ValueOrError.Error; \
        else \
            return result.ValueOrError.Value; \
        return (valueType){0}; \
    } \
    \
    static inline void MPT_CONCAT(ModC_ResultName, _Free)(ModC_ResultName* resultPtr) \
    { \
        if(!resultPtr) \
            return; \
         \
        if(!resultPtr->HasError) \
        { \
            *resultPtr = (ModC_ResultName){0}; \
            return; \
        } \
        ModC_String_Free(&resultPtr->ValueOrError.Error->ErrorMsg); \
        ModC_Allocator errorAllocator = resultPtr->ValueOrError.Error->Allocator; \
        ModC_Allocator_Free(errorAllocator, resultPtr->ValueOrError.Error); \
        ModC_Allocator_Destroy(&errorAllocator); \
        *resultPtr = (ModC_ResultName){0}; \
        return; \
    } \
    \
    static inline ModC_String MPT_CONCAT(ModC_ResultName, _ToString)(   const ModC_ResultName result, \
                                                                        ModC_Allocator allocator) \
    { \
        if(!result.HasError) \
            return (ModC_String){0}; \
        \
        ModC_Error* errorPtr = result.ValueOrError.Error; \
        ModC_String outputString = ModC_String_Create(allocator, 512); \
        \
        /* TODO: snprintf the whole thing? */ \
        MODC_CHAIN( ModC_String_AppendConstView, (&outputString, ModC_FromConstCStr("Error:\n  ")), \
                    ModC_String_AppendString, (errorPtr->ErrorMsg)); \
        if(errorPtr->ErrorCode != 0) \
        { \
            ModC_String_AppendConstView(&outputString, ModC_FromConstCStr("\nError Code: ")); \
            ModC_String_AppendFormat(&outputString, "%d", errorPtr->ErrorCode); \
        } \
        \
        ModC_String_AppendConstView(&outputString, ModC_FromConstCStr("\n\nStack trace:")); \
        for(int i = 0; i < errorPtr->TracesSize; ++i) \
        { \
            /* Trace */ \
            MODC_CHAIN \
            ( \
                ModC_String_AppendConstView, (&outputString, ModC_FromConstCStr("\n  at ")), \
                ModC_String_AppendView, (errorPtr->Traces[i].File), \
                ModC_String_AppendConstView, (ModC_FromConstCStr(":")), \
                ModC_String_AppendFormat, ("%d", errorPtr->Traces[i].Line), \
                ModC_String_AppendConstView, (ModC_FromConstCStr(" in ")),  \
                ModC_String_AppendView, (errorPtr->Traces[i].Function), \
                ModC_String_AppendConstView, (ModC_FromConstCStr("()")) \
            ); \
        } \
        return outputString; \
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
#define MODC_LAST_RESULT tempResult
#define MODC_LAST_ERROR MODC_LAST_RESULT.ValueOrError.Error
#define MODC_LAST_ERROR_MSG MODC_LAST_ERROR->ErrorMsg
#define MDOC_DECLARE_LAST_RESULT(ModC_ResultName) \
    ModC_ResultName MODC_LAST_RESULT; (void)MODC_LAST_RESULT

#define INTERN_MODC_RESULT_TRY(ModC_ResultName, expr, failedAction, counter) \
    MPT_CONCAT(ModC_ResultName, _ValueOrDefault)(expr); \
    if(ModC_GlobalError) \
    { \
        MODC_LAST_RESULT =    (ModC_ResultName) \
                                { \
                                    .HasError = true, \
                                    .ValueOrError.Error = ModC_GlobalError \
                                }; \
        ModC_GlobalError = NULL; \
        MODC_ERROR_APPEND_TRACE(MODC_LAST_RESULT.ValueOrError.Error); \
        MPT_REMOVE_PARENTHESIS(failedAction); \
    }


#define MODC_RESULT_TRY(ModC_ResultName, expr, failedAction) \
    INTERN_MODC_RESULT_TRY(ModC_ResultName, expr, failedAction, __COUNTER__)


#define MODC_ERROR_MSG_EC_ALLOC(ModC_ResultName, msg, allocator, errorCode) \
    MPT_CONCAT(ModC_ResultName, _InternCreateErrorMsgEc)(   (msg), \
                                                            __FILE__, \
                                                            __func__, \
                                                            __LINE__, \
                                                            (errorCode), \
                                                            (allocator))

#define MODC_ERROR_MSG_ALLOC(ModC_ResultName, msg, allocator) \
    MODC_ERROR_MSG_EC_ALLOC(ModC_ResultName, msg, allocator, 0)

#define MODC_ERROR_MSG(ModC_ResultName, msg) \
    MODC_ERROR_MSG_EC_ALLOC(ModC_ResultName, msg, MODC_PERFORM_ALLOC(), 0)

#define MODC_ERROR_MSG_EC(ModC_ResultName, msg, errorCode) \
    MODC_ERROR_MSG_EC_ALLOC(ModC_ResultName, msg, MODC_PERFORM_ALLOC(), errorCode)


#define MODC_RESULT_VALUE(ModC_ResultName, val) \
    ((ModC_ResultName) \
    { \
        .HasError = false, \
        .ValueOrError.Value = val \
    })

#define MODC_RESULT_FREE_RESOURCE(ModC_ResultName, resultPtr) \
    MPT_CONCAT(ModC_ResultName, _Free)(resultPtr)

#define MODC_RESULT_TO_STRING_ALLOC(ModC_ResultName, resultVal, allocator) \
    MPT_CONCAT(ModC_ResultName, _ToString)(resultVal, allocator)

#define MODC_RESULT_TO_STRING(ModC_ResultName, resultVal) \
    MPT_CONCAT(ModC_ResultName, _ToString)(resultVal, MODC_PERFORM_ALLOC())

#define INTERNAL_MODC_ASSERT_EC_ALLOC(expr, errorCode, failedAction, ModC_ResultName, allocator) \
    do \
    { \
        if(!(expr)) \
        { \
            ModC_StringOrConstView assertView = \
                ModC_StringOrConstView_View(ModC_FromConstCStr("Expression \"" #expr "\" has failed.")); \
            MODC_LAST_RESULT = MODC_ERROR_MSG_EC_ALLOC(   ModC_ResultName,  \
                                                            assertView, \
                                                            allocator, \
                                                            errorCode); \
            MPT_REMOVE_PARENTHESIS(failedAction); \
        } \
    } \
    while(false)

#define MODC_ASSERT_ALLOC(expr, failedAction, ModC_ResultName, allocator) \
    INTERNAL_MODC_ASSERT_EC_ALLOC(expr, 0, failedAction, ModC_ResultName, allocator)

#define MODC_ASSERT(expr, failedAction, ModC_ResultName) \
    INTERNAL_MODC_ASSERT_EC_ALLOC(expr, 0, failedAction, ModC_ResultName, (MODC_PERFORM_ALLOC()))

#define MODC_ASSERT_EC_ALLOC(expr, errorCode, failedAction, ModC_ResultName, allocator) \
    INTERNAL_MODC_ASSERT_EC_ALLOC(expr, errorCode, failedAction, ModC_ResultName, allocator)

#define MODC_ASSERT_EC(expr, errorCode, failedAction, ModC_ResultName) \
    INTERNAL_MODC_ASSERT_EC_ALLOC(expr, errorCode, failedAction, ModC_ResultName, (MODC_PERFORM_ALLOC()))

#endif
