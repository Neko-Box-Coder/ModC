#define ARENA_IMPLEMENTATION

#include "ModC/Allocator.h"
#include "ModC/Defer.h"
#include "ModC/GenericContainers.h"
#include "ModC/Strings/Strings.h"
#include "ModC/Tokenization.h"

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

#ifndef MODC_DEFAULT_ALLOC
    #define MODC_DEFAULT_ALLOC() ModC_CreateHeapAllocator()
#endif

#undef ModC_ResultName
#define ModC_ResultName ModC_Result_Int32

static inline ModC_Result_Int32 TestResult()
{
    if(true)
        return MODC_ERROR_CSTR("Failed to seek file");
    else
        return MODC_RESULT_VALUE(5);
}

static inline ModC_Result_Int32 TestResult2()
{
    ModC_Result_Int32 intResult = TestResult();
    int32_t unwrappedVal = MODC_RESULT_TRY(intResult, MODC_RET_ERROR());
    return MODC_RESULT_VALUE(unwrappedVal);
}

#undef ModC_ResultName
#define ModC_ResultName ModC_Result_Void

ModC_Result_Void Main(int argc, char* argv[])
{
    FILE* modcFile = NULL;
    ModC_Allocator mainArena;
    ModC_String fileContent;
    
    #define RUN_DEFER_RET_ERR() MODC_DEFER_BREAK( MODC_RET_ERROR() )
    
    MODC_DEFER_SCOPE_START
    {
        if(argc == 1)
        {
            printf("Usage: %s <path>\n", argv[0]);
            MODC_DEFER_BREAK(return MODC_RESULT_VALUE(0));
        }
        
        ModC_StringView filePath = ModC_StringView_Create(argv[1], strlen(argv[1]));
        printf("Compiling %s\n", filePath.Data);
        
        modcFile = fopen(filePath.Data, "r");
        if(!modcFile)
            MODC_DEFER_BREAK( return MODC_ERROR_STR_FMT(("Failed to open file: %s", strerror(errno))) );
        
        MODC_DEFER({ fclose(modcFile); modcFile = NULL; });
        
        //Get file size
        int64_t fileSize;
        {
            int fseekResult = fseek(modcFile, 0, SEEK_END);
            MODC_ASSERT(fseekResult == 0, (" fseekResult: %i.", fseekResult), RUN_DEFER_RET_ERR());
            fileSize = ftell(modcFile);
            MODC_ASSERT(fileSize >= 0, (" fileSize: "PRIi64".", fileSize), RUN_DEFER_RET_ERR());
        }
        
        fseek(modcFile, 0, 0);
        mainArena = ModC_CreateArenaAllocator(fileSize + 8192);
        MODC_DEFER(ModC_Allocator_Destroy(&mainArena));
        
        fileContent = ModC_String_Create(ModC_ShareArenaAllocator(mainArena.Allocator), fileSize);
        ModC_String_Resize(&fileContent, fileSize);
        MODC_ASSERT(fileContent.Length == fileSize, "", RUN_DEFER_RET_ERR());
        
        uint32_t actuallyRead = fread(fileContent.Data, 1, fileSize, modcFile);
        MODC_ASSERT(actuallyRead == fileSize, 
                    (" actuallyRead: %"PRIu64", fileSize: %"PRIi64, actuallyRead, fileSize),
                    RUN_DEFER_RET_ERR());
    }
    MODC_DEFER_SCOPE_END
    #undef RUN_DEFER_RET_ERR
    
    return MODC_RESULT_VALUE(0);
}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    #if 1
    {
        #undef ModC_ResultName
        #define ModC_ResultName ModC_Result_Void
        
        ModC_Result_Void result = Main(argc, argv);
        if(result.HasError)
        {
            MODC_ERROR_APPEND_TRACE(result.ValueOrError.Error);
            ModC_String resultStr = MODC_RESULT_TO_STRING(result);
            printf("%s\n", resultStr.Data);
            ModC_String_Free(&resultStr);
        }
        MODC_RESULT_FREE_RESOURCE(&result);
    }
    #else
    {
        #undef ModC_ResultName
        #define ModC_ResultName ModC_Result_Int32
        
        ModC_Result_Int32 result = TestResult2();
        if(result.HasError)
        {
            MODC_ERROR_APPEND_TRACE(result.ValueOrError.Error);
            ModC_String resultStr = MODC_RESULT_TO_STRING(result);
            printf("%s\n", resultStr.Data);
            ModC_String_Free(&resultStr);
        }
        MODC_RESULT_FREE_RESOURCE(&result);
    }
    #endif
    
    
    return 0;
}
