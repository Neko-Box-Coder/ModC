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
    ModC_StatementType_ElseStatement,
    ModC_StatementType_ForStatement,
    ModC_StatementType_WhileStatement,
    ModC_StatementType_SwitchStatement,
    ModC_StatementType_ReturnStatement,
    ModC_StatementType_Count,   //16
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

typedef struct ModC_TypeDeclarationInfo
{
    enum
    {
        ModC_Type_Struct,
        ModC_Type_Enum
    } Type;
    
    ModC_String TypeName;   //TODO: Use token index instead
} ModC_TypeDeclarationInfo;

typedef struct ModC_VariableDeclareAssignInfo
{
    uint32_t TypeIndexInStatement;
    uint32_t IdentifierIndexInStatement;
    bool HasAsignment;
    uint32_t AssignIndexInStatement;
} ModC_VariableDeclareAssignInfo;

typedef struct ModC_FunctionDeclarationInfo
{
    uint32_t TypeIndexInStatement;
    uint32_t IdentifierIndexInStatement;
    bool HaveArguments;
    uint32_t ArgumentIndexInStatement;
} ModC_FunctionDeclarationInfo;


#define MODC_TAGGED_UNION_NAME ModC_StatementInfoUnion
#define MODC_VALUE_TYPES    ModC_Void, \
                            ModC_TypeDeclarationInfo, \
                            ModC_VariableDeclareAssignInfo, \
                            ModC_FunctionDeclarationInfo
#include "ModC/TaggedUnion.h"

typedef struct ModC_Statement ModC_Statement;
struct ModC_Statement
{
    ModC_StatementType StatementType;
    ModC_StatementTokensUnion Tokens;
    ModC_StatementInfoUnion Info;
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

MODC_DEFINE_RESULT_STRUCT(ModC_Result_ConstStringView, ModC_ConstStringView)


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
        case MODC_TAG_TYPE_S(ModC_CompoundStatement):
            return 2;
        case MODC_TAG_TYPE_S(ModC_TokenIndexList):
            return statementUnion->MODC_TAG_DATA_S(ModC_TokenIndexList).Length;
        case MODC_TAG_TYPE_S(ModC_TokenIndexRange):
        {
            const ModC_TokenIndexRange* range = 
                &statementUnion->MODC_TAG_DATA_S(ModC_TokenIndexRange);
            return range->EndIndex - range->StartIndex;
        }
        default:
            return 0;
    }
}

