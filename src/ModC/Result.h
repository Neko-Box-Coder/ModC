#ifndef MODC_RESULT_H
#define MODC_RESULT_H

/* Docs

#### Definitions

To define a result type:
`DEFINE_RESULT_STRUCT(ResultName, valueType)`
Equivalent to
```c
typedef struct ResultName
{
    bool HasError;
    union
    {
        valueType Value;
        Error* Error;
    } ValueOrError;
} ResultName;
...
```

Define `DEFAULT_ALLOC()` to use non `_ALLOC` macro variants
Define `ResultNameState` to use `_S` macro variants


#### Functions:

`static inline ModC_ConstStringView ModC_GetFileName(const ModC_ConstStringView path);`

```c
typedef struct Trace
{
    ModC_StringView File;
    ModC_StringView Function;
    int32_t Line;
} Trace;
static inline void TraceCreate(const char* file, const char* function, int line, Trace* outTrace);
```

```c
typedef struct Error
{
    Trace Traces[MAX_TRACES];
    uint8_t TracesSize;
    ModC_String ErrorMsg;
    int32_t ErrorCode;
} Error;
```

Macro:
`void ERROR_APPEND_TRACE(Error* ErrorPtr);`

Macro:
    You can use `LAST_ERROR` which points to the error used to return.
    You can use `LAST_ERROR_MSG` which points to the error message used to return.
    Use `RET_ERROR()` or `RET_ERROR_S()` inside `failedAction` to return the error.
    
    NOTE:   Check is done after the the returned pointer. 
            Therefore avoid dereference on types that can have invalid values, such as bool.
`valueType* RESULT_TRY(result, failedAction);`


Macro:
    `allocated` is `true` if `msg` can be freed with `RESULT_FREE_RESOURCE`
```c
ResultName ERROR_MSG_EC_ALLOC(  ResultName,
                                ModC_StringUnion msg,
                                Allocator allocator,
                                int32_t errorCode);
ResultName ERROR_MSG_EC(ResultName, ModC_StringUnion msg,int32_t errorCode);

ResultName ERROR_MSG_EC_ALLOC_S(ModC_StringUnion msg,
                                Allocator allocator,
                                int32_t errorCode);
ResultName ERROR_MSG_EC_S(ModC_StringUnion msg, int32_t errorCode);
```

Macro:
```c
ResultName ERROR_MSG_ALLOC(ResultName, ModC_StringUnion msg, Allocator allocator);
ResultName ERROR_MSG(ResultName, ModC_StringUnion msg);

ResultName ERROR_MSG_ALLOC_S(ModC_StringUnion msg, Allocator allocator);
ResultName ERROR_MSG_S(ModC_StringUnion msg);
```

Macro:
```c
ResultName RESULT_ERROR(ResultName, Error* error);
ResultName RESULT_ERROR_S(Error* error);
```


Macro:
```c
ResultName ERROR_STR_FMT(ResultName, (format, ...));
ResultName ERROR_STR_FMT(ResultName, (format, ...), int32_t errorCode);

ResultName ERROR_STR_FMT_ALLOC(ResultName, Allocator allocator, (format, ...));
ResultName ERROR_STR_FMT_ALLOC(ResultName, Allocator allocator, (format, ...), int32_t errorCode);

ResultName ERROR_CSTR(ResultName, cstr);
ResultName ERROR_CSTR(ResultName, cstr, int32_t errorCode);

ResultName ERROR_CSTR_ALLOC(ResultName, Allocator allocator, cstr);
ResultName ERROR_CSTR_ALLOC(ResultName, Allocator allocator, cstr, int32_t errorCode);

ResultName ERROR_STR_FMT_S((format, ...));
ResultName ERROR_STR_FMT_S((format, ...), int32_t errorCode);

ResultName ERROR_STR_FMT_ALLOC_S(Allocator allocator, (format, ...));
ResultName ERROR_STR_FMT_ALLOC_S(Allocator allocator, (format, ...), int32_t errorCode);

ResultName ERROR_CSTR_S(cstr);
ResultName ERROR_CSTR_S(cstr, int32_t errorCode);

ResultName ERROR_CSTR_ALLOC_S(Allocator allocator, cstr);
ResultName ERROR_CSTR_ALLOC_S(Allocator allocator, cstr, int32_t errorCode);

```

Macro:
`ResultName RESULT_VALUE(ResultName, valueType val);`
`ResultName RESULT_VALUE_S(valueType val);`

Macro:
`void RESULT_FREE_RESOURCE(ResultName, ResultName* result);`
`void RESULT_FREE_RESOURCE_S(ResultName* result);`

Macro:
```c
ModC_String RESULT_TO_STRING_ALLOC(ResultName, 
                                        ResultName resultVal, 
                                        Allocator allocator);
ModC_String RESULT_TO_STRING(ResultName, ResultName resultVal);

ModC_String RESULT_TO_STRING_ALLOC_S(  ResultName resultVal, 
                                            Allocator allocator);
ModC_String RESULT_TO_STRING_S(ResultName resultVal);
```


Macros:
    `ResultNameState` needs to be defined first
    You can use `LAST_ERROR` which points to the error used to return.
    You can use `LAST_ERROR_MSG` which points to the error message used to return.
    You can specify the error code with the `_EC` macro variants.
    Use `RET_ERROR()` or `RET_ERROR_S()` inside `failedAction` to return the error.
```c
CHECK_ALLOC(expr, (formatAppend), allocator, failedAction);
CHECK(expr, (formatAppend), failedAction);

CHECK_EC_ALLOC(expr, errorCode, (formatAppend), allocator, failedAction);
CHECK_EC(expr, errorCode, (formatAppend), failedAction);
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

typedef struct Trace
{
    ModC_StringView File;
    ModC_StringView Function;
    int32_t Line;
} Trace;

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

static inline void TraceCreate(const char* file, const char* function, int line, Trace* outTrace)
{
    if(!outTrace)
        return;
    
    ModC_ConstStringView constFileView = CHAIN( ModC_ConstStringView_Create, (file, strlen(file)),
                                                ModC_GetFileName, ());
    ModC_ConstStringView constFuncView = ModC_ConstStringView_Create(function, strlen(function));
    *outTrace = (Trace)
                {
                    .File = ModC_ConstStringView_Unconst(&constFileView),
                    .Function = ModC_ConstStringView_Unconst(&constFuncView),
                    .Line = line
                };
}


#define MAX_TRACES 64

typedef struct Error
{
    Trace Traces[MAX_TRACES];
    Allocator Allocator;
    ModC_String ErrorMsg;
    uint8_t TracesSize;
    int32_t ErrorCode;
} Error;

static Error* GlobalError = NULL;
static Error* GlobalRetError = NULL;

#define DEFINE_RESULT_STRUCT(ModC_DefResultName, valueType) \
    typedef struct ModC_DefResultName \
    { \
        bool HasError; \
        union \
        { \
            valueType Value; \
            Error* Error; \
        } ValueOrError; \
    } ModC_DefResultName; \
    \
    static inline ModC_DefResultName MPT_CONCAT(ModC_DefResultName, _CreateValue)(valueType val) \
    { \
        return (ModC_DefResultName) { .HasError = false, .ValueOrError.Value = val }; \
    } \
    \
    static inline ModC_DefResultName MPT_CONCAT(ModC_DefResultName, _CreateError)(Error* err) \
    { \
        return (ModC_DefResultName) { .HasError = true, .ValueOrError.Error = err }; \
    } \
    \
    static inline valueType \
    MPT_CONCAT(ModC_DefResultName, _ValueOrDefault)(const ModC_DefResultName result) \
    { \
        if(result.HasError) \
            GlobalError = result.ValueOrError.Error; \
        else \
            return result.ValueOrError.Value; \
        return (valueType){0}; \
    } \
    \
    static inline ModC_String \
    MPT_CONCAT(ModC_DefResultName, _ToString)( const ModC_DefResultName result, \
                                            Allocator allocator) \
    { \
        if(!result.HasError) \
            return (ModC_String){0}; \
        \
        Error* errorPtr = result.ValueOrError.Error; \
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
    
static inline Error* 
ModC_Error_InternCreateErrorMsgEc(  ModC_StringUnion msg,
                                    const char* file,
                                    const char* func,
                                    int32_t line,
                                    int32_t errorCode,
                                    Allocator allocator)
{
    Error* modcErrorPtr = Allocator_Malloc(&allocator, sizeof(Error));
    if(modcErrorPtr)
    {
        *modcErrorPtr = (Error){0};
        modcErrorPtr->Allocator = allocator;
        TraceCreate(file, func, line, &modcErrorPtr->Traces[modcErrorPtr->TracesSize++]);
        
        //Error.ErrorMsg
        {
            if(msg.Type == TU_TYPE(ModC_StringUnion, ModC_String))
                modcErrorPtr->ErrorMsg = msg.TU_DATA(ModC_StringUnion, ModC_String);
            else
            {
                modcErrorPtr->ErrorMsg = ModC_String_Create(allocator, 256);
                ModC_ConstStringView* msgView = &msg.TU_DATA(ModC_StringUnion, ModC_ConstStringView);
                ModC_String_AddRange(&modcErrorPtr->ErrorMsg, msgView->Data, msgView->Length);
            }
        }
        modcErrorPtr->ErrorCode = errorCode;
    }
    //TODO: Add debug_break()
    return modcErrorPtr;
}

#define ERROR_APPEND_TRACE(ErrorPtr) \
    do \
    { \
        if((ErrorPtr) && ErrorPtr->TracesSize < MAX_TRACES) \
            TraceCreate(__FILE__, __func__, __LINE__, &ErrorPtr->Traces[ErrorPtr->TracesSize++]); \
    } while(false)

//Failed reuslt name
#define LAST_ERROR GlobalError
#define LAST_ERROR_MSG GlobalError->ErrorMsg
#define RET_ERROR(ResultName) \
    do \
    { \
        GlobalRetError = GlobalError; \
        GlobalError = NULL; \
        return MPT_DELAYED_CONCAT(ResultName, _CreateError) (GlobalRetError); \
    } while(0)

#define RET_ERROR_S() RET_ERROR(ResultNameState)

#if 0
    #define INTERN_RESULT_TRY(ModC_ExprResultName, expr, failedAction, counter) \
        MPT_DELAYED_CONCAT(ModC_ExprResultName, _ValueOrDefault)(expr); \
        do \
        { \
            if(GlobalError) \
            { \
                ERROR_APPEND_TRACE(GlobalError); \
                MPT_REMOVE_PARENTHESIS(failedAction); \
            } \
        } while(0)


    #define RESULT_TRY(ModC_ExprResultName, expr, failedAction) \
        INTERN_RESULT_TRY(ModC_ExprResultName, expr, failedAction, __COUNTER__)
#else
    #define RESULT_TRY(result, ... /* failedAction */) \
        &result.ValueOrError.Value; \
        do \
        { \
            if(result.HasError) \
            { \
                GlobalError = result.ValueOrError.Error; \
                ERROR_APPEND_TRACE(result.ValueOrError.Error); \
                __VA_ARGS__; \
            } \
        } while(0)
