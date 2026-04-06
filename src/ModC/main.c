#define ARENA_IMPLEMENTATION

#include "ModC/Allocator.h"
#include "ModC/Defer.h"
#include "ModC/GenericContainers.h"
#include "ModC/Strings/Strings.h"
#include "ModC/Tokenization.h"
#include "ModC/Classification.h"

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

static inline ModC_Result_Int32 TestResult(void)
{
    #undef ModC_ResultName_State
    #define ModC_ResultName_State ModC_Result_Int32
    
    if(true)
        return MODC_ERROR_CSTR_S("Failed to seek file");
    else
        return MODC_RESULT_VALUE_S(5);
}

static inline ModC_Result_Int32 TestResult2(void)
{
    ModC_Result_Int32 intResult = TestResult();
    int32_t* unwrappedVal = MODC_RESULT_TRY(intResult, MODC_RET_ERROR_S());
    return MODC_RESULT_VALUE_S(*unwrappedVal);
}

ModC_Result_Void Main(int argc, char* argv[])
{
    (void)&TestResult2;
    #undef ModC_ResultName_State
    #define ModC_ResultName_State ModC_Result_Void
    #undef ModC_TaggedUnionName_State
    #define ModC_TaggedUnionName_State ModC_StatementTokensUnion
    
    FILE* modcFile = NULL;
    ModC_Allocator mainArena;
    ModC_Allocator statementListArena;
    ModC_String fileContent;
    
    MODC_DEFER_SCOPE_START(0)
    {
        if(argc == 1)
        {
            printf("Usage: %s <path>\n", argv[0]);
            MODC_DEFER_BREAK(0, return MODC_RESULT_VALUE_S(0));
        }
        
        ModC_StringView filePath = ModC_StringView_Create(argv[1], strlen(argv[1]));
        printf("Compiling %s\n", filePath.Data);
        
        modcFile = fopen(filePath.Data, "r");
        if(!modcFile)
        {
            MODC_DEFER_BREAK(0, return MODC_ERROR_STR_FMT_S(("Failed to open file: %s", 
                                                            strerror(errno))));
        }
        
        MODC_DEFER(0, { fclose(modcFile); modcFile = NULL; });
        
        //Get file size
        int64_t fileSize;
        {
            int fseekResult = fseek(modcFile, 0, SEEK_END);
            MODC_CHECK( fseekResult == 0, 
                        (" fseekResult: %i.", fseekResult), 
                        MODC_DEFER_BREAK(0, MODC_RET_ERROR_S()));
            
            fileSize = ftell(modcFile);
            MODC_CHECK( fileSize >= 0, 
                        (" fileSize: "PRIi64".", fileSize), 
                        MODC_DEFER_BREAK(0, MODC_RET_ERROR_S()));
        }
        
        fseek(modcFile, 0, 0);
        mainArena = ModC_CreateArenaAllocator(fileSize);
        MODC_DEFER(0, ModC_Allocator_Destroy(&mainArena));
        
        fileContent = ModC_String_Create(ModC_Allocator_Share(&mainArena), fileSize);
        ModC_String_Resize(&fileContent, fileSize);
        MODC_CHECK( fileContent.Length == fileSize, "", MODC_DEFER_BREAK(0, MODC_RET_ERROR_S()));
        
        uint32_t actuallyRead = fread(fileContent.Data, 1, fileSize, modcFile);
        MODC_CHECK( actuallyRead == fileSize, 
                    (" actuallyRead: %"PRIu64", fileSize: %"PRIi64, actuallyRead, fileSize),
                    MODC_DEFER_BREAK(0, MODC_RET_ERROR_S()));
    
        ModC_Result_TokenList tokenListResult = 
            ModC_Tokenization(  ModC_ConstStringView_Create(fileContent.Data, fileContent.Length),
                                //ModC_CreateHeapAllocator());
                                ModC_Allocator_Share(&mainArena));
        ModC_TokenList* tokenList = MODC_RESULT_TRY(tokenListResult, 
                                                    MODC_DEFER_BREAK(0, MODC_RET_ERROR_S()));
        
        MODC_DEFER(0, ModC_TokenList_Free(tokenList));
        
        for(int i = 0; i < tokenList->Length; ++i)
        {
            ModC_ConstStringView typeStr = ModC_TokenType_ToCStr(tokenList->Data[i].TokenType);
            ModC_ConstStringView tokenTextView = ModC_Token_TokenTextView(&tokenList->Data[i]);
            printf( "Token: \"%.*s\", Token Type[%i]: %.*s\n", 
                    (int)tokenTextView.Length, tokenTextView.Data,
                    i, 
                    (int)typeStr.Length, typeStr.Data);
        }
        
        ModC_Result_StatementList statementListResult = ModC_CreateStatements(  tokenList, 
                                                                                &statementListArena);
        ModC_StatementList* statementList = MODC_RESULT_TRY(statementListResult, 
                                                            MODC_DEFER_BREAK(0, MODC_RET_ERROR_S()));
        MODC_DEFER(0, ModC_Allocator_Destroy(&statementListArena));
        
        ModC_Result_Void voidResult = 
            ModC_CleanAndClassifyStatements(statementList, 
                                            ModC_Allocator_Share(&mainArena),
                                            ModC_Allocator_Share(&statementListArena),
                                            tokenList,
                                            //TODO: Use scratch arena.
                                            ModC_Allocator_Share(&mainArena));
        (void)MODC_RESULT_TRY(voidResult, MODC_DEFER_BREAK(0, MODC_RET_ERROR_S()));
        
        
        for(int i = 0; i < statementList->Length; ++i)
        {
            ModC_ConstStringView statementTypeStr = 
                ModC_StatementType_ToConstStringView(statementList->Data[i].StatementType);
            
            printf( "statementList[%i]:\n" "StatementType: %.*s\n", 
                    i, (int)statementTypeStr.Length, statementTypeStr.Data);
            
            ModC_Result_Void checkResult = MODC_RESULT_VALUE_S(0);
            //bool invalidLength = false;
            static_assert((int)MODC_TAG_TYPE_S(Count) == 3, "");
            switch(statementList->Data[i].Tokens.Type)
            {
                case MODC_TAG_TYPE_S(ModC_CompoundStatement):
                {
                    ModC_CompoundStatement* compoundStatement = 
                        &statementList->Data[i].Tokens.MODC_TAG_DATA_S(ModC_CompoundStatement);
                    
                    printf("ModC_CompoundStatement.StartTokenIndex: %"PRIu32 "\n", 
                        compoundStatement->StartTokenIndex);
                    
                    for(int j = 0; j < compoundStatement->ChildStatements.Length; ++j)
                    {
                        printf( "ModC_CompoundStatement.ChildStatements[%i]: %"PRIu32 "\n", 
                                j, compoundStatement->ChildStatements.Data[j]);
                    }
                    
                    printf("ModC_CompoundStatement.EndTokenIndex: %"PRIu32 "\n", 
                        compoundStatement->EndTokenIndex);
                    printf( "ModC_CompoundStatement.Implicit: %s\n",  
                            (compoundStatement->Implicit ? "true" : "false"));
                    break;
                }
                case MODC_TAG_TYPE_S(ModC_TokenIndexList):
                {
                    ModC_TokenIndexList* tokenIndexList = 
                        &statementList->Data[i].Tokens.MODC_TAG_DATA_S(ModC_TokenIndexList);
                    
                    for(int j = 0; j < tokenIndexList->Length; ++j)
                        printf("ModC_TokenIndexList[%i]: %"PRIu32 "\n", j, tokenIndexList->Data[j]);
                    
                    //Print tokens
                    printf("Tokens: \"");
                    for(int j = 0; j < tokenIndexList->Length; ++j)
                    {
                        ModC_ConstStringView tokenText = 
                            ModC_Token_TokenTextView(&tokenList->Data[tokenIndexList->Data[j]]);
                        MODC_CHECK( tokenText.Length > 0, 
                                    ("Invalid token text"), 
                                    checkResult = MODC_RESULT_ERROR_S(MODC_LAST_ERROR); break);
                        if(j != tokenIndexList->Length - 1)
                            printf("%.*s ", (int)tokenText.Length, tokenText.Data);
                        else
                            printf("%.*s", (int)tokenText.Length, tokenText.Data);
                    }
                    printf("\"\n");
                    break;
                }
                case MODC_TAG_TYPE_S(ModC_TokenIndexRange):
                {
                    ModC_TokenIndexRange* tokenIndexRange = 
                        &statementList->Data[i].Tokens.MODC_TAG_DATA_S(ModC_TokenIndexRange);
                    
                    printf( "ModC_TokenIndexRange.StartIndex: %"PRIu32 "\n", 
                            tokenIndexRange->StartIndex);
                    printf("ModC_TokenIndexRange.EndIndex: %"PRIu32 "\n", tokenIndexRange->EndIndex);
                    //Print tokens
                    printf("Tokens: \"");
                    for(int j = tokenIndexRange->StartIndex; j < tokenIndexRange->EndIndex; ++j)
                    {
                        ModC_ConstStringView tokenText = ModC_Token_TokenTextView(&tokenList->Data[j]);
                        MODC_CHECK( tokenText.Length > 0, 
                                    ("Invalid token text"), 
                                    checkResult = MODC_RESULT_ERROR_S(MODC_LAST_ERROR); break);
                        if(j != tokenIndexRange->EndIndex - 1)
                            printf("%.*s ", (int)tokenText.Length, tokenText.Data);
                        else
                            printf("%.*s", (int)tokenText.Length, tokenText.Data);
                    }
                    printf("\"\n");
                    break;
                }
                default:
                    break;
            } //switch(statementList->Data[i].Value.Type)
            
            (void)MODC_RESULT_TRY(checkResult, MODC_DEFER_BREAK(0, MODC_RET_ERROR_S()));
            printf("\n");
            
            
            
        } //for(int i = 0; i < statementList->Length; ++i)
        
        
        
        
        
        
    }
    MODC_DEFER_SCOPE_END(0)
    
    return MODC_RESULT_VALUE_S(0);
}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    #if 1
    {
        #undef ModC_ResultName_State
        #define ModC_ResultName_State ModC_Result_Void
        
        ModC_Result_Void result = Main(argc, argv);
        if(result.HasError)
        {
            MODC_ERROR_APPEND_TRACE(result.ValueOrError.Error);
            ModC_String resultStr = MODC_RESULT_TO_STRING_S(result);
            printf("%.*s\n", (int)resultStr.Length, resultStr.Data);
            ModC_String_Free(&resultStr);
        }
        MODC_RESULT_FREE_RESOURCE_S(&result);
    }
    #else
    {
        #undef ModC_ResultName_State
        #define ModC_ResultName_State ModC_Result_Int32
        
        ModC_Result_Int32 result = TestResult2();
        if(result.HasError)
        {
            MODC_ERROR_APPEND_TRACE(result.ValueOrError.Error);
            ModC_String resultStr = MODC_RESULT_TO_STRING_S(result);
            printf("%.*s\n", (int)resultStr.Length, resultStr.Data);
            ModC_String_Free(&resultStr);
        }
        MODC_RESULT_FREE_RESOURCE_S(&result);
    }
    #endif
    
    
    return 0;
}
