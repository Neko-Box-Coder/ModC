#ifndef MODC_CLASSIFICATION_H
#define MODC_CLASSIFICATION_H

#ifndef DEFAULT_ALLOC
    #define DEFAULT_ALLOC() CreateHeapAllocator()
#endif

#include "ModC/Statement.h"
#include "ModC/Tokenization.h"
#include "ModC/GenericContainers.h"
#include "ModC/Result.h"

#define NO_DECLTYPE
#include "uthash.h"
typedef struct TypeEntry
{
    String Type;
    UT_hash_handle hh;
} TypeEntry;

#define RETURN_VISUALIZED_ERROR(tokenPtr, source, spanLine, fmtMsg, ...) \
        do \
        { \
            String visualizeStr = Token_VisualizeLocation(  tokenPtr, \
                                                            CreateHeapAllocator(), \
                                                            spanLine, \
                                                            source); \
            \
            return ERROR_STR_FMT_S((fmtMsg "\n%.*s", \
                                    __VA_ARGS__, \
                                    visualizeStr.Length, \
                                    visualizeStr.Data)); \
        } \
        while(0)

static inline Result_Void TryClassifyAsTypeDeclaration( Statement* statement,
                                                        const TokenList* tokens,
                                                        const ConstStringView source,
                                                        Allocator statementsArena,
                                                        Allocator scratchAllocator,
                                                        bool inTypeDecl,
                                                        bool inFuncImpl,
                                                        TypeEntry** rootTypeHashSet,
                                                        TypeEntry** funcTypeHashSet)
{
    #undef ResultNameState
    #define ResultNameState Result_Void
    #undef TaggedUnionNameState
    #define TaggedUnionNameState StatementInfoUnion
    #undef uthash_malloc
    #define uthash_malloc(sz) Allocator_Malloc(&scratchAllocator, sz)
    #undef uthash_free
    #define uthash_free(ptr, sz) Allocator_Free(&scratchAllocator, ptr)
    
    if(statement->StatementType == StatementType_Compound || inTypeDecl)
        return RESULT_VALUE_S(0);
    
    uint32_t tokenCount = Statement_GetTokenCount(statement);
    CHECK(tokenCount > 0, (""), RET_ERROR_S());
    
    Result_ConstStringView constStringViewResult = Statement_GetTokenTextViewAt(statement, tokens, 0);
    ConstStringView firstTokenTextView = *RESULT_TRY(constStringViewResult, RET_ERROR_S());
    if(ConstStringView_IsEqualLiteral(&firstTokenTextView, "struct"))
    {
        statement->StatementType = StatementType_TypeDeclaration;
        statement->Info = TU_INIT_S(TypeDeclarationInfo, { .Type = Type_Struct });
    }
    else if(ConstStringView_IsEqualLiteral(&firstTokenTextView, "enum"))
    {
        statement->StatementType = StatementType_TypeDeclaration;
        statement->Info = TU_INIT_S(TypeDeclarationInfo, { .Type = Type_Enum });
    }
    else
        return RESULT_VALUE_S(0);
    
    if(tokenCount == 1)
    {
        Result_TokenPtr tokenPtrResult = Statement_GetTokenAt(statement, tokens, 0);
        Token* tokenPtr = *RESULT_TRY(tokenPtrResult, RET_ERROR_S());
        RETURN_VISUALIZED_ERROR(tokenPtr, 
                                source, 
                                false,
                                "%s", 
                                "Missing identifier when declaring struct or enum");
    }
    
    constStringViewResult = Statement_GetTokenTextViewAt(statement, tokens, 1);
    ConstStringView typeNameTextView = *RESULT_TRY(constStringViewResult, RET_ERROR_S());
    {
        TypeDeclarationInfo* typeDeclInfo = &statement->Info.TU_DATA_S(TypeDeclarationInfo);
        typeDeclInfo->TypeName = String_FromData(   statementsArena, 
                                                    typeNameTextView.Data, 
                                                    typeNameTextView.Length);
        typeNameTextView = ConstStringView_Create(  typeDeclInfo->TypeName.Data, 
                                                    typeDeclInfo->TypeName.Length);
    }
    TypeEntry* foundEntry = NULL;
    HASH_FIND(hh, *rootTypeHashSet, typeNameTextView.Data, typeNameTextView.Length, foundEntry);
    
    if(!foundEntry && inFuncImpl)
        HASH_FIND(hh, *funcTypeHashSet, typeNameTextView.Data, typeNameTextView.Length, foundEntry);
    
    if(foundEntry)
    {
        Result_TokenPtr tokenPtrResult = Statement_GetTokenAt(statement, tokens, 1);
        Token* tokenPtr = *RESULT_TRY(tokenPtrResult, RET_ERROR_S());
        RETURN_VISUALIZED_ERROR(tokenPtr, 
                                source, 
                                false,
                                "Type %.*s already defined", 
                                typeNameTextView.Length,
                                typeNameTextView.Data);
    }
    
    TypeEntry* entry = Allocator_Malloc(&scratchAllocator, sizeof(TypeEntry));
    CHECK(entry, (""), RET_ERROR_S());
    entry->Type = String_FromData(scratchAllocator, typeNameTextView.Data, typeNameTextView.Length);
    if(inFuncImpl)
        HASH_ADD_KEYPTR(hh, *funcTypeHashSet, entry->Type.Data, entry->Type.Length, entry);
    else
        HASH_ADD_KEYPTR(hh, *rootTypeHashSet, entry->Type.Data, entry->Type.Length, entry);
    
    return RESULT_VALUE_S(0);
}

