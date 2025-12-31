#ifndef MODC_RESULT_H
#define MODC_RESULT_H

/* Docs

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
You can use `MODC_FAILED_RESULT` to get the result that contains the error in `failedAction`
`valueType MODC_RESULT_UNWRAP(ModC_ResultName, expr, failedAction);`

Macro:
`allocated` is `true` if `msg` can be freed with `MODC_RESULT_FREE`
```c
ModC_ResultName MODC_ERROR_MSG_EC(  ModC_ResultName, 
                                    const char* msg, 
                                    bool allocated, 
                                    int32_t errorCode);
```

Macro:
`ModC_ResultName MODC_ERROR_MSG(ModC_ResultName, const char* msg, bool allocated);`

Macro:
`ModC_ResultName MODC_RESULT_VALUE(ModC_ResultName, valueType val);`

Macro:
`void MODC_RESULT_FREE_RESOURCE(ModC_ResultName, ModC_ResultName* result);`
*/


#include "ModC/Strings/Strings.h"
#include "ModC/Defer.h"
#include "ModC/Allocator.h"
#include "ModC/ChainUtil.h"
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
    
    return ModC_ConstStringView_ConstSubview(path, lastSlash, -1);
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

typedef struct
{
    ModC_Trace Traces[MODC_MAX_TRACES];
    uint8_t TracesSize;
    ModC_String ErrorMsg;
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
    MPT_CONCAT(ModC_ResultName, _InternCreateErrorMsgEc)(   const char* msgPtr, \
                                                            bool allocated, \
                                                            const char* file, \
                                                            const char* func, \
                                                            int32_t line, \
                                                            int32_t errorCode, \
                                                            const ModC_Allocator allocator) \
    { \
        ModC_Error* modcErrorPtr = ModC_Allocator_Malloc(allocator, sizeof(ModC_Error)); \
        if(modcErrorPtr) \
        { \
            *modcErrorPtr = (ModC_Error){ .TracesSize = 0 }; \
            /* ModC_Error.Traces and ModC_Error.TracesSize */ \
            ModC_Trace_Create(file, func, line, &modcErrorPtr->Traces[modcErrorPtr->TracesSize++]); \
            \
            /* ModC_Error.ErrorMsg */ \
            { \
                if(allocated) \
                    modcErrorPtr->ErrorMsg = ModC_String_FromCStr(allocator, (char*)msgPtr); \
                else \
                { \
                    modcErrorPtr->ErrorMsg = ModC_String_Create(allocator, 256); \
                    ModC_String_Append( &modcErrorPtr->ErrorMsg, \
                                        ModC_ConstStringView_FromConstCStr(msgPtr)); \
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
        ModC_String outputString; \
        ModC_String tempString; \
        MODC_DEFER_SCOPE_START \
        { \
            ModC_Error* errorPtr = result.ValueOrError.Error; \
            outputString = ModC_String_Create(allocator, 512); \
            \
            /* TODO: snprintf the whole thing? */ \
            ModC_String_Append(&outputString, ModC_ConstStringView_FromConstCStr("Error:\n  ")); \
            ModC_String_Append(&outputString, ModC_String_ConstView(errorPtr->ErrorMsg)); \
            \
            tempString = ModC_String_Create(allocator, 256); \
            MODC_DEFER(ModC_String_Free(&tempString)); \
            \
            if(errorPtr->ErrorCode != 0) \
            { \
                ModC_String_Append( &outputString,  \
                                    ModC_ConstStringView_FromConstCStr("\nError Code: ")); \
                { \
                    int writeLen = snprintf(NULL, 0, "%d", errorPtr->ErrorCode); \
                    if(writeLen >= 0) \
                    { \
                        ModC_String_Resize(&tempString, writeLen); \
                        snprintf(tempString.Data, writeLen + 1, "%d", errorPtr->ErrorCode); \
                        ModC_String_Append(&outputString, ModC_String_ConstView(tempString)); \
                        ModC_String_Resize(&tempString, 0); \
                    } \
                } \
            } \
            \
            ModC_String_Append( &outputString,  \
                                ModC_ConstStringView_FromConstCStr("\n\nStack trace:")); \
            \
            for(int i = 0; i < errorPtr->TracesSize; ++i) \
            { \
                /* TODO: snprintf? */ \
                ModC_String_Append( &outputString,  \
                                    ModC_ConstStringView_FromConstCStr("\n  at ")); \
                /* Trace */ \
                { \
                    ModC_String_Append( &outputString,  \
                                        ModC_StringView_ConstView(errorPtr->Traces[i].File)); \
                    ModC_String_Append(&outputString, ModC_ConstStringView_FromConstCStr(":")); \
                    { \
                        int writeLen = snprintf(NULL, 0, "%d", errorPtr->Traces[i].Line); \
                        if(writeLen >= 0) \
                        { \
                            ModC_String_Resize(&tempString, writeLen); \
                            snprintf(tempString.Data, writeLen + 1, "%d", errorPtr->Traces[i].Line); \
                            ModC_String_Append(&outputString, ModC_String_ConstView(tempString)); \
                            ModC_String_Resize(&tempString, 0); \
                        } \
                    } \
                    ModC_String_Append(&outputString, ModC_ConstStringView_FromConstCStr(" in ")); \
                    ModC_String_Append( &outputString,  \
                                        ModC_StringView_ConstView(errorPtr->Traces[i].Function)); \
                    ModC_String_Append(&outputString, ModC_ConstStringView_FromConstCStr("()")); \
                } \
            } \
        } \
        MODC_DEFER_SCOPE_END \
        return outputString; \
    }
    
#define MODC_ERROR_APPEND_TRACE(ModC_ErrorPtr) \
    do \
    { \
        if(ModC_ErrorPtr && ModC_ErrorPtr->TracesSize < MODC_MAX_TRACES) \
        { \
            ModC_Trace_Create(  __FILE__, \
                                __func__, \
                                __LINE__, \
                                &ModC_ErrorPtr->Traces[ModC_ErrorPtr->TracesSize++]); \
        } \
    } while(false)

//Failed reuslt name
#define MODC_FAILED_RESULT tempResult

#define INTERN_MODC_RESULT_UNWRAP(ModC_ResultName, expr, failedAction, counter) \
    MPT_CONCAT(ModC_ResultName, _ValueOrDefault)(expr); \
    if(ModC_GlobalError) \
    { \
        ModC_ResultName MODC_FAILED_RESULT =    (ModC_ResultName) \
                                                { \
                                                    .HasError = true, \
                                                    .ValueOrError.Error = ModC_GlobalError \
                                                }; \
        ModC_GlobalError = NULL; \
        MODC_ERROR_APPEND_TRACE(MODC_FAILED_RESULT.ValueOrError.Error); \
        failedAction; \
    }


#define MODC_RESULT_UNWRAP(ModC_ResultName, expr, failedAction) \
    INTERN_MODC_RESULT_UNWRAP(ModC_ResultName, expr, failedAction, __COUNTER__)


#define MODC_ERROR_MSG_EC(ModC_ResultName, msg, allocated, allocator, errorCode) \
    MPT_CONCAT(ModC_ResultName, _InternCreateErrorMsgEc)(   msg, \
                                                            allocated, \
                                                            __FILE__, \
                                                            __func__, \
                                                            __LINE__, \
                                                            errorCode, \
                                                            allocator)

#define MODC_ERROR_MSG(ModC_ResultName, msg, allocated, allocator) \
    MODC_ERROR_MSG_EC(ModC_ResultName, msg, allocated, allocator, 0)

#define MODC_RESULT_VALUE(ModC_ResultName, val) \
    ((ModC_ResultName) \
    { \
        .HasError = false, \
        .ValueOrError.Value = val \
    })

#define MODC_RESULT_FREE_RESOURCE(ModC_ResultName, resultPtr) \
    MPT_CONCAT(ModC_ResultName, _Free)(resultPtr)



#endif
