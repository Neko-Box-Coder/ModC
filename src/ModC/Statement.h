#ifndef MODC_STATEMENT_H
#define MODC_STATEMENT_H

#ifndef DEFAULT_ALLOC
    #define DEFAULT_ALLOC() ModC_CreateHeapAllocator()
#endif

#include "ModC/Tokenization.h"
#include "ModC/GenericContainers.h"
#include "ModC/Result.h"
#include "ModC/Operators.h"
#include "ModC/Keyword.h"

typedef enum ModC_StatementType
{
    ModC_StatementType_Invalid,
    ModC_StatementType_Unknown,
    ModC_StatementType_TypeDeclaration,
    ModC_StatementType_EnumValues,
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
    ModC_StatementType_CaseStatement,
    ModC_StatementType_Count,   //18
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

#define TU_NAME ModC_StatementTokensUnion
#define VALUE_TYPES ModC_CompoundStatement,ModC_TokenIndexList,ModC_TokenIndexRange
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

typedef struct ModC_AssignmentInfo
{
    uint32_t AssignIndexInStatement;
} ModC_AssignmentInfo;


#define TU_NAME ModC_StatementInfoUnion
#define VALUE_TYPES Void, \
                    ModC_TypeDeclarationInfo, \
                    ModC_VariableDeclareAssignInfo, \
                    ModC_FunctionDeclarationInfo, \
                    ModC_AssignmentInfo
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

#define LIST_NAME ModC_StatementList
#define VALUE_TYPE ModC_Statement
#define NO_TYPEDEF 1
//TODO: Value item free
#include "ModC/List.h"

DEFINE_RESULT_STRUCT(ModC_ResultStatementPtr, ModC_Statement*)
DEFINE_RESULT_STRUCT(ModC_Result_ConstStringView, ModC_ConstStringView)
DEFINE_RESULT_STRUCT(ModC_Result_StatementList, ModC_StatementList)

#include "static_assert.h/assert.h"

#include <stdbool.h>
#include <stdint.h>

//TODO: Use statement to get token count instead
static inline uint32_t 
ModC_StatementTokensUnion_GetTokenCount(const ModC_StatementTokensUnion* statementUnion)
{
    #undef TaggedUnionNameState
    #define TaggedUnionNameState ModC_StatementTokensUnion
    
    if(!statementUnion)
        return 0;
    
    switch(statementUnion->Type)
    {
        case TU_TYPE_S(ModC_CompoundStatement):
            return 2;
        case TU_TYPE_S(ModC_TokenIndexList):
            return statementUnion->TU_DATA_S(ModC_TokenIndexList).Length;
        case TU_TYPE_S(ModC_TokenIndexRange):
        {
            const ModC_TokenIndexRange* range = 
                &statementUnion->TU_DATA_S(ModC_TokenIndexRange);
            return range->EndIndex - range->StartIndex;
        }
        default:
            return 0;
    }
}

static inline ModC_Result_Uint32
ModC_StatementTokensUnion_GetTokenIndexAt(  const ModC_StatementTokensUnion* statementUnion, 
                                            const ModC_TokenList* tokens,
                                            uint32_t indexInStatement)
{
    #undef ResultNameState
    #define ResultNameState ModC_Result_Uint32
    #undef TaggedUnionNameState
    #define TaggedUnionNameState ModC_StatementTokensUnion
    
    CHECK(statementUnion != NULL, (""), RET_ERROR_S());
    CHECK(tokens != NULL, (""), RET_ERROR_S());
    
    uint32_t tokenIndex = 0;
    switch(statementUnion->Type)
    {
        case TU_TYPE_S(ModC_CompoundStatement):
        {
            //NOTE: Shouldn't use this function for compound statement..., but whatever
            const ModC_CompoundStatement* compound = 
                &statementUnion->TU_DATA_S(ModC_CompoundStatement);
            if(indexInStatement == 0)
                tokenIndex = compound->StartTokenIndex;
            else if(indexInStatement == 1)
                tokenIndex = compound->EndTokenIndex;
            else
                return ERROR_STR_FMT_S("Invalid index for accessing %"PRIu32, indexInStatement);
            break;
        }
        case TU_TYPE_S(ModC_TokenIndexList):
        {
            const ModC_TokenIndexList* tokenIndexList = 
                &statementUnion->TU_DATA_S(ModC_TokenIndexList);
            
            CHECK(tokenIndexList->Length > 0, ("Empty statement"), RET_ERROR_S());
            CHECK(  indexInStatement < tokenIndexList->Length, 
                    ("Invalid index for accessing, index: %"PRIu32", length: %"PRIu64, 
                    indexInStatement, tokenIndexList->Length),
                    RET_ERROR_S());
            tokenIndex = tokenIndexList->Data[indexInStatement];
            break;
        }
        case TU_TYPE_S(ModC_TokenIndexRange):
        {
            const ModC_TokenIndexRange* range = &statementUnion->TU_DATA_S(ModC_TokenIndexRange);
            CHECK(range->EndIndex > range->StartIndex, ("Empty statement"), RET_ERROR_S());
            CHECK(  indexInStatement < range->EndIndex - range->StartIndex, 
                    ("Invalid index for accessing, index: %"PRIu32", length: %"PRIu32,
                    indexInStatement, range->EndIndex - range->StartIndex),
                    RET_ERROR_S());
            tokenIndex = range->StartIndex + indexInStatement;
            break;
        }
        default:
            return ERROR_CSTR_S("Unexpected statement union type");
    }
    
    CHECK(  tokenIndex < tokens->Length, 
            ("Token index access out of bound, tokenIndex %"PRIu32", tokens->Length: %"PRIu64,
            tokenIndex, tokens->Length),
            RET_ERROR_S());
    return RESULT_VALUE_S(tokenIndex);
}

static inline ModC_Result_TokenPtr 
ModC_StatementTokensUnion_GetTokenAt(   const ModC_StatementTokensUnion* statementUnion, 
                                        const ModC_TokenList* tokens,
                                        uint32_t indexInStatement)
{
    #undef ResultNameState
    #define ResultNameState ModC_Result_TokenPtr
    
    ModC_Result_Uint32 uint32Result = ModC_StatementTokensUnion_GetTokenIndexAt(statementUnion, 
                                                                                tokens, 
                                                                                indexInStatement);
    uint32_t tokenIndex = *RESULT_TRY(uint32Result, RET_ERROR_S());
    return RESULT_VALUE_S(&tokens->Data[tokenIndex]);
}

static inline ModC_Result_ConstStringView 
ModC_StatementTokensUnion_GetTokenTextViewAt(   const ModC_StatementTokensUnion* statementUnion, 
                                                const ModC_TokenList* tokens,
                                                uint32_t indexInStatement)
{
    #undef ResultNameState
    #define ResultNameState ModC_Result_ConstStringView 
    
    ModC_Token* token = NULL;
    {
        ModC_Result_TokenPtr tokenPtrResult = ModC_StatementTokensUnion_GetTokenAt( statementUnion, 
                                                                                    tokens, 
                                                                                    indexInStatement);
        token = *RESULT_TRY(tokenPtrResult, RET_ERROR_S());
    }
    
    return RESULT_VALUE_S(ModC_Token_TokenTextView(token));
}


static inline ModC_Result_Uint32
ModC_StatementTokensUnion_ContainsTokenText(const ModC_StatementTokensUnion* statementUnion, 
                                            const ModC_TokenList* tokens,
                                            ModC_ConstStringView checkText)
{
    #undef ResultNameState
    #define ResultNameState ModC_Result_Uint32
    
    uint32_t tokensCount = ModC_StatementTokensUnion_GetTokenCount(statementUnion);
    for(uint32_t i = 0; i < tokensCount; ++i)
    {
        ModC_Result_ConstStringView constStringViewResult =
            ModC_StatementTokensUnion_GetTokenTextViewAt(statementUnion, tokens, i);
        
        ModC_ConstStringView tokenStringView = *RESULT_TRY(constStringViewResult, RET_ERROR_S());
        if(ModC_StringLikeEqual(tokenStringView, checkText))
            return RESULT_VALUE_S(i);
    }
    
    return RESULT_VALUE_S(tokensCount);
}

static inline ModC_ConstStringView ModC_StatementType_ToConstStringView(ModC_StatementType type)
{
    static_assert((int)ModC_StatementType_Count == 18, "");
    switch(type)
    {
        #define RET_TO_STR(enumVal) \
            case enumVal: \
                return ModC_ConstStringView_FromLiteral(#enumVal)
        
        RET_TO_STR(ModC_StatementType_Invalid);
        RET_TO_STR(ModC_StatementType_Unknown);
        RET_TO_STR(ModC_StatementType_TypeDeclaration);
        RET_TO_STR(ModC_StatementType_EnumValues);
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
        RET_TO_STR(ModC_StatementType_CaseStatement);
        RET_TO_STR(ModC_StatementType_Count);
        
        #undef RET_TO_STR
    }
    return (ModC_ConstStringView){0};
}


static inline ModC_ResultStatementPtr ModC_Statement_CreateCompound(ModC_Allocator allocator, 
                                                                    ModC_StatementList* statementList,
                                                                    uint32_t parentIndex,
                                                                    bool implicit,
                                                                    uint32_t reserveStatementsCount)
{
    #undef ResultNameState
    #define ResultNameState ModC_ResultStatementPtr
    #undef TaggedUnionNameState
    #define TaggedUnionNameState ModC_StatementTokensUnion
    
    CHECK(statementList != NULL, (""), RET_ERROR_S());
    CHECK(allocator.Type == ModC_AllocatorType_SharedArena, (""), RET_ERROR_S());
    
    uint64_t oldLength = statementList->Length;
    ModC_StatementIndexList childStatements = ModC_Uint32List_Create(   allocator, 
                                                                        reserveStatementsCount);
    CHECK(childStatements.Cap > 0, ("Failed to allocate"), RET_ERROR_S());
    ModC_StatementList_AddValue(statementList, 
                                (ModC_Statement)
                                {
                                    .StatementType = ModC_StatementType_Compound,
                                    .Tokens = TU_INIT_S(ModC_CompoundStatement, 
                                                        {
                                                            .StartTokenIndex = 0,
                                                            .ChildStatements = childStatements,
                                                            .EndTokenIndex = 0,
                                                            .Implicit = implicit
                                                        }),
                                    .Info = TU_INIT(ModC_StatementInfoUnion, Void, 0),
                                    .Index = oldLength,
                                    .ParentIndex = parentIndex
                                });
    childStatements = (ModC_StatementIndexList){0};
    CHECK(statementList->Length != oldLength, ("Failed to allocate"), RET_ERROR_S());
    
    ModC_Statement* retStatementPtr = &statementList->Data[statementList->Length - 1];
    return RESULT_VALUE_S(retStatementPtr);
}

static inline ModC_ResultStatementPtr ModC_Statement_CreatePlain(   ModC_Allocator allocator, 
                                                                    ModC_StatementList* statementList,
                                                                    uint32_t parentIndex)
{
    #undef ResultNameState
    #define ResultNameState ModC_ResultStatementPtr
    #undef TaggedUnionNameState
    #define TaggedUnionNameState ModC_StatementTokensUnion
    
    CHECK(statementList != NULL, (""), RET_ERROR_S());
    CHECK(allocator.Type == ModC_AllocatorType_SharedArena, (""), RET_ERROR_S());
    
    uint64_t oldLength = statementList->Length;
    ModC_StatementList_AddValue(statementList, 
                                (ModC_Statement)
                                {
                                    .StatementType = ModC_StatementType_Unknown,
                                    .Tokens = TU_INIT_S(ModC_TokenIndexRange, {0}),
                                    .Info = TU_INIT(ModC_StatementInfoUnion, Void, 0),
                                    .Index = oldLength,
                                    .ParentIndex = parentIndex
                                });
    CHECK(statementList->Length != oldLength, ("Failed to allocate"), RET_ERROR_S());
    
    ModC_Statement* retStatementPtr = &statementList->Data[statementList->Length - 1];
    //CHECK( retStatementPtr->Tokens.TU_DATA(ModC_StatementTokensUnion, 
    //                                            ModC_TokenIndexList).Cap > 0,
    //            ("Failed to allocate"), 
    //            RET_ERROR_S());
    
    return RESULT_VALUE_S(retStatementPtr);
}

static inline ModC_Result_Void ModC_Statement_ToString( ModC_Statement* this, 
                                                        ModC_TokenList* tokenList,
                                                        ModC_String* inOutString, 
                                                        bool append)
{
    #undef ResultNameState
    #define ResultNameState ModC_Result_Void
    #undef TaggedUnionNameState
    #define TaggedUnionNameState ModC_StatementTokensUnion
    
    if(!inOutString || !tokenList)
        return RESULT_VALUE_S(0);
    
    if(!append)
        ModC_String_Resize(inOutString, 0);
    
    ModC_ConstStringView statementTypeStr = ModC_StatementType_ToConstStringView(this->StatementType);
    ModC_String_AddRange(inOutString, statementTypeStr.Data, statementTypeStr.Length);
    ModC_String_AppendLiteral(inOutString, " - ");
    
    static_assert((int)TU_TYPE_S(Count) == 3, "");
    switch(this->Tokens.Type)
    {
        case TU_TYPE_S(ModC_CompoundStatement):
        {
            ModC_CompoundStatement* compoundStatement = &this   ->Tokens
                                                                .TU_DATA_S(ModC_CompoundStatement);
            ModC_String_AppendFormat(   inOutString, 
                                        "ModC_CompoundStatement: %" PRIu32 " - %" PRIu32 " "
                                        "(Implicit: %s), (Children: ", 
                                        compoundStatement->StartTokenIndex,
                                        compoundStatement->EndTokenIndex,
                                        (compoundStatement->Implicit ? "true" : "false"));
            
            if(compoundStatement->ChildStatements.Length >= 2)
            {
                for(int j = 0; j < compoundStatement->ChildStatements.Length - 1; ++j)
                {
                    ModC_String_AppendFormat(   inOutString, 
                                                "%" PRIu32 ", ", 
                                                compoundStatement->ChildStatements.Data[j]);
                }
            }
            
            if(compoundStatement->ChildStatements.Length > 0)
            {
                uint32_t lastIndex = compoundStatement->ChildStatements.Length - 1;
                ModC_String_AppendFormat(   inOutString, 
                                            "%" PRIu32, 
                                            compoundStatement->ChildStatements.Data[lastIndex]);
            }
            ModC_String_AddValue(inOutString, ')');
            break;
        }
        case TU_TYPE_S(ModC_TokenIndexList):
        {
            ModC_TokenIndexList* tokenIndexList = &this->Tokens.TU_DATA_S(ModC_TokenIndexList);
            ModC_String_AppendFormat(inOutString, "ModC_TokenIndexList: (Indices: ");
            if(tokenIndexList->Length >= 2)
            {
                for(int j = 0; j < tokenIndexList->Length - 1; ++j)
                    ModC_String_AppendFormat(inOutString, "%" PRIu32 ", ", tokenIndexList->Data[j]);
            }
            
            if(tokenIndexList->Length > 0)
            {
                ModC_String_AppendFormat(   inOutString, 
                                            "%" PRIu32, 
                                            tokenIndexList->Data[tokenIndexList->Length - 1]);
            }
            
            ModC_String_AppendFormat(inOutString, "), \"");
            for(int j = 0; j < tokenIndexList->Length; ++j)
            {
                ModC_ConstStringView tokenText = 
                    ModC_Token_TokenTextView(&tokenList->Data[tokenIndexList->Data[j]]);
                CHECK(tokenText.Length > 0, ("Invalid token text"), RET_ERROR_S());
                ModC_String_AppendFormat(inOutString, "%.*s ", (int)tokenText.Length, tokenText.Data);
            }
            ModC_String_AppendLiteral(inOutString, "\"");
            break;
        }
        case TU_TYPE_S(ModC_TokenIndexRange):
        {
            ModC_TokenIndexRange* tokenIndexRange = &this->Tokens.TU_DATA_S(ModC_TokenIndexRange);
            ModC_String_AppendFormat(   inOutString, 
                                        "ModC_TokenIndexRange: %" PRIu32 " - %" PRIu32 ", \"",
                                        tokenIndexRange->StartIndex,
                                        tokenIndexRange->EndIndex);
            
            for(int j = tokenIndexRange->StartIndex; j < tokenIndexRange->EndIndex; ++j)
            {
                ModC_ConstStringView tokenText = ModC_Token_TokenTextView(&tokenList->Data[j]);
                CHECK(tokenText.Length > 0, ("Invalid token text"), RET_ERROR_S());
                ModC_String_AppendFormat(inOutString, "%.*s ", (int)tokenText.Length, tokenText.Data);
            }
            ModC_String_AppendLiteral(inOutString, "\"");
            break;
        }
        default:
            break;
    } //switch(this->Tokens.Type)
    
    return RESULT_VALUE_S(0);
}

#if 0
    void ModC_Statement_Free(ModC_Statement* this)
    {
        #undef TaggedUnionNameState
        #define TaggedUnionNameState ModC_StatementTokensUnion
        
        if(!this)
            return;
        
        //No children, just free the token list
        if(this->Tokens.Type == TU_TYPE_S(TokenIndexList))
        {
            ModC_Uint32List_Free(&this->Tokens.TU_DATA_S(TokenIndexList));
            *this = (ModC_Statement){0};
            return;
        }
        
        //Get to the deepest child
        ModC_Statement* currentStatement = this;
        while(currentStatement->Tokens.Type == TU_TYPE_S(ModC_ChildStatements))
        {
            ModC_ChildStatements* currentChildren =
                &currentStatement->Tokens.TU_DATA_S(ModC_ChildStatements);
            
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
    #undef ResultNameState
    #define ResultNameState ModC_Result_Void
    #undef TaggedUnionNameState
    #define TaggedUnionNameState ModC_StatementTokensUnion
    
    CHECK(  statementList->Data[currentParentIndex].Tokens.Type ==
            TU_TYPE_S(ModC_CompoundStatement),
            ("Expecting parent to be type compound, found type index %d instead",
            (int)statementList->Data[currentParentIndex].Tokens.Type),
            RET_ERROR_S());
    
    ModC_StatementIndexList* parentStatementList =
        &statementList  ->Data[currentParentIndex]
                        .Tokens
                        .TU_DATA_S(ModC_CompoundStatement)
                        .ChildStatements;
    ModC_Uint32List_AddValue(parentStatementList, statementIndex);
    
    return RESULT_VALUE_S(0);
}

//Normalizes the statements by removing spaces, comments and newlines and merge operators if possible
static inline ModC_Result_Void ModC_Statement_Normalize(ModC_Statement* statement,
                                                        ModC_Allocator tokensAllcoator,
                                                        ModC_Allocator statementsArena,
                                                        ModC_TokenList* tokens,
                                                        ModC_Allocator scratchAllocator)
{
    #undef ResultNameState
    #define ResultNameState ModC_Result_Void
    #undef TaggedUnionNameState
    #define TaggedUnionNameState ModC_StatementTokensUnion
    
    CHECK(  statement->Tokens.Type != TU_TYPE_S(ModC_CompoundStatement),
            ("Unexpected statement union type"),
            RET_ERROR_S());
        
    ModC_TokenIndexList tokenIndices;
    ModC_String tempMergedOperator;
    
    DEFER_SCOPE_START(0)
    {
        uint32_t tokensCount = ModC_StatementTokensUnion_GetTokenCount(&statement->Tokens);
        tokenIndices = ModC_Uint32List_Create(scratchAllocator, tokensCount);
        DEFER(0, ModC_Uint32List_Free(&tokenIndices));
        
        tempMergedOperator = ModC_String_Create(scratchAllocator, 3);
        DEFER(0, ModC_String_Free(&tempMergedOperator));
        
        uint32_t minLookBack = 0;
        bool skipped = false;
        for(uint32_t i = 0; i < tokensCount; ++i)
        {
            ModC_Result_TokenPtr tokenPtrResult = 
                ModC_StatementTokensUnion_GetTokenAt(&statement->Tokens, tokens, i);
            ModC_Token* currentToken = *RESULT_TRY(tokenPtrResult, DEFER_BREAK(0, RET_ERROR_S()));
            
            if(currentToken->TokenType == ModC_TokenType_Operator)
            {
                bool operatorNext = false;
                if(i != tokensCount - 1)
                {
                    tokenPtrResult = ModC_StatementTokensUnion_GetTokenAt(  &statement->Tokens, 
                                                                            tokens, 
                                                                            i + 1);
                    ModC_Token* nextToken = *RESULT_TRY(tokenPtrResult, DEFER_BREAK(0, RET_ERROR_S()));
                    operatorNext = nextToken->TokenType == ModC_TokenType_Operator;
                }
                
                //If we reached the end of continuous operator tokens,
                //merge operators tokens into a single token if possible
                if(!operatorNext && minLookBack != i)
                {
                    CHECK(minLookBack < i, (""), DEFER_BREAK(0, RET_ERROR_S()));
                    
                    ModC_String_Resize(&tempMergedOperator, 0);
                    for(uint32_t j = minLookBack; j <= i; ++j)
                    {
                        tokenPtrResult = ModC_StatementTokensUnion_GetTokenAt(  &statement->Tokens, 
                                                                                tokens, 
                                                                                j);
                        ModC_Token* lookBackToken = *RESULT_TRY(tokenPtrResult, 
                                                                DEFER_BREAK(0, RET_ERROR_S()));
                        
                        CHECK( lookBackToken->TokenType == ModC_TokenType_Operator, 
                                (""), 
                                DEFER_BREAK(0, RET_ERROR_S()));
                        
                        ModC_ConstStringView opChar = ModC_Token_TokenTextView(lookBackToken);
                        CHECK(  opChar.Length == 1, 
                                ("Unexpected operator text length: %"PRIu32, 
                                opChar.Length),
                                DEFER_BREAK(0, RET_ERROR_S()));
                        
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
                        ModC_Token* minLookBackToken = *RESULT_TRY( tokenPtrResult, 
                                                                    DEFER_BREAK(0, RET_ERROR_S()));
                        
                        //Modify current token to concatenated operator string
                        if(currentToken->TokenText.Type == TU_TYPE(ModC_StringUnion, ModC_String))
                        {
                            ModC_String* tokenStr = 
                                &minLookBackToken->TokenText.TU_DATA( ModC_StringUnion, ModC_String);
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
                            minLookBackToken->TokenText = TU_INIT(  ModC_StringUnion, 
                                                                    ModC_String, 
                                                                    tokenStr);
                        }
                        
                        ModC_Result_Uint32 uint32Result = 
                            ModC_StatementTokensUnion_GetTokenIndexAt(  &statement->Tokens,
                                                                        tokens,
                                                                        minLookBack);
                        uint32_t minLookBackTokenIndex = *RESULT_TRY(   uint32Result, 
                                                                        DEFER_BREAK(0, RET_ERROR_S()));
                        
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
                            uint32_t lookBackTokenIndex = *RESULT_TRY(  uint32Result, 
                                                                        DEFER_BREAK(0, RET_ERROR_S()));
                            ModC_Uint32List_AddValue(&tokenIndices, lookBackTokenIndex);
                        }
                    }
                } //if(!operatorNext && minLookBack != i)
                else if(!operatorNext)
                {
                    ModC_Result_Uint32 uint32Result = 
                        ModC_StatementTokensUnion_GetTokenIndexAt(&statement->Tokens, tokens, i);
                    uint32_t currentTokenIndex = *RESULT_TRY(   uint32Result, 
                                                                DEFER_BREAK(0, RET_ERROR_S()));
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
                uint32_t currentTokenIndex = *RESULT_TRY(   uint32Result, 
                                                            DEFER_BREAK(0, RET_ERROR_S()));
                ModC_Uint32List_AddValue(&tokenIndices, currentTokenIndex);
            }
        } //for(uint32_t i = 0; i < tokensCount; ++i)
        
        //If we have skip any tokens from the original tokens in this statement, replace the tokens 
        //with the new token list for this statement
        if(skipped)
        {
            statement->Tokens = TU_INIT_S(  ModC_TokenIndexList, 
                                            ModC_Uint32List_Create( statementsArena, 
                                                                    tokenIndices.Length));
            ModC_Uint32List_AddRange(   &statement->Tokens.TU_DATA_S(ModC_TokenIndexList),
                                        tokenIndices.Data,
                                        tokenIndices.Length);
        }
    }
    DEFER_SCOPE_END(0)
    
    return RESULT_VALUE_S(0);
}

static inline ModC_Result_Uint32 ModC_Statement_Next(   const ModC_Statement* statement, 
                                                        const ModC_Statement* prevStatement, 
                                                        const ModC_StatementList* statements,
                                                        bool* isEnd)
{
    #undef ResultNameState
    #define ResultNameState ModC_Result_Uint32
    #undef TaggedUnionNameState
    #define TaggedUnionNameState ModC_StatementTokensUnion
    
    CHECK(statement != NULL, (""), RET_ERROR_S());
    CHECK(statements != NULL, (""), RET_ERROR_S());
    
    *isEnd = false;
    
    //Traverse to the next statement
    {
        //TODO: We can probably move this part out
        //If we are at root, try going to first childs
        if(statement->ParentIndex == statement->Index)
        {
            CHECK(  statement->StatementType == ModC_StatementType_Compound, 
                    ("Root must be compound statement"), 
                    RET_ERROR_S());
            
            if(statement->Tokens.TU_DATA_S(ModC_CompoundStatement).ChildStatements.Length == 0)
            {
                *isEnd = true;
                return RESULT_VALUE_S(0);
            }
            
            return RESULT_VALUE_S(statement ->Tokens
                                            .TU_DATA_S(ModC_CompoundStatement)
                                            .ChildStatements
                                            .Data[0]);
        }
        
        //If compound and we are not going up, go to first child if any
        if( statement->StatementType == ModC_StatementType_Compound &&
            prevStatement->ParentIndex != statement->Index &&
            statement->Tokens.TU_DATA_S(ModC_CompoundStatement).ChildStatements.Length != 0)
        {
            return RESULT_VALUE_S(statement ->Tokens
                                            .TU_DATA_S(ModC_CompoundStatement)
                                            .ChildStatements
                                            .Data[0]);
        }
        
        const ModC_Statement* parentStatement = &statements->Data[statement->ParentIndex];
        CHECK(  parentStatement->StatementType == ModC_StatementType_Compound, 
                ("Parent statement must be compound statement"), 
                RET_ERROR_S());
        
        const ModC_CompoundStatement* parentCompound = 
            &parentStatement->Tokens.TU_DATA_S(ModC_CompoundStatement);
        
        uint32_t childIndex = ModC_Uint32List_Find( &parentCompound->ChildStatements, 
                                                    &statement->Index);
        CHECK(  childIndex != parentCompound->ChildStatements.Length, 
                ("Failed to find child in parent. Corrupted Tree?"), 
                RET_ERROR_S());
        
        //Continue to the next child if we are not at the end
        if(childIndex < parentCompound->ChildStatements.Length - 1)
            return RESULT_VALUE_S(parentCompound->ChildStatements.Data[childIndex + 1]);
        //Otherwise go up if we are not under root
        else if(parentStatement->Index != parentStatement->ParentIndex)
            return RESULT_VALUE_S(parentStatement->Index);
        //Otherwise, we are done
        else
        {
            *isEnd = true;
            return RESULT_VALUE_S(0);
        }
    }
}

static inline ModC_Result_Uint32 ModC_EndCurrentStatement(  bool countCurrentToken, 
                                                            uint32_t i, 
                                                            uint32_t currentParentIndex,
                                                            ModC_Allocator sharedArena,
                                                            const ModC_TokenList* tokens,
                                                            uint32_t* startTokenIndex,
                                                            ModC_StatementList* statementList)
{
    #undef ResultNameState
    #define ResultNameState ModC_Result_Uint32
    #undef TaggedUnionNameState
    #define TaggedUnionNameState ModC_StatementTokensUnion
    
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
        return RESULT_VALUE_S(i);
    }
    
    ModC_ResultStatementPtr statementPtrResult = ModC_Statement_CreatePlain(sharedArena,
                                                                            statementList,
                                                                            currentParentIndex);
    ModC_Statement* prevStatementPtr = *RESULT_TRY(statementPtrResult, RET_ERROR_S());
    CHECK(prevStatementPtr->Tokens.Type == TU_TYPE_S(ModC_TokenIndexRange), (""), RET_ERROR_S());
    ModC_TokenIndexRange* indexRange = &prevStatementPtr->Tokens.TU_DATA_S(ModC_TokenIndexRange);
    
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

    (void)RESULT_TRY(voidResult, RET_ERROR_S());
    return RESULT_VALUE_S(i);
}

static inline ModC_Result_StatementList ModC_CreateStatements(  const ModC_TokenList* tokens, 
                                                                const ModC_ConstStringView source,
                                                                ModC_Allocator scratchAllocator,
                                                                ModC_Allocator* outStatementsArena)
{
    #undef ResultNameState
    #define ResultNameState ModC_Result_StatementList
    #undef TaggedUnionNameState
    #define TaggedUnionNameState ModC_StatementTokensUnion
    
    CHECK(tokens != NULL, (""), RET_ERROR_S());
    CHECK(outStatementsArena != NULL, (""), RET_ERROR_S());
    
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
            i = *RESULT_TRY(uint32Result, RET_ERROR_S()); \
        } \
        while(false)
    
    #define CHECK_AND_VISUALIZE_ERROR(cond, msg) \
        do \
        { \
            if(!(cond)) \
            { \
                ModC_String visualizeStr = ModC_Token_VisualizeLocation(&tokens->Data[i], \
                                                                        ModC_CreateHeapAllocator(), \
                                                                        false, \
                                                                        source); \
                \
                return ERROR_STR_FMT_S((msg "\n%.*s", \
                                        visualizeStr.Length, \
                                        visualizeStr.Data)); \
            } \
        } \
        while(0)
    
    uint32_t startTokenIndex = 0;
    uint32_t currentParentIndex = 0;
    ModC_BoolList blockStartComplex = ModC_BoolList_Create(scratchAllocator, 16);
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
            {
                //`case xxx:` count as a statement
                ModC_ConstStringView tokenView = ModC_Token_TokenTextView(&tokens->Data[i]);
                if(!ModC_ConstStringView_IsEqualLiteral(&tokenView, ":"))
                    break;
                
                END_CURRENT_STATEMENT(true);
                break;
            }
            case ModC_CharTokenType_BlockStart:
            {
                //Find the first previous token that we care
                uint32_t lastTokenIndex = i;
                for(int32_t j = i - 1; j >= startTokenIndex; --j)
                {
                    if( tokens->Data[j].TokenType == ModC_TokenType_Space ||
                        tokens->Data[j].TokenType == ModC_TokenType_Newline ||
                        tokens->Data[j].TokenType == ModC_TokenType_Comment)
                    {
                        continue;
                    }
                    lastTokenIndex = j;
                    break;
                }

                if(lastTokenIndex != i)
                {
                    //Not complex statement
                    if( tokens->Data[lastTokenIndex].TokenType != ModC_TokenType_Identifier &&
                        tokens->Data[lastTokenIndex].TokenType != ModC_TokenType_InvokeEnd)
                    {
                        ModC_BoolList_AddValue(&blockStartComplex, false);
                        break;
                    }
                    
                    END_CURRENT_STATEMENT(false);
                }
                
                ModC_BoolList_AddValue(&blockStartComplex, true);
                
                //Create compound as parent
                statementPtrResult = ModC_Statement_CreateCompound( sharedArena, 
                                                                    &statementList, 
                                                                    currentParentIndex,
                                                                    false,
                                                                    32);
                ModC_Statement* newStatement = *RESULT_TRY(statementPtrResult, RET_ERROR_S());
                CHECK_AND_VISUALIZE_ERROR(  newStatement->Tokens.Type == 
                                            TU_TYPE_S(ModC_CompoundStatement),
                                            "Unexpected type");
                ModC_Result_Void voidResult = ModC_AddStatementToParent(statementList.Length - 1,
                                                                        currentParentIndex,
                                                                        &statementList);
                (void)RESULT_TRY(voidResult, RET_ERROR_S());
                
                ModC_CompoundStatement* compoundData = 
                    &newStatement->Tokens.TU_DATA_S(ModC_CompoundStatement);
                compoundData->StartTokenIndex = i;
                startTokenIndex = i + 1;
                currentParentIndex = statementList.Length - 1;
                break;
            }
            case ModC_CharTokenType_BlockEnd:
            {
                if(blockStartComplex.Length == 0)   //Mismatching number of block start and ends
                    break;
                
                if(!blockStartComplex.Data[blockStartComplex.Length - 1])
                {
                    ModC_BoolList_Resize(&blockStartComplex, blockStartComplex.Length - 1);
                    break;
                }
                ModC_BoolList_Resize(&blockStartComplex, blockStartComplex.Length - 1);
                
                //Finish compound parent
                END_CURRENT_STATEMENT(false);
                ModC_Statement* parentStatement = &statementList.Data[currentParentIndex];
                CHECK_AND_VISUALIZE_ERROR(  parentStatement->Tokens.Type == 
                                            TU_TYPE_S(ModC_CompoundStatement), 
                                            "Unexpected type");
                
                ModC_CompoundStatement* parentCompound = 
                    &parentStatement->Tokens.TU_DATA_S(ModC_CompoundStatement);
                CHECK_AND_VISUALIZE_ERROR(  !parentCompound->Implicit,
                                            "Expected non implicit for parent when block end");
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
                
                //Check compiler directives (#)
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
    
    #undef END_CURRENT_STATEMENT
    #undef CHECK_AND_VISUALIZE_ERROR
    
    return RESULT_VALUE_S(statementList);
}

#endif
