#ifndef MODC_PARSER_H
#define MODC_PARSER_H

#ifndef MODC_DEFAULT_ALLOC
    #define MODC_DEFAULT_ALLOC() ModC_CreateHeapAllocator()
#endif

#include "ModC/Tokenization.h"
#include "ModC/GenericContainers.h"
#include "ModC/Result.h"
#include "ModC/Keyword.h"
#include "ModC/Operators.h"

typedef enum ModC_StatementType
{
    ModC_StatementType_Invalid,
    ModC_StatementType_Unknown,
    ModC_StatementType_TypeDeclaration,
    ModC_StatementType_FunctionDeclaration,
    ModC_StatementType_VariableDeclaration,
    ModC_StatementType_Compound,
    ModC_StatementType_CompilerDirective,
    ModC_StatementType_Assignment,
    ModC_StatementType_VariableDeclareAssignment,
    ModC_StatementType_PureExpression,
    ModC_StatementType_IfStatement,
    ModC_StatementType_ForStatement,
    ModC_StatementType_WhileStatement,
    ModC_StatementType_SwitchStatement,
    ModC_StatementType_Count,   //14
} ModC_StatementType;

struct ModC_StatementList;
typedef struct ModC_StatementList ModC_StatementList;

typedef struct ModC_TokenIndexRange
{
    uint32_t StartIndex;    //Inclusive
    uint32_t EndIndex;      //Exclusive
} ModC_TokenIndexRange;

typedef ModC_Uint32List ModC_StatementIndexList;

//TODO: Add function for checking start and end token alignment
typedef struct ModC_CompoundStatement
{
    uint32_t StartTokenIndex;
    ModC_StatementIndexList ChildStatements;
    uint32_t EndTokenIndex;
    bool Implicit;      //No start and end token index if true
} ModC_CompoundStatement;

typedef ModC_Uint32List ModC_TokenIndexList;

#define MODC_TAGGED_UNION_NAME ModC_StatementTokensUnion
#define MODC_VALUE_TYPES ModC_CompoundStatement,ModC_TokenIndexList,ModC_TokenIndexRange
#include "ModC/TaggedUnion.h"

typedef struct ModC_Statement ModC_Statement;
struct ModC_Statement
{
    ModC_StatementType StatementType;
    ModC_StatementTokensUnion Tokens;
    uint32_t Index;
    uint32_t ParentIndex;
};

#define MODC_LIST_NAME ModC_StatementList
#define MODC_VALUE_TYPE ModC_Statement
#define MODC_NO_TYPEDEF 1
//TODO: Value item free
#include "ModC/List.h"

MODC_DEFINE_RESULT_STRUCT(ModC_ResultStatementPtr, ModC_Statement*)

MODC_DEFINE_RESULT_STRUCT(ModC_Result_StatementList, ModC_StatementList)


#include "uthash.h"
typedef struct ModC_TypeEntry
{
    ModC_String Type;
    UT_hash_handle hh;
} ModC_TypeEntry;


#include "static_assert.h/assert.h"

#include <stdbool.h>
#include <stdint.h>


uint32_t ModC_StatementTokensUnion_GetTokenCount(const ModC_StatementTokensUnion* statementUnion)
{
    #undef ModC_TaggedUnionName_State
    #define ModC_TaggedUnionName_State ModC_StatementTokensUnion
    
    if(!statementUnion)
        return 0;
    
    switch(statementUnion->Type)
    {
        case MODC_TAGGED_TYPE_S(ModC_CompoundStatement):
            return 2;
        case MODC_TAGGED_TYPE_S(ModC_TokenIndexList):
            return statementUnion->Data.MODC_TAGGED_FIELD_S(ModC_TokenIndexList).Length;
        case MODC_TAGGED_TYPE_S(ModC_TokenIndexRange):
        {
            const ModC_TokenIndexRange* range = 
                &statementUnion->Data.MODC_TAGGED_FIELD_S(ModC_TokenIndexRange);
            return range->EndIndex - range->StartIndex;
        }
        default:
            return 0;
    }
}


