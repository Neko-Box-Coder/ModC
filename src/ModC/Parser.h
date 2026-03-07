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
    MODC_CHECK(statementList->Length != oldLength, ("Failed to allocate"), MODC_RET_ERROR_S());
    
    ModC_Statement* retStatementPtr = &statementList->Data[statementList->Length - 1];
    MODC_CHECK( retStatementPtr ->Value
                                .Data
                                .MODC_TAGGED_FIELD(ModC_StatementUnion, ModC_CompoundStatement)
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
                                    .Value = MODC_TAGGED_INIT(  ModC_StatementUnion, 
                                                                ModC_TokenIndexRange,
                                                                {0}),
                                    .Index = oldLength,
                                    .ParentIndex = parentIndex
                                });
    MODC_CHECK(statementList->Length != oldLength, ("Failed to allocate"), MODC_RET_ERROR_S());
    
    ModC_Statement* retStatementPtr = &statementList->Data[statementList->Length - 1];
    //MODC_CHECK( retStatementPtr->Value.Data.MODC_TAGGED_FIELD(  ModC_StatementUnion, 
    //                                                            ModC_TokenIndexList).Cap > 0,
    //            ("Failed to allocate"), 
    //            MODC_RET_ERROR_S());
    
    return MODC_RESULT_VALUE_S(retStatementPtr);
}

#if 0
    void ModC_Statement_Free(ModC_Statement* this)
    {
        #undef ModC_TaggedUnionName_State
        #define ModC_TaggedUnionName_State ModC_StatementUnion
        
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

static inline ModC_Result_Void ModC_AddStatementToParent(   uint32_t statementIndex, 
                                                            uint32_t currentParentIndex,
                                                            ModC_StatementList* statementList)
{
    #undef ModC_ResultName_State
    #define ModC_ResultName_State ModC_Result_Void
    #undef ModC_TaggedUnionName_State
    #define ModC_TaggedUnionName_State ModC_StatementUnion
    
    MODC_CHECK( statementList->Data[currentParentIndex].Value.Type ==
                MODC_TAGGED_TYPE_S(ModC_CompoundStatement),
                ("Expecting parent to be type compound, found type index %d instead",
                (int)statementList->Data[currentParentIndex].Value.Type),
                MODC_RET_ERROR_S());
    
    ModC_StatementIndexList* parentStatementList =
        &statementList  ->Data[currentParentIndex]
                        .Value
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
    #define ModC_TaggedUnionName_State ModC_StatementUnion
    
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
    MODC_CHECK( prevStatementPtr->Value.Type == MODC_TAGGED_TYPE_S(ModC_TokenIndexRange),
                (""),
                MODC_RET_ERROR_S());
    
    ModC_TokenIndexRange* indexRange =
        &prevStatementPtr->Value.Data.MODC_TAGGED_FIELD_S(ModC_TokenIndexRange);
    
    indexRange->StartIndex = *startTokenIndex;
    indexRange->EndIndex = countCurrentToken ? i + 1 : i;
    
    //TODO: Maybe not needed
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

    (void)MODC_RESULT_TRY(voidResult, MODC_RET_ERROR_S());
    return MODC_RESULT_VALUE_S(i);
}

static inline ModC_Result_StatementList ModC_CreateStatements(  const ModC_TokenList* tokens, 
                                                                ModC_Allocator* outStatementsArena)
{
    #undef ModC_ResultName_State
    #define ModC_ResultName_State ModC_Result_StatementList
    #undef ModC_TaggedUnionName_State
    #define ModC_TaggedUnionName_State ModC_StatementUnion
    
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
                MODC_CHECK( newStatement->Value.Type == MODC_TAGGED_TYPE_S(ModC_CompoundStatement),
                            ("Unexpected type"),
                            MODC_RET_ERROR_S());
                MODC_CHECK(i == startTokenIndex, (""), MODC_RET_ERROR_S());
                ModC_Result_Void voidResult = ModC_AddStatementToParent(statementList.Length - 1,
                                                                        currentParentIndex,
                                                                        &statementList);
                (void)MODC_RESULT_TRY(voidResult, MODC_RET_ERROR_S());
                
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
                i = *MODC_RESULT_TRY(uint32Result, MODC_RET_ERROR_S());
                
                //Finish compound parent
                ModC_Statement* parentStatement = &statementList.Data[currentParentIndex];
                MODC_CHECK( parentStatement->Value.Type == MODC_TAGGED_TYPE_S(ModC_CompoundStatement),
                            ("Unexpected type"),
                            MODC_RET_ERROR_S());
                
                ModC_CompoundStatement* parentCompound = 
                    &parentStatement->Value.Data.MODC_TAGGED_FIELD_S(ModC_CompoundStatement);
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
        i = *MODC_RESULT_TRY(uint32Result, MODC_RET_ERROR_S());
    }
    
    return MODC_RESULT_VALUE_S(statementList);
}

