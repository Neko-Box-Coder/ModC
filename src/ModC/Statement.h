#ifndef MODC_STATEMENT_H
#define MODC_STATEMENT_H

#ifndef DEFAULT_ALLOC
    #define DEFAULT_ALLOC() CreateHeapAllocator()
#endif

#include "ModC/Tokenization.h"
#include "ModC/GenericContainers.h"
#include "ModC/Result.h"
#include "ModC/Operators.h"
#include "ModC/Keyword.h"

typedef enum StatementType
{
    StatementType_Invalid,
    StatementType_Unknown,
    StatementType_TypeDeclaration,
    StatementType_EnumValues,
    StatementType_FunctionDeclaration,
    StatementType_VariableDeclaration,
    StatementType_Compound,
    StatementType_CompilerDirective,
    StatementType_Assignment,
    StatementType_VariableDeclareAssignment,
    StatementType_PureExpression,
    StatementType_IfStatement,
    StatementType_ElseStatement,
    StatementType_ForStatement,
    StatementType_WhileStatement,
    StatementType_SwitchStatement,
    StatementType_ReturnStatement,
    StatementType_CaseStatement,
    StatementType_Count,   //18
} StatementType;

struct StatementList;
typedef struct StatementList StatementList;

typedef struct TokenIndexRange
{
    uint32_t StartIndex;    //Inclusive
    uint32_t EndIndex;      //Exclusive
} TokenIndexRange;

typedef Uint32List StatementIndexList;

//TODO: Add function for checking start and end token alignment
typedef struct CompoundStatement
{
    uint32_t StartTokenIndex;
    StatementIndexList ChildStatements;
    uint32_t EndTokenIndex;
    bool Implicit;      //No start and end token index if true
} CompoundStatement;

typedef Uint32List TokenIndexList;

#define TU_NAME StatementTokensUnion
#define VALUE_TYPES CompoundStatement,TokenIndexList,TokenIndexRange
#include "ModC/TaggedUnion.h"

typedef struct TypeDeclarationInfo
{
    enum
    {
        Type_Struct,
        Type_Enum
    } Type;
    
    String TypeName;   //TODO: Use token index instead
} TypeDeclarationInfo;

typedef struct VariableDeclareAssignInfo
{
    uint32_t TypeIndexInStatement;
    uint32_t IdentifierIndexInStatement;
    bool HasAsignment;
    uint32_t AssignIndexInStatement;
} VariableDeclareAssignInfo;

typedef struct FunctionDeclarationInfo
{
    uint32_t TypeIndexInStatement;
    uint32_t IdentifierIndexInStatement;
    bool HaveArguments;
    uint32_t ArgumentIndexInStatement;
} FunctionDeclarationInfo;

typedef struct AssignmentInfo
{
    uint32_t AssignIndexInStatement;
} AssignmentInfo;


#define TU_NAME StatementInfoUnion
#define VALUE_TYPES Void, \
                    TypeDeclarationInfo, \
                    VariableDeclareAssignInfo, \
                    FunctionDeclarationInfo, \
                    AssignmentInfo
#include "ModC/TaggedUnion.h"

typedef struct Statement Statement;
struct Statement
{
    StatementType StatementType;
    StatementTokensUnion Tokens;
    StatementInfoUnion Info;
    uint32_t Index;
    uint32_t ParentIndex;
};

#define LIST_NAME StatementList
#define VALUE_TYPE Statement
#define NO_TYPEDEF 1
//TODO: Value item free
#include "ModC/List.h"

DEFINE_RESULT_STRUCT(ResultStatementPtr, Statement*)
DEFINE_RESULT_STRUCT(Result_ConstStringView, ConstStringView)
DEFINE_RESULT_STRUCT(Result_StatementList, StatementList)

#include "static_assert.h/assert.h"

#include <stdbool.h>
#include <stdint.h>

//TODO: Use statement to get token count instead
static inline uint32_t StatementTokensUnion_GetTokenCount(const StatementTokensUnion* statementUnion)
{
    #undef TaggedUnionNameState
    #define TaggedUnionNameState StatementTokensUnion
    
    if(!statementUnion)
        return 0;
    
    switch(statementUnion->Type)
    {
        case TU_TYPE_S(CompoundStatement):
            return 2;
        case TU_TYPE_S(TokenIndexList):
            return statementUnion->TU_DATA_S(TokenIndexList).Length;
        case TU_TYPE_S(TokenIndexRange):
        {
            const TokenIndexRange* range = 
                &statementUnion->TU_DATA_S(TokenIndexRange);
            return range->EndIndex - range->StartIndex;
        }
        default:
            return 0;
    }
}

