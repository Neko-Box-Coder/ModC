#define ARENA_IMPLEMENTATION
#define MODC_DEFAULT_ALLOC_FUNC ModC_CreateHeapAllocator
#define MODC_DEFAULT_ALLOC_ARGS


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


static inline ModC_ResultInt32 TestResult()
{
    if(true)
    {
        return MODC_ERROR_MSG(  ModC_ResultInt32, 
                                ModC_StringOrConstView_View(ModC_FromConstCStr("Failed to seek file")));
    }
    else
        return MODC_RESULT_VALUE(ModC_ResultInt32, 5);
}

static inline ModC_ResultInt32 TestResult2()
{
    MDOC_DECLARE_LAST_RESULT(ModC_ResultInt32);
    int32_t unwrappedVal = MODC_RESULT_TRY(ModC_ResultInt32, TestResult(), return MODC_LAST_RESULT);
    return MODC_RESULT_VALUE(ModC_ResultInt32, unwrappedVal);
}

static inline ModC_ResultVoid Tokenization(ModC_String fileContent)
{
    //TODO(NOW)
}

ModC_ResultVoid Main(int argc, char* argv[])
{
    FILE* modcFile = NULL;
    ModC_Allocator mainArena;
    ModC_String msgStr;
    ModC_String fileContent;
    MDOC_DECLARE_LAST_RESULT(ModC_ResultVoid);
    
    #define RUN_DEFER_AND_RET() MODC_RUN_DEFER_NOW_AND(return MODC_LAST_RESULT)
    
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
            msgStr = ModC_String_Create(ModC_CreateHeapAllocator(), 64);
            ModC_String_AppendFormat(&msgStr, "%s %s", "Failed to open file: ", strerror(errno));
            MODC_RUN_DEFER_NOW_AND(return MODC_ERROR_MSG(   ModC_ResultVoid, 
                                                            ModC_StringOrConstView_String(msgStr)));
        }
        MODC_DEFER({ fclose(modcFile); modcFile = NULL; });
        
        //Get file size
        int64_t fileSize;
        {
            int fseekResult = fseek(modcFile, 0, SEEK_END);
            MODC_ASSERT(fseekResult == 0, 
                        ModC_AppendFormat(&MODC_LAST_ERROR_MSG, " fseekResult: %i.", fseekResult);
                        RUN_DEFER_AND_RET(),
                        ModC_ResultVoid);
            fileSize = ftell(modcFile);
            MODC_ASSERT(fileSize >= 0,
                        ModC_AppendFormat(&MODC_LAST_ERROR_MSG, " fileSize: %"PRIi64".", fileSize);
                        RUN_DEFER_AND_RET(),
                        ModC_ResultVoid);
        }
        
        fseek(modcFile, 0, 0);
        mainArena = ModC_CreateOwnedArenaAllocator(fileSize + 8192);
        MODC_DEFER(ModC_Allocator_Destroy(&mainArena));
        
        fileContent = ModC_String_Create(   ModC_CreateSharedArenaAllocator(mainArena.Allocator), 
                                            fileSize);
        ModC_String_Resize(&fileContent, fileSize);
        MODC_ASSERT(fileContent.Length == fileSize, RUN_DEFER_AND_RET(), ModC_ResultVoid);
        
        uint32_t actuallyRead = fread(fileContent.Data, 1, fileSize, modcFile);
        MODC_ASSERT(actuallyRead == fileSize, 
                    ModC_AppendFormat(  &MODC_LAST_ERROR_MSG, 
                                        " actuallyRead: %"PRIu64", fileSize: %"PRIi64, 
                                        actuallyRead,
                                        fileSize);
                    RUN_DEFER_AND_RET(), 
                    ModC_ResultVoid);
        
        
        
    }
    MODC_DEFER_SCOPE_END
    #undef RUN_DEFER_AND_RET
    
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
            ModC_String resultStr = MODC_RESULT_TO_STRING(ModC_ResultVoid, result);
            printf("%s\n", resultStr.Data);
            ModC_String_Free(&resultStr);
        }
        MODC_RESULT_FREE_RESOURCE(ModC_ResultVoid, &result);
    }
    #else
    {
        ModC_ResultInt32 result = TestResult2();
        if(result.HasError)
        {
            MODC_ERROR_APPEND_TRACE(result.ValueOrError.Error);
            ModC_String resultStr = MODC_RESULT_TO_STRING(ModC_ResultInt32, result);
            ModC_String_Free(&resultStr);
        }
        MODC_RESULT_FREE_RESOURCE(ModC_ResultInt32, &result);
    }
    #endif
    
    
    return 0;
}