static inline ModC_Result_Void ModC_TryMergeOperators(  ModC_Statement* const statement,
                                                        ModC_Allocator tokensAllcoator,
                                                        ModC_Allocator statementsArena,
                                                        ModC_TokenList* tokens,
                                                        ModC_Allocator scratchAllocator)
{
    #undef ModC_ResultName_State
    #define ModC_ResultName_State ModC_Result_Void
    #undef ModC_TaggedUnionName_State
    #define ModC_TaggedUnionName_State ModC_StatementUnion
    
    MODC_CHECK( statement->Value.Type == MODC_TAGGED_TYPE_S(ModC_TokenIndexRange),
                ("Unexpected statement union type"),
                MODC_RET_ERROR_S());
    
    ModC_TokenIndexRange* indexRange = &statement   ->Value
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
            statement->Value = 
                MODC_TAGGED_INIT_S( ModC_TokenIndexList, 
                                    ModC_Uint32List_Create( statementsArena, 
                                                            tokenIndices.Length));
            ModC_Uint32List_AddRange(   &statement  ->Value
                                                    .Data
                                                    .MODC_TAGGED_FIELD_S(ModC_TokenIndexList),
                                        tokenIndices.Data,
                                        tokenIndices.Length);
        }
    }
    MODC_DEFER_SCOPE_END(0)
    
    return MODC_RESULT_VALUE_S(0);
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
    #define ModC_TaggedUnionName_State ModC_StatementUnion
    
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
        if(rootStatement->Value
                        .Data
                        .MODC_TAGGED_FIELD_S(ModC_CompoundStatement)
                        .ChildStatements
                        .Length == 0)
        {
            return MODC_RESULT_VALUE_S(0);
        }
        
        currentStatementIndex = rootStatement   ->Value
                                                .Data
                                                .MODC_TAGGED_FIELD_S(ModC_CompoundStatement)
                                                .ChildStatements
                                                .Data[0];
    }
    
    //Iterate all statements
    while(true)
    {
        ModC_Statement* statement = &statements->Data[currentStatementIndex];
        
        MODC_CHECK( statement->StatementType == ModC_StatementType_Compound ||
                    statement->StatementType == ModC_StatementType_Unknown, 
                    ("Unexpected statement type"),
                    MODC_RET_ERROR_S());
        
        //If compound, go to first child if any
        if( statement->StatementType == ModC_StatementType_Compound &&
            statement   ->Value
                        .Data
                        .MODC_TAGGED_FIELD_S(ModC_CompoundStatement)
                        .ChildStatements
                        .Length != 0)
        {
            currentStatementIndex = statement   ->Value
                                                .Data
                                                .MODC_TAGGED_FIELD_S(ModC_CompoundStatement)
                                                .ChildStatements
                                                .Data[0];
            continue;
        }
        
        if(statement->StatementType != ModC_StatementType_Compound)
        {
            ModC_Result_Void voidResult = ModC_TryMergeOperators(   statement,
                                                                    tokensAllcoator,
                                                                    statementsArena,
                                                                    tokens,
                                                                    scratchAllocator);
            (void)MODC_RESULT_TRY(voidResult, MODC_RET_ERROR_S());
            
            //TODO(NOW): Classify statements
            
            
        } //if(statement->StatementType != ModC_StatementType_Compound)
        
        //Traverse to the next statement
        uint32_t origStatementIndex = currentStatementIndex;
        while(currentStatementIndex == origStatementIndex)
        {
            ModC_Statement* parentStatement = &statements->Data[statement->ParentIndex];
            MODC_CHECK( parentStatement->StatementType == ModC_StatementType_Compound, 
                        (""), 
                        MODC_RET_ERROR_S());
            
            ModC_CompoundStatement* parentCompound = 
                &parentStatement->Value.Data.MODC_TAGGED_FIELD_S(ModC_CompoundStatement);
            
            uint32_t childIndex = ModC_Uint32List_Find( &parentCompound->ChildStatements, 
                                                        &statement->Index);
            MODC_CHECK(childIndex != parentCompound->ChildStatements.Length, (""), MODC_RET_ERROR_S());
            
            //Continue to the next child if we are not at the end
            if(childIndex < parentCompound->ChildStatements.Length - 1)
                currentStatementIndex = parentCompound->ChildStatements.Data[childIndex + 1];
            //Otherwise go up if we are not under root
            else if(parentStatement->Index != parentStatement->ParentIndex)
                statement = parentStatement;
            //Otherwise, we are done
            else
                return MODC_RESULT_VALUE_S(0);
        }
        
    } //While(true)
    
    return MODC_ERROR_CSTR_S("Unreachable reached...");
}

#endif