static inline Result_Uint32
StatementTokensUnion_GetTokenIndexAt(   const StatementTokensUnion* statementUnion, 
                                        const TokenList* tokens,
                                        uint32_t indexInStatement)
{
    #undef ResultNameState
    #define ResultNameState Result_Uint32
    #undef TaggedUnionNameState
    #define TaggedUnionNameState StatementTokensUnion
    
    CHECK(statementUnion != NULL, (""), RET_ERROR_S());
    CHECK(tokens != NULL, (""), RET_ERROR_S());
    
    uint32_t tokenIndex = 0;
    switch(statementUnion->Type)
    {
        case TU_TYPE_S(CompoundStatement):
        {
            //NOTE: Shouldn't use this function for compound statement..., but whatever
            const CompoundStatement* compound = &statementUnion->TU_DATA_S(CompoundStatement);
            if(indexInStatement == 0)
                tokenIndex = compound->StartTokenIndex;
            else if(indexInStatement == 1)
                tokenIndex = compound->EndTokenIndex;
            else
                return ERROR_STR_FMT_S("Invalid index for accessing %"PRIu32, indexInStatement);
            break;
        }
        case TU_TYPE_S(TokenIndexList):
        {
            const TokenIndexList* tokenIndexList = &statementUnion->TU_DATA_S(TokenIndexList);
            CHECK(tokenIndexList->Length > 0, ("Empty statement"), RET_ERROR_S());
            CHECK(  indexInStatement < tokenIndexList->Length, 
                    ("Invalid index for accessing, index: %"PRIu32", length: %"PRIu64, 
                    indexInStatement, tokenIndexList->Length),
                    RET_ERROR_S());
            tokenIndex = tokenIndexList->Data[indexInStatement];
            break;
        }
        case TU_TYPE_S(TokenIndexRange):
        {
            const TokenIndexRange* range = &statementUnion->TU_DATA_S(TokenIndexRange);
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

static inline Result_TokenPtr 
StatementTokensUnion_GetTokenAt(const StatementTokensUnion* statementUnion, 
                                const TokenList* tokens,
                                uint32_t indexInStatement)
{
    #undef ResultNameState
    #define ResultNameState Result_TokenPtr
    
    Result_Uint32 uint32Result = StatementTokensUnion_GetTokenIndexAt(  statementUnion, 
                                                                        tokens, 
                                                                        indexInStatement);
    uint32_t tokenIndex = *RESULT_TRY(uint32Result, RET_ERROR_S());
    return RESULT_VALUE_S(&tokens->Data[tokenIndex]);
}

static inline Result_ConstStringView 
StatementTokensUnion_GetTokenTextViewAt(const StatementTokensUnion* statementUnion, 
                                        const TokenList* tokens,
                                        uint32_t indexInStatement)
{
    #undef ResultNameState
    #define ResultNameState Result_ConstStringView 
    
    Token* token = NULL;
    {
        Result_TokenPtr tokenPtrResult = StatementTokensUnion_GetTokenAt(   statementUnion, 
                                                                            tokens, 
                                                                            indexInStatement);
        token = *RESULT_TRY(tokenPtrResult, RET_ERROR_S());
    }
    
    return RESULT_VALUE_S(Token_TokenTextView(token));
}


static inline Result_Uint32
StatementTokensUnion_ContainsTokenText( const StatementTokensUnion* statementUnion, 
                                        const TokenList* tokens,
                                        ConstStringView checkText)
{
    #undef ResultNameState
    #define ResultNameState Result_Uint32
    
    uint32_t tokensCount = StatementTokensUnion_GetTokenCount(statementUnion);
    for(uint32_t i = 0; i < tokensCount; ++i)
    {
        Result_ConstStringView constStringViewResult =
            StatementTokensUnion_GetTokenTextViewAt(statementUnion, tokens, i);
        
        ConstStringView tokenStringView = *RESULT_TRY(constStringViewResult, RET_ERROR_S());
        if(StringLikeEqual(tokenStringView, checkText))
            return RESULT_VALUE_S(i);
    }
    
    return RESULT_VALUE_S(tokensCount);
}

static inline ConstStringView StatementType_ToConstStringView(StatementType type)
{
    static_assert((int)StatementType_Count == 18, "");
    switch(type)
    {
        #define RET_TO_STR(enumVal) \
            case enumVal: \
                return ConstStringView_FromLiteral(#enumVal)
        
        RET_TO_STR(StatementType_Invalid);
        RET_TO_STR(StatementType_Unknown);
        RET_TO_STR(StatementType_TypeDeclaration);
        RET_TO_STR(StatementType_EnumValues);
        RET_TO_STR(StatementType_FunctionDeclaration);
        RET_TO_STR(StatementType_VariableDeclaration);
        RET_TO_STR(StatementType_Compound);
        RET_TO_STR(StatementType_CompilerDirective);
        RET_TO_STR(StatementType_Assignment);
        RET_TO_STR(StatementType_VariableDeclareAssignment);
        RET_TO_STR(StatementType_PureExpression);
        RET_TO_STR(StatementType_IfStatement);
        RET_TO_STR(StatementType_ElseStatement);
        RET_TO_STR(StatementType_ForStatement);
        RET_TO_STR(StatementType_WhileStatement);
        RET_TO_STR(StatementType_SwitchStatement);
        RET_TO_STR(StatementType_ReturnStatement);
        RET_TO_STR(StatementType_CaseStatement);
        RET_TO_STR(StatementType_Count);
        
        #undef RET_TO_STR
    }
    return (ConstStringView){0};
}


static inline ResultStatementPtr Statement_CreateCompound(  Allocator allocator, 
                                                            StatementList* statementList,
                                                            uint32_t parentIndex,
                                                            bool implicit,
                                                            uint32_t reserveStatementsCount)
{
    #undef ResultNameState
    #define ResultNameState ResultStatementPtr
    #undef TaggedUnionNameState
    #define TaggedUnionNameState StatementTokensUnion
    
    CHECK(statementList != NULL, (""), RET_ERROR_S());
    CHECK(allocator.Type == AllocatorType_SharedArena, (""), RET_ERROR_S());
    
    uint64_t oldLength = statementList->Length;
    StatementIndexList childStatements = Uint32List_Create(allocator, reserveStatementsCount);
    CHECK(childStatements.Cap > 0, ("Failed to allocate"), RET_ERROR_S());
    StatementList_AddValue( statementList, 
                            (Statement)
                            {
                                .StatementType = StatementType_Compound,
                                .Tokens = TU_INIT_S(CompoundStatement, 
                                                    {
                                                        .StartTokenIndex = 0,
                                                        .ChildStatements = childStatements,
                                                        .EndTokenIndex = 0,
                                                        .Implicit = implicit
                                                    }),
                                .Info = TU_INIT(StatementInfoUnion, Void, 0),
                                .Index = oldLength,
                                .ParentIndex = parentIndex
                            });
    childStatements = (StatementIndexList){0};
    CHECK(statementList->Length != oldLength, ("Failed to allocate"), RET_ERROR_S());
    
    Statement* retStatementPtr = &statementList->Data[statementList->Length - 1];
    return RESULT_VALUE_S(retStatementPtr);
}

static inline ResultStatementPtr Statement_CreatePlain( Allocator allocator, 
                                                        StatementList* statementList,
                                                        uint32_t parentIndex)
{
    #undef ResultNameState
    #define ResultNameState ResultStatementPtr
    #undef TaggedUnionNameState
    #define TaggedUnionNameState StatementTokensUnion
    
    CHECK(statementList != NULL, (""), RET_ERROR_S());
    CHECK(allocator.Type == AllocatorType_SharedArena, (""), RET_ERROR_S());
    
    uint64_t oldLength = statementList->Length;
    StatementList_AddValue( statementList, 
                            (Statement)
                            {
                                .StatementType = StatementType_Unknown,
                                .Tokens = TU_INIT_S(TokenIndexRange, {0}),
                                .Info = TU_INIT(StatementInfoUnion, Void, 0),
                                .Index = oldLength,
                                .ParentIndex = parentIndex
                            });
    CHECK(statementList->Length != oldLength, ("Failed to allocate"), RET_ERROR_S());
    
    Statement* retStatementPtr = &statementList->Data[statementList->Length - 1];
    //CHECK(  retStatementPtr->Tokens.TU_DATA(StatementTokensUnion, TokenIndexList).Cap > 0,
    //        ("Failed to allocate"), 
    //        RET_ERROR_S());
    
    return RESULT_VALUE_S(retStatementPtr);
}

static inline Result_Void Statement_ToString(   Statement* this, 
                                                TokenList* tokenList,
                                                String* inOutString, 
                                                bool append)
{
    #undef ResultNameState
    #define ResultNameState Result_Void
    #undef TaggedUnionNameState
    #define TaggedUnionNameState StatementTokensUnion
    
    if(!inOutString || !tokenList)
        return RESULT_VALUE_S(0);
    
    if(!append)
        String_Resize(inOutString, 0);
    
    ConstStringView statementTypeStr = StatementType_ToConstStringView(this->StatementType);
    String_AddRange(inOutString, statementTypeStr.Data, statementTypeStr.Length);
    String_AppendLiteral(inOutString, " - ");
    
    static_assert((int)TU_TYPE_S(Count) == 3, "");
    switch(this->Tokens.Type)
    {
        case TU_TYPE_S(CompoundStatement):
        {
            CompoundStatement* compoundStatement = &this->Tokens.TU_DATA_S(CompoundStatement);
            String_AppendFormat(inOutString, 
                                "CompoundStatement: %" PRIu32 " - %" PRIu32 " "
                                "(Implicit: %s), (Children: ", 
                                compoundStatement->StartTokenIndex,
                                compoundStatement->EndTokenIndex,
                                (compoundStatement->Implicit ? "true" : "false"));
            
            if(compoundStatement->ChildStatements.Length >= 2)
            {
                for(int j = 0; j < compoundStatement->ChildStatements.Length - 1; ++j)
                {
                    String_AppendFormat(inOutString, 
                                        "%" PRIu32 ", ", 
                                        compoundStatement->ChildStatements.Data[j]);
                }
            }
            
            if(compoundStatement->ChildStatements.Length > 0)
            {
                uint32_t lastIndex = compoundStatement->ChildStatements.Length - 1;
                String_AppendFormat(inOutString, 
                                    "%" PRIu32, 
                                    compoundStatement->ChildStatements.Data[lastIndex]);
            }
            String_AddValue(inOutString, ')');
            break;
        }
        case TU_TYPE_S(TokenIndexList):
        {
            TokenIndexList* tokenIndexList = &this->Tokens.TU_DATA_S(TokenIndexList);
            String_AppendFormat(inOutString, "TokenIndexList: (Indices: ");
            if(tokenIndexList->Length >= 2)
            {
                for(int j = 0; j < tokenIndexList->Length - 1; ++j)
                    String_AppendFormat(inOutString, "%" PRIu32 ", ", tokenIndexList->Data[j]);
            }
            
            if(tokenIndexList->Length > 0)
            {
                String_AppendFormat(inOutString, 
                                    "%" PRIu32, 
                                    tokenIndexList->Data[tokenIndexList->Length - 1]);
            }
            
            String_AppendFormat(inOutString, "), \"");
            for(int j = 0; j < tokenIndexList->Length; ++j)
            {
                ConstStringView tokenText = 
                    Token_TokenTextView(&tokenList->Data[tokenIndexList->Data[j]]);
                CHECK(tokenText.Length > 0, ("Invalid token text"), RET_ERROR_S());
                String_AppendFormat(inOutString, "%.*s ", (int)tokenText.Length, tokenText.Data);
            }
            String_AppendLiteral(inOutString, "\"");
            break;
        }
        case TU_TYPE_S(TokenIndexRange):
        {
            TokenIndexRange* tokenIndexRange = &this->Tokens.TU_DATA_S(TokenIndexRange);
            String_AppendFormat(inOutString, 
                                "TokenIndexRange: %" PRIu32 " - %" PRIu32 ", \"",
                                tokenIndexRange->StartIndex,
                                tokenIndexRange->EndIndex);
            
            for(int j = tokenIndexRange->StartIndex; j < tokenIndexRange->EndIndex; ++j)
            {
                ConstStringView tokenText = Token_TokenTextView(&tokenList->Data[j]);
                CHECK(tokenText.Length > 0, ("Invalid token text"), RET_ERROR_S());
                String_AppendFormat(inOutString, "%.*s ", (int)tokenText.Length, tokenText.Data);
            }
            String_AppendLiteral(inOutString, "\"");
            break;
        }
        default:
            break;
    } //switch(this->Tokens.Type)
    
    return RESULT_VALUE_S(0);
}

#if 0
    void Statement_Free(Statement* this)
    {
        #undef TaggedUnionNameState
        #define TaggedUnionNameState StatementTokensUnion
        
        if(!this)
            return;
        
        //No children, just free the token list
        if(this->Tokens.Type == TU_TYPE_S(TokenIndexList))
        {
            Uint32List_Free(&this->Tokens.TU_DATA_S(TokenIndexList));
            *this = (Statement){0};
            return;
        }
        
        //Get to the deepest child
        Statement* currentStatement = this;
        while(currentStatement->Tokens.Type == TU_TYPE_S(ChildStatements))
        {
            ChildStatements* currentChildren =
                &currentStatement->Tokens.TU_DATA_S(ChildStatements);
            
            if(currentChildren->Statements->Length == 0)
                break;
            
            currentStatement = &currentChildren->Statements->Data[0];
        }
        
        //Walk and free the whole tree
    }
#endif

static inline Result_Void AddStatementToParent( uint32_t statementIndex, 
                                                uint32_t currentParentIndex,
                                                StatementList* statementList)
{
    #undef ResultNameState
    #define ResultNameState Result_Void
    #undef TaggedUnionNameState
    #define TaggedUnionNameState StatementTokensUnion
    
    CHECK(  statementList->Data[currentParentIndex].Tokens.Type ==
            TU_TYPE_S(CompoundStatement),
            ("Expecting parent to be type compound, found type index %d instead",
            (int)statementList->Data[currentParentIndex].Tokens.Type),
            RET_ERROR_S());
    
    StatementIndexList* parentStatementList = &statementList->Data[currentParentIndex]
                                                            .Tokens
                                                            .TU_DATA_S(CompoundStatement)
                                                            .ChildStatements;
    Uint32List_AddValue(parentStatementList, statementIndex);
    
    return RESULT_VALUE_S(0);
}

//Normalizes the statements by removing spaces, comments and newlines and merge operators if possible
static inline Result_Void Statement_Normalize(  Statement* statement,
                                                Allocator tokensAllcoator,
                                                Allocator statementsArena,
                                                TokenList* tokens,
                                                Allocator scratchAllocator)
{
    #undef ResultNameState
    #define ResultNameState Result_Void
    #undef TaggedUnionNameState
    #define TaggedUnionNameState StatementTokensUnion
    
    CHECK(  statement->Tokens.Type != TU_TYPE_S(CompoundStatement),
            ("Unexpected statement union type"),
            RET_ERROR_S());
        
    TokenIndexList tokenIndices;
    String tempMergedOperator;
    
    DEFER_SCOPE_START(0)
    {
        uint32_t tokensCount = StatementTokensUnion_GetTokenCount(&statement->Tokens);
        tokenIndices = Uint32List_Create(scratchAllocator, tokensCount);
        DEFER(0, Uint32List_Free(&tokenIndices));
        
        tempMergedOperator = String_Create(scratchAllocator, 3);
        DEFER(0, String_Free(&tempMergedOperator));
        
        uint32_t minLookBack = 0;
        bool skipped = false;
        for(uint32_t i = 0; i < tokensCount; ++i)
        {
            Result_TokenPtr tokenPtrResult = StatementTokensUnion_GetTokenAt(   &statement->Tokens, 
                                                                                tokens, 
                                                                                i);
            Token* currentToken = *RESULT_TRY(tokenPtrResult, DEFER_BREAK(0, RET_ERROR_S()));
            if(currentToken->TokenType == TokenType_Operator)
            {
                bool operatorNext = false;
                if(i != tokensCount - 1)
                {
                    tokenPtrResult = StatementTokensUnion_GetTokenAt(   &statement->Tokens, 
                                                                        tokens, 
                                                                        i + 1);
                    Token* nextToken = *RESULT_TRY(tokenPtrResult, DEFER_BREAK(0, RET_ERROR_S()));
                    operatorNext = nextToken->TokenType == TokenType_Operator;
                }
                
                //If we reached the end of continuous operator tokens,
                //merge operators tokens into a single token if possible
                if(!operatorNext && minLookBack != i)
                {
                    CHECK(minLookBack < i, (""), DEFER_BREAK(0, RET_ERROR_S()));
                    
                    String_Resize(&tempMergedOperator, 0);
                    for(uint32_t j = minLookBack; j <= i; ++j)
                    {
                        tokenPtrResult = StatementTokensUnion_GetTokenAt(  &statement->Tokens, 
                                                                                tokens, 
                                                                                j);
                        Token* lookBackToken = *RESULT_TRY(tokenPtrResult, 
                                                                DEFER_BREAK(0, RET_ERROR_S()));
                        
                        CHECK( lookBackToken->TokenType == TokenType_Operator, 
                                (""), 
                                DEFER_BREAK(0, RET_ERROR_S()));
                        
                        ConstStringView opChar = Token_TokenTextView(lookBackToken);
                        CHECK(  opChar.Length == 1, 
                                ("Unexpected operator text length: %"PRIu32, 
                                opChar.Length),
                                DEFER_BREAK(0, RET_ERROR_S()));
                        
                        String_AddValue(&tempMergedOperator, opChar.Data[0]);
                    }
                    
                    //If the merged tokens are valid, modify the first (continuous) token to be merged
                    ConstStringView mergedView = 
                        ConstStringView_Create(tempMergedOperator.Data, 
                                                    tempMergedOperator.Length);
                    if(ModC_IsValidComplexOperator(mergedView))
                    {
                        skipped = true;
                        tokenPtrResult = StatementTokensUnion_GetTokenAt(  &statement->Tokens, 
                                                                                tokens, 
                                                                                minLookBack);
                        Token* minLookBackToken = *RESULT_TRY(  tokenPtrResult, 
                                                                DEFER_BREAK(0, RET_ERROR_S()));
                        
                        //Modify current token to concatenated operator string
                        if(currentToken->TokenText.Type == TU_TYPE(StringUnion, String))
                        {
                            String* tokenStr = 
                                &minLookBackToken->TokenText.TU_DATA( StringUnion, String);
                            String_AddRange(tokenStr, 
                                            &tempMergedOperator.Data[1], 
                                            tempMergedOperator.Length - 1);
                        }
                        else
                        {
                            String tokenStr = String_FromData(  tokensAllcoator, 
                                                                tempMergedOperator.Data, 
                                                                tempMergedOperator.Length);
                            minLookBackToken->TokenText = TU_INIT(  StringUnion, 
                                                                    String, 
                                                                    tokenStr);
                        }
                        
                        Result_Uint32 uint32Result = 
                            StatementTokensUnion_GetTokenIndexAt(   &statement->Tokens,
                                                                    tokens,
                                                                    minLookBack);
                        uint32_t minLookBackTokenIndex = *RESULT_TRY(   uint32Result, 
                                                                        DEFER_BREAK(0, RET_ERROR_S()));
                        
                        Uint32List_AddValue(&tokenIndices, minLookBackTokenIndex);
                    }
                    //Otherwise just add the tokens as they are
                    else
                    {
                        for(uint32_t j = minLookBack; j <= i; ++j)
                        {
                            tokenPtrResult = 
                                StatementTokensUnion_GetTokenAt(&statement->Tokens, tokens, j);
                            Result_Uint32 uint32Result = 
                                StatementTokensUnion_GetTokenIndexAt(&statement->Tokens, tokens, j);
                            uint32_t lookBackTokenIndex = *RESULT_TRY(  uint32Result, 
                                                                        DEFER_BREAK(0, RET_ERROR_S()));
                            Uint32List_AddValue(&tokenIndices, lookBackTokenIndex);
                        }
                    }
                } //if(!operatorNext && minLookBack != i)
                else if(!operatorNext)
                {
                    Result_Uint32 uint32Result = 
                        StatementTokensUnion_GetTokenIndexAt(&statement->Tokens, tokens, i);
                    uint32_t currentTokenIndex = *RESULT_TRY(   uint32Result, 
                                                                DEFER_BREAK(0, RET_ERROR_S()));
                    Uint32List_AddValue(&tokenIndices, currentTokenIndex);
                }
            } //if(currentToken->TokenType == TokenType_Operator)
            //Skip all comments, spaces, newlines, etc...
            else if(Token_IsSkippable(currentToken))
            {
                //TODO: Attach comments to statements
                skipped = true;
                minLookBack = i + 1;
            }
            //Otherwise, don't skip current token
            else
            {
                minLookBack = i + 1;
                Result_Uint32 uint32Result = 
                    StatementTokensUnion_GetTokenIndexAt(&statement->Tokens, tokens, i);
                uint32_t currentTokenIndex = *RESULT_TRY(   uint32Result, 
                                                            DEFER_BREAK(0, RET_ERROR_S()));
                Uint32List_AddValue(&tokenIndices, currentTokenIndex);
            }
        } //for(uint32_t i = 0; i < tokensCount; ++i)
        
        //If we have skip any tokens from the original tokens in this statement, replace the tokens 
        //with the new token list for this statement
        if(skipped)
        {
            statement->Tokens = TU_INIT_S(  TokenIndexList, 
                                            Uint32List_Create( statementsArena, tokenIndices.Length));
            Uint32List_AddRange(&statement->Tokens.TU_DATA_S(TokenIndexList),
                                tokenIndices.Data,
                                tokenIndices.Length);
        }
    }
    DEFER_SCOPE_END(0)
    
    return RESULT_VALUE_S(0);
}

static inline Result_Uint32 Statement_Next( const Statement* statement, 
                                            const Statement* prevStatement, 
                                            const StatementList* statements,
                                            bool* isEnd)
{
    #undef ResultNameState
    #define ResultNameState Result_Uint32
    #undef TaggedUnionNameState
    #define TaggedUnionNameState StatementTokensUnion
    
    CHECK(statement != NULL, (""), RET_ERROR_S());
    CHECK(statements != NULL, (""), RET_ERROR_S());
    
    *isEnd = false;
    
    //Traverse to the next statement
    {
        //TODO: We can probably move this part out
        //If we are at root, try going to first childs
        if(statement->ParentIndex == statement->Index)
        {
            CHECK(  statement->StatementType == StatementType_Compound, 
                    ("Root must be compound statement"), 
                    RET_ERROR_S());
            
            if(statement->Tokens.TU_DATA_S(CompoundStatement).ChildStatements.Length == 0)
            {
                *isEnd = true;
                return RESULT_VALUE_S(0);
            }
            
            return RESULT_VALUE_S(statement ->Tokens
                                            .TU_DATA_S(CompoundStatement)
                                            .ChildStatements
                                            .Data[0]);
        }
        
        //If compound and we are not going up, go to first child if any
        if( statement->StatementType == StatementType_Compound &&
            prevStatement->ParentIndex != statement->Index &&
            statement->Tokens.TU_DATA_S(CompoundStatement).ChildStatements.Length != 0)
        {
            return RESULT_VALUE_S(statement ->Tokens
                                            .TU_DATA_S(CompoundStatement)
                                            .ChildStatements
                                            .Data[0]);
        }
        
        const Statement* parentStatement = &statements->Data[statement->ParentIndex];
        CHECK(  parentStatement->StatementType == StatementType_Compound, 
                ("Parent statement must be compound statement"), 
                RET_ERROR_S());
        
        const CompoundStatement* parentCompound = 
            &parentStatement->Tokens.TU_DATA_S(CompoundStatement);
        
        uint32_t childIndex = Uint32List_Find(&parentCompound->ChildStatements, &statement->Index);
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

static inline Result_Uint32 EndCurrentStatement(bool countCurrentToken, 
                                                uint32_t i, 
                                                uint32_t currentParentIndex,
                                                Allocator sharedArena,
                                                const TokenList* tokens,
                                                uint32_t* startTokenIndex,
                                                StatementList* statementList)
{
    #undef ResultNameState
    #define ResultNameState Result_Uint32
    #undef TaggedUnionNameState
    #define TaggedUnionNameState StatementTokensUnion
    
    bool allWhiteSpaceOrNewline = true;
    uint32_t endIndex = countCurrentToken ? i + 1 : i;
    for(uint32_t checkIndex = *startTokenIndex; checkIndex < endIndex; ++checkIndex)
    {
        if( tokens->Data[checkIndex].TokenType != TokenType_Space &&
            tokens->Data[checkIndex].TokenType != TokenType_Newline &&
            tokens->Data[checkIndex].TokenType != TokenType_Comment)
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
    
    ResultStatementPtr statementPtrResult = Statement_CreatePlain(  sharedArena,
                                                                    statementList,
                                                                    currentParentIndex);
    Statement* prevStatementPtr = *RESULT_TRY(statementPtrResult, RET_ERROR_S());
    CHECK(prevStatementPtr->Tokens.Type == TU_TYPE_S(TokenIndexRange), (""), RET_ERROR_S());
    TokenIndexRange* indexRange = &prevStatementPtr->Tokens.TU_DATA_S(TokenIndexRange);
    
    indexRange->StartIndex = *startTokenIndex;
    indexRange->EndIndex = countCurrentToken ? i + 1 : i;
    
    //TODO: Maybe not needed
    //Trim newlines, spaces and comments
    if(false)
    {
        uint32_t j = 0;
        for(j = indexRange->StartIndex; j < indexRange->EndIndex; ++j)
        {
            if( tokens->Data[j].TokenType != TokenType_Space &&
                tokens->Data[j].TokenType != TokenType_Newline &&
                tokens->Data[j].TokenType != TokenType_Comment)
            {
                break;
            }
        }
        indexRange->StartIndex = j;
        
        for(j = indexRange->EndIndex - 1; j >= indexRange->StartIndex; --j)
        {
            if( tokens->Data[j].TokenType != TokenType_Space &&
                tokens->Data[j].TokenType != TokenType_Newline &&
                tokens->Data[j].TokenType != TokenType_Comment)
            {
                break;
            }
        }
        indexRange->EndIndex = j + 1;
    }
    
    //TODO: Attach comments to statements
    
    *startTokenIndex = countCurrentToken ? ++i : i;
    
    Result_Void voidResult = AddStatementToParent(  statementList->Length - 1, 
                                                    currentParentIndex, 
                                                    statementList);

    (void)RESULT_TRY(voidResult, RET_ERROR_S());
    return RESULT_VALUE_S(i);
}

static inline Result_StatementList CreateStatements(const TokenList* tokens, 
                                                    const ConstStringView source,
                                                    Allocator scratchAllocator,
                                                    Allocator* outStatementsArena)
{
    #undef ResultNameState
    #define ResultNameState Result_StatementList
    #undef TaggedUnionNameState
    #define TaggedUnionNameState StatementTokensUnion
    
    CHECK(tokens != NULL, (""), RET_ERROR_S());
    CHECK(outStatementsArena != NULL, (""), RET_ERROR_S());
    
    *outStatementsArena = CreateArenaAllocator(1024);   //TODO: Proper reserve count
    Allocator sharedArena = Allocator_Share(outStatementsArena);
    
    //TODO: Proper reserve count
    StatementList statementList = StatementList_Create(*outStatementsArena, 16);
    ResultStatementPtr statementPtrResult = Statement_CreateCompound(   sharedArena, 
                                                                        &statementList, 
                                                                        0,
                                                                        true,
                                                                        128);
    
    #define END_CURRENT_STATEMENT(countCurrentToken) \
        do \
        { \
            Result_Uint32 uint32Result = EndCurrentStatement(   countCurrentToken, \
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
                String visualizeStr = Token_VisualizeLocation(  &tokens->Data[i], \
                                                                CreateHeapAllocator(), \
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
    BoolList blockStartComplex = BoolList_Create(scratchAllocator, 16);
    for(uint32_t i = 0; i < tokens->Length; ++i)
    {
        static_assert(TokenType_Count == 19, "");
        switch((CharTokenType)tokens->Data[i].TokenType)
        {
            case CharTokenType_Identifier:
            {
                /*
                Special case for "else". Since it can be like this
                ```
                else
                    ...
                ```
                where it can be mixed in with normal statement
                */
                ConstStringView tokenView = Token_TokenTextView(&tokens->Data[i]);
                if(!ConstStringView_IsEqualLiteral(&tokenView, "else"))
                    break;
                
                if(startTokenIndex != i)
                    END_CURRENT_STATEMENT(false);
                
                END_CURRENT_STATEMENT(true);
                break;
            }
            case CharTokenType_Operator:
            {
                //`case xxx:` count as a statement
                ConstStringView tokenView = Token_TokenTextView(&tokens->Data[i]);
                if(!ConstStringView_IsEqualLiteral(&tokenView, ":"))
                    break;
                
                END_CURRENT_STATEMENT(true);
                break;
            }
            case CharTokenType_BlockStart:
            {
                //Find the first previous token that we care
                uint32_t lastTokenIndex = i;
                for(int32_t j = i - 1; j >= startTokenIndex; --j)
                {
                    if( tokens->Data[j].TokenType == TokenType_Space ||
                        tokens->Data[j].TokenType == TokenType_Newline ||
                        tokens->Data[j].TokenType == TokenType_Comment)
                    {
                        continue;
                    }
                    lastTokenIndex = j;
                    break;
                }

                if(lastTokenIndex != i)
                {
                    //Not complex statement
                    if( tokens->Data[lastTokenIndex].TokenType != TokenType_Identifier &&
                        tokens->Data[lastTokenIndex].TokenType != TokenType_InvokeEnd)
                    {
                        BoolList_AddValue(&blockStartComplex, false);
                        break;
                    }
                    
                    END_CURRENT_STATEMENT(false);
                }
                
                BoolList_AddValue(&blockStartComplex, true);
                
                //Create compound as parent
                statementPtrResult = Statement_CreateCompound(  sharedArena, 
                                                                &statementList, 
                                                                currentParentIndex,
                                                                false,
                                                                32);
                Statement* newStatement = *RESULT_TRY(statementPtrResult, RET_ERROR_S());
                CHECK_AND_VISUALIZE_ERROR(  newStatement->Tokens.Type == 
                                            TU_TYPE_S(CompoundStatement),
                                            "Unexpected type");
                Result_Void voidResult = AddStatementToParent(  statementList.Length - 1,
                                                                currentParentIndex,
                                                                &statementList);
                (void)RESULT_TRY(voidResult, RET_ERROR_S());
                CompoundStatement* compoundData = &newStatement->Tokens.TU_DATA_S(CompoundStatement);
                compoundData->StartTokenIndex = i;
                startTokenIndex = i + 1;
                currentParentIndex = statementList.Length - 1;
                break;
            }
            case CharTokenType_BlockEnd:
            {
                if(blockStartComplex.Length == 0)   //Mismatching number of block start and ends
                    break;
                
                if(!blockStartComplex.Data[blockStartComplex.Length - 1])
                {
                    BoolList_Resize(&blockStartComplex, blockStartComplex.Length - 1);
                    break;
                }
                BoolList_Resize(&blockStartComplex, blockStartComplex.Length - 1);
                
                //Finish compound parent
                END_CURRENT_STATEMENT(false);
                Statement* parentStatement = &statementList.Data[currentParentIndex];
                CHECK_AND_VISUALIZE_ERROR(  parentStatement->Tokens.Type == 
                                            TU_TYPE_S(CompoundStatement), 
                                            "Unexpected type");
                
                CompoundStatement* parentCompound = &parentStatement->Tokens
                                                                    .TU_DATA_S(CompoundStatement);
                CHECK_AND_VISUALIZE_ERROR(  !parentCompound->Implicit,
                                            "Expected non implicit for parent when block end");
                parentCompound->EndTokenIndex = i;
                startTokenIndex = i + 1;
                currentParentIndex = parentStatement->ParentIndex;
                break;
            }
            case CharTokenType_InvokeStart:
                break;
            case CharTokenType_InvokeEnd:
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
                        if( tokens->Data[j].TokenType == TokenType_Space ||
                            tokens->Data[j].TokenType == TokenType_Newline ||
                            tokens->Data[j].TokenType == TokenType_Comment)
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
                        if(tokens->Data[j].TokenType == TokenType_InvokeEnd)
                            ++invokeCounter;
                        else if(tokens->Data[j].TokenType == TokenType_InvokeStart)
                            --invokeCounter;
                    }
                }
                
                //Didn't find the invoke start token
                if(invokeStartIndex == startTokenIndex)
                    break;
                
                //Keyword must be an identifier
                if(tokens->Data[invokeStartIndex].TokenType != TokenType_Identifier)
                    break;
                
                //If the token before invoke start is a keyword, end the current statement
                if(IsInvokableKeyword(Token_TokenTextView(&tokens->Data[invokeStartIndex])))
                    END_CURRENT_STATEMENT(true);
                break;
            }
            case CharTokenType_Semicolon:
            {
                END_CURRENT_STATEMENT(true);
                break;
            }
            case CharTokenType_StringLiteral:
                break;
            case CharTokenType_CharLiteral:
                break;
            case CharTokenType_IntLiteral:
                break;
            case CharTokenType_Space:
                break;
            case CharTokenType_Newline:
            {
                if(i == startTokenIndex)
                    break;
                
                //Find the beginning of the line
                uint32_t lineFirstToken = startTokenIndex;
                for(int64_t j = i - 1; j >= 0; --j)
                {
                    if(tokens->Data[j].TokenType == TokenType_Newline)
                    {
                        lineFirstToken = j + 1;
                        break;
                    }
                }
                
                //Then find the first non skippable token
                //uint32_t firstParsableToken = startTokenIndex;
                for(uint32_t j = lineFirstToken; j < i; ++j)
                {
                    if(!Token_IsSkippable(&tokens->Data[j]))
                    {
                        lineFirstToken = j;
                        break;
                    }
                }
                
                //Check compiler directives (#)
                if( tokens->Data[lineFirstToken].TokenType == TokenType_Operator &&
                    Token_TokenTextView(&tokens->Data[lineFirstToken]).Length == 1 &&
                    Token_TokenTextView(&tokens->Data[lineFirstToken]).Data[0] == '#')
                {
                    END_CURRENT_STATEMENT(true);
                    //NOTE: We know it is a compiler directive statement, but we will classify it later.
                }
                
                break;
            }
            case CharTokenType_Undef:
                break;
        } //switch((CharTokenType)tokens->Data[i].TokenType)
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
