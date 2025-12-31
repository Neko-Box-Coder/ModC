#define ARENA_IMPLEMENTATION


#include "ModC/Allocator.h"
#include "ModC/Defer.h"
#include "ModC/Containers.h"

//Dependencies
#include "static_assert.h/assert.h"
#include "arena-allocator/arena.h"

//System includes
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>

//Sanity checks
static_assert(sizeof(int) == sizeof(int32_t), "");

#ifndef __COUNTER__
    #error "__COUNTER__ needs to be supported"
#endif


ModC_ResultInt32 TestResult()
{
    if(true)
    {
        ModC_ConstStringView retView = ModC_FromConstCStr("Failed to seek file");
        return MODC_ERROR_MSG(  ModC_ResultInt32, 
                                ((ModC_StringOrConstView) { .IsString = false, .Value.View = retView }),
                                ModC_CreateHeapAllocator());
    }
    else
        return MODC_RESULT_VALUE(ModC_ResultInt32, 5);
}

ModC_ResultInt32 TestResult2()
{
    MDOC_DECLARE_FAILED_RESULT(ModC_ResultInt32);
    int32_t unwrappedVal = 
        MODC_RESULT_UNWRAP(ModC_ResultInt32, TestResult(), return MODC_FAILED_RESULT);
    
    return MODC_RESULT_VALUE(ModC_ResultInt32, unwrappedVal);
}

ModC_ResultVoid Main(int argc, char* argv[])
{
    FILE* modcFile = NULL;
    Arena* arena = NULL;
    MDOC_DECLARE_FAILED_RESULT(ModC_ResultVoid);
    (void)arena;
    
    MODC_DEFER_SCOPE_START
    {
        if(argc == 1)
        {
            printf("Usage: %s <path>\n", argv[0]);
            MODC_RUN_DEFER_NOW_AND(return MODC_RESULT_VALUE(ModC_ResultVoid, {0}));
        }
        
        ModC_StringView filePath = { .Data = argv[1], .Length = strlen(argv[1]), .NullEnd = true };
        printf("Compiling %s\n", filePath.Data);
        
        modcFile = fopen(filePath.Data, "r");
        if(!modcFile)
        {
            ModC_String msgStr = ModC_String_Create(ModC_CreateHeapAllocator(), 64);
            ModC_String_AppendConstView(&msgStr, ModC_FromConstCStr("Failed to open file: "));
            ModC_String_AppendConstView(&msgStr, ModC_FromConstCStr(strerror(errno)));
            MODC_RUN_DEFER_NOW_AND(return MODC_ERROR_MSG(   ModC_ResultVoid, 
                                                            ((ModC_StringOrConstView)
                                                            {
                                                                .IsString = true,
                                                                .Value.String = msgStr
                                                            }),
                                                            ModC_CreateHeapAllocator()));
        }
        MODC_DEFER({ fclose(modcFile); modcFile = NULL; printf("fclosed\n"); });
        
        
        if(fseek(modcFile, 0, SEEK_END) != 0)
        {
            MODC_RUN_DEFER_NOW_AND
            (
                return MODC_ERROR_MSG(  ModC_ResultVoid, 
                                        ((ModC_StringOrConstView)
                                        {
                                            .IsString = false, 
                                            .Value.View = ModC_FromConstCStr("Failed to seek file")
                                        }),
                                        ModC_CreateHeapAllocator())
            );
        }
        
        int64_t fileSize = ftell(modcFile);
        fileSize = -1;
        MODC_ASSERT_GT_EQ(  fileSize, 
                            0, 
                            {
                                ModC_String* errorMsg = 
                                    &MODC_FAILED_RESULT.ValueOrError.Error->ErrorMsg;
                                ModC_String_AppendFormat(errorMsg, " fileSize: %"PRIi64".", fileSize);
                                MODC_RUN_DEFER_NOW_AND(return MODC_FAILED_RESULT);
                            },
                            ModC_ResultVoid, 
                            ModC_CreateHeapAllocator());
        (void)fileSize;
        //arena = arena_create()
        
        //fread()
        
        
        
        //if(true)
        //    MODC_DEFER_RETURN(0);
        
        MODC_DEFER(printf("should be run before fclosed\n"));
    }
    MODC_DEFER_SCOPE_END
    
    return MODC_RESULT_VALUE(ModC_ResultVoid, {0});
}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    #if 1
    {
        ModC_ResultVoid result = Main(argc, argv);
        if(result.HasError)
        {
            MODC_ERROR_APPEND_TRACE(result.ValueOrError.Error);
            printf("%s\n", ModC_ResultVoid_ToString(result, ModC_CreateHeapAllocator()).Data);
        }
        MODC_RESULT_FREE_RESOURCE(ModC_ResultVoid, &result);
    }
    #else
    {
        ModC_ResultInt32 result = TestResult2();
        if(result.HasError)
        {
            MODC_ERROR_APPEND_TRACE(result.ValueOrError.Error);
            printf("%s\n", ModC_ResultInt32_ToString(result, ModC_CreateHeapAllocator()).Data);
        }
        MODC_RESULT_FREE_RESOURCE(ModC_ResultInt32, &result);
    }
    #endif
    
    
    return 0;
}