#endif


#define ERROR_MSG_EC_ALLOC(ResultName, msg, allocator, errorCode) \
    MPT_DELAYED_CONCAT2(ResultName, _CreateError) \
    ( \
        ModC_Error_InternCreateErrorMsgEc(  (msg), \
                                            __FILE__, \
                                            __func__, \
                                            __LINE__, \
                                            (errorCode), \
                                            (allocator)) \
    )

#define ERROR_MSG_EC_ALLOC_S(msg, allocator, errorCode) \
    ERROR_MSG_EC_ALLOC(ResultNameState, msg, allocator, errorCode)

#define ERROR_MSG_ALLOC(ResultName, msg, allocator) \
    ERROR_MSG_EC_ALLOC(ResultName, msg, allocator, 0)

#define ERROR_MSG_ALLOC_S(msg, allocator) ERROR_MSG_ALLOC(ResultNameState, msg, allocator)

#define ERROR_MSG(ResultName, msg) \
    ERROR_MSG_EC_ALLOC(ResultName, msg, DEFAULT_ALLOC(), 0)

#define ERROR_MSG_S(msg) ERROR_MSG(ResultNameState, msg)

#define ERROR_MSG_EC(ResultName, msg, errorCode) \
    ERROR_MSG_EC_ALLOC(ResultName, msg, DEFAULT_ALLOC(), errorCode)