static inline Result_Void TryClassifyEnumValues(Statement* statement,
                                                const StatementList* statements, 
                                                bool inTypeDecl)
{
    #undef ResultNameState
    #define ResultNameState Result_Void
    #undef TaggedUnionNameState
    #define TaggedUnionNameState StatementTokensUnion
    
    if(statement->StatementType == StatementType_Compound || !inTypeDecl)
        return RESULT_VALUE_S(0);
    
    Statement* parent = &statements->Data[statement->ParentIndex];
    Statement* grandparent = &statements->Data[parent->ParentIndex];
    
    CHECK(grandparent->StatementType == StatementType_Compound, (""), RET_ERROR_S());
    CompoundStatement* grandparentChildren = &grandparent->Tokens.TU_DATA_S(CompoundStatement);
    uint64_t foundIndex = Uint32List_Find(&grandparentChildren->ChildStatements, &parent->Index);
    CHECK(foundIndex != grandparentChildren->ChildStatements.Length, (""), RET_ERROR_S());
    
    if(foundIndex == 0)
        return RESULT_VALUE_S(0);
    
    const Statement* typeDecl = 
        &statements->Data[grandparentChildren->ChildStatements.Data[foundIndex - 1]];
    
    if(typeDecl->StatementType != StatementType_TypeDeclaration)
        return RESULT_VALUE_S(0);
    
    if(typeDecl->Info.TU_DATA(  StatementInfoUnion, 
                                TypeDeclarationInfo).Type != Type_Enum)
    {
        return RESULT_VALUE_S(0);
    }
    
    statement->StatementType = StatementType_EnumValues;
    return RESULT_VALUE_S(0);
}



static inline Result_Void TryClassifyAsCompilerDirective(   Statement* statement, 
                                                            const TokenList* tokens)
{
    #undef ResultNameState
    #define ResultNameState Result_Void
    
    if(statement->StatementType == StatementType_Compound)
        return RESULT_VALUE_S(0);
    
    uint32_t tokenCount = Statement_GetTokenCount(statement);
    CHECK(tokenCount > 0, (""), RET_ERROR_S());
    
    Result_TokenPtr tokenPtrResult = Statement_GetTokenAt(statement, tokens, 0);
    Token* token = *RESULT_TRY(tokenPtrResult, RET_ERROR_S());
    if(token->TokenType != TokenType_Operator)
        return RESULT_VALUE_S(0);
    
    Result_ConstStringView constStringViewResult = Statement_GetTokenTextViewAt(statement, tokens, 0);
    ConstStringView firstTokenTextView = *RESULT_TRY(constStringViewResult, RET_ERROR_S());
    if(ConstStringView_IsEqualLiteral(&firstTokenTextView, "#"))
        statement->StatementType = StatementType_CompilerDirective;
    
    return RESULT_VALUE_S(0);
}