ModC_Result_TokenPtr ModC_StatementTokensUnion_GetTokenAt(const ModC_StatementTokensUnion* statementUnion, 
                                                    const ModC_TokenList* tokens,
                                                    uint32_t indexInStatement)
{
    #undef ModC_ResultName_State
    #define ModC_ResultName_State ModC_Result_TokenPtr
    #undef ModC_TaggedUnionName_State
    #define ModC_TaggedUnionName_State ModC_StatementTokensUnion
    
    MODC_CHECK(statementUnion != NULL, (""), MODC_RET_ERROR_S());
    MODC_CHECK(tokens != NULL, (""), MODC_RET_ERROR_S());
    
    uint32_t tokenIndex = 0;
    switch(statementUnion->Type)
    {
        case MODC_TAGGED_TYPE_S(ModC_CompoundStatement):
        {
            //NOTE: Shouldn't use this function for compound statement..., but whatever
            const ModC_CompoundStatement* compound = 
                &statementUnion->Data.MODC_TAGGED_FIELD_S(ModC_CompoundStatement);
            if(indexInStatement == 0)
                tokenIndex = compound->StartTokenIndex;
            else if(indexInStatement == 1)
                tokenIndex = compound->EndTokenIndex;
            else
                return MODC_ERROR_STR_FMT_S("Invalid index for accessing %"PRIu32, indexInStatement);
            break;
        }
        case MODC_TAGGED_TYPE_S(ModC_TokenIndexList):
        {
            const ModC_TokenIndexList* tokenIndexList = 
                &statementUnion->Data.MODC_TAGGED_FIELD_S(ModC_TokenIndexList);
            
            MODC_CHECK(tokenIndexList->Length > 0, ("Empty statement"), MODC_RET_ERROR_S());
            MODC_CHECK( indexInStatement < tokenIndexList->Length, 
                        ("Invalid index for accessing, index: %"PRIu32", length: %"PRIu64, 
                        indexInStatement, tokenIndexList->Length),
                        MODC_RET_ERROR_S());
            tokenIndex = tokenIndexList->Data[indexInStatement];
            break;
        }
        case MODC_TAGGED_TYPE_S(ModC_TokenIndexRange):
        {
            const ModC_TokenIndexRange* range = 
                &statementUnion->Data.MODC_TAGGED_FIELD_S(ModC_TokenIndexRange);
            
            MODC_CHECK(range->EndIndex > range->StartIndex, ("Empty statement"), MODC_RET_ERROR_S());
            MODC_CHECK( indexInStatement < range->EndIndex - range->StartIndex, 
                        ("Invalid index for accessing, index: %"PRIu32", length: %"PRIu32,
                        indexInStatement, range->EndIndex - range->StartIndex),
                        MODC_RET_ERROR_S());
            tokenIndex = range->StartIndex + indexInStatement;
            break;
        }
        default:
            return MODC_ERROR_CSTR_S("Unexpected statement union type");
    }
    
    MODC_CHECK( tokenIndex < tokens->Length, 
                ("Token index access out of bound, tokenIndex %"PRIu32", tokens->Length: %"PRIu64,
                tokenIndex, tokens->Length),
                MODC_RET_ERROR_S());
    return MODC_RESULT_VALUE_S(&tokens->Data[tokenIndex]);
}