#define ERROR_MSG_EC_S(msg, errorCode) ERROR_MSG_EC(ResultNameState, msg, errorCode)

#define RESULT_ERROR(ResultName, errorPtr) \
    MPT_DELAYED_CONCAT(ResultName, _CreateError)(errorPtr) \

#define RESULT_ERROR_S(errorPtr) RESULT_ERROR(ResultNameState, errorPtr)

#define INTERN_MODC_STR_VIEW_FROM_STR_FMT(allocator, strfmts) \
    TU_INIT(ModC_StringUnion, \
            ModC_String, \
            ModC_String_FromFormat(allocator, MPT_REMOVE_PARENTHESIS(strfmts)))

#define INTERN_ERROR_STR_FMT_ALLOC_4(ResultName, allocator, strfmts, errorCode) \
    MPT_DELAYED_CONCAT2(ResultName, _CreateError) \
    ( \
        ModC_Error_InternCreateErrorMsgEc(  INTERN_MODC_STR_VIEW_FROM_STR_FMT(allocator, strfmts), \
                                            __FILE__, \
                                            __func__, \
                                            __LINE__, \
                                            (errorCode), \
                                            (allocator)) \
    )

#define INTERN_ERROR_STR_FMT_ALLOC_3(ResultName, allocator, strfmts) \
    INTERN_ERROR_STR_FMT_ALLOC_4(ResultName, allocator, strfmts, 0)