static inline Result_Void TryClassifyAsVariableDeclareAssignment(   Statement* statement,
                                                                    const TokenList* tokens,
                                                                    const ConstStringView source,
                                                                    bool inTypeDecl,
                                                                    bool inFuncImpl,
                                                                    TypeEntry** rootTypeHashSet,
                                                                    TypeEntry** funcTypeHashSet)
{
    #undef ResultNameState
    #define ResultNameState Result_Void
    #undef TaggedUnionNameState
    #define TaggedUnionNameState StatementInfoUnion
    
    if(statement->StatementType == StatementType_Compound)
        return RESULT_VALUE_S(0);
    
    uint32_t tokenCount = Statement_GetTokenCount(statement);
    CHECK(tokenCount > 0, (""), RET_ERROR_S());
    
    //Check if last token is semicolon
    {
        Result_TokenPtr tokenPtrResult = Statement_GetTokenAt(statement, tokens, tokenCount - 1);
        Token* token = *RESULT_TRY(tokenPtrResult, RET_ERROR_S());
        if(token->TokenType != TokenType_Semicolon)
            return RESULT_VALUE_S(0);
    }
    
    //At least <Type> <Identifier> <Semicolon>
    if(tokenCount < 3)
        return RESULT_VALUE_S(0);
    
    Token* typeToken;
    Token* identifierToken;
    
    //NOTE: Hardcode type to be index 0 and identifier to be index 1 for now
    for(int i = 0; i < 2; ++i)
    {
        Result_TokenPtr tokenPtrResult = Statement_GetTokenAt(statement, tokens, i);
        Token** outPtr = i == 0 ? &typeToken : &identifierToken;
        *outPtr = *RESULT_TRY(tokenPtrResult, RET_ERROR_S());
        if((*outPtr)->TokenType != TokenType_Identifier)
            return RESULT_VALUE_S(0);
    }
    
    if( typeToken->TokenType != TokenType_Identifier ||
        identifierToken->TokenType != TokenType_Identifier)
    {
        return RESULT_VALUE_S(0);
    }
    
    bool typeExist = false;
    ConstStringView typeTokenText = Token_TokenTextView(typeToken);
    if(inFuncImpl)
    {
        TypeEntry* foundEntry = NULL;
        HASH_FIND(hh, *funcTypeHashSet, typeTokenText.Data, typeTokenText.Length, foundEntry);
        typeExist = foundEntry != NULL;
    }
    
    if(!typeExist)
    {
        TypeEntry* foundEntry = NULL;
        HASH_FIND(hh, *rootTypeHashSet, typeTokenText.Data, typeTokenText.Length, foundEntry);
        typeExist = foundEntry != NULL;
    }
    
    if(!typeExist)
    {
        RETURN_VISUALIZED_ERROR(typeToken, 
                                source, 
                                false,
                                "Failed to find type %.*s", 
                                typeTokenText.Length, 
                                typeTokenText.Data);
    }
    
    Result_Uint32 uint32Result = Statement_ContainsTokenText(   statement, 
                                                                tokens, 
                                                                ConstStringView_FromLiteral("="));
    uint32_t foundIndex = *RESULT_TRY(uint32Result, RET_ERROR_S());
    
    //Check if there's any equal sign, if there is, maybe it is 
    //StatementType_VariableDeclareAssignment
    if(foundIndex != tokenCount)
    {
        if(inTypeDecl)
        {
            Result_TokenPtr tokenPtrResult = Statement_GetTokenAt(statement, tokens, foundIndex);
            Token* tokenPtr = *RESULT_TRY(tokenPtrResult, RET_ERROR_S());
            RETURN_VISUALIZED_ERROR(tokenPtr, 
                                    source, 
                                    false,
                                    "%s", 
                                    "Assignment cannot happen in type declaration");
        }
        
        statement->StatementType = StatementType_VariableDeclareAssignment;
        statement->Info = TU_INIT_S(VariableDeclareAssignInfo, 
                                    {
                                        .TypeIndexInStatement = 0,
                                        .IdentifierIndexInStatement = 1,
                                        .HasAsignment = true,
                                        .AssignIndexInStatement = foundIndex
                                    });
    }
    //Otherwise, maybe it is StatementType_VariableDeclaration
    else
    {
        statement->StatementType = StatementType_VariableDeclaration;
        statement->Info = TU_INIT_S(VariableDeclareAssignInfo, 
                                    {
                                        .TypeIndexInStatement = 0,
                                        .IdentifierIndexInStatement = 1,
                                        .HasAsignment = false,
                                        .AssignIndexInStatement = 0
                                    });
    }
    
    return RESULT_VALUE_S(0);
}