ModC_ConstStringView ModC_StatementType_ToConstStringView(ModC_StatementType type)
{
    static_assert((int)ModC_StatementType_Count == 14, "");
    switch(type)
    {
        #define RET_TO_STR(enumVal) \
            case enumVal: \
                return ModC_ConstStringView_FromLiteral(#enumVal)
        
        RET_TO_STR(ModC_StatementType_Invalid);
        RET_TO_STR(ModC_StatementType_Unknown);
        RET_TO_STR(ModC_StatementType_TypeDeclaration);
        RET_TO_STR(ModC_StatementType_FunctionDeclaration);
        RET_TO_STR(ModC_StatementType_VariableDeclaration);
        RET_TO_STR(ModC_StatementType_Compound);
        RET_TO_STR(ModC_StatementType_CompilerDirective);
        RET_TO_STR(ModC_StatementType_Assignment);
        RET_TO_STR(ModC_StatementType_VariableDeclareAssignment);
        RET_TO_STR(ModC_StatementType_PureExpression);
        RET_TO_STR(ModC_StatementType_IfStatement);
        RET_TO_STR(ModC_StatementType_ForStatement);
        RET_TO_STR(ModC_StatementType_WhileStatement);
        RET_TO_STR(ModC_StatementType_SwitchStatement);
        RET_TO_STR(ModC_StatementType_Count);
        
        #undef RET_TO_STR
    }
    return (ModC_ConstStringView){0};
}


ModC_ResultStatementPtr ModC_Statement_CreateCompound(  ModC_Allocator allocator, 
                                                        ModC_StatementList* statementList,
                                                        uint32_t parentIndex,
                                                        bool implicit,
                                                        uint32_t reserveStatementsCount)
{
    #undef ModC_ResultName_State
    #define ModC_ResultName_State ModC_ResultStatementPtr
    
    MODC_CHECK(statementList != NULL, (""), MODC_RET_ERROR_S());
    MODC_CHECK(allocator.Type == ModC_AllocatorType_SharedArena, (""), MODC_RET_ERROR_S());
    
    uint64_t oldLength = statementList->Length;
    ModC_StatementList_AddValue(statementList, 
                                (ModC_Statement)
                                {
                                    .StatementType = ModC_StatementType_Compound,
                                    .Tokens =    
                                        MODC_TAGGED_INIT
                                        (
                                            ModC_StatementTokensUnion, 
                                            ModC_CompoundStatement, 
                                            (ModC_CompoundStatement)
                                            {
                                                .StartTokenIndex = 0,
                                                .ChildStatements = 
                                                    ModC_Uint32List_Create( allocator, 
                                                                            reserveStatementsCount),
                                                .EndTokenIndex = 0,
                                                .Implicit = implicit
                                            }
                                        ),
                                    .Index = oldLength,
                                    .ParentIndex = parentIndex
                                });
    MODC_CHECK(statementList->Length != oldLength, ("Failed to allocate"), MODC_RET_ERROR_S());
    
    ModC_Statement* retStatementPtr = &statementList->Data[statementList->Length - 1];
    MODC_CHECK( retStatementPtr ->Tokens
                                .Data
                                .MODC_TAGGED_FIELD(ModC_StatementTokensUnion, ModC_CompoundStatement)
                                .ChildStatements
                                .Cap > 0,
                ("Failed to allocate"), 
                MODC_RET_ERROR_S());
    
    return MODC_RESULT_VALUE_S(retStatementPtr);
}

ModC_ResultStatementPtr ModC_Statement_CreatePlain( ModC_Allocator allocator, 
                                                    ModC_StatementList* statementList,
                                                    uint32_t parentIndex)
{
    #undef ModC_ResultName_State
    #define ModC_ResultName_State ModC_ResultStatementPtr
    
    MODC_CHECK(statementList != NULL, (""), MODC_RET_ERROR_S());
    MODC_CHECK(allocator.Type == ModC_AllocatorType_SharedArena, (""), MODC_RET_ERROR_S());
    
    uint64_t oldLength = statementList->Length;
    ModC_StatementList_AddValue(statementList, 
                                (ModC_Statement)
                                {
                                    .StatementType = ModC_StatementType_Unknown,
                                    .Tokens = MODC_TAGGED_INIT( ModC_StatementTokensUnion, 
                                                                ModC_TokenIndexRange,
                                                                {0}),
                                    .Index = oldLength,
                                    .ParentIndex = parentIndex
                                });
    MODC_CHECK(statementList->Length != oldLength, ("Failed to allocate"), MODC_RET_ERROR_S());
    
    ModC_Statement* retStatementPtr = &statementList->Data[statementList->Length - 1];
    //MODC_CHECK( retStatementPtr->Tokens.Data.MODC_TAGGED_FIELD( ModC_StatementTokensUnion, 
    //                                                            ModC_TokenIndexList).Cap > 0,
    //            ("Failed to allocate"), 
    //            MODC_RET_ERROR_S());
    
    return MODC_RESULT_VALUE_S(retStatementPtr);
}

#if 0
    void ModC_Statement_Free(ModC_Statement* this)
    {
        #undef ModC_TaggedUnionName_State
        #define ModC_TaggedUnionName_State ModC_StatementTokensUnion
        
        if(!this)
            return;
        
        //No children, just free the token list
        if(this->Tokens.Type == MODC_TAGGED_TYPE_S(TokenIndexList))
        {
            ModC_Uint32List_Free(&this->Tokens.Data.MODC_TAGGED_FIELD_S(TokenIndexList));
            *this = (ModC_Statement){0};
            return;
        }
        
        //Get to the deepest child
        ModC_Statement* currentStatement = this;
        while(currentStatement->Tokens.Type == MODC_TAGGED_TYPE_S(ModC_ChildStatements))
        {
            ModC_ChildStatements* currentChildren =
                &currentStatement->Tokens.Data.MODC_TAGGED_FIELD_S(ModC_ChildStatements);
            
            if(currentChildren->Statements->Length == 0)
                break;
            
            currentStatement = &currentChildren->Statements->Data[0];
        }
        
        //Walk and free the whole tree
    }
#endif

static inline ModC_Result_Void ModC_AddStatementToParent(   uint32_t statementIndex, 
                                                            uint32_t currentParentIndex,
                                                            ModC_StatementList* statementList)
{
    #undef ModC_ResultName_State
    #define ModC_ResultName_State ModC_Result_Void
    #undef ModC_TaggedUnionName_State
    #define ModC_TaggedUnionName_State ModC_StatementTokensUnion
    
    MODC_CHECK( statementList->Data[currentParentIndex].Tokens.Type ==
                MODC_TAGGED_TYPE_S(ModC_CompoundStatement),
                ("Expecting parent to be type compound, found type index %d instead",
                (int)statementList->Data[currentParentIndex].Tokens.Type),
                MODC_RET_ERROR_S());
    
    ModC_StatementIndexList* parentStatementList =
        &statementList  ->Data[currentParentIndex]
                        .Tokens
                        .Data
                        .MODC_TAGGED_FIELD_S(ModC_CompoundStatement)
                        .ChildStatements;
    ModC_Uint32List_AddValue(parentStatementList, statementIndex);
    
    return MODC_RESULT_VALUE_S(0);
}


static inline ModC_Result_Uint32 ModC_EndCurrentStatment(   bool countCurrentToken, 
                                                            uint32_t i, 
                                                            uint32_t currentParentIndex,
                                                            ModC_Allocator sharedArena,
                                                            const ModC_TokenList* tokens,
                                                            uint32_t* startTokenIndex,
                                                            ModC_StatementList* statementList)
{
    #undef ModC_ResultName_State
    #define ModC_ResultName_State ModC_Result_Uint32
    #undef ModC_TaggedUnionName_State
    #define ModC_TaggedUnionName_State ModC_StatementTokensUnion
    
    if(i == *startTokenIndex)
        return MODC_RESULT_VALUE_S(i);
    
    bool allWhiteSpaceOrNewline = true;
    uint32_t endIndex = countCurrentToken ? i + 1 : i;
    for(uint32_t checkIndex = *startTokenIndex; checkIndex < endIndex; ++checkIndex)
    {
        if( tokens->Data[checkIndex].TokenType != ModC_TokenType_Space &&
            tokens->Data[checkIndex].TokenType != ModC_TokenType_Newline &&
            tokens->Data[checkIndex].TokenType != ModC_TokenType_Comment)
        {
            allWhiteSpaceOrNewline = false;
            break;
        }
    }

    if(allWhiteSpaceOrNewline)
    {
        *startTokenIndex = countCurrentToken ? ++i : i;
        return MODC_RESULT_VALUE_S(i);
    }
    
    ModC_ResultStatementPtr statementPtrResult = ModC_Statement_CreatePlain(sharedArena,
                                                                            statementList,
                                                                            currentParentIndex);
    ModC_Statement* prevStatementPtr = *MODC_RESULT_TRY(statementPtrResult, MODC_RET_ERROR_S());
    MODC_CHECK( prevStatementPtr->Tokens.Type == MODC_TAGGED_TYPE_S(ModC_TokenIndexRange),
                (""),
                MODC_RET_ERROR_S());
    
    ModC_TokenIndexRange* indexRange =
        &prevStatementPtr->Tokens.Data.MODC_TAGGED_FIELD_S(ModC_TokenIndexRange);
    
    indexRange->StartIndex = *startTokenIndex;
    indexRange->EndIndex = countCurrentToken ? i + 1 : i;
    
    //TODO: Maybe not needed
    //Trim newlines, spaces and comments
    if(false)
    {
        uint32_t j = 0;
        for(j = indexRange->StartIndex; j < indexRange->EndIndex; ++j)
        {
            if( tokens->Data[j].TokenType != ModC_TokenType_Space &&
                tokens->Data[j].TokenType != ModC_TokenType_Newline &&
                tokens->Data[j].TokenType != ModC_TokenType_Comment)
            {
                break;
            }
        }
        indexRange->StartIndex = j;
        
        for(j = indexRange->EndIndex - 1; j >= indexRange->StartIndex; --j)
        {
            if( tokens->Data[j].TokenType != ModC_TokenType_Space &&
                tokens->Data[j].TokenType != ModC_TokenType_Newline &&
                tokens->Data[j].TokenType != ModC_TokenType_Comment)
            {
                break;
            }
        }
        indexRange->EndIndex = j + 1;
    }
    
    //TODO: Attach comments to statements
    
    *startTokenIndex = countCurrentToken ? ++i : i;
    
    ModC_Result_Void voidResult = ModC_AddStatementToParent(statementList->Length - 1, 
                                                            currentParentIndex, 
                                                            statementList);

    (void)MODC_RESULT_TRY(voidResult, MODC_RET_ERROR_S());
    return MODC_RESULT_VALUE_S(i);
}

static inline ModC_Result_StatementList ModC_CreateStatements(  const ModC_TokenList* tokens, 
                                                                ModC_Allocator* outStatementsArena)
{
    #undef ModC_ResultName_State
    #define ModC_ResultName_State ModC_Result_StatementList
    #undef ModC_TaggedUnionName_State
    #define ModC_TaggedUnionName_State ModC_StatementTokensUnion
    
    MODC_CHECK(tokens != NULL, (""), MODC_RET_ERROR_S());
    MODC_CHECK(outStatementsArena != NULL, (""), MODC_RET_ERROR_S());
    
    *outStatementsArena = ModC_CreateArenaAllocator(1024);   //TODO: Proper reserve count
    ModC_Allocator sharedArena = ModC_Allocator_Share(outStatementsArena);
    
    //TODO: Proper reserve count
    ModC_StatementList statementList = ModC_StatementList_Create(*outStatementsArena, 16);
    ModC_ResultStatementPtr statementPtrResult = ModC_Statement_CreateCompound( sharedArena, 
                                                                                &statementList, 
                                                                                0,
                                                                                true,
                                                                                128);
    uint32_t startTokenIndex = 0;
    uint32_t currentParentIndex = 0;
    for(uint32_t i = 0; i < tokens->Length; ++i)
    {
        static_assert(ModC_TokenType_Count == 19, "");
        switch((ModC_CharTokenType)tokens->Data[i].TokenType)
        {
            case ModC_CharTokenType_Identifier:
                break;
            case ModC_CharTokenType_Operator:
                break;
            case ModC_CharTokenType_BlockStart:
            {
                ModC_Result_Uint32 uint32Result = ModC_EndCurrentStatment(  false, 
                                                                            i,
                                                                            currentParentIndex, 
                                                                            sharedArena,
                                                                            tokens,
                                                                            &startTokenIndex,
                                                                            &statementList);
                i = *MODC_RESULT_TRY(uint32Result, MODC_RET_ERROR_S());
                
                //Create compound as parent
                statementPtrResult = ModC_Statement_CreateCompound( sharedArena, 
                                                                    &statementList, 
                                                                    currentParentIndex,
                                                                    false,
                                                                    32);
                ModC_Statement* newStatement = *MODC_RESULT_TRY(statementPtrResult, MODC_RET_ERROR_S());
                MODC_CHECK( newStatement->Tokens.Type == MODC_TAGGED_TYPE_S(ModC_CompoundStatement),
                            ("Unexpected type"),
                            MODC_RET_ERROR_S());
                MODC_CHECK(i == startTokenIndex, (""), MODC_RET_ERROR_S());
                ModC_Result_Void voidResult = ModC_AddStatementToParent(statementList.Length - 1,
                                                                        currentParentIndex,
                                                                        &statementList);
                (void)MODC_RESULT_TRY(voidResult, MODC_RET_ERROR_S());
                
                ModC_CompoundStatement* compoundData = 
                    &newStatement->Tokens.Data.MODC_TAGGED_FIELD_S(ModC_CompoundStatement);
                compoundData->StartTokenIndex = i;
                startTokenIndex = i + 1;
                currentParentIndex = statementList.Length - 1;
                break;
            }
            case ModC_CharTokenType_BlockEnd:
            {
                ModC_Result_Uint32 uint32Result = ModC_EndCurrentStatment(  false, 
                                                                            i,
                                                                            currentParentIndex, 
                                                                            sharedArena,
                                                                            tokens,
                                                                            &startTokenIndex,
                                                                            &statementList);
                i = *MODC_RESULT_TRY(uint32Result, MODC_RET_ERROR_S());
                
                //Finish compound parent
                ModC_Statement* parentStatement = &statementList.Data[currentParentIndex];
                MODC_CHECK( parentStatement->Tokens.Type == MODC_TAGGED_TYPE_S(ModC_CompoundStatement),
                            ("Unexpected type"),
                            MODC_RET_ERROR_S());
                
                ModC_CompoundStatement* parentCompound = 
                    &parentStatement->Tokens.Data.MODC_TAGGED_FIELD_S(ModC_CompoundStatement);
                MODC_CHECK( !parentCompound->Implicit,
                            ("Expected non implicit for parent when block end"),
                            MODC_RET_ERROR_S());
                parentCompound->EndTokenIndex = i;
                startTokenIndex = i + 1;
                currentParentIndex = parentStatement->ParentIndex;
                break;
            }
            case ModC_CharTokenType_InvokeStart:
                break;
            case ModC_CharTokenType_InvokeEnd:
            {
                //NOTE: This is wrong anyway, but deal with it at syntax analysis
                if(i == startTokenIndex)
                    break;
                
                //Find the corresponding invoke start, then check if the token before that is a 
                //keyword
                int invokeCounter = 1;
                uint32_t invokeStartIndex = startTokenIndex;
                for(uint32_t j = i - 1; j >= startTokenIndex; --j)
                {
                    if(invokeCounter == 0)
                    {
                        if( tokens->Data[j].TokenType == ModC_TokenType_Space ||
                            tokens->Data[j].TokenType == ModC_TokenType_Newline ||
                            tokens->Data[j].TokenType == ModC_TokenType_Comment)
                        {
                            continue;
                        }
                        else
                        {
                            invokeStartIndex = j;
                            break;
                        }
                    }
                    else
                    {
                        if(tokens->Data[j].TokenType == ModC_TokenType_InvokeEnd)
                            ++invokeCounter;
                        else if(tokens->Data[j].TokenType == ModC_TokenType_InvokeStart)
                            --invokeCounter;
                    }
                }
                
                //Didn't find the invoke start token
                if(invokeStartIndex == startTokenIndex)
                    break;
                
                //Keyword must be an identifier
                if(tokens->Data[invokeStartIndex].TokenType != ModC_TokenType_Identifier)
                    break;
                
                //If the token before invoke start is a keyword, end the current statement
                if(ModC_IsInvokableKeyword(ModC_Token_TokenTextView(&tokens->Data[invokeStartIndex])))
                {
                    ModC_Result_Uint32 uint32Result = ModC_EndCurrentStatment(  true, 
                                                                                i,
                                                                                currentParentIndex, 
                                                                                sharedArena,
                                                                                tokens,
                                                                                &startTokenIndex,
                                                                                &statementList);
                    i = *MODC_RESULT_TRY(uint32Result, MODC_RET_ERROR_S());
                }
                break;
            }
            case ModC_CharTokenType_Semicolon:
            {
                ModC_Result_Uint32 uint32Result = ModC_EndCurrentStatment(  true, 
                                                                            i,
                                                                            currentParentIndex, 
                                                                            sharedArena,
                                                                            tokens,
                                                                            &startTokenIndex,
                                                                            &statementList);
                i = *MODC_RESULT_TRY(uint32Result, MODC_RET_ERROR_S());
                break;
            }
            case ModC_CharTokenType_StringLiteral:
                break;
            case ModC_CharTokenType_CharLiteral:
                break;
            case ModC_CharTokenType_IntLiteral:
                break;
            case ModC_CharTokenType_Space:
                break;
            case ModC_CharTokenType_Newline:
            {
                //Check compiler directives (#)
                if(i == startTokenIndex)
                    break;
                
                //Find the beginning of the line
                uint32_t lineFirstToken = startTokenIndex;
                for(int64_t j = i - 1; j >= 0; --j)
                {
                    if(tokens->Data[j].TokenType == ModC_TokenType_Newline)
                    {
                        lineFirstToken = j + 1;
                        break;
                    }
                }
                
                //Then find the first non skippable token
                //uint32_t firstParsableToken = startTokenIndex;
                for(uint32_t j = lineFirstToken; j < i; ++j)
                {
                    if(!ModC_Token_IsSkippable(&tokens->Data[j]))
                    {
                        lineFirstToken = j;
                        break;
                    }
                }
                
                if( tokens->Data[lineFirstToken].TokenType == ModC_TokenType_Operator &&
                    ModC_Token_TokenTextView(&tokens->Data[lineFirstToken]).Length == 1 &&
                    ModC_Token_TokenTextView(&tokens->Data[lineFirstToken]).Data[0] == '#')
                {
                    ModC_Result_Uint32 uint32Result = ModC_EndCurrentStatment(  true, 
                                                                                i,
                                                                                currentParentIndex, 
                                                                                sharedArena,
                                                                                tokens,
                                                                                &startTokenIndex,
                                                                                &statementList);
                    i = *MODC_RESULT_TRY(uint32Result, MODC_RET_ERROR_S());
                    //NOTE: We know it is a compiler directive statement, but we will classify it later.
                }
                
                break;
            }
            case ModC_CharTokenType_Undef:
                break;
        } //switch((ModC_CharTokenType)tokens->Data[i].TokenType)
    } //for(uint32_t i = 0; i < tokens->Length; ++i)
    
    //Last statement, check empty case as well
    if(tokens->Length != 0)
    {
        uint32_t i = tokens->Length - 1;
        ModC_Result_Uint32 uint32Result = ModC_EndCurrentStatment(  true, 
                                                                    i,
                                                                    currentParentIndex, 
                                                                    sharedArena,
                                                                    tokens,
                                                                    &startTokenIndex,
                                                                    &statementList);
        i = *MODC_RESULT_TRY(uint32Result, MODC_RET_ERROR_S());
    }
    
    return MODC_RESULT_VALUE_S(statementList);
}

//Normalizes the statements by removing spaces, comments and newlines and merge operators if possible
static inline ModC_Result_Void ModC_Statement_Normalize(ModC_Statement* statement,
                                                        ModC_Allocator tokensAllcoator,
                                                        ModC_Allocator statementsArena,
                                                        ModC_TokenList* tokens,
                                                        ModC_Allocator scratchAllocator)
{
    #undef ModC_ResultName_State
    #define ModC_ResultName_State ModC_Result_Void
    #undef ModC_TaggedUnionName_State
    #define ModC_TaggedUnionName_State ModC_StatementTokensUnion
    
    MODC_CHECK( statement->Tokens.Type == MODC_TAGGED_TYPE_S(ModC_TokenIndexRange),
                ("Unexpected statement union type"),
                MODC_RET_ERROR_S());
    
    ModC_TokenIndexRange* indexRange = &statement   ->Tokens
                                                    .Data
                                                    .MODC_TAGGED_FIELD_S(ModC_TokenIndexRange);
    ModC_TokenIndexList tokenIndices;
    ModC_String tempMergedOperator;
    
    MODC_DEFER_SCOPE_START(0)
    {
        tokenIndices = 
            ModC_Uint32List_Create( scratchAllocator, 
                                    (indexRange->EndIndex - indexRange->StartIndex));
        MODC_DEFER(0, ModC_Uint32List_Free(&tokenIndices));
        
        tempMergedOperator = ModC_String_Create(scratchAllocator, 3);
        MODC_DEFER(0, ModC_String_Free(&tempMergedOperator));
        
        uint32_t minLookBack = indexRange->StartIndex;
        bool skipped = false;
        for(uint32_t i = indexRange->StartIndex; i < indexRange->EndIndex; ++i)
        {
            if(tokens->Data[i].TokenType == ModC_TokenType_Operator)
            {
                //If we reached the end of continuous operator tokens,
                //merge operators tokens into a single token if possible
                bool operatorNext = i != indexRange->EndIndex - 1 &&
                                    tokens->Data[i + 1].TokenType == ModC_TokenType_Operator;
                if(!operatorNext && minLookBack != i)
                {
                    MODC_CHECK(minLookBack < i, (""), MODC_DEFER_BREAK(0, MODC_RET_ERROR_S()));
                    
                    ModC_String_Resize(&tempMergedOperator, 0);
                    for(uint32_t j = minLookBack; j <= i; ++j)
                    {
                        MODC_CHECK( tokens->Data[j].TokenType == ModC_TokenType_Operator, 
                                    (""), 
                                    MODC_DEFER_BREAK(0, MODC_RET_ERROR_S()));
                        
                        ModC_ConstStringView opChar = 
                            ModC_Token_TokenTextView(&tokens->Data[j]);
                        
                        MODC_CHECK( opChar.Length == 1, 
                                    ("Unexpected operator text length: %"PRIu32, 
                                    opChar.Length),
                                    MODC_DEFER_BREAK(0, MODC_RET_ERROR_S()));
                        
                        ModC_String_AddValue(&tempMergedOperator, opChar.Data[0]);
                    }
                    
                    ModC_ConstStringView mergedView = 
                        ModC_ConstStringView_Create(tempMergedOperator.Data, 
                                                    tempMergedOperator.Length);
                    if(ModC_IsValidComplexOperator(mergedView))
                    {
                        skipped = true;
                        
                        //Modify token to concatenated operator string
                        if( tokens->Data[i].TokenText.Type == 
                            MODC_TAGGED_TYPE(ModC_StringUnion, ModC_String))
                        {
                            ModC_String* tokenStr = 
                                &tokens ->Data[minLookBack]
                                        .TokenText
                                        .Data
                                        .MODC_TAGGED_FIELD(ModC_StringUnion, ModC_String);
                            
                            ModC_String_AddRange(   tokenStr, 
                                                    &tempMergedOperator.Data[1], 
                                                    tempMergedOperator.Length - 1);
                        }
                        else
                        {
                            ModC_String tokenStr = 
                                ModC_String_FromData(   tokensAllcoator, 
                                                        tempMergedOperator.Data, 
                                                        tempMergedOperator.Length);
                            
                            tokens->Data[minLookBack].TokenText = 
                                MODC_TAGGED_INIT(ModC_StringUnion, ModC_String, tokenStr);
                        }
                        
                        ModC_Uint32List_AddValue(&tokenIndices, minLookBack);
                    }
                    else
                    {
                        for(uint32_t j = minLookBack; j <= i; ++j)
                            ModC_Uint32List_AddValue(&tokenIndices, j);
                    }
                } //if(!operatorNext && minLookBack != i)
                else if(!operatorNext)
                    ModC_Uint32List_AddValue(&tokenIndices, i);
            } //if(tokens->Data[i].TokenType == ModC_TokenType_Operator)
            //Skip all comments, spaces, newlines, etc...
            else if(ModC_Token_IsSkippable(&tokens->Data[i]))
            {
                //TODO: Attach comments to statements
                skipped = true;
                minLookBack = i + 1;
            }
            //Otherwise, don't skip current token
            else
            {
                minLookBack = i + 1;
                ModC_Uint32List_AddValue(&tokenIndices, i);
            }
        } //for(uint32_t i = indexRange->StartIndex; i < indexRange->EndIndex; ++i)
        
        if(skipped)
        {
            indexRange = NULL;
            statement->Tokens = 
                MODC_TAGGED_INIT_S( ModC_TokenIndexList, 
                                    ModC_Uint32List_Create( statementsArena, 
                                                            tokenIndices.Length));
            ModC_Uint32List_AddRange(   &statement  ->Tokens
                                                    .Data
                                                    .MODC_TAGGED_FIELD_S(ModC_TokenIndexList),
                                        tokenIndices.Data,
                                        tokenIndices.Length);
        }
    }
    MODC_DEFER_SCOPE_END(0)
    
    return MODC_RESULT_VALUE_S(0);
}

static inline ModC_Result_Uint32 ModC_Statement_Next(   const ModC_Statement* statement, 
                                                        const ModC_StatementList* statements,
                                                        uint32_t currentStatementIndex,
                                                        bool* isEnd)
{
    #undef ModC_ResultName_State
    #define ModC_ResultName_State ModC_Result_Uint32
    #undef ModC_TaggedUnionName_State
    #define ModC_TaggedUnionName_State ModC_StatementTokensUnion
    
    MODC_CHECK(statement != NULL, (""), MODC_RET_ERROR_S());
    MODC_CHECK(statements != NULL, (""), MODC_RET_ERROR_S());
    
    *isEnd = false;
    
    //If compound, go to first child if any
    if( statement->StatementType == ModC_StatementType_Compound &&
        statement   ->Tokens
                    .Data
                    .MODC_TAGGED_FIELD_S(ModC_CompoundStatement)
                    .ChildStatements
                    .Length != 0)
    {
        currentStatementIndex = statement   ->Tokens
                                            .Data
                                            .MODC_TAGGED_FIELD_S(ModC_CompoundStatement)
                                            .ChildStatements
                                            .Data[0];
        return MODC_RESULT_VALUE_S(currentStatementIndex);
    }
    
    //Traverse to the next statement
    uint32_t origStatementIndex = currentStatementIndex;
    while(currentStatementIndex == origStatementIndex)
    {
        const ModC_Statement* parentStatement = &statements->Data[statement->ParentIndex];
        MODC_CHECK( parentStatement->StatementType == ModC_StatementType_Compound, 
                    ("Parent statement must be compound statement"), 
                    MODC_RET_ERROR_S());
        
        const ModC_CompoundStatement* parentCompound = 
            &parentStatement->Tokens.Data.MODC_TAGGED_FIELD_S(ModC_CompoundStatement);
        
        uint32_t childIndex = ModC_Uint32List_Find( &parentCompound->ChildStatements, 
                                                    &statement->Index);
        MODC_CHECK( childIndex != parentCompound->ChildStatements.Length, 
                    ("Failed to find child in parent. Corrupted Tree?"), 
                    MODC_RET_ERROR_S());
        
        //Continue to the next child if we are not at the end
        if(childIndex < parentCompound->ChildStatements.Length - 1)
            currentStatementIndex = parentCompound->ChildStatements.Data[childIndex + 1];
        //Otherwise go up if we are not under root
        else if(parentStatement->Index != parentStatement->ParentIndex)
            statement = parentStatement;
        //Otherwise, we are done
        else
        {
            *isEnd = true;
            return MODC_RESULT_VALUE_S(0);
        }
    }
    
    return MODC_RESULT_VALUE_S(currentStatementIndex);
}

static inline ModC_Result_Void ModC_CleanAndClassifyStatements( ModC_StatementList* statements, 
                                                                ModC_Allocator tokensAllcoator,
                                                                ModC_Allocator statementsArena,
                                                                ModC_TokenList* tokens,
                                                                ModC_Allocator scratchAllocator)
{
    #undef ModC_ResultName_State
    #define ModC_ResultName_State ModC_Result_Void
    #undef ModC_TaggedUnionName_State
    #define ModC_TaggedUnionName_State ModC_StatementTokensUnion
    
    MODC_CHECK(statements != NULL, (""), MODC_RET_ERROR_S());
    MODC_CHECK(tokens != NULL, (""), MODC_RET_ERROR_S());
    MODC_CHECK( statementsArena.Type == ModC_AllocatorType_SharedArena ||
                statementsArena.Type == ModC_AllocatorType_OwnedArena, 
                (""),
                MODC_RET_ERROR_S());
    
    
    //ModC_StatementIndexList currentProcess
    if(statements->Length == 0)
        return MODC_RESULT_VALUE_S(0);
    
    uint32_t currentStatementIndex = 0;
    {
        ModC_Statement* rootStatement = &statements->Data[0];
        
        MODC_CHECK( rootStatement->StatementType == ModC_StatementType_Compound, 
                    ("Root node must be compound statement"),
                    MODC_RET_ERROR_S());
        //Empty?
        if(rootStatement->Tokens
                        .Data
                        .MODC_TAGGED_FIELD_S(ModC_CompoundStatement)
                        .ChildStatements
                        .Length == 0)
        {
            return MODC_RESULT_VALUE_S(0);
        }
    }
    
    
    ModC_TypeEntry* rootTypeHashSet = NULL;
    ModC_TypeEntry* funcTypeHashSet = NULL;
    
    //TODO: Pull these from centralized place
    char* defaultTypes[] = 
    {
        "int", "int8", "int16", "int32", "uint", "uint8", "uint16", "uint32", 
        "char", "float", "double", "bool"
    };
    
    for(int i = 0; i < sizeof(defaultTypes) / sizeof(defaultTypes[0]); ++i)
    {
        ModC_TypeEntry* defaultTypeEntry = ModC_Allocator_Malloc(    &scratchAllocator, 
                                                                    sizeof(ModC_TypeEntry));
        defaultTypeEntry->Type = ModC_String_FromData(  scratchAllocator, 
                                                        defaultTypes[i], 
                                                        strlen(defaultTypes[i]));
        HASH_ADD_KEYPTR(hh, 
                        rootTypeHashSet, 
                        defaultTypeEntry->Type.Data, 
                        defaultTypeEntry->Type.Length,
                        defaultTypeEntry);
    }
    
    
    #undef uthash_malloc
    #undef uthash_free
    #define uthash_malloc(sz) ModC_Allocator_Malloc(&scratchAllocator, sz)
    #define uthash_free(ptr, sz) ModC_Allocator_Free(&scratchAllocator, ptr)
    
    MODC_DEFER_SCOPE_START(0)
    {
        MODC_DEFER(0,   if(!rootTypeHashSet)
                            HASH_CLEAR(hh, rootTypeHashSet);
                        if(!funcTypeHashSet)
                            HASH_CLEAR(hh, funcTypeHashSet));
        
        bool inTypeDecl = false;
        bool inFuncImpl = false;
        
        //Iterate all statements
        do
        {
            bool isEnd = false;
            ModC_Statement* statement = &statements->Data[0];
            ModC_Result_Uint32 uint32Result = ModC_Statement_Next(  statement,
                                                                    statements,
                                                                    currentStatementIndex,
                                                                    &isEnd);
            currentStatementIndex = *MODC_RESULT_TRY(uint32Result, MODC_RET_ERROR_S());
            if(isEnd)
                break;
            
            MODC_CHECK( statement->StatementType == ModC_StatementType_Compound ||
                        statement->StatementType == ModC_StatementType_Unknown, 
                        ("Unexpected statement type"),
                        MODC_DEFER_BREAK(0, MODC_RET_ERROR_S()));
            
            if(statement->StatementType != ModC_StatementType_Compound)
            {
                MODC_CHECK( statement->StatementType == ModC_StatementType_Unknown,
                            ("Unexpected statement type"),
                            MODC_RET_ERROR_S());
                
                ModC_Result_Void voidResult = ModC_Statement_Normalize( statement,
                                                                        tokensAllcoator,
                                                                        statementsArena,
                                                                        tokens,
                                                                        scratchAllocator);
                (void)MODC_RESULT_TRY(voidResult, MODC_DEFER_BREAK(0, MODC_RET_ERROR_S()));
                MODC_CHECK( ModC_StatementTokensUnion_GetTokenCount(&statement->Tokens) > 0, 
                            (""), 
                            MODC_DEFER_BREAK(0, MODC_RET_ERROR_S()));
                
                //Classify statements
                static_assert((int)ModC_StatementType_Count == 14, "");
                
                //Root
                if(!inTypeDecl && !inFuncImpl)
                {
                    //Check ModC_StatementType_TypeDeclaration
                    do
                    {
                        ModC_Token* curToken = NULL;
                        {
                            ModC_Result_TokenPtr tokenPtrResult =
                                ModC_StatementTokensUnion_GetTokenAt(&statement->Tokens, tokens, 0);
                            ModC_Token** tokenPtr = 
                                MODC_RESULT_TRY(tokenPtrResult, MODC_DEFER_BREAK(0, MODC_RET_ERROR_S()));
                            curToken = *tokenPtr;
                        }
                        
                        //Check next type if it doesn't start with struct or enum, for now
                        ModC_ConstStringView tokenTextView = ModC_Token_TokenTextView(curToken);
                        if( !ModC_ConstStringView_IsEqualLiteral(&tokenTextView, "struct") &&
                            !ModC_ConstStringView_IsEqualLiteral(&tokenTextView, "enum"))
                        {
                            break;
                        }
                        
                        
                        
                        
                    }
                    while(false);
                    
                    
                    //TODO(NOW)
                    
                    
                    //We can only have
                    //ModC_StatementType_FunctionDeclaration
                    //ModC_StatementType_VariableDeclaration
                    //ModC_StatementType_Compound
                    //ModC_StatementType_CompilerDirective
                }
                //Inside struct or enum (Which can be inside function impl as well)
                else if(inTypeDecl)
                {
                    
                }
                //Inside function impl
                else if(inFuncImpl)
                {
                    
                }
                
                //If we are not in a function implementation
                    //We can only have
                        //ModC_StatementType_TypeDeclaration
                        //ModC_StatementType_FunctionDeclaration
                        //ModC_StatementType_VariableDeclaration
                        //ModC_StatementType_Compound
                        //ModC_StatementType_CompilerDirective
                //Otherwise
                    //We can have
                        //ModC_StatementType_TypeDeclaration
                        //ModC_StatementType_VariableDeclaration
                        //ModC_StatementType_Compound
                        //ModC_StatementType_CompilerDirective
                        //ModC_StatementType_Assignment
                        //ModC_StatementType_VariableDeclareAssignment
                        //ModC_StatementType_PureExpression
                        //ModC_StatementType_IfStatement
                        //ModC_StatementType_ForStatement
                        //ModC_StatementType_WhileStatement
                        //ModC_StatementType_SwitchStatement
                
                
                
            } //if(statement->StatementType != ModC_StatementType_Compound)
            
            
        } //do
        while(true);
    }
    MODC_DEFER_SCOPE_END(0)
    
    return MODC_RESULT_VALUE_S(0);
}

#endif