//ResultName ERROR_STR_FMT_ALLOC(ResultName, Allocator allocator, (format, ...));
//ResultName ERROR_STR_FMT_ALLOC(ResultName, Allocator allocator, (format, ...), int32_t errorCode);
#define ERROR_STR_FMT_ALLOC(...) \
    MPT_OVERLOAD_MACRO(INTERN_ERROR_STR_FMT_ALLOC, __VA_ARGS__)

#define ERROR_STR_FMT_ALLOC_S(...) ERROR_STR_FMT_ALLOC(ResultNameState, __VA_ARGS__)

//ResultName ERROR_STR_FMT(ResultName, (format, ...));
//ResultName ERROR_STR_FMT(ResultName, (format, ...), int32_t errorCode);
#define ERROR_STR_FMT(ResultName, ...) \
    ERROR_STR_FMT_ALLOC(ResultName, DEFAULT_ALLOC(), __VA_ARGS__)

#define ERROR_STR_FMT_S(...) ERROR_STR_FMT(ResultNameState, __VA_ARGS__)

#define INTERN_ERROR_CSTR_ALLOC_4(ResultName, allocator, cstr, errorCode) \
    MPT_DELAYED_CONCAT2(ResultName, _CreateError) \
    ( \
        ModC_Error_InternCreateErrorMsgEc(  ModC_StringUnion_ViewFromLiteral(cstr), \
                                            __FILE__, \
                                            __func__, \
                                            __LINE__, \
                                            (errorCode), \
                                            (allocator)) \
    )

#define INTERN_ERROR_CSTR_ALLOC_3(ResultName, allocator, cstr) \
    INTERN_ERROR_CSTR_ALLOC_4(ResultName, allocator, cstr, 0)

//ResultName ERROR_CSTR_ALLOC(ResultName, Allocator allocator, cstr);
//ResultName ERROR_CSTR_ALLOC(ResultName, Allocator allocator, cstr, int32_t errorCode);
#define ERROR_CSTR_ALLOC(...) MPT_OVERLOAD_MACRO(INTERN_ERROR_CSTR_ALLOC, __VA_ARGS__)

#define ERROR_CSTR_ALLOC_S(...) ERROR_CSTR_ALLOC(ResultNameState, __VA_ARGS__)