static inline Result_Void TryClassifyAsFunctionDeclaration( Statement* statement,
                                                            const TokenList* tokens,
                                                            const ConstStringView source,
                                                            bool inTypeDecl,
                                                            bool inFuncImpl,
                                                            TypeEntry** rootTypeHashSet)
{
    #undef ResultNameState
    #define ResultNameState Result_Void
    #undef TaggedUnionNameState
    #define TaggedUnionNameState StatementInfoUnion
    
    if(statement->StatementType == StatementType_Compound || inTypeDecl || inFuncImpl)
        return RESULT_VALUE_S(0);
    
    uint32_t tokenCount = Statement_GetTokenCount(statement);
    CHECK(tokenCount > 0, (""), RET_ERROR_S());
    
    //Check if last token is end paresthesia
    {
        Result_TokenPtr tokenPtrResult = Statement_GetTokenAt(statement, tokens, tokenCount - 1);
        Token* token = *RESULT_TRY(tokenPtrResult, RET_ERROR_S());
        if(token->TokenType != TokenType_InvokeEnd)
            return RESULT_VALUE_S(0);
    }
    
    //<Type> <Identifier> <Open paren> [<Arguments>...] <Close paren>
    if(tokenCount < 4)
        return RESULT_VALUE_S(0);
    
    Token* typeToken;
    Token* identifierToken;
    Token* argumentToken = NULL;
    
    //NOTE: Hardcode type to be index 0 and identifier to be index 1 for now
    for(int i = 0; i < 4; ++i)
    {
        Result_TokenPtr tokenPtrResult = Statement_GetTokenAt(statement, tokens, i);
        Token* curToken = *RESULT_TRY(tokenPtrResult, RET_ERROR_S());
        Token** outPtr = NULL;
        
        if(i == 0)
        {
            if(curToken->TokenType != TokenType_Identifier)
                return RESULT_VALUE_S(0);
            outPtr = &typeToken;
        }
        else if(i == 1)
        {
            if(curToken->TokenType != TokenType_Identifier)
                return RESULT_VALUE_S(0);
            outPtr = &identifierToken;
        }
        else if(i == 2 && curToken->TokenType != TokenType_InvokeStart)
            return RESULT_VALUE_S(0);
        else if(i == 3 && curToken->TokenType != TokenType_InvokeEnd)
            outPtr = &argumentToken;
        
        if(outPtr)
            *outPtr = curToken;
    }
    
    ConstStringView typeTokenText = Token_TokenTextView(typeToken);
    {
        TypeEntry* foundEntry = NULL;
        HASH_FIND(hh, *rootTypeHashSet, typeTokenText.Data, typeTokenText.Length, foundEntry);
        
        if(!foundEntry)
        {
            RETURN_VISUALIZED_ERROR(typeToken, 
                                    source, 
                                    false,
                                    "Failed to find type %.*s", 
                                    typeTokenText.Length,
                                    typeTokenText.Data);
        }
    }
    
    statement->StatementType = StatementType_FunctionDeclaration;
    statement->Info = TU_INIT_S(FunctionDeclarationInfo, 
                                {
                                    .TypeIndexInStatement = 0,
                                    .IdentifierIndexInStatement = 1,
                                    .HaveArguments = argumentToken != NULL,
                                    .ArgumentIndexInStatement = argumentToken != NULL ? 3 : 0
                                });
    
    return RESULT_VALUE_S(0);
}


static inline Result_Void TryClassifyAsReturn(  Statement* statement,
                                                const TokenList* tokens,
                                                bool inTypeDecl,
                                                bool inFuncImpl)
{
    #undef ResultNameState
    #define ResultNameState Result_Void
    
    if(statement->StatementType == StatementType_Compound || inTypeDecl || !inFuncImpl)
        return RESULT_VALUE_S(0);
    
    uint32_t tokenCount = Statement_GetTokenCount(statement);
    CHECK(tokenCount > 0, (""), RET_ERROR_S());
    
    //Check if last token is semicolon
    {
        Result_TokenPtr tokenPtrResult = Statement_GetTokenAt(statement, tokens, tokenCount - 1);
        Token* token = *RESULT_TRY(tokenPtrResult, RET_ERROR_S());
        if(token->TokenType != TokenType_Semicolon)
            return RESULT_VALUE_S(0);
    }
    
    //return <Identifier> <Semicolon>
    if(tokenCount < 3)
        return RESULT_VALUE_S(0);
    
    Result_ConstStringView viewResult = Statement_GetTokenTextViewAt(statement, tokens, 0);
    ConstStringView firstTokenView = *RESULT_TRY(viewResult, RET_ERROR_S());
    if(!ConstStringView_IsEqualLiteral(&firstTokenView, "return"))
        return RESULT_VALUE_S(0);
    
    statement->StatementType = StatementType_ReturnStatement;
    return RESULT_VALUE_S(0);
}

