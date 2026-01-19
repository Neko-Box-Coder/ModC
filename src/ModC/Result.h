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

Define `MODC_PERFORM_ALLOC()` to use non `_ALLOC` macro variants


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
    `ModC_ResultName` needs to be defined first
    You can use `MODC_LAST_ERROR` which points to the error used to return.
    You can use `MODC_LAST_ERROR_MSG` which points to the error message used to return.
    Use `MODC_RET_ERROR()` inside `failedAction` to return the error.
`valueType MODC_RESULT_TRY(ModC_ExprResultName, expr, failedAction);`


Macro:
    `allocated` is `true` if `msg` can be freed with `MODC_RESULT_FREE`
    `ModC_ResultName` needs to be defined first
```c
ModC_ResultName MODC_ERROR_MSG_EC_ALLOC(ModC_StringOrConstView msg,
                                        ModC_Allocator allocator,
                                        int32_t errorCode);

ModC_ResultName MODC_ERROR_MSG_EC(ModC_StringOrConstView msg,int32_t errorCode);
```

Macro:
    `ModC_ResultName` needs to be defined first
```c
ModC_ResultName MODC_ERROR_MSG_ALLOC(ModC_StringOrConstView msg, ModC_Allocator allocator);

ModC_ResultName MODC_ERROR_MSG(ModC_StringOrConstView msg);
```

Macro:
    `ModC_ResultName` needs to be defined first
`ModC_ResultName MODC_RESULT_VALUE(valueType val);`

Macro:
`void MODC_RESULT_FREE_RESOURCE(ModC_ResultName* result);`

Macro:
```c
ModC_String MODC_RESULT_TO_STRING_ALLOC(ModC_ResultName, 
                                        ModC_ResultName resultVal, 
                                        ModC_Allocator allocator);
ModC_String MODC_RESULT_TO_STRING(ModC_ResultName, ModC_ResultName resultVal);
```

Macros:
    `ModC_ResultName` needs to be defined first
    You can use `MODC_LAST_ERROR` which points to the error used to return.
    You can use `MODC_LAST_ERROR_MSG` which points to the error message used to return.
    You can specify the error code with the `_EC` macro variants.
    Use `MODC_RET_ERROR()` inside `failedAction` to return the error.
```c
MODC_ASSERT_ALLOC(expr, failedAction, (formatAppend), allocator);
MODC_ASSERT(expr, (formatAppend), failedAction);

MODC_ASSERT_EC_ALLOC(expr, errorCode, (formatAppend), failedAction, allocator);
MODC_ASSERT_EC(expr, errorCode, (formatAppend), failedAction);
```
*/


#include "ModC/Strings/Strings.h"
#include "ModC/Allocator.h"
#include "ModC/ChainUtil.h"

#include "MacroPowerToys/Miscellaneous.h"
#include "MacroPowerToys/RemoveParenthesisInList.h"

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
        ModC_String_AppendCStr(&outputString, "\n\nStack trace:"); \
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
ModC_Error_InternCreateErrorMsgEc(  ModC_StringOrConstView msg,
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
            if(msg.IsString)
                modcErrorPtr->ErrorMsg = msg.Value.String;
            else
            {
                modcErrorPtr->ErrorMsg = ModC_String_Create(allocator, 256);
                ModC_String_AddRange(   &modcErrorPtr->ErrorMsg,
                                        msg.Value.View.Data,
                                        msg.Value.View.Length);
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


#define MODC_ERROR_MSG_EC_ALLOC(msg, allocator, errorCode) \
    MPT_DELAYED_CONCAT2(ModC_ResultName, _CreateError) \
    ( \
        ModC_Error_InternCreateErrorMsgEc(  (msg), \
        __FILE__, \
        __func__, \
        __LINE__, \
        (errorCode), \
        (allocator)) \
    )

#define MODC_ERROR_MSG_ALLOC(msg, allocator) \
    MODC_ERROR_MSG_EC_ALLOC(msg, allocator, 0)

#define MODC_ERROR_MSG(msg) \
    MODC_ERROR_MSG_EC_ALLOC(msg, MODC_PERFORM_ALLOC(), 0)

#define MODC_ERROR_MSG_EC(msg, errorCode) \
    MODC_ERROR_MSG_EC_ALLOC(msg, MODC_PERFORM_ALLOC(), errorCode)


#define MODC_RESULT_VALUE(val) MPT_DELAYED_CONCAT(ModC_ResultName, _CreateValue)(val)

#define MODC_RESULT_FREE_RESOURCE(resultPtr) \
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

#define MODC_RESULT_TO_STRING_ALLOC(resultVal, allocator) \
    MPT_DELAYED_CONCAT(ModC_ResultName, _ToString)(resultVal, allocator)

#define MODC_RESULT_TO_STRING(resultVal) \
    MPT_DELAYED_CONCAT(ModC_ResultName, _ToString)(resultVal, MODC_PERFORM_ALLOC())

#define INTERNAL_MODC_ASSERT_EC_ALLOC(expr, errorCode, formatAppend, failedAction, allocator) \
    do \
    { \
        if(!(expr)) \
        { \
            ModC_ConstStringView exprView = \
                ModC_ConstStringView_FromCStr("Expression \"" #expr "\" has failed."); \
            ModC_StringOrConstView assertView = ModC_StringOrConstView_View(exprView); \
            ModC_GlobalError = ModC_Error_InternCreateErrorMsgEc(  assertView, \
                                                                    __FILE__, \
                                                                    __func__, \
                                                                    __LINE__, \
                                                                    (errorCode), \
                                                                    (allocator)); \
            ModC_String_AppendFormat(   &ModC_GlobalError->ErrorMsg, \
                                        MPT_REMOVE_PARENTHESIS(formatAppend)); \
            MPT_REMOVE_PARENTHESIS(failedAction); \
        } \
    } \
    while(false)

#define MODC_ASSERT_ALLOC(expr, formatAppend, failedAction, allocator) \
    INTERNAL_MODC_ASSERT_EC_ALLOC(expr, 0, formatAppend, failedAction, allocator)

#define MODC_ASSERT(expr, formatAppend, failedAction) \
    INTERNAL_MODC_ASSERT_EC_ALLOC(expr, 0, formatAppend, failedAction, (MODC_PERFORM_ALLOC()))

#define MODC_ASSERT_EC_ALLOC(expr, errorCode, formatAppend, failedAction, allocator) \
    INTERNAL_MODC_ASSERT_EC_ALLOC(expr, errorCode, formatAppend, failedAction, allocator)

#define MODC_ASSERT_EC(expr, errorCode, formatAppend, failedAction) \
    INTERNAL_MODC_ASSERT_EC_ALLOC(expr, errorCode, formatAppend, failedAction, (MODC_PERFORM_ALLOC()))

#endif
