#ifndef MODC_PARSER_H
#define MODC_PARSER_H

#ifndef MODC_DEFAULT_ALLOC
    #define MODC_DEFAULT_ALLOC() ModC_CreateHeapAllocator()
#endif

#include "ModC/Tokenization.h"
#include "ModC/GenericContainers.h"
#include "ModC/Result.h"
#include "ModC/Keyword.h"

typedef enum ModC_StatementType
{
    ModC_StatementType_Invalid,
    ModC_StatementType_Unknown,
    ModC_StatementType_TypeDeclaration,
    ModC_StatementType_VariableDeclaration,
    ModC_StatementType_Logic,
    ModC_StatementType_Compound,
    ModC_StatementType_CompilerDirective,
    ModC_StatementType_Count,
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

#define MODC_TAGGED_UNION_NAME ModC_StatementUnion
#define MODC_VALUE_TYPES ModC_CompoundStatement,ModC_TokenIndexList,ModC_TokenIndexRange
#include "ModC/TaggedUnion.h"

typedef struct ModC_Statement ModC_Statement;
struct ModC_Statement
{
    ModC_StatementType StatementType;
    ModC_StatementUnion Value;
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

#include "static_assert.h/assert.h"

#include <stdbool.h>
#include <stdint.h>





ModC_ConstStringView ModC_StatementType_ToConstStringView(ModC_StatementType type)
{
    static_assert((int)ModC_StatementType_Count == 7, "");
    switch(type)
    {
        case ModC_StatementType_Invalid:
            return ModC_ConstStringView_FromLiteral("ModC_StatementType_Invalid");
        case ModC_StatementType_Unknown:
            return ModC_ConstStringView_FromLiteral("ModC_StatementType_Unknown");
        case ModC_StatementType_TypeDeclaration:
            return ModC_ConstStringView_FromLiteral("ModC_StatementType_TypeDeclaration");
        case ModC_StatementType_VariableDeclaration:
            return ModC_ConstStringView_FromLiteral("ModC_StatementType_VariableDeclaration");
        case ModC_StatementType_Logic:
            return ModC_ConstStringView_FromLiteral("ModC_StatementType_Logic");
        case ModC_StatementType_Compound:
            return ModC_ConstStringView_FromLiteral("ModC_StatementType_Compound");
        case ModC_StatementType_CompilerDirective:
            return ModC_ConstStringView_FromLiteral("ModC_StatementType_CompilerDirective");
        case ModC_StatementType_Count:
            return ModC_ConstStringView_FromLiteral("ModC_StatementType_Count");
    }
    return (ModC_ConstStringView){0};
}


#undef ModC_ResultName
#define ModC_ResultName ModC_ResultStatementPtr
ModC_ResultStatementPtr ModC_Statement_CreateCompound(  ModC_Allocator allocator, 
                                                        ModC_StatementList* statementList,
                                                        uint32_t parentIndex,
                                                        bool implicit,
                                                        uint32_t reserveStatementsCount)
{
    MODC_CHECK(statementList != NULL, (""), MODC_RET_ERROR());
    MODC_CHECK(allocator.Type == ModC_AllocatorType_SharedArena, (""), MODC_RET_ERROR());
    
    uint64_t oldLength = statementList->Length;
    ModC_StatementList_AddValue(statementList, 
                                (ModC_Statement)
                                {
                                    .StatementType = ModC_StatementType_Compound,
                                    .Value =    
                                        MODC_TAGGED_INIT
                                        (
                                            ModC_StatementUnion, 
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
    MODC_CHECK(statementList->Length != oldLength, ("Failed to allocate"), MODC_RET_ERROR());
    
    ModC_Statement* retStatementPtr = &statementList->Data[statementList->Length - 1];
    MODC_CHECK( retStatementPtr ->Value
                                .Data
                                .MODC_TAGGED_FIELD(ModC_StatementUnion, ModC_CompoundStatement)
                                .ChildStatements
                                .Cap > 0,
                ("Failed to allocate"), 
                MODC_RET_ERROR());
    
    return MODC_RESULT_VALUE_S(retStatementPtr);
}

#undef ModC_ResultName
#define ModC_ResultName ModC_ResultStatementPtr
ModC_ResultStatementPtr ModC_Statement_CreatePlain( ModC_Allocator allocator, 
                                                    ModC_StatementList* statementList,
                                                    uint32_t parentIndex)
{
    MODC_CHECK(statementList != NULL, (""), MODC_RET_ERROR());
    MODC_CHECK(allocator.Type == ModC_AllocatorType_SharedArena, (""), MODC_RET_ERROR());
    
    uint64_t oldLength = statementList->Length;
    ModC_StatementList_AddValue(statementList, 
                                (ModC_Statement)
                                {
                                    .StatementType = ModC_StatementType_Unknown,
                                    .Value = MODC_TAGGED_INIT(  ModC_StatementUnion, 
                                                                ModC_TokenIndexRange,
                                                                {0}),
                                    .Index = oldLength,
                                    .ParentIndex = parentIndex
                                });
    MODC_CHECK(statementList->Length != oldLength, ("Failed to allocate"), MODC_RET_ERROR());
    
    ModC_Statement* retStatementPtr = &statementList->Data[statementList->Length - 1];
    //MODC_CHECK( retStatementPtr->Value.Data.MODC_TAGGED_FIELD(  ModC_StatementUnion, 
    //                                                            ModC_TokenIndexList).Cap > 0,
    //            ("Failed to allocate"), 
    //            MODC_RET_ERROR());
    
    return MODC_RESULT_VALUE_S(retStatementPtr);
}

#if 0
    #undef ModC_TaggedUnionName
    #define ModC_TaggedUnionName ModC_StatementUnion
    void ModC_Statement_Free(ModC_Statement* this)
    {
        if(!this)
            return;
        
        //No children, just free the token list
        if(this->Value.Type == MODC_TAGGED_TYPE_S(TokenIndexList))
        {
            ModC_Uint32List_Free(&this->Value.Data.MODC_TAGGED_FIELD_S(TokenIndexList));
            *this = (ModC_Statement){0};
            return;
        }
        
        //Get to the deepest child
        ModC_Statement* currentStatement = this;
        while(currentStatement->Value.Type == MODC_TAGGED_TYPE_S(ModC_ChildStatements))
        {
            ModC_ChildStatements* currentChildren =
                &currentStatement->Value.Data.MODC_TAGGED_FIELD_S(ModC_ChildStatements);
            
            if(currentChildren->Statements->Length == 0)
                break;
            
            currentStatement = &currentChildren->Statements->Data[0];
        }
        
        //Walk and free the whole tree
    }
#endif

#undef ModC_ResultName
#define ModC_ResultName ModC_Result_Void
#undef ModC_TaggedUnionName
#define ModC_TaggedUnionName ModC_StatementUnion
static inline ModC_Result_Void ModC_AddStatementToParent(   uint32_t statementIndex, 
                                                            uint32_t currentParentIndex,
                                                            ModC_StatementList* statementList)
{
    MODC_CHECK( statementList->Data[currentParentIndex].Value.Type ==
                MODC_TAGGED_TYPE_S(ModC_CompoundStatement),
                ("Expecting parent to be type compound, found type index %d instead",
                (int)statementList->Data[currentParentIndex].Value.Type),
                MODC_RET_ERROR());
    
    ModC_StatementIndexList* parentStatementList =
        &statementList  ->Data[currentParentIndex]
                        .Value
                        .Data
                        .MODC_TAGGED_FIELD_S(ModC_CompoundStatement)
                        .ChildStatements;
    ModC_Uint32List_AddValue(parentStatementList, statementIndex);
    
    return MODC_RESULT_VALUE_S(0);
}


#undef ModC_ResultName
#define ModC_ResultName ModC_Result_Uint32
#undef ModC_TaggedUnionName
#define ModC_TaggedUnionName ModC_StatementUnion
static inline ModC_Result_Uint32 ModC_EndCurrentStatment(   bool countCurrentToken, 
                                                            uint32_t i, 
                                                            uint32_t currentParentIndex,
                                                            ModC_Allocator sharedArena,
                                                            const ModC_TokenList* tokens,
                                                            uint32_t* startTokenIndex,
                                                            ModC_StatementList* statementList)
{
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
    ModC_Statement* prevStatementPtr = *MODC_RESULT_TRY(statementPtrResult, MODC_RET_ERROR());
    MODC_CHECK( prevStatementPtr->Value.Type == MODC_TAGGED_TYPE_S(ModC_TokenIndexRange),
                (""),
                MODC_RET_ERROR());
    
    ModC_TokenIndexRange* indexRange =
        &prevStatementPtr->Value.Data.MODC_TAGGED_FIELD_S(ModC_TokenIndexRange);
    
    indexRange->StartIndex = *startTokenIndex;
    indexRange->EndIndex = countCurrentToken ? i + 1 : i;
    
    //Trim newlines, spaces and comments
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

    (void)MODC_RESULT_TRY(voidResult, MODC_RET_ERROR());
    return MODC_RESULT_VALUE_S(i);
}

#undef ModC_ResultName
#define ModC_ResultName ModC_Result_StatementList
#undef ModC_TaggedUnionName
#define ModC_TaggedUnionName ModC_StatementUnion
static inline ModC_Result_StatementList ModC_CreateStatements(  const ModC_TokenList* tokens, 
                                                                ModC_Allocator* outArenaAllocator)
{
    MODC_CHECK(tokens != NULL, (""), MODC_RET_ERROR());
    MODC_CHECK(outArenaAllocator != NULL, (""), MODC_RET_ERROR());
    
    *outArenaAllocator = ModC_CreateArenaAllocator(1024);   //TODO: Proper reserve count
    ModC_Allocator sharedArena = ModC_Allocator_Share(outArenaAllocator);
    
    //TODO: Proper reserve count
    ModC_StatementList statementList = ModC_StatementList_Create(*outArenaAllocator, 16);
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
                i = *MODC_RESULT_TRY(uint32Result, MODC_RET_ERROR());
                
                //Create compound as parent
                statementPtrResult = ModC_Statement_CreateCompound( sharedArena, 
                                                                    &statementList, 
                                                                    currentParentIndex,
                                                                    false,
                                                                    32);
                ModC_Statement* newStatement = *MODC_RESULT_TRY(statementPtrResult, MODC_RET_ERROR());
                MODC_CHECK( newStatement->Value.Type == MODC_TAGGED_TYPE_S(ModC_CompoundStatement),
                            ("Unexpected type"),
                            MODC_RET_ERROR());
                MODC_CHECK(i == startTokenIndex, (""), MODC_RET_ERROR());
                ModC_Result_Void voidResult = ModC_AddStatementToParent(statementList.Length - 1,
                                                                        currentParentIndex,
                                                                        &statementList);
                (void)MODC_RESULT_TRY(voidResult, MODC_RET_ERROR());
                
                ModC_CompoundStatement* compoundData = 
                    &newStatement->Value.Data.MODC_TAGGED_FIELD_S(ModC_CompoundStatement);
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
                i = *MODC_RESULT_TRY(uint32Result, MODC_RET_ERROR());
                
                //Finish compound parent
                ModC_Statement* parentStatement = &statementList.Data[currentParentIndex];
                MODC_CHECK( parentStatement->Value.Type == MODC_TAGGED_TYPE_S(ModC_CompoundStatement),
                            ("Unexpected type"),
                            MODC_RET_ERROR());
                
                ModC_CompoundStatement* parentCompound = 
                    &parentStatement->Value.Data.MODC_TAGGED_FIELD_S(ModC_CompoundStatement);
                MODC_CHECK( !parentCompound->Implicit,
                            ("Expected non implicit for parent when block end"),
                            MODC_RET_ERROR());
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
                    i = *MODC_RESULT_TRY(uint32Result, MODC_RET_ERROR());
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
                i = *MODC_RESULT_TRY(uint32Result, MODC_RET_ERROR());
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
                    i = *MODC_RESULT_TRY(uint32Result, MODC_RET_ERROR());
                    //We know it is a compiler directive statement, but we will classify it later.
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
        i = *MODC_RESULT_TRY(uint32Result, MODC_RET_ERROR());
    }
    
    return MODC_RESULT_VALUE_S(statementList);
}

static inline void ModC_ParseTokens(const ModC_TokenList* tokens)
{
    (void)tokens;
    
    
    
}

#endif