static inline Result_Void TryClassifyKeywordInvokable(  Statement* statement,
                                                        const TokenList* tokens,
                                                        bool inTypeDecl,
                                                        bool inFuncImpl)
{
    #undef ResultNameState
    #define ResultNameState Result_Void
    
    if(statement->StatementType == StatementType_Compound || inTypeDecl || !inFuncImpl)
        return RESULT_VALUE_S(0);
    
    uint32_t tokenCount = Statement_GetTokenCount(statement);
    CHECK(tokenCount > 0, (""), RET_ERROR_S());
    
    //Check if last token is end parenthesis
    {
        Result_TokenPtr tokenPtrResult = Statement_GetTokenAt(statement, tokens, tokenCount - 1);
        Token* token = *RESULT_TRY(tokenPtrResult, RET_ERROR_S());
        if(token->TokenType != TokenType_InvokeEnd)
            return RESULT_VALUE_S(0);
    }
    
    //<keyword> <open paren> ... <end paren>
    if(tokenCount < 3)
        return RESULT_VALUE_S(0);
    
    Result_ConstStringView viewResult = Statement_GetTokenTextViewAt(statement, tokens, 0);
    ConstStringView firstTokenView = *RESULT_TRY(viewResult, RET_ERROR_S());
    if(ConstStringView_IsEqualLiteral(&firstTokenView, "if"))
        statement->StatementType = StatementType_IfStatement;
    else if(ConstStringView_IsEqualLiteral(&firstTokenView, "for"))
        statement->StatementType = StatementType_ForStatement;
    else if(ConstStringView_IsEqualLiteral(&firstTokenView, "while"))
        statement->StatementType = StatementType_WhileStatement;
    else if(ConstStringView_IsEqualLiteral(&firstTokenView, "switch"))
        statement->StatementType = StatementType_SwitchStatement;
    
    return RESULT_VALUE_S(0);
}

static inline Result_Void TryClassifyAsElse(Statement* statement,
                                            const TokenList* tokens,
                                            bool inTypeDecl,
                                            bool inFuncImpl)
{
    #undef ResultNameState
    #define ResultNameState Result_Void
    
    if(statement->StatementType == StatementType_Compound || inTypeDecl || !inFuncImpl)
        return RESULT_VALUE_S(0);
    
    uint32_t tokenCount = Statement_GetTokenCount(statement);
    CHECK(tokenCount > 0, (""), RET_ERROR_S());
    
    if(tokenCount != 1)
        return RESULT_VALUE_S(0);
    
    Result_ConstStringView viewResult = Statement_GetTokenTextViewAt(statement, tokens, 0);
    ConstStringView tokenView = *RESULT_TRY(viewResult, RET_ERROR_S());
    if(ConstStringView_IsEqualLiteral(&tokenView, "else"))
        statement->StatementType = StatementType_ElseStatement;
    
    return RESULT_VALUE_S(0);
}

//NOTE: TryClassifyAsVariableDeclareAssignment should be called before this
static inline Result_Void TryClassifyAssignment(Statement* statement,
                                                const TokenList* tokens,
                                                bool inTypeDecl,
                                                bool inFuncImpl)
{
    #undef ResultNameState
    #define ResultNameState Result_Void
    #undef TaggedUnionNameState
    #define TaggedUnionNameState StatementInfoUnion
    
    if(statement->StatementType == StatementType_Compound || inTypeDecl || !inFuncImpl)
        return RESULT_VALUE_S(0);
    
    uint32_t tokenCount = Statement_GetTokenCount(statement);
    CHECK(tokenCount > 0, (""), RET_ERROR_S());
    
    //Check if last token is semicolon
    {
        Result_TokenPtr tokenPtrResult = Statement_GetTokenAt(statement, tokens, tokenCount - 1);
        Token* token = *RESULT_TRY(tokenPtrResult, RET_ERROR_S());
        if(token->TokenType != TokenType_Semicolon)
            return RESULT_VALUE_S(0);
    }
    
    //At least <Identifier> <Assignment> <Value> <Semicolon>
    if(tokenCount < 4)
        return RESULT_VALUE_S(0);
    
    Result_Uint32 uint32Result = Statement_ContainsTokenText(   statement, 
                                                                tokens, 
                                                                ConstStringView_FromLiteral("="));
    uint32_t foundIndex = *RESULT_TRY(uint32Result, RET_ERROR_S());
    if(foundIndex == tokenCount)
        return RESULT_VALUE_S(0);
    
    statement->StatementType = StatementType_Assignment;
    statement->Info = TU_INIT_S(AssignmentInfo, { .AssignIndexInStatement = foundIndex });
    return RESULT_VALUE_S(0);
}

