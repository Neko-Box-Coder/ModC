
#include "ModC/Allocator.h"
#include "ModC/Defer.h"
#include "ModC/Containers.h"


//Dependencies
#include "static_assert.h/assert.h"
#define ARENA_IMPLEMENTATION
#include "arena-allocator/arena.h"

//System includes
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

//Sanity checks
static_assert(sizeof(int) == sizeof(int32_t), "");

#ifndef __COUNTER__
    #error "__COUNTER__ needs to be supported"
#endif


ModC_ResultInt32 TestResult()
{
    if(true)
    {
        return MODC_ERROR_MSG(ModC_ResultInt32, "Test Failure", false, ModC_CreateHeapAllocator());
    }
    else
        return MODC_RESULT_VALUE(ModC_ResultInt32, 5);
}

ModC_ResultInt32 TestResult2()
{
    int32_t unwrappedVal = 
        MODC_RESULT_UNWRAP(ModC_ResultInt32, TestResult(), return MODC_FAILED_RESULT);
    
    return MODC_RESULT_VALUE(ModC_ResultInt32, unwrappedVal);
}

ModC_ResultVoid Main(int argc, char* argv[])
{
    FILE* modcFile = NULL;
    Arena* arena = NULL;
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
            printf("Failed to open file: %s", strerror(errno));
            MODC_RUN_DEFER_NOW_AND(return MODC_ERROR_MSG(   ModC_ResultVoid, 
                                                            "Failed to open file", 
                                                            false,
                                                            ModC_CreateHeapAllocator()));
        }
        MODC_DEFER({ fclose(modcFile); modcFile = NULL; printf("fclosed\n"); });
        
        
        if(fseek(modcFile, 0, SEEK_END) != 0)
        {
            printf("Failed to seek file");
            MODC_RUN_DEFER_NOW_AND(return MODC_ERROR_MSG(   ModC_ResultVoid, 
                                                            "Failed to seek file",
                                                            false,
                                                            ModC_CreateHeapAllocator()));
        }
        
        uint32_t fileSize = ftell(modcFile);
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
    #if 0
        ModC_ResultVoid result = Main(argc, argv);
        if(result.HasError)
            printf("%s\n", ModC_ResultVoid_ToString(result, ModC_CreateHeapAllocator()).Data);
    #else
        ModC_ResultInt32 result = TestResult2();
        if(result.HasError)
            printf("%s\n", ModC_ResultInt32_ToString(result, ModC_CreateHeapAllocator()).Data);
    #endif
    
    
    return 0;
}