//ResultName ERROR_CSTR(ResultName, cstr);
//ResultName ERROR_CSTR(ResultName, cstr, int32_t errorCode);
#define ERROR_CSTR(ResultName, ...) \
    ERROR_CSTR_ALLOC(ResultName, DEFAULT_ALLOC(), __VA_ARGS__)

#define ERROR_CSTR_S(...) ERROR_CSTR(ResultNameState, __VA_ARGS__)


#define RESULT_VALUE(ResultName, ... /* value */) \
    MPT_DELAYED_CONCAT(ResultName, _CreateValue)(__VA_ARGS__)

#define RESULT_VALUE_S(... /* value */) RESULT_VALUE(ResultNameState, __VA_ARGS__)

#define RESULT_FREE_RESOURCE(ResultName, resultPtr) \
    do \
    { \
        if((resultPtr) == NULL) \
            break; \
         \
        if(!(resultPtr)->HasError) \
        { \
            *(resultPtr) = (ResultName){0}; \
            break; \
        } \
        ModC_String_Free(&(resultPtr)->ValueOrError.Error->ErrorMsg); \
        Allocator errorAllocator = (resultPtr)->ValueOrError.Error->Allocator; \
        Allocator_Free(&errorAllocator, (resultPtr)->ValueOrError.Error); \
        Allocator_Destroy(&errorAllocator); \
        *(resultPtr) = (ResultName){0}; \
        break; \
    } while(0)

#define RESULT_FREE_RESOURCE_S(resultPtr) RESULT_FREE_RESOURCE(ResultNameState, resultPtr)


#define RESULT_TO_STRING_ALLOC(ResultName, resultVal, allocator) \
    MPT_DELAYED_CONCAT(ResultName, _ToString)(resultVal, allocator)

#define RESULT_TO_STRING_ALLOC_S(resultVal, allocator) \
    RESULT_TO_STRING_ALLOC(ResultNameState, resultVal, allocator)

#define RESULT_TO_STRING(ResultName, resultVal) \
    MPT_DELAYED_CONCAT(ResultName, _ToString)(resultVal, DEFAULT_ALLOC())

#define RESULT_TO_STRING_S(resultVal) RESULT_TO_STRING(ResultNameState, resultVal)

#define INTERNAL_CHECK_EC_ALLOC(expr, errorCode, formatAppend, allocator, ... /* failedAction */) \
    do \
    { \
        if(!(expr)) \
        { \
            ModC_ConstStringView exprView = \
                ModC_ConstStringView_FromLiteral("Expression \"" #expr "\" has failed. "); \
            ModC_StringUnion assertView = TU_INIT(ModC_StringUnion, ModC_ConstStringView, exprView); \
            GlobalError = ModC_Error_InternCreateErrorMsgEc(assertView, \
                                                            __FILE__, \
                                                            __func__, \
                                                            __LINE__, \
                                                            (errorCode), \
                                                            (allocator)); \
            ModC_String_AppendFormat(   &GlobalError->ErrorMsg, \
                                        MPT_REMOVE_PARENTHESIS(formatAppend)); \
            __VA_ARGS__; \
        } \
    } \
    while(false)

#define CHECK(expr, formatAppend, ... /* failedAction */) \
    INTERNAL_CHECK_EC_ALLOC(expr, 0, formatAppend, (DEFAULT_ALLOC()), __VA_ARGS__)

#define CHECK_ALLOC(expr, formatAppend, allocator, ... /* failedAction */) \
    INTERNAL_CHECK_EC_ALLOC(expr, 0, formatAppend, allocator, __VA_ARGS__)

#define CHECK_EC_ALLOC(expr, errorCode, formatAppend, allocator, ... /* failedAction */) \
    INTERNAL_CHECK_EC_ALLOC(expr, errorCode, formatAppend, allocator, __VA_ARGS__)

#define CHECK_EC(expr, errorCode, formatAppend, ... /* failedAction */) \
    INTERNAL_CHECK_EC_ALLOC(expr, errorCode, formatAppend, (DEFAULT_ALLOC()), __VA_ARGS__)

#endif