static inline Result_Void TryClassifyCase(  Statement* statement,
                                            const TokenList* tokens,
                                            bool inTypeDecl,
                                            bool inFuncImpl)
{
    #undef ResultNameState
    #define ResultNameState Result_Void
    
    if(statement->StatementType == StatementType_Compound || inTypeDecl || !inFuncImpl)
        return RESULT_VALUE_S(0);
    
    uint32_t tokenCount = Statement_GetTokenCount(statement);
    CHECK(tokenCount > 0, (""), RET_ERROR_S());
    
    //Check if last token is colon
    {
        Result_TokenPtr tokenPtrResult = Statement_GetTokenAt(statement, tokens, tokenCount - 1);
        Token* token = *RESULT_TRY(tokenPtrResult, RET_ERROR_S());
        if(token->TokenType != TokenType_Operator)
            return RESULT_VALUE_S(0);
    
        ConstStringView tokenText = Token_TokenTextView(token);
        if(!ConstStringView_IsEqualLiteral(&tokenText, ":"))
            return RESULT_VALUE_S(0);
    }
    
    //At least <case> <identifier> <colon>
    if(tokenCount != 3)
        return RESULT_VALUE_S(0);
    
    //Check if first token is case
    {
        Result_TokenPtr tokenPtrResult = Statement_GetTokenAt(statement, tokens, 0);
        Token* token = *RESULT_TRY(tokenPtrResult, RET_ERROR_S());
        if(token->TokenType != TokenType_Identifier)
            return RESULT_VALUE_S(0);
    
        ConstStringView tokenText = Token_TokenTextView(token);
        if(!ConstStringView_IsEqualLiteral(&tokenText, "case"))
            return RESULT_VALUE_S(0);
    }
    
    statement->StatementType = StatementType_CaseStatement;
    return RESULT_VALUE_S(0);
}