ModC_Result_Uint32
ModC_StatementTokensUnion_GetTokenIndexAt(  const ModC_StatementTokensUnion* statementUnion, 
                                            const ModC_TokenList* tokens,
                                            uint32_t indexInStatement)
{
    #undef ModC_ResultName_State
    #define ModC_ResultName_State ModC_Result_Uint32
    #undef ModC_TaggedUnionName_State
    #define ModC_TaggedUnionName_State ModC_StatementTokensUnion
    
    MODC_CHECK(statementUnion != NULL, (""), MODC_RET_ERROR_S());
    MODC_CHECK(tokens != NULL, (""), MODC_RET_ERROR_S());
    
    uint32_t tokenIndex = 0;
    switch(statementUnion->Type)
    {
        case MODC_TAG_TYPE_S(ModC_CompoundStatement):
        {
            //NOTE: Shouldn't use this function for compound statement..., but whatever
            const ModC_CompoundStatement* compound = 
                &statementUnion->MODC_TAG_DATA_S(ModC_CompoundStatement);
            if(indexInStatement == 0)
                tokenIndex = compound->StartTokenIndex;
            else if(indexInStatement == 1)
                tokenIndex = compound->EndTokenIndex;
            else
                return MODC_ERROR_STR_FMT_S("Invalid index for accessing %"PRIu32, indexInStatement);
            break;
        }
        case MODC_TAG_TYPE_S(ModC_TokenIndexList):
        {
            const ModC_TokenIndexList* tokenIndexList = 
                &statementUnion->MODC_TAG_DATA_S(ModC_TokenIndexList);
            
            MODC_CHECK(tokenIndexList->Length > 0, ("Empty statement"), MODC_RET_ERROR_S());
            MODC_CHECK( indexInStatement < tokenIndexList->Length, 
                        ("Invalid index for accessing, index: %"PRIu32", length: %"PRIu64, 
                        indexInStatement, tokenIndexList->Length),
                        MODC_RET_ERROR_S());
            tokenIndex = tokenIndexList->Data[indexInStatement];
            break;
        }
        case MODC_TAG_TYPE_S(ModC_TokenIndexRange):
        {
            const ModC_TokenIndexRange* range = 
                &statementUnion->MODC_TAG_DATA_S(ModC_TokenIndexRange);
            
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
    return MODC_RESULT_VALUE_S(tokenIndex);
}

ModC_Result_TokenPtr 
ModC_StatementTokensUnion_GetTokenAt(   const ModC_StatementTokensUnion* statementUnion, 
                                        const ModC_TokenList* tokens,
                                        uint32_t indexInStatement)
{
    #undef ModC_ResultName_State
    #define ModC_ResultName_State ModC_Result_TokenPtr
    
    ModC_Result_Uint32 uint32Result = ModC_StatementTokensUnion_GetTokenIndexAt(statementUnion, 
                                                                                tokens, 
                                                                                indexInStatement);
    uint32_t tokenIndex = *MODC_RESULT_TRY(uint32Result, MODC_RET_ERROR_S());
    return MODC_RESULT_VALUE_S(&tokens->Data[tokenIndex]);
}

ModC_Result_ConstStringView 
ModC_StatementTokensUnion_GetTokenTextViewAt(   const ModC_StatementTokensUnion* statementUnion, 
                                                const ModC_TokenList* tokens,
                                                uint32_t indexInStatement)
{
    #undef ModC_ResultName_State
    #define ModC_ResultName_State ModC_Result_ConstStringView 
    
    ModC_Token* token = NULL;
    {
        ModC_Result_TokenPtr tokenPtrResult = ModC_StatementTokensUnion_GetTokenAt( statementUnion, 
                                                                                    tokens, 
                                                                                    indexInStatement);
        token = *MODC_RESULT_TRY(tokenPtrResult, MODC_RET_ERROR_S());
    }
    
    return MODC_RESULT_VALUE_S(ModC_Token_TokenTextView(token));
}


ModC_Result_Uint32
ModC_StatementTokensUnion_ContainsTokenText(const ModC_StatementTokensUnion* statementUnion, 
                                            const ModC_TokenList* tokens,
                                            ModC_ConstStringView checkText)
{
    #undef ModC_ResultName_State
    #define ModC_ResultName_State ModC_Result_Uint32
    
    uint32_t tokensCount = ModC_StatementTokensUnion_GetTokenCount(statementUnion);
    for(uint32_t i = 0; i < tokensCount; ++i)
    {
        ModC_Result_ConstStringView constStringViewResult =
            ModC_StatementTokensUnion_GetTokenTextViewAt(statementUnion, tokens, i);
        
        ModC_ConstStringView tokenStringView = *MODC_RESULT_TRY(constStringViewResult, 
                                                                MODC_RET_ERROR_S());
        if(ModC_StringLikeEqual(tokenStringView, checkText))
            return MODC_RESULT_VALUE_S(i);
    }
    
    return MODC_RESULT_VALUE_S(tokensCount);
}

ModC_ConstStringView ModC_StatementType_ToConstStringView(ModC_StatementType type)
{
    static_assert((int)ModC_StatementType_Count == 16, "");
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
        RET_TO_STR(ModC_StatementType_ElseStatement);
        RET_TO_STR(ModC_StatementType_ForStatement);
        RET_TO_STR(ModC_StatementType_WhileStatement);
        RET_TO_STR(ModC_StatementType_SwitchStatement);
        RET_TO_STR(ModC_StatementType_ReturnStatement);
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
    #undef ModC_TaggedUnionName_State
    #define ModC_TaggedUnionName_State ModC_StatementTokensUnion
    
    MODC_CHECK(statementList != NULL, (""), MODC_RET_ERROR_S());
    MODC_CHECK(allocator.Type == ModC_AllocatorType_SharedArena, (""), MODC_RET_ERROR_S());
    
    uint64_t oldLength = statementList->Length;
    ModC_StatementIndexList childStatements = ModC_Uint32List_Create(   allocator, 
                                                                        reserveStatementsCount);
    MODC_CHECK(childStatements.Cap > 0, ("Failed to allocate"), MODC_RET_ERROR_S());
    ModC_StatementList_AddValue(statementList, 
                                (ModC_Statement)
                                {
                                    .StatementType = ModC_StatementType_Compound,
                                    .Tokens = 
                                        MODC_TAG_INIT_S(ModC_CompoundStatement, 
                                                        {
                                                            .StartTokenIndex = 0,
                                                            .ChildStatements = childStatements,
                                                            .EndTokenIndex = 0,
                                                            .Implicit = implicit
                                                        }
                                        ),
                                    .Info = MODC_TAG_INIT(ModC_StatementInfoUnion, ModC_Void, 0),
                                    .Index = oldLength,
                                    .ParentIndex = parentIndex
                                });
    childStatements = (ModC_StatementIndexList){0};
    MODC_CHECK(statementList->Length != oldLength, ("Failed to allocate"), MODC_RET_ERROR_S());
    
    ModC_Statement* retStatementPtr = &statementList->Data[statementList->Length - 1];
    return MODC_RESULT_VALUE_S(retStatementPtr);
}

ModC_ResultStatementPtr ModC_Statement_CreatePlain( ModC_Allocator allocator, 
                                                    ModC_StatementList* statementList,
                                                    uint32_t parentIndex)
{
    #undef ModC_ResultName_State
    #define ModC_ResultName_State ModC_ResultStatementPtr
    #undef ModC_TaggedUnionName_State
    #define ModC_TaggedUnionName_State ModC_StatementTokensUnion
    
    MODC_CHECK(statementList != NULL, (""), MODC_RET_ERROR_S());
    MODC_CHECK(allocator.Type == ModC_AllocatorType_SharedArena, (""), MODC_RET_ERROR_S());
    
    uint64_t oldLength = statementList->Length;
    ModC_StatementList_AddValue(statementList, 
                                (ModC_Statement)
                                {
                                    .StatementType = ModC_StatementType_Unknown,
                                    .Tokens = MODC_TAG_INIT_S(ModC_TokenIndexRange, {0}),
                                    .Info = MODC_TAG_INIT(ModC_StatementInfoUnion, ModC_Void, 0),
                                    .Index = oldLength,
                                    .ParentIndex = parentIndex
                                });
    MODC_CHECK(statementList->Length != oldLength, ("Failed to allocate"), MODC_RET_ERROR_S());
    
    ModC_Statement* retStatementPtr = &statementList->Data[statementList->Length - 1];
    //MODC_CHECK( retStatementPtr->Tokens.MODC_TAG_DATA(  ModC_StatementTokensUnion, 
    //                                                    ModC_TokenIndexList).Cap > 0,
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
        if(this->Tokens.Type == MODC_TAG_TYPE_S(TokenIndexList))
        {
            ModC_Uint32List_Free(&this->Tokens.MODC_TAG_DATA_S(TokenIndexList));
            *this = (ModC_Statement){0};
            return;
        }
        
        //Get to the deepest child
        ModC_Statement* currentStatement = this;
        while(currentStatement->Tokens.Type == MODC_TAG_TYPE_S(ModC_ChildStatements))
        {
            ModC_ChildStatements* currentChildren =
                &currentStatement->Tokens.MODC_TAG_DATA_S(ModC_ChildStatements);
            
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
                MODC_TAG_TYPE_S(ModC_CompoundStatement),
                ("Expecting parent to be type compound, found type index %d instead",
                (int)statementList->Data[currentParentIndex].Tokens.Type),
                MODC_RET_ERROR_S());
    
    ModC_StatementIndexList* parentStatementList =
        &statementList  ->Data[currentParentIndex]
                        .Tokens
                        .MODC_TAG_DATA_S(ModC_CompoundStatement)
                        .ChildStatements;
    ModC_Uint32List_AddValue(parentStatementList, statementIndex);
    
    return MODC_RESULT_VALUE_S(0);
}


static inline ModC_Result_Uint32 ModC_EndCurrentStatement(  bool countCurrentToken, 
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
    MODC_CHECK( prevStatementPtr->Tokens.Type == MODC_TAG_TYPE_S(ModC_TokenIndexRange),
                (""),
                MODC_RET_ERROR_S());
    
    ModC_TokenIndexRange* indexRange = &prevStatementPtr->Tokens.MODC_TAG_DATA_S(ModC_TokenIndexRange);
    
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
    
    #define END_CURRENT_STATEMENT(countCurrentToken) \
        do \
        { \
            ModC_Result_Uint32 uint32Result = ModC_EndCurrentStatement( countCurrentToken, \
                                                                        i, \
                                                                        currentParentIndex, \
                                                                        sharedArena, \
                                                                        tokens, \
                                                                        &startTokenIndex, \
                                                                        &statementList); \
            i = *MODC_RESULT_TRY(uint32Result, MODC_RET_ERROR_S()); \
        } \
        while(false)
    
    uint32_t startTokenIndex = 0;
    uint32_t currentParentIndex = 0;
    for(uint32_t i = 0; i < tokens->Length; ++i)
    {
        static_assert(ModC_TokenType_Count == 19, "");
        switch((ModC_CharTokenType)tokens->Data[i].TokenType)
        {
            case ModC_CharTokenType_Identifier:
            {
                /*
                Special case for "else". Since it can be like this
                ```
                else
                    ...
                ```
                where it can be mixed in with normal statement
                */
                ModC_ConstStringView tokenView = ModC_Token_TokenTextView(&tokens->Data[i]);
                if(!ModC_ConstStringView_IsEqualLiteral(&tokenView, "else"))
                    break;
                
                if(startTokenIndex != i)
                    END_CURRENT_STATEMENT(false);
                
                END_CURRENT_STATEMENT(true);
                break;
            }
            case ModC_CharTokenType_Operator:
                break;
            case ModC_CharTokenType_BlockStart:
            {
                END_CURRENT_STATEMENT(false);
                
                //Create compound as parent
                statementPtrResult = ModC_Statement_CreateCompound( sharedArena, 
                                                                    &statementList, 
                                                                    currentParentIndex,
                                                                    false,
                                                                    32);
                ModC_Statement* newStatement = *MODC_RESULT_TRY(statementPtrResult, MODC_RET_ERROR_S());
                MODC_CHECK( newStatement->Tokens.Type == MODC_TAG_TYPE_S(ModC_CompoundStatement),
                            ("Unexpected type"),
                            MODC_RET_ERROR_S());
                MODC_CHECK(i == startTokenIndex, (""), MODC_RET_ERROR_S());
                ModC_Result_Void voidResult = ModC_AddStatementToParent(statementList.Length - 1,
                                                                        currentParentIndex,
                                                                        &statementList);
                (void)MODC_RESULT_TRY(voidResult, MODC_RET_ERROR_S());
                
                ModC_CompoundStatement* compoundData = 
                    &newStatement->Tokens.MODC_TAG_DATA_S(ModC_CompoundStatement);
                compoundData->StartTokenIndex = i;
                startTokenIndex = i + 1;
                currentParentIndex = statementList.Length - 1;
                break;
            }
            case ModC_CharTokenType_BlockEnd:
            {
                END_CURRENT_STATEMENT(false);
                
                //Finish compound parent
                ModC_Statement* parentStatement = &statementList.Data[currentParentIndex];
                MODC_CHECK( parentStatement->Tokens.Type == MODC_TAG_TYPE_S(ModC_CompoundStatement),
                            ("Unexpected type"),
                            MODC_RET_ERROR_S());
                
                ModC_CompoundStatement* parentCompound = 
                    &parentStatement->Tokens.MODC_TAG_DATA_S(ModC_CompoundStatement);
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
                    END_CURRENT_STATEMENT(true);
                break;
            }
            case ModC_CharTokenType_Semicolon:
            {
                END_CURRENT_STATEMENT(true);
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
                    END_CURRENT_STATEMENT(true);
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
        END_CURRENT_STATEMENT(true);
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
    
    MODC_CHECK( statement->Tokens.Type != MODC_TAG_TYPE_S(ModC_CompoundStatement),
                ("Unexpected statement union type"),
                MODC_RET_ERROR_S());
        
    ModC_TokenIndexList tokenIndices;
    ModC_String tempMergedOperator;
    
    MODC_DEFER_SCOPE_START(0)
    {
        uint32_t tokensCount = ModC_StatementTokensUnion_GetTokenCount(&statement->Tokens);
        tokenIndices = ModC_Uint32List_Create(scratchAllocator, tokensCount);
        MODC_DEFER(0, ModC_Uint32List_Free(&tokenIndices));
        
        tempMergedOperator = ModC_String_Create(scratchAllocator, 3);
        MODC_DEFER(0, ModC_String_Free(&tempMergedOperator));
        
        uint32_t minLookBack = 0;
        bool skipped = false;
        for(uint32_t i = 0; i < tokensCount; ++i)
        {
            ModC_Result_TokenPtr tokenPtrResult = 
                ModC_StatementTokensUnion_GetTokenAt(&statement->Tokens, tokens, i);
            ModC_Token* currentToken = *MODC_RESULT_TRY(tokenPtrResult, 
                                                        MODC_DEFER_BREAK(0, MODC_RET_ERROR_S()));
            
            if(currentToken->TokenType == ModC_TokenType_Operator)
            {
                bool operatorNext = false;
                if(i != tokensCount - 1)
                {
                    tokenPtrResult = ModC_StatementTokensUnion_GetTokenAt(  &statement->Tokens, 
                                                                            tokens, 
                                                                            i + 1);
                    ModC_Token* nextToken = *MODC_RESULT_TRY(   tokenPtrResult, 
                                                                MODC_DEFER_BREAK(0, MODC_RET_ERROR_S()));
                    operatorNext = nextToken->TokenType == ModC_TokenType_Operator;
                }
                
                //If we reached the end of continuous operator tokens,
                //merge operators tokens into a single token if possible
                if(!operatorNext && minLookBack != i)
                {
                    MODC_CHECK(minLookBack < i, (""), MODC_DEFER_BREAK(0, MODC_RET_ERROR_S()));
                    
                    ModC_String_Resize(&tempMergedOperator, 0);
                    for(uint32_t j = minLookBack; j <= i; ++j)
                    {
                        tokenPtrResult = ModC_StatementTokensUnion_GetTokenAt(  &statement->Tokens, 
                                                                                tokens, 
                                                                                j);
                        ModC_Token* lookBackToken = 
                            *MODC_RESULT_TRY(tokenPtrResult, MODC_DEFER_BREAK(0, MODC_RET_ERROR_S()));
                        
                        MODC_CHECK( lookBackToken->TokenType == ModC_TokenType_Operator, 
                                    (""), 
                                    MODC_DEFER_BREAK(0, MODC_RET_ERROR_S()));
                        
                        ModC_ConstStringView opChar = ModC_Token_TokenTextView(lookBackToken);
                        MODC_CHECK( opChar.Length == 1, 
                                    ("Unexpected operator text length: %"PRIu32, 
                                    opChar.Length),
                                    MODC_DEFER_BREAK(0, MODC_RET_ERROR_S()));
                        
                        ModC_String_AddValue(&tempMergedOperator, opChar.Data[0]);
                    }
                    
                    //If the merged tokens are valid, modify the first (continuous) token to be merged
                    ModC_ConstStringView mergedView = 
                        ModC_ConstStringView_Create(tempMergedOperator.Data, 
                                                    tempMergedOperator.Length);
                    if(ModC_IsValidComplexOperator(mergedView))
                    {
                        skipped = true;
                        tokenPtrResult = ModC_StatementTokensUnion_GetTokenAt(  &statement->Tokens, 
                                                                                tokens, 
                                                                                minLookBack);
                        ModC_Token* minLookBackToken = 
                            *MODC_RESULT_TRY(tokenPtrResult, MODC_DEFER_BREAK(0, MODC_RET_ERROR_S()));
                        
                        //Modify current token to concatenated operator string
                        if(currentToken->TokenText.Type == MODC_TAG_TYPE(   ModC_StringUnion, 
                                                                            ModC_String))
                        {
                            ModC_String* tokenStr = 
                                &minLookBackToken->TokenText.MODC_TAG_DATA( ModC_StringUnion, 
                                                                            ModC_String);
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
                            minLookBackToken->TokenText = MODC_TAG_INIT(ModC_StringUnion, 
                                                                        ModC_String, 
                                                                        tokenStr);
                        }
                        
                        ModC_Result_Uint32 uint32Result = 
                            ModC_StatementTokensUnion_GetTokenIndexAt(  &statement->Tokens,
                                                                        tokens,
                                                                        minLookBack);
                        uint32_t minLookBackTokenIndex = 
                            *MODC_RESULT_TRY(uint32Result, MODC_DEFER_BREAK(0, MODC_RET_ERROR_S()));
                        
                        ModC_Uint32List_AddValue(&tokenIndices, minLookBackTokenIndex);
                    }
                    //Otherwise just add the tokens as they are
                    else
                    {
                        for(uint32_t j = minLookBack; j <= i; ++j)
                        {
                            tokenPtrResult = 
                                ModC_StatementTokensUnion_GetTokenAt(&statement->Tokens, tokens, j);
                            ModC_Result_Uint32 uint32Result = 
                                ModC_StatementTokensUnion_GetTokenIndexAt(  &statement->Tokens, 
                                                                            tokens, 
                                                                            j);
                            uint32_t lookBackTokenIndex = 
                                *MODC_RESULT_TRY(uint32Result, MODC_DEFER_BREAK(0, MODC_RET_ERROR_S()));
                            ModC_Uint32List_AddValue(&tokenIndices, lookBackTokenIndex);
                        }
                    }
                } //if(!operatorNext && minLookBack != i)
                else if(!operatorNext)
                {
                    ModC_Result_Uint32 uint32Result = 
                        ModC_StatementTokensUnion_GetTokenIndexAt(&statement->Tokens, tokens, i);
                    uint32_t currentTokenIndex = 
                        *MODC_RESULT_TRY(uint32Result, MODC_DEFER_BREAK(0, MODC_RET_ERROR_S()));
                    ModC_Uint32List_AddValue(&tokenIndices, currentTokenIndex);
                }
            } //if(currentToken->TokenType == ModC_TokenType_Operator)
            //Skip all comments, spaces, newlines, etc...
            else if(ModC_Token_IsSkippable(currentToken))
            {
                //TODO: Attach comments to statements
                skipped = true;
                minLookBack = i + 1;
            }
            //Otherwise, don't skip current token
            else
            {
                minLookBack = i + 1;
                ModC_Result_Uint32 uint32Result = 
                    ModC_StatementTokensUnion_GetTokenIndexAt(&statement->Tokens, tokens, i);
                uint32_t currentTokenIndex = 
                    *MODC_RESULT_TRY(uint32Result, MODC_DEFER_BREAK(0, MODC_RET_ERROR_S()));
                ModC_Uint32List_AddValue(&tokenIndices, currentTokenIndex);
            }
        } //for(uint32_t i = 0; i < tokensCount; ++i)
        
        //If we have skip any tokens from the original tokens in this statement, replace the tokens 
        //with the new token list for this statement
        if(skipped)
        {
            statement->Tokens = 
                MODC_TAG_INIT_S(ModC_TokenIndexList, 
                                ModC_Uint32List_Create(statementsArena, tokenIndices.Length));
            ModC_Uint32List_AddRange(   &statement->Tokens.MODC_TAG_DATA_S(ModC_TokenIndexList),
                                        tokenIndices.Data,
                                        tokenIndices.Length);
        }
    }
    MODC_DEFER_SCOPE_END(0)
    
    return MODC_RESULT_VALUE_S(0);
}

static inline ModC_Result_Uint32 ModC_Statement_Next(   const ModC_Statement* statement, 
                                                        const ModC_Statement* prevStatement, 
                                                        const ModC_StatementList* statements,
                                                        bool* isEnd)
{
    #undef ModC_ResultName_State
    #define ModC_ResultName_State ModC_Result_Uint32
    #undef ModC_TaggedUnionName_State
    #define ModC_TaggedUnionName_State ModC_StatementTokensUnion
    
    MODC_CHECK(statement != NULL, (""), MODC_RET_ERROR_S());
    MODC_CHECK(statements != NULL, (""), MODC_RET_ERROR_S());
    
    *isEnd = false;
    
    //Traverse to the next statement
    {
        //TODO: We can probably move this part out
        //If we are at root, try going to first childs
        if(statement->ParentIndex == statement->Index)
        {
            MODC_CHECK( statement->StatementType == ModC_StatementType_Compound, 
                        ("Root must be compound statement"), 
                        MODC_RET_ERROR_S());
            
            if(statement->Tokens.MODC_TAG_DATA_S(ModC_CompoundStatement).ChildStatements.Length == 0)
            {
                *isEnd = true;
                return MODC_RESULT_VALUE_S(0);
            }
            
            return MODC_RESULT_VALUE_S(statement->Tokens
                                                .MODC_TAG_DATA_S(ModC_CompoundStatement)
                                                .ChildStatements
                                                .Data[0]);
        }
        
        //If compound and we are not going up, go to first child if any
        if( statement->StatementType == ModC_StatementType_Compound &&
            prevStatement->ParentIndex != statement->Index &&
            statement->Tokens.MODC_TAG_DATA_S(ModC_CompoundStatement).ChildStatements.Length != 0)
        {
            return MODC_RESULT_VALUE_S(statement->Tokens
                                                .MODC_TAG_DATA_S(ModC_CompoundStatement)
                                                .ChildStatements
                                                .Data[0]);
        }
        
        const ModC_Statement* parentStatement = &statements->Data[statement->ParentIndex];
        MODC_CHECK( parentStatement->StatementType == ModC_StatementType_Compound, 
                    ("Parent statement must be compound statement"), 
                    MODC_RET_ERROR_S());
        
        const ModC_CompoundStatement* parentCompound = 
            &parentStatement->Tokens.MODC_TAG_DATA_S(ModC_CompoundStatement);
        
        uint32_t childIndex = ModC_Uint32List_Find( &parentCompound->ChildStatements, 
                                                    &statement->Index);
        MODC_CHECK( childIndex != parentCompound->ChildStatements.Length, 
                    ("Failed to find child in parent. Corrupted Tree?"), 
                    MODC_RET_ERROR_S());
        
        //Continue to the next child if we are not at the end
        if(childIndex < parentCompound->ChildStatements.Length - 1)
            return MODC_RESULT_VALUE_S(parentCompound->ChildStatements.Data[childIndex + 1]);
        //Otherwise go up if we are not under root
        else if(parentStatement->Index != parentStatement->ParentIndex)
            return MODC_RESULT_VALUE_S(parentStatement->Index);
        //Otherwise, we are done
        else
        {
            *isEnd = true;
            return MODC_RESULT_VALUE_S(0);
        }
    }
}


static inline ModC_Result_Void ModC_TryClassifyAsTypeDeclaration(   ModC_Statement* statement,
                                                                    const ModC_TokenList* tokens,
                                                                    ModC_Allocator statementsArena,
                                                                    ModC_Allocator scratchAllocator,
                                                                    bool inTypeDecl,
                                                                    bool inFuncImpl,
                                                                    ModC_TypeEntry** rootTypeHashSet,
                                                                    ModC_TypeEntry** funcTypeHashSet)
{
    #undef ModC_ResultName_State
    #define ModC_ResultName_State ModC_Result_Void
    #undef ModC_TaggedUnionName_State
    #define ModC_TaggedUnionName_State ModC_StatementInfoUnion
    #undef uthash_malloc
    #define uthash_malloc(sz) ModC_Allocator_Malloc(&scratchAllocator, sz)
    #undef uthash_free
    #define uthash_free(ptr, sz) ModC_Allocator_Free(&scratchAllocator, ptr)
    
    if(statement->StatementType == ModC_StatementType_Compound || inTypeDecl)
        return MODC_RESULT_VALUE_S(0);
    
    uint32_t tokenCount = ModC_StatementTokensUnion_GetTokenCount(&statement->Tokens);
    MODC_CHECK(tokenCount > 0, (""), MODC_RET_ERROR_S());
    
    ModC_Result_ConstStringView constStringViewResult =
        ModC_StatementTokensUnion_GetTokenTextViewAt(&statement->Tokens, tokens, 0);
    
    ModC_ConstStringView firstTokenTextView = *MODC_RESULT_TRY( constStringViewResult, 
                                                                MODC_RET_ERROR_S());
    
    if(ModC_ConstStringView_IsEqualLiteral(&firstTokenTextView, "struct"))
    {
        statement->StatementType = ModC_StatementType_TypeDeclaration;
        statement->Info = MODC_TAG_INIT_S(  ModC_TypeDeclarationInfo, 
                                            { .Type = ModC_Type_Struct });
    }
    else if(ModC_ConstStringView_IsEqualLiteral(&firstTokenTextView, "enum"))
    {
        statement->StatementType = ModC_StatementType_TypeDeclaration;
        statement->Info = MODC_TAG_INIT_S(  ModC_TypeDeclarationInfo, 
                                            { .Type = ModC_Type_Enum });
    }
    else
        return MODC_RESULT_VALUE_S(0);
    
    if(tokenCount == 1)
        return MODC_ERROR_CSTR_S("Missing identifier when declaring struct or enum");
    
    constStringViewResult = ModC_StatementTokensUnion_GetTokenTextViewAt(   &statement->Tokens, 
                                                                            tokens, 
                                                                            1);
    ModC_ConstStringView typeNameTextView = *MODC_RESULT_TRY(constStringViewResult, 
                                                                MODC_RET_ERROR_S());
    {
        ModC_TypeDeclarationInfo* typeDeclInfo = 
            &statement->Info.MODC_TAG_DATA_S(ModC_TypeDeclarationInfo);
        typeDeclInfo->TypeName = ModC_String_FromData(  statementsArena, 
                                                        typeNameTextView.Data, 
                                                        typeNameTextView.Length);
        typeNameTextView = ModC_ConstStringView_Create( typeDeclInfo->TypeName.Data, 
                                                        typeDeclInfo->TypeName.Length);
    }
    ModC_TypeEntry* foundEntry = NULL;
    HASH_FIND(hh, *rootTypeHashSet, typeNameTextView.Data, typeNameTextView.Length, foundEntry);
    
    if(!foundEntry && inFuncImpl)
        HASH_FIND(hh, *funcTypeHashSet, typeNameTextView.Data, typeNameTextView.Length, foundEntry);
    
    if(foundEntry)
    {
        return MODC_ERROR_STR_FMT_S(("Type %.*s already defined", 
                                    typeNameTextView.Length,
                                    typeNameTextView.Data));
    }
    
    ModC_TypeEntry* entry = ModC_Allocator_Malloc(&scratchAllocator, sizeof(ModC_TypeEntry));
    MODC_CHECK(entry, (""), MODC_RET_ERROR_S());    entry->Type = ModC_String_FromData( scratchAllocator, 
                                        typeNameTextView.Data, 
                                        typeNameTextView.Length);
    if(inFuncImpl)
        HASH_ADD_KEYPTR(hh, *funcTypeHashSet, entry->Type.Data, entry->Type.Length, entry);
    else
        HASH_ADD_KEYPTR(hh, *rootTypeHashSet, entry->Type.Data, entry->Type.Length, entry);
    
    return MODC_RESULT_VALUE_S(0);
}

static inline ModC_Result_Void ModC_TryClassifyAsCompilerDirective( ModC_Statement* statement,
                                                                    const ModC_TokenList* tokens)
{
    #undef ModC_ResultName_State
    #define ModC_ResultName_State ModC_Result_Void
    
    if(statement->StatementType == ModC_StatementType_Compound)
        return MODC_RESULT_VALUE_S(0);
    
    uint32_t tokenCount = ModC_StatementTokensUnion_GetTokenCount(&statement->Tokens);
    MODC_CHECK(tokenCount > 0, (""), MODC_RET_ERROR_S());
    
    ModC_Result_TokenPtr tokenPtrResult =
        ModC_StatementTokensUnion_GetTokenAt(&statement->Tokens, tokens, 0);
    
    ModC_Token* token = *MODC_RESULT_TRY(tokenPtrResult, MODC_RET_ERROR_S());
    
    if(token->TokenType != ModC_TokenType_Operator)
        return MODC_RESULT_VALUE_S(0);
    
    ModC_Result_ConstStringView constStringViewResult =
        ModC_StatementTokensUnion_GetTokenTextViewAt(&statement->Tokens, tokens, 0);
    
    ModC_ConstStringView firstTokenTextView = *MODC_RESULT_TRY( constStringViewResult, 
                                                                MODC_RET_ERROR_S());
    
    if(ModC_ConstStringView_IsEqualLiteral(&firstTokenTextView, "#"))
        statement->StatementType = ModC_StatementType_CompilerDirective;
    
    return MODC_RESULT_VALUE_S(0);
}

static inline ModC_Result_Void 
ModC_TryClassifyAsVariableDeclareAssignment(ModC_Statement* statement,
                                            const ModC_TokenList* tokens,
                                            bool inTypeDecl,
                                            bool inFuncImpl,
                                            ModC_TypeEntry** rootTypeHashSet,
                                            ModC_TypeEntry** funcTypeHashSet)
{
    #undef ModC_ResultName_State
    #define ModC_ResultName_State ModC_Result_Void
    #undef ModC_TaggedUnionName_State
    #define ModC_TaggedUnionName_State ModC_StatementInfoUnion
    
    if(statement->StatementType == ModC_StatementType_Compound)
        return MODC_RESULT_VALUE_S(0);
    
    uint32_t tokenCount = ModC_StatementTokensUnion_GetTokenCount(&statement->Tokens);
    MODC_CHECK(tokenCount > 0, (""), MODC_RET_ERROR_S());
    
    //Check if last token is semicolon
    {
        ModC_Result_TokenPtr tokenPtrResult =
            ModC_StatementTokensUnion_GetTokenAt(&statement->Tokens, tokens, tokenCount - 1);
        
        ModC_Token* token = *MODC_RESULT_TRY(tokenPtrResult, MODC_RET_ERROR_S());
        if(token->TokenType != ModC_TokenType_Semicolon)
            return MODC_RESULT_VALUE_S(0);
    }
    
    //At least <Type> <Identifier> <Semicolon>
    if(tokenCount < 3)
        return MODC_RESULT_VALUE_S(0);
    
    ModC_Token* typeToken;
    ModC_Token* identifierToken;
    
    //NOTE: Hardcode type to be index 0 and identifier to be index 1 for now
    for(int i = 0; i < 2; ++i)
    {
        ModC_Result_TokenPtr tokenPtrResult =
            ModC_StatementTokensUnion_GetTokenAt(&statement->Tokens, tokens, i);
        
        ModC_Token** outPtr = i == 0 ? &typeToken : &identifierToken;
        *outPtr = *MODC_RESULT_TRY(tokenPtrResult, MODC_RET_ERROR_S());
        if((*outPtr)->TokenType != ModC_TokenType_Identifier)
            return MODC_RESULT_VALUE_S(0);
    }
    
    if( typeToken->TokenType != ModC_TokenType_Identifier ||
        identifierToken->TokenType != ModC_TokenType_Identifier)
    {
        return MODC_RESULT_VALUE_S(0);
    }
    
    bool typeExist = false;
    ModC_ConstStringView typeTokenText = ModC_Token_TokenTextView(typeToken);
    if(inFuncImpl)
    {
        ModC_TypeEntry* foundEntry = NULL;
        HASH_FIND(hh, *funcTypeHashSet, typeTokenText.Data, typeTokenText.Length, foundEntry);
        typeExist = foundEntry != NULL;
    }
    
    if(!typeExist)
    {
        ModC_TypeEntry* foundEntry = NULL;
        HASH_FIND(hh, *rootTypeHashSet, typeTokenText.Data, typeTokenText.Length, foundEntry);
        typeExist = foundEntry != NULL;
    }
    
    if(!typeExist)
    {
        return MODC_ERROR_STR_FMT_S(("Failed to find type %.*s", 
                                    typeTokenText.Length, 
                                    typeTokenText.Data));
    }
    
    ModC_Result_Uint32 uint32Result = 
        ModC_StatementTokensUnion_ContainsTokenText(&statement->Tokens, 
                                                    tokens, 
                                                    ModC_ConstStringView_FromLiteral("="));
    uint32_t foundIndex = *MODC_RESULT_TRY(uint32Result, MODC_RET_ERROR_S());
    
    //Check if there's any equal sign, if there is, maybe it is 
    //ModC_StatementType_VariableDeclareAssignment
    if(foundIndex != tokenCount)
    {
        if(inTypeDecl)
            return MODC_ERROR_CSTR_S("Assignment cannot happen in type declaration");
        
        statement->StatementType = ModC_StatementType_VariableDeclareAssignment;
        statement->Info = MODC_TAG_INIT_S(  ModC_VariableDeclareAssignInfo, 
                                            {
                                                .TypeIndexInStatement = 0,
                                                .IdentifierIndexInStatement = 1,
                                                .HasAsignment = true,
                                                .AssignIndexInStatement = foundIndex
                                            });
    }
    //Otherwise, maybe it is ModC_StatementType_VariableDeclaration
    else
    {
        statement->StatementType = ModC_StatementType_VariableDeclaration;
        statement->Info = MODC_TAG_INIT_S(  ModC_VariableDeclareAssignInfo, 
                                            {
                                                .TypeIndexInStatement = 0,
                                                .IdentifierIndexInStatement = 1,
                                                .HasAsignment = false,
                                                .AssignIndexInStatement = 0
                                            });
    }
    
    return MODC_RESULT_VALUE_S(0);
}

static inline ModC_Result_Void 
ModC_TryClassifyAsFunctionDeclaration(  ModC_Statement* statement,
                                        const ModC_TokenList* tokens,
                                        bool inTypeDecl,
                                        bool inFuncImpl,
                                        ModC_TypeEntry** rootTypeHashSet)
{
    #undef ModC_ResultName_State
    #define ModC_ResultName_State ModC_Result_Void
    #undef ModC_TaggedUnionName_State
    #define ModC_TaggedUnionName_State ModC_StatementInfoUnion
    
    if(statement->StatementType == ModC_StatementType_Compound || inTypeDecl || inFuncImpl)
        return MODC_RESULT_VALUE_S(0);
    
    uint32_t tokenCount = ModC_StatementTokensUnion_GetTokenCount(&statement->Tokens);
    MODC_CHECK(tokenCount > 0, (""), MODC_RET_ERROR_S());
    
    //Check if last token is end paresthesia
    {
        ModC_Result_TokenPtr tokenPtrResult =
            ModC_StatementTokensUnion_GetTokenAt(&statement->Tokens, tokens, tokenCount - 1);
        
        ModC_Token* token = *MODC_RESULT_TRY(tokenPtrResult, MODC_RET_ERROR_S());
        if(token->TokenType != ModC_TokenType_InvokeEnd)
            return MODC_RESULT_VALUE_S(0);
    }
    
    //<Type> <Identifier> <Open paren> [<Arguments>...] <Close paren>
    if(tokenCount < 4)
        return MODC_RESULT_VALUE_S(0);
    
    ModC_Token* typeToken;
    ModC_Token* identifierToken;
    ModC_Token* argumentToken = NULL;
    
    //NOTE: Hardcode type to be index 0 and identifier to be index 1 for now
    for(int i = 0; i < 4; ++i)
    {
        ModC_Result_TokenPtr tokenPtrResult =
            ModC_StatementTokensUnion_GetTokenAt(&statement->Tokens, tokens, i);
        
        ModC_Token* curToken = *MODC_RESULT_TRY(tokenPtrResult, MODC_RET_ERROR_S());
        ModC_Token** outPtr = NULL;
        
        if(i == 0)
        {
            if(curToken->TokenType != ModC_TokenType_Identifier)
                return MODC_RESULT_VALUE_S(0);
            outPtr = &typeToken;
        }
        else if(i == 1)
        {
            if(curToken->TokenType != ModC_TokenType_Identifier)
                return MODC_RESULT_VALUE_S(0);
            outPtr = &identifierToken;
        }
        else if(i == 2 && curToken->TokenType != ModC_TokenType_InvokeStart)
            return MODC_RESULT_VALUE_S(0);
        else if(i == 3 && curToken->TokenType != ModC_TokenType_InvokeEnd)
            outPtr = &argumentToken;
        
        if(outPtr)
            *outPtr = curToken;
    }
    
    ModC_ConstStringView typeTokenText = ModC_Token_TokenTextView(typeToken);
    {
        ModC_TypeEntry* foundEntry = NULL;
        HASH_FIND(hh, *rootTypeHashSet, typeTokenText.Data, typeTokenText.Length, foundEntry);
        
        if(!foundEntry)
        {
            return MODC_ERROR_STR_FMT_S(("Failed to find type %.*s", 
                                        typeTokenText.Length, 
                                        typeTokenText.Data));
        }
    }
    
    statement->StatementType = ModC_StatementType_FunctionDeclaration;
    statement->Info = MODC_TAG_INIT_S(  ModC_FunctionDeclarationInfo, 
                                        {
                                            .TypeIndexInStatement = 0,
                                            .IdentifierIndexInStatement = 1,
                                            .HaveArguments = argumentToken != NULL,
                                            .ArgumentIndexInStatement = argumentToken != NULL ? 3 : 0
                                        });
    
    return MODC_RESULT_VALUE_S(0);
}


static inline ModC_Result_Void ModC_TryClassifyAsReturn(ModC_Statement* statement,
                                                        const ModC_TokenList* tokens,
                                                        bool inTypeDecl,
                                                        bool inFuncImpl)
{
    #undef ModC_ResultName_State
    #define ModC_ResultName_State ModC_Result_Void
    
    if(statement->StatementType == ModC_StatementType_Compound || inTypeDecl || !inFuncImpl)
        return MODC_RESULT_VALUE_S(0);
    
    uint32_t tokenCount = ModC_StatementTokensUnion_GetTokenCount(&statement->Tokens);
    MODC_CHECK(tokenCount > 0, (""), MODC_RET_ERROR_S());
    
    //Check if last token is semicolon
    {
        ModC_Result_TokenPtr tokenPtrResult =
            ModC_StatementTokensUnion_GetTokenAt(&statement->Tokens, tokens, tokenCount - 1);
        
        ModC_Token* token = *MODC_RESULT_TRY(tokenPtrResult, MODC_RET_ERROR_S());
        if(token->TokenType != ModC_TokenType_Semicolon)
            return MODC_RESULT_VALUE_S(0);
    }
    
    //return <Identifier> <Semicolon>
    if(tokenCount < 3)
        return MODC_RESULT_VALUE_S(0);
    
    ModC_Result_ConstStringView viewResult =
        ModC_StatementTokensUnion_GetTokenTextViewAt(&statement->Tokens, tokens, 0);
    
    ModC_ConstStringView firstTokenView = *MODC_RESULT_TRY(viewResult, MODC_RET_ERROR_S());
    
    if(!ModC_ConstStringView_IsEqualLiteral(&firstTokenView, "return"))
        return MODC_RESULT_VALUE_S(0);
    
    statement->StatementType = ModC_StatementType_ReturnStatement;
    return MODC_RESULT_VALUE_S(0);
}

static inline ModC_Result_Void ModC_TryClassifyKeywordInvokable(ModC_Statement* statement,
                                                                const ModC_TokenList* tokens,
                                                                bool inTypeDecl,
                                                                bool inFuncImpl)
{
    #undef ModC_ResultName_State
    #define ModC_ResultName_State ModC_Result_Void
    
    if(statement->StatementType == ModC_StatementType_Compound || inTypeDecl || !inFuncImpl)
        return MODC_RESULT_VALUE_S(0);
    
    uint32_t tokenCount = ModC_StatementTokensUnion_GetTokenCount(&statement->Tokens);
    MODC_CHECK(tokenCount > 0, (""), MODC_RET_ERROR_S());
    
    //Check if last token is end parenthesis
    {
        ModC_Result_TokenPtr tokenPtrResult =
            ModC_StatementTokensUnion_GetTokenAt(&statement->Tokens, tokens, tokenCount - 1);
        
        ModC_Token* token = *MODC_RESULT_TRY(tokenPtrResult, MODC_RET_ERROR_S());
        if(token->TokenType != ModC_TokenType_InvokeEnd)
            return MODC_RESULT_VALUE_S(0);
    }
    
    //<keyword> <open paren> ... <end paren>
    if(tokenCount < 3)
        return MODC_RESULT_VALUE_S(0);
    
    ModC_Result_ConstStringView viewResult =
        ModC_StatementTokensUnion_GetTokenTextViewAt(&statement->Tokens, tokens, 0);
    
    ModC_ConstStringView firstTokenView = *MODC_RESULT_TRY(viewResult, MODC_RET_ERROR_S());
    if(ModC_ConstStringView_IsEqualLiteral(&firstTokenView, "if"))
        statement->StatementType = ModC_StatementType_IfStatement;
    else if(ModC_ConstStringView_IsEqualLiteral(&firstTokenView, "for"))
        statement->StatementType = ModC_StatementType_ForStatement;
    else if(ModC_ConstStringView_IsEqualLiteral(&firstTokenView, "while"))
        statement->StatementType = ModC_StatementType_WhileStatement;
    else if(ModC_ConstStringView_IsEqualLiteral(&firstTokenView, "switch"))
        statement->StatementType = ModC_StatementType_SwitchStatement;
    
    return MODC_RESULT_VALUE_S(0);
}

static inline ModC_Result_Void ModC_TryClassifyAsElse(  ModC_Statement* statement,
                                                        const ModC_TokenList* tokens,
                                                        bool inTypeDecl,
                                                        bool inFuncImpl)
{
    #undef ModC_ResultName_State
    #define ModC_ResultName_State ModC_Result_Void
    
    if(statement->StatementType == ModC_StatementType_Compound || inTypeDecl || !inFuncImpl)
        return MODC_RESULT_VALUE_S(0);
    
    uint32_t tokenCount = ModC_StatementTokensUnion_GetTokenCount(&statement->Tokens);
    MODC_CHECK(tokenCount > 0, (""), MODC_RET_ERROR_S());
    
    if(tokenCount != 1)
        return MODC_RESULT_VALUE_S(0);
    
    ModC_Result_ConstStringView viewResult = 
        ModC_StatementTokensUnion_GetTokenTextViewAt(&statement->Tokens, tokens, 0);
    
    ModC_ConstStringView tokenView = *MODC_RESULT_TRY(viewResult, MODC_RET_ERROR_S());
    if(ModC_ConstStringView_IsEqualLiteral(&tokenView, "else"))
        statement->StatementType = ModC_StatementType_ElseStatement;
    
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
        if(rootStatement->Tokens.MODC_TAG_DATA_S(ModC_CompoundStatement).ChildStatements.Length == 0)
            return MODC_RESULT_VALUE_S(0);
    }
    
    
    ModC_TypeEntry* rootTypeHashSet = NULL;
    
    //TODO: This won't work when there are nested scopes in func
    ModC_TypeEntry* funcTypeHashSet = NULL;
    
    //TODO: Pull these from centralized place instead
    char* defaultTypes[] = 
    {
        "int", "int8", "int16", "int32", "uint", "uint8", "uint16", "uint32", 
        "char", "float", "double", "bool"
    };
    
    #undef uthash_malloc
    #define uthash_malloc(sz) ModC_Allocator_Malloc(&scratchAllocator, sz)
    #undef uthash_free
    #define uthash_free(ptr, sz) ModC_Allocator_Free(&scratchAllocator, ptr)
    
    for(int i = 0; i < sizeof(defaultTypes) / sizeof(defaultTypes[0]); ++i)
    {
        //printf("i: %d\n", i);
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
    
    MODC_DEFER_SCOPE_START(0)
    {
        MODC_DEFER(0,   if(!rootTypeHashSet)
                            HASH_CLEAR(hh, rootTypeHashSet);
                        if(!funcTypeHashSet)
                            HASH_CLEAR(hh, funcTypeHashSet));
        
        //bool inTypeDecl = false;
        //bool inFuncImpl = false;
        int funcScope = -1;
        int typeScope = -1;
        int currentScope = 0;
        
        //Iterate all statements
        ModC_Statement* prevStatement = &statements->Data[currentStatementIndex];
        do
        {
            bool isEnd = false;
            ModC_Statement* statement = &statements->Data[currentStatementIndex];
            ModC_Result_Uint32 uint32Result = ModC_Statement_Next(  statement, 
                                                                    prevStatement, 
                                                                    statements, 
                                                                    &isEnd);
            currentStatementIndex = *MODC_RESULT_TRY(uint32Result, MODC_RET_ERROR_S());
            if(isEnd)
                break;
            
            prevStatement = statement;
            statement = &statements->Data[currentStatementIndex];
            
            bool onExitCompound = prevStatement->ParentIndex == statement->Index;
            if(statement->StatementType == ModC_StatementType_Compound)
            {
                if(onExitCompound)
                {
                    --currentScope;
                    if(funcScope != -1 && funcScope == currentScope)
                        funcScope = -1;
                    if(typeScope != -1 && typeScope == currentScope)
                        typeScope = -1;
                }
                else
                {
                    if(prevStatement->StatementType == ModC_StatementType_FunctionDeclaration)
                        funcScope = currentScope;
                    else if(prevStatement->StatementType == ModC_StatementType_TypeDeclaration)
                        typeScope = currentScope;
                    ++currentScope;
                }
                continue;
            }
            
            MODC_CHECK( statement->StatementType == ModC_StatementType_Unknown,
                        ("Unexpected statement type"),
                        MODC_RET_ERROR_S());
            
            ModC_Result_Void voidResult = ModC_Statement_Normalize( statement,
                                                                    tokensAllcoator,
                                                                    statementsArena,
                                                                    tokens,
                                                                    scratchAllocator);
            (void)MODC_RESULT_TRY(voidResult, MODC_DEFER_BREAK(0, MODC_RET_ERROR_S()));
            
            uint32_t tokenCount = ModC_StatementTokensUnion_GetTokenCount(&statement->Tokens);
            MODC_CHECK(tokenCount > 0, (""), MODC_DEFER_BREAK(0, MODC_RET_ERROR_S()));
            
            //Classify statements
            static_assert((int)ModC_StatementType_Count == 16, "");
            
            #undef ModC_TaggedUnionName_State
            #define ModC_TaggedUnionName_State ModC_StatementInfoUnion
            
            #define TRY_CLASSIFY_TYPE_DECLARATION() \
                if(statement->StatementType == ModC_StatementType_Unknown) \
                { \
                    voidResult = ModC_TryClassifyAsTypeDeclaration( statement, \
                                                                    tokens, \
                                                                    statementsArena, \
                                                                    scratchAllocator, \
                                                                    typeScope != -1, \
                                                                    funcScope != -1, \
                                                                    &rootTypeHashSet, \
                                                                    &funcTypeHashSet); \
                    (void)MODC_RESULT_TRY(voidResult, MODC_DEFER_BREAK(0, MODC_RET_ERROR_S())); \
                }
            
            #define TRY_CLASSIFY_COMPILER_DIRECTIVE() \
                if(statement->StatementType == ModC_StatementType_Unknown) \
                { \
                    voidResult = ModC_TryClassifyAsCompilerDirective(statement, tokens); \
                    (void)MODC_RESULT_TRY(voidResult, MODC_DEFER_BREAK(0, MODC_RET_ERROR_S())); \
                }
            
            #define TRY_CLASSIFY_VAR_DECLARE_ASSIGN() \
                if(statement->StatementType == ModC_StatementType_Unknown) \
                { \
                    voidResult = ModC_TryClassifyAsVariableDeclareAssignment(   statement, \
                                                                                tokens, \
                                                                                typeScope != -1, \
                                                                                funcScope != -1, \
                                                                                &rootTypeHashSet, \
                                                                                &funcTypeHashSet); \
                    (void)MODC_RESULT_TRY(voidResult, MODC_DEFER_BREAK(0, MODC_RET_ERROR_S())); \
                }
            
            #define TRY_CLASSIFY_FUNC_DECLARE() \
                if(statement->StatementType == ModC_StatementType_Unknown) \
                { \
                    voidResult = ModC_TryClassifyAsFunctionDeclaration( statement, \
                                                                        tokens, \
                                                                        typeScope != -1, \
                                                                        funcScope != -1, \
                                                                        &rootTypeHashSet); \
                    (void)MODC_RESULT_TRY(voidResult, MODC_DEFER_BREAK(0, MODC_RET_ERROR_S())); \
                }
            
            #define TRY_CLASSIFY_RETURN() \
                if(statement->StatementType == ModC_StatementType_Unknown) \
                { \
                    voidResult = ModC_TryClassifyAsReturn(  statement, \
                                                            tokens, \
                                                            typeScope != -1, \
                                                            funcScope != -1); \
                    (void)MODC_RESULT_TRY(voidResult, MODC_DEFER_BREAK(0, MODC_RET_ERROR_S())); \
                }
            
            //TODO: classify invokable keywords
            //TODO: classify else
            #define TRY_CLASSIFY_INVOKABLE() \
                if(statement->StatementType == ModC_StatementType_Unknown) \
                { \
                    voidResult = ModC_TryClassifyKeywordInvokable(  statement, \
                                                                    tokens, \
                                                                    typeScope != -1, \
                                                                    funcScope != -1); \
                    (void)MODC_RESULT_TRY(voidResult, MODC_DEFER_BREAK(0, MODC_RET_ERROR_S())); \
                }
            
            #define TRY_CLASSIFY_ELSE() \
                if(statement->StatementType == ModC_StatementType_Unknown) \
                { \
                    voidResult = ModC_TryClassifyAsElse(statement, \
                                                        tokens, \
                                                        typeScope != -1, \
                                                        funcScope != -1); \
                    (void)MODC_RESULT_TRY(voidResult, MODC_DEFER_BREAK(0, MODC_RET_ERROR_S())); \
                }
            
            //Root
            if(typeScope == -1 && funcScope == -1)
            {
                TRY_CLASSIFY_TYPE_DECLARATION();
                TRY_CLASSIFY_COMPILER_DIRECTIVE();
                TRY_CLASSIFY_VAR_DECLARE_ASSIGN();
                TRY_CLASSIFY_FUNC_DECLARE();
            }
            //Inside struct or enum (Which can be inside function impl as well)
            else if(typeScope != -1)
            {
                TRY_CLASSIFY_VAR_DECLARE_ASSIGN();
            }
            //Inside function impl
            else if(funcScope != -1)
            {
                TRY_CLASSIFY_INVOKABLE();
                TRY_CLASSIFY_ELSE();
                TRY_CLASSIFY_RETURN();
                TRY_CLASSIFY_TYPE_DECLARATION();
                TRY_CLASSIFY_COMPILER_DIRECTIVE();
                TRY_CLASSIFY_VAR_DECLARE_ASSIGN();
            }
            
            //If we are not in a function implementation
                //We can only have
                    //ModC_StatementType_TypeDeclaration
                    //ModC_StatementType_FunctionDeclaration
                    //ModC_StatementType_VariableDeclaration
                    //ModC_StatementType_Compound
                    //ModC_StatementType_CompilerDirective
                    //ModC_StatementType_VariableDeclareAssignment
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
        } //do
        while(true);
    }
    MODC_DEFER_SCOPE_END(0)
    
    return MODC_RESULT_VALUE_S(0);
}

#endif