static inline Result_Void CleanAndClassifyStatements(   StatementList* statements, 
                                                        Allocator tokensAllcoator,
                                                        Allocator statementsArena,
                                                        TokenList* tokens,
                                                        const ConstStringView source,
                                                        Allocator scratchAllocator)
{
    #undef ResultNameState
    #define ResultNameState Result_Void
    #undef TaggedUnionNameState
    #define TaggedUnionNameState StatementTokensUnion
    
    CHECK(statements != NULL, (""), RET_ERROR_S());
    CHECK(tokens != NULL, (""), RET_ERROR_S());
    CHECK(  statementsArena.Type == AllocatorType_SharedArena ||
            statementsArena.Type == AllocatorType_OwnedArena, 
            (""),
            RET_ERROR_S());
    
    
    //StatementIndexList currentProcess
    if(statements->Length == 0)
        return RESULT_VALUE_S(0);
    
    uint32_t currentStatementIndex = 0;
    {
        Statement* rootStatement = &statements->Data[0];
        
        CHECK(  rootStatement->StatementType == StatementType_Compound, 
                ("Root node must be compound statement"),
                RET_ERROR_S());
        //Empty?
        if(rootStatement->Tokens.TU_DATA_S(CompoundStatement).ChildStatements.Length == 0)
            return RESULT_VALUE_S(0);
    }
    
    
    TypeEntry* rootTypeHashSet = NULL;
    
    //TODO: This won't work when there are nested scopes in func
    TypeEntry* funcTypeHashSet = NULL;
    
    //TODO: Pull these from centralized place instead
    char* defaultTypes[] = 
    {
        "int", "int8", "int16", "int32", "uint", "uint8", "uint16", "uint32", 
        "char", "float", "double", "bool"
    };
    
    #undef uthash_malloc
    #define uthash_malloc(sz) Allocator_Malloc(&scratchAllocator, sz)
    #undef uthash_free
    #define uthash_free(ptr, sz) Allocator_Free(&scratchAllocator, ptr)
    
    for(int i = 0; i < sizeof(defaultTypes) / sizeof(defaultTypes[0]); ++i)
    {
        //printf("i: %d\n", i);
        TypeEntry* defaultTypeEntry = Allocator_Malloc(&scratchAllocator, sizeof(TypeEntry));
        defaultTypeEntry->Type = String_FromData(   scratchAllocator, 
                                                    defaultTypes[i], 
                                                    strlen(defaultTypes[i]));
        HASH_ADD_KEYPTR(hh, 
                        rootTypeHashSet, 
                        defaultTypeEntry->Type.Data, 
                        defaultTypeEntry->Type.Length,
                        defaultTypeEntry);
    }
    
    DEFER_SCOPE_START(0)
    {
        DEFER(0,    if(!rootTypeHashSet)
                        HASH_CLEAR(hh, rootTypeHashSet);
                    if(!funcTypeHashSet)
                        HASH_CLEAR(hh, funcTypeHashSet));
        
        int funcScope = -1;
        int typeScope = -1;
        int currentScope = 0;
        
        //Iterate all statements
        Statement* prevStatement = &statements->Data[currentStatementIndex];
        do
        {
            bool isEnd = false;
            Statement* statement = &statements->Data[currentStatementIndex];
            Result_Uint32 uint32Result = Statement_Next(statement, prevStatement, statements, &isEnd);
            currentStatementIndex = *RESULT_TRY(uint32Result, RET_ERROR_S());
            if(isEnd)
                break;
            
            prevStatement = statement;
            statement = &statements->Data[currentStatementIndex];
            
            bool onExitCompound = prevStatement->ParentIndex == statement->Index;
            if(statement->StatementType == StatementType_Compound)
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
                    if(prevStatement->StatementType == StatementType_FunctionDeclaration)
                        funcScope = currentScope;
                    else if(prevStatement->StatementType == StatementType_TypeDeclaration)
                        typeScope = currentScope;
                    ++currentScope;
                }
                continue;
            }
            
            CHECK(  statement->StatementType == StatementType_Unknown,
                    ("Unexpected statement type"),
                    RET_ERROR_S());
            
            Result_Void voidResult = Statement_Normalize(   statement,
                                                            tokensAllcoator,
                                                            statementsArena,
                                                            tokens,
                                                            scratchAllocator);
            (void)RESULT_TRY(voidResult, DEFER_BREAK(0, RET_ERROR_S()));
            
            uint32_t tokenCount = Statement_GetTokenCount(statement);
            CHECK(tokenCount > 0, (""), DEFER_BREAK(0, RET_ERROR_S()));
            
            //Classify statements
            static_assert((int)StatementType_Count == 18, "");
            
            #undef TaggedUnionNameState
            #define TaggedUnionNameState StatementInfoUnion
            
            #define TRY_CLASSIFY_TYPE_DECLARATION() \
                if(statement->StatementType == StatementType_Unknown) \
                { \
                    voidResult = TryClassifyAsTypeDeclaration(  statement, \
                                                                tokens, \
                                                                source, \
                                                                statementsArena, \
                                                                scratchAllocator, \
                                                                typeScope != -1, \
                                                                funcScope != -1, \
                                                                &rootTypeHashSet, \
                                                                &funcTypeHashSet); \
                    (void)RESULT_TRY(voidResult, DEFER_BREAK(0, RET_ERROR_S())); \
                }
            
            #define TRY_CLASSIFY_ENUM_VALUES() \
                if(statement->StatementType == StatementType_Unknown) \
                { \
                    voidResult = TryClassifyEnumValues( statement, \
                                                        statements, \
                                                        typeScope != -1); \
                }
            
            #define TRY_CLASSIFY_COMPILER_DIRECTIVE() \
                if(statement->StatementType == StatementType_Unknown) \
                { \
                    voidResult = TryClassifyAsCompilerDirective(statement, tokens); \
                    (void)RESULT_TRY(voidResult, DEFER_BREAK(0, RET_ERROR_S())); \
                }
            
            #define TRY_CLASSIFY_VAR_DECLARE_ASSIGN() \
                if(statement->StatementType == StatementType_Unknown) \
                { \
                    voidResult = TryClassifyAsVariableDeclareAssignment(statement, \
                                                                        tokens, \
                                                                        source, \
                                                                        typeScope != -1, \
                                                                        funcScope != -1, \
                                                                        &rootTypeHashSet, \
                                                                        &funcTypeHashSet); \
                    (void)RESULT_TRY(voidResult, DEFER_BREAK(0, RET_ERROR_S())); \
                }
            
            #define TRY_CLASSIFY_FUNC_DECLARE() \
                if(statement->StatementType == StatementType_Unknown) \
                { \
                    voidResult = TryClassifyAsFunctionDeclaration(  statement, \
                                                                    tokens, \
                                                                    source, \
                                                                    typeScope != -1, \
                                                                    funcScope != -1, \
                                                                    &rootTypeHashSet); \
                    (void)RESULT_TRY(voidResult, DEFER_BREAK(0, RET_ERROR_S())); \
                }
            
            #define TRY_CLASSIFY_RETURN() \
                if(statement->StatementType == StatementType_Unknown) \
                { \
                    voidResult = TryClassifyAsReturn(   statement, \
                                                        tokens, \
                                                        typeScope != -1, \
                                                        funcScope != -1); \
                    (void)RESULT_TRY(voidResult, DEFER_BREAK(0, RET_ERROR_S())); \
                }
            
            #define TRY_CLASSIFY_INVOKABLE() \
                if(statement->StatementType == StatementType_Unknown) \
                { \
                    voidResult = TryClassifyKeywordInvokable(   statement, \
                                                                tokens, \
                                                                typeScope != -1, \
                                                                funcScope != -1); \
                    (void)RESULT_TRY(voidResult, DEFER_BREAK(0, RET_ERROR_S())); \
                }
            
            #define TRY_CLASSIFY_ELSE() \
                if(statement->StatementType == StatementType_Unknown) \
                { \
                    voidResult = TryClassifyAsElse( statement, \
                                                    tokens, \
                                                    typeScope != -1, \
                                                    funcScope != -1); \
                    (void)RESULT_TRY(voidResult, DEFER_BREAK(0, RET_ERROR_S())); \
                }
            
            #define TRY_CLASSIFY_ASSIGNMENT() \
                if(statement->StatementType == StatementType_Unknown) \
                { \
                    voidResult = TryClassifyAssignment( statement, \
                                                        tokens, \
                                                        typeScope != -1, \
                                                        funcScope != -1); \
                    (void)RESULT_TRY(voidResult, DEFER_BREAK(0, RET_ERROR_S())); \
                }
            
            #define TRY_CLASSIFY_CASE() \
                if(statement->StatementType == StatementType_Unknown) \
                { \
                    voidResult = TryClassifyCase(   statement, \
                                                    tokens, \
                                                    typeScope != -1, \
                                                    funcScope != -1); \
                    (void)RESULT_TRY(voidResult, DEFER_BREAK(0, RET_ERROR_S())); \
                }
            
            #define REPORT_FAILURE() \
                do \
                { \
                    Result_TokenPtr tokenPtrResult = Statement_GetTokenAt(statement, tokens, 0); \
                    Token* tokenPtr = *RESULT_TRY(tokenPtrResult, DEFER_BREAK(0, RET_ERROR_S())); \
                    RETURN_VISUALIZED_ERROR(tokenPtr, source, true, "%s", "Can't classify expression"); \
                } \
                while(0)
            
            //Root
            if(typeScope == -1 && funcScope == -1)
            {
                //In root, you can have:
                //StatementType_TypeDeclaration
                //StatementType_FunctionDeclaration
                //StatementType_VariableDeclaration
                //StatementType_Compound
                //StatementType_CompilerDirective
                //StatementType_VariableDeclareAssignment
                
                TRY_CLASSIFY_TYPE_DECLARATION();
                TRY_CLASSIFY_COMPILER_DIRECTIVE();
                TRY_CLASSIFY_VAR_DECLARE_ASSIGN();
                TRY_CLASSIFY_FUNC_DECLARE();
                if(statement->StatementType == StatementType_Unknown)
                    REPORT_FAILURE();
            }
            //Inside struct or enum (Which can be inside function impl as well)
            else if(typeScope != -1)
            {
                TRY_CLASSIFY_VAR_DECLARE_ASSIGN();
                TRY_CLASSIFY_ENUM_VALUES();
                if(statement->StatementType == StatementType_Unknown)
                    REPORT_FAILURE();
            }
            //Inside function impl
            else if(funcScope != -1)
            {
                //In function implementation, you can have:
                //StatementType_TypeDeclaration
                //StatementType_VariableDeclaration
                //StatementType_Compound
                //StatementType_CompilerDirective
                //StatementType_Assignment
                //StatementType_VariableDeclareAssignment
                //StatementType_PureExpression
                //StatementType_IfStatement
                //StatementType_ElseStatement
                //StatementType_ForStatement
                //StatementType_WhileStatement
                //StatementType_SwitchStatement
                
                TRY_CLASSIFY_INVOKABLE();
                TRY_CLASSIFY_ELSE();
                TRY_CLASSIFY_CASE();
                TRY_CLASSIFY_RETURN();
                TRY_CLASSIFY_TYPE_DECLARATION();
                TRY_CLASSIFY_COMPILER_DIRECTIVE();
                TRY_CLASSIFY_VAR_DECLARE_ASSIGN();
                TRY_CLASSIFY_ASSIGNMENT();
                if(statement->StatementType == StatementType_Unknown)
                    statement->StatementType = StatementType_PureExpression;
            }
        } //do
        while(true);
    }
    DEFER_SCOPE_END(0)
    
    return RESULT_VALUE_S(0);

    #undef TRY_CLASSIFY_TYPE_DECLARATION
    #undef TRY_CLASSIFY_ENUM_VALUES
    #undef TRY_CLASSIFY_COMPILER_DIRECTIVE
    #undef TRY_CLASSIFY_VAR_DECLARE_ASSIGN
    #undef TRY_CLASSIFY_FUNC_DECLARE
    #undef TRY_CLASSIFY_RETURN
    #undef TRY_CLASSIFY_INVOKABLE
    #undef TRY_CLASSIFY_ELSE
    #undef TRY_CLASSIFY_ASSIGNMENT
    #undef TRY_CLASSIFY_CASE
    #undef REPORT_FAILURE
}

#undef RETURN_VISUALIZED_ERROR

#endif
