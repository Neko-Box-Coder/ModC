#ifndef MODC_CLASSIFICATION_H
#define MODC_CLASSIFICATION_H

#ifndef DEFAULT_ALLOC
    #define DEFAULT_ALLOC() ModC_CreateHeapAllocator()
#endif

#include "ModC/Statement.h"
#include "ModC/Tokenization.h"
#include "ModC/GenericContainers.h"
#include "ModC/Result.h"

#define NO_DECLTYPE
#include "uthash.h"
typedef struct ModC_TypeEntry
{
    ModC_String Type;
    UT_hash_handle hh;
} ModC_TypeEntry;

#define RETURN_VISUALIZED_ERROR(tokenPtr, source, spanLine, fmtMsg, ...) \
        do \
        { \
            ModC_String visualizeStr = ModC_Token_VisualizeLocation(tokenPtr, \
                                                                    ModC_CreateHeapAllocator(), \
                                                                    spanLine, \
                                                                    source); \
            \
            return ERROR_STR_FMT_S((fmtMsg "\n%.*s", \
                                    __VA_ARGS__, \
                                    visualizeStr.Length, \
                                    visualizeStr.Data)); \
        } \
        while(0)

static inline ModC_Result_Void ModC_TryClassifyAsTypeDeclaration(   ModC_Statement* statement,
                                                                    const ModC_TokenList* tokens,
                                                                    const ModC_ConstStringView source,
                                                                    ModC_Allocator statementsArena,
                                                                    ModC_Allocator scratchAllocator,
                                                                    bool inTypeDecl,
                                                                    bool inFuncImpl,
                                                                    ModC_TypeEntry** rootTypeHashSet,
                                                                    ModC_TypeEntry** funcTypeHashSet)
{
    #undef ResultNameState
    #define ResultNameState ModC_Result_Void
    #undef TaggedUnionNameState
    #define TaggedUnionNameState ModC_StatementInfoUnion
    #undef uthash_malloc
    #define uthash_malloc(sz) ModC_Allocator_Malloc(&scratchAllocator, sz)
    #undef uthash_free
    #define uthash_free(ptr, sz) ModC_Allocator_Free(&scratchAllocator, ptr)
    
    if(statement->StatementType == ModC_StatementType_Compound || inTypeDecl)
        return RESULT_VALUE_S(0);
    
    uint32_t tokenCount = ModC_StatementTokensUnion_GetTokenCount(&statement->Tokens);
    CHECK(tokenCount > 0, (""), RET_ERROR_S());
    
    ModC_Result_ConstStringView constStringViewResult =
        ModC_StatementTokensUnion_GetTokenTextViewAt(&statement->Tokens, tokens, 0);
    
    ModC_ConstStringView firstTokenTextView = *RESULT_TRY(constStringViewResult, RET_ERROR_S());
    if(ModC_ConstStringView_IsEqualLiteral(&firstTokenTextView, "struct"))
    {
        statement->StatementType = ModC_StatementType_TypeDeclaration;
        statement->Info = TU_INIT_S(ModC_TypeDeclarationInfo, { .Type = ModC_Type_Struct });
    }
    else if(ModC_ConstStringView_IsEqualLiteral(&firstTokenTextView, "enum"))
    {
        statement->StatementType = ModC_StatementType_TypeDeclaration;
        statement->Info = TU_INIT_S(ModC_TypeDeclarationInfo, { .Type = ModC_Type_Enum });
    }
    else
        return RESULT_VALUE_S(0);
    
    if(tokenCount == 1)
    {
        ModC_Result_TokenPtr tokenPtrResult = 
            ModC_StatementTokensUnion_GetTokenAt(&statement->Tokens, tokens, 0);
        ModC_Token* tokenPtr = *RESULT_TRY(tokenPtrResult, RET_ERROR_S());
        RETURN_VISUALIZED_ERROR(tokenPtr, 
                                source, 
                                false,
                                "%s", 
                                "Missing identifier when declaring struct or enum");
    }
    
    constStringViewResult = ModC_StatementTokensUnion_GetTokenTextViewAt(   &statement->Tokens, 
                                                                            tokens, 
                                                                            1);
    ModC_ConstStringView typeNameTextView = *RESULT_TRY(constStringViewResult, RET_ERROR_S());
    {
        ModC_TypeDeclarationInfo* typeDeclInfo = &statement->Info.TU_DATA_S(ModC_TypeDeclarationInfo);
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
        ModC_Result_TokenPtr tokenPtrResult = 
            ModC_StatementTokensUnion_GetTokenAt(&statement->Tokens, tokens, 1);
        ModC_Token* tokenPtr = *RESULT_TRY(tokenPtrResult, RET_ERROR_S());
        RETURN_VISUALIZED_ERROR(tokenPtr, 
                                source, 
                                false,
                                "Type %.*s already defined", 
                                typeNameTextView.Length,
                                typeNameTextView.Data);
    }
    
    ModC_TypeEntry* entry = ModC_Allocator_Malloc(&scratchAllocator, sizeof(ModC_TypeEntry));
    CHECK(entry, (""), RET_ERROR_S());
    entry->Type = ModC_String_FromData( scratchAllocator, 
                                        typeNameTextView.Data, 
                                        typeNameTextView.Length);
    if(inFuncImpl)
        HASH_ADD_KEYPTR(hh, *funcTypeHashSet, entry->Type.Data, entry->Type.Length, entry);
    else
        HASH_ADD_KEYPTR(hh, *rootTypeHashSet, entry->Type.Data, entry->Type.Length, entry);
    
    return RESULT_VALUE_S(0);
}

static inline ModC_Result_Void ModC_TryClassifyEnumValues(  ModC_Statement* statement,
                                                            const ModC_StatementList* statements, 
                                                            bool inTypeDecl)
{
    #undef ResultNameState
    #define ResultNameState ModC_Result_Void
    #undef TaggedUnionNameState
    #define TaggedUnionNameState ModC_StatementTokensUnion
    
    if(statement->StatementType == ModC_StatementType_Compound || !inTypeDecl)
        return RESULT_VALUE_S(0);
    
    ModC_Statement* parent = &statements->Data[statement->ParentIndex];
    ModC_Statement* grandparent = &statements->Data[parent->ParentIndex];
    
    CHECK(grandparent->StatementType == ModC_StatementType_Compound, (""), RET_ERROR_S());
    ModC_CompoundStatement* grandparentChildren = &grandparent  ->Tokens
                                                                .TU_DATA_S(ModC_CompoundStatement);
    uint64_t foundIndex = ModC_Uint32List_Find(&grandparentChildren->ChildStatements, &parent->Index);
    CHECK(foundIndex != grandparentChildren->ChildStatements.Length, (""), RET_ERROR_S());
    
    if(foundIndex == 0)
        return RESULT_VALUE_S(0);
    
    const ModC_Statement* typeDecl = 
        &statements->Data[grandparentChildren->ChildStatements.Data[foundIndex - 1]];
    
    if(typeDecl->StatementType != ModC_StatementType_TypeDeclaration)
        return RESULT_VALUE_S(0);
    
    if(typeDecl->Info.TU_DATA(  ModC_StatementInfoUnion, 
                                ModC_TypeDeclarationInfo).Type != ModC_Type_Enum)
    {
        return RESULT_VALUE_S(0);
    }
    
    statement->StatementType = ModC_StatementType_EnumValues;
    return RESULT_VALUE_S(0);
}



static inline ModC_Result_Void ModC_TryClassifyAsCompilerDirective( ModC_Statement* statement,
                                                                    const ModC_TokenList* tokens)
{
    #undef ResultNameState
    #define ResultNameState ModC_Result_Void
    
    if(statement->StatementType == ModC_StatementType_Compound)
        return RESULT_VALUE_S(0);
    
    uint32_t tokenCount = ModC_StatementTokensUnion_GetTokenCount(&statement->Tokens);
    CHECK(tokenCount > 0, (""), RET_ERROR_S());
    
    ModC_Result_TokenPtr tokenPtrResult =
        ModC_StatementTokensUnion_GetTokenAt(&statement->Tokens, tokens, 0);
    
    ModC_Token* token = *RESULT_TRY(tokenPtrResult, RET_ERROR_S());
    
    if(token->TokenType != ModC_TokenType_Operator)
        return RESULT_VALUE_S(0);
    
    ModC_Result_ConstStringView constStringViewResult =
        ModC_StatementTokensUnion_GetTokenTextViewAt(&statement->Tokens, tokens, 0);
    
    ModC_ConstStringView firstTokenTextView = *RESULT_TRY(constStringViewResult, RET_ERROR_S());
    if(ModC_ConstStringView_IsEqualLiteral(&firstTokenTextView, "#"))
        statement->StatementType = ModC_StatementType_CompilerDirective;
    
    return RESULT_VALUE_S(0);
}

static inline ModC_Result_Void 
ModC_TryClassifyAsVariableDeclareAssignment(ModC_Statement* statement,
                                            const ModC_TokenList* tokens,
                                            const ModC_ConstStringView source,
                                            bool inTypeDecl,
                                            bool inFuncImpl,
                                            ModC_TypeEntry** rootTypeHashSet,
                                            ModC_TypeEntry** funcTypeHashSet)
{
    #undef ResultNameState
    #define ResultNameState ModC_Result_Void
    #undef TaggedUnionNameState
    #define TaggedUnionNameState ModC_StatementInfoUnion
    
    if(statement->StatementType == ModC_StatementType_Compound)
        return RESULT_VALUE_S(0);
    
    uint32_t tokenCount = ModC_StatementTokensUnion_GetTokenCount(&statement->Tokens);
    CHECK(tokenCount > 0, (""), RET_ERROR_S());
    
    //Check if last token is semicolon
    {
        ModC_Result_TokenPtr tokenPtrResult =
            ModC_StatementTokensUnion_GetTokenAt(&statement->Tokens, tokens, tokenCount - 1);
        
        ModC_Token* token = *RESULT_TRY(tokenPtrResult, RET_ERROR_S());
        if(token->TokenType != ModC_TokenType_Semicolon)
            return RESULT_VALUE_S(0);
    }
    
    //At least <Type> <Identifier> <Semicolon>
    if(tokenCount < 3)
        return RESULT_VALUE_S(0);
    
    ModC_Token* typeToken;
    ModC_Token* identifierToken;
    
    //NOTE: Hardcode type to be index 0 and identifier to be index 1 for now
    for(int i = 0; i < 2; ++i)
    {
        ModC_Result_TokenPtr tokenPtrResult =
            ModC_StatementTokensUnion_GetTokenAt(&statement->Tokens, tokens, i);
        
        ModC_Token** outPtr = i == 0 ? &typeToken : &identifierToken;
        *outPtr = *RESULT_TRY(tokenPtrResult, RET_ERROR_S());
        if((*outPtr)->TokenType != ModC_TokenType_Identifier)
            return RESULT_VALUE_S(0);
    }
    
    if( typeToken->TokenType != ModC_TokenType_Identifier ||
        identifierToken->TokenType != ModC_TokenType_Identifier)
    {
        return RESULT_VALUE_S(0);
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
        RETURN_VISUALIZED_ERROR(typeToken, 
                                source, 
                                false,
                                "Failed to find type %.*s", 
                                typeTokenText.Length, 
                                typeTokenText.Data);
    }
    
    ModC_Result_Uint32 uint32Result = 
        ModC_StatementTokensUnion_ContainsTokenText(&statement->Tokens, 
                                                    tokens, 
                                                    ModC_ConstStringView_FromLiteral("="));
    uint32_t foundIndex = *RESULT_TRY(uint32Result, RET_ERROR_S());
    
    //Check if there's any equal sign, if there is, maybe it is 
    //ModC_StatementType_VariableDeclareAssignment
    if(foundIndex != tokenCount)
    {
        if(inTypeDecl)
        {
            ModC_Result_TokenPtr tokenPtrResult =
                ModC_StatementTokensUnion_GetTokenAt(&statement->Tokens, tokens, foundIndex);
            ModC_Token* tokenPtr = *RESULT_TRY(tokenPtrResult, RET_ERROR_S());
            RETURN_VISUALIZED_ERROR(tokenPtr, 
                                    source, 
                                    false,
                                    "%s", 
                                    "Assignment cannot happen in type declaration");
        }
        
        statement->StatementType = ModC_StatementType_VariableDeclareAssignment;
        statement->Info = TU_INIT_S(ModC_VariableDeclareAssignInfo, 
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
        statement->Info = TU_INIT_S(ModC_VariableDeclareAssignInfo, 
                                    {
                                        .TypeIndexInStatement = 0,
                                        .IdentifierIndexInStatement = 1,
                                        .HasAsignment = false,
                                        .AssignIndexInStatement = 0
                                    });
    }
    
    return RESULT_VALUE_S(0);
}

static inline ModC_Result_Void 
ModC_TryClassifyAsFunctionDeclaration(  ModC_Statement* statement,
                                        const ModC_TokenList* tokens,
                                        const ModC_ConstStringView source,
                                        bool inTypeDecl,
                                        bool inFuncImpl,
                                        ModC_TypeEntry** rootTypeHashSet)
{
    #undef ResultNameState
    #define ResultNameState ModC_Result_Void
    #undef TaggedUnionNameState
    #define TaggedUnionNameState ModC_StatementInfoUnion
    
    if(statement->StatementType == ModC_StatementType_Compound || inTypeDecl || inFuncImpl)
        return RESULT_VALUE_S(0);
    
    uint32_t tokenCount = ModC_StatementTokensUnion_GetTokenCount(&statement->Tokens);
    CHECK(tokenCount > 0, (""), RET_ERROR_S());
    
    //Check if last token is end paresthesia
    {
        ModC_Result_TokenPtr tokenPtrResult =
            ModC_StatementTokensUnion_GetTokenAt(&statement->Tokens, tokens, tokenCount - 1);
        
        ModC_Token* token = *RESULT_TRY(tokenPtrResult, RET_ERROR_S());
        if(token->TokenType != ModC_TokenType_InvokeEnd)
            return RESULT_VALUE_S(0);
    }
    
    //<Type> <Identifier> <Open paren> [<Arguments>...] <Close paren>
    if(tokenCount < 4)
        return RESULT_VALUE_S(0);
    
    ModC_Token* typeToken;
    ModC_Token* identifierToken;
    ModC_Token* argumentToken = NULL;
    
    //NOTE: Hardcode type to be index 0 and identifier to be index 1 for now
    for(int i = 0; i < 4; ++i)
    {
        ModC_Result_TokenPtr tokenPtrResult =
            ModC_StatementTokensUnion_GetTokenAt(&statement->Tokens, tokens, i);
        
        ModC_Token* curToken = *RESULT_TRY(tokenPtrResult, RET_ERROR_S());
        ModC_Token** outPtr = NULL;
        
        if(i == 0)
        {
            if(curToken->TokenType != ModC_TokenType_Identifier)
                return RESULT_VALUE_S(0);
            outPtr = &typeToken;
        }
        else if(i == 1)
        {
            if(curToken->TokenType != ModC_TokenType_Identifier)
                return RESULT_VALUE_S(0);
            outPtr = &identifierToken;
        }
        else if(i == 2 && curToken->TokenType != ModC_TokenType_InvokeStart)
            return RESULT_VALUE_S(0);
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
            RETURN_VISUALIZED_ERROR(typeToken, 
                                    source, 
                                    false,
                                    "Failed to find type %.*s", 
                                    typeTokenText.Length,
                                    typeTokenText.Data);
        }
    }
    
    statement->StatementType = ModC_StatementType_FunctionDeclaration;
    statement->Info = TU_INIT_S(ModC_FunctionDeclarationInfo, 
                                {
                                    .TypeIndexInStatement = 0,
                                    .IdentifierIndexInStatement = 1,
                                    .HaveArguments = argumentToken != NULL,
                                    .ArgumentIndexInStatement = argumentToken != NULL ? 3 : 0
                                });
    
    return RESULT_VALUE_S(0);
}


static inline ModC_Result_Void ModC_TryClassifyAsReturn(ModC_Statement* statement,
                                                        const ModC_TokenList* tokens,
                                                        bool inTypeDecl,
                                                        bool inFuncImpl)
{
    #undef ResultNameState
    #define ResultNameState ModC_Result_Void
    
    if(statement->StatementType == ModC_StatementType_Compound || inTypeDecl || !inFuncImpl)
        return RESULT_VALUE_S(0);
    
    uint32_t tokenCount = ModC_StatementTokensUnion_GetTokenCount(&statement->Tokens);
    CHECK(tokenCount > 0, (""), RET_ERROR_S());
    
    //Check if last token is semicolon
    {
        ModC_Result_TokenPtr tokenPtrResult =
            ModC_StatementTokensUnion_GetTokenAt(&statement->Tokens, tokens, tokenCount - 1);
        
        ModC_Token* token = *RESULT_TRY(tokenPtrResult, RET_ERROR_S());
        if(token->TokenType != ModC_TokenType_Semicolon)
            return RESULT_VALUE_S(0);
    }
    
    //return <Identifier> <Semicolon>
    if(tokenCount < 3)
        return RESULT_VALUE_S(0);
    
    ModC_Result_ConstStringView viewResult =
        ModC_StatementTokensUnion_GetTokenTextViewAt(&statement->Tokens, tokens, 0);
    
    ModC_ConstStringView firstTokenView = *RESULT_TRY(viewResult, RET_ERROR_S());
    
    if(!ModC_ConstStringView_IsEqualLiteral(&firstTokenView, "return"))
        return RESULT_VALUE_S(0);
    
    statement->StatementType = ModC_StatementType_ReturnStatement;
    return RESULT_VALUE_S(0);
}

static inline ModC_Result_Void ModC_TryClassifyKeywordInvokable(ModC_Statement* statement,
                                                                const ModC_TokenList* tokens,
                                                                bool inTypeDecl,
                                                                bool inFuncImpl)
{
    #undef ResultNameState
    #define ResultNameState ModC_Result_Void
    
    if(statement->StatementType == ModC_StatementType_Compound || inTypeDecl || !inFuncImpl)
        return RESULT_VALUE_S(0);
    
    uint32_t tokenCount = ModC_StatementTokensUnion_GetTokenCount(&statement->Tokens);
    CHECK(tokenCount > 0, (""), RET_ERROR_S());
    
    //Check if last token is end parenthesis
    {
        ModC_Result_TokenPtr tokenPtrResult =
            ModC_StatementTokensUnion_GetTokenAt(&statement->Tokens, tokens, tokenCount - 1);
        
        ModC_Token* token = *RESULT_TRY(tokenPtrResult, RET_ERROR_S());
        if(token->TokenType != ModC_TokenType_InvokeEnd)
            return RESULT_VALUE_S(0);
    }
    
    //<keyword> <open paren> ... <end paren>
    if(tokenCount < 3)
        return RESULT_VALUE_S(0);
    
    ModC_Result_ConstStringView viewResult =
        ModC_StatementTokensUnion_GetTokenTextViewAt(&statement->Tokens, tokens, 0);
    
    ModC_ConstStringView firstTokenView = *RESULT_TRY(viewResult, RET_ERROR_S());
    if(ModC_ConstStringView_IsEqualLiteral(&firstTokenView, "if"))
        statement->StatementType = ModC_StatementType_IfStatement;
    else if(ModC_ConstStringView_IsEqualLiteral(&firstTokenView, "for"))
        statement->StatementType = ModC_StatementType_ForStatement;
    else if(ModC_ConstStringView_IsEqualLiteral(&firstTokenView, "while"))
        statement->StatementType = ModC_StatementType_WhileStatement;
    else if(ModC_ConstStringView_IsEqualLiteral(&firstTokenView, "switch"))
        statement->StatementType = ModC_StatementType_SwitchStatement;
    
    return RESULT_VALUE_S(0);
}

static inline ModC_Result_Void ModC_TryClassifyAsElse(  ModC_Statement* statement,
                                                        const ModC_TokenList* tokens,
                                                        bool inTypeDecl,
                                                        bool inFuncImpl)
{
    #undef ResultNameState
    #define ResultNameState ModC_Result_Void
    
    if(statement->StatementType == ModC_StatementType_Compound || inTypeDecl || !inFuncImpl)
        return RESULT_VALUE_S(0);
    
    uint32_t tokenCount = ModC_StatementTokensUnion_GetTokenCount(&statement->Tokens);
    CHECK(tokenCount > 0, (""), RET_ERROR_S());
    
    if(tokenCount != 1)
        return RESULT_VALUE_S(0);
    
    ModC_Result_ConstStringView viewResult = 
        ModC_StatementTokensUnion_GetTokenTextViewAt(&statement->Tokens, tokens, 0);
    
    ModC_ConstStringView tokenView = *RESULT_TRY(viewResult, RET_ERROR_S());
    if(ModC_ConstStringView_IsEqualLiteral(&tokenView, "else"))
        statement->StatementType = ModC_StatementType_ElseStatement;
    
    return RESULT_VALUE_S(0);
}

//NOTE: ModC_TryClassifyAsVariableDeclareAssignment should be called before this
static inline ModC_Result_Void ModC_TryClassifyAssignment(  ModC_Statement* statement,
                                                            const ModC_TokenList* tokens,
                                                            bool inTypeDecl,
                                                            bool inFuncImpl)
{
    #undef ResultNameState
    #define ResultNameState ModC_Result_Void
    #undef TaggedUnionNameState
    #define TaggedUnionNameState ModC_StatementInfoUnion
    
    if(statement->StatementType == ModC_StatementType_Compound || inTypeDecl || !inFuncImpl)
        return RESULT_VALUE_S(0);
    
    uint32_t tokenCount = ModC_StatementTokensUnion_GetTokenCount(&statement->Tokens);
    CHECK(tokenCount > 0, (""), RET_ERROR_S());
    
    //Check if last token is semicolon
    {
        ModC_Result_TokenPtr tokenPtrResult =
            ModC_StatementTokensUnion_GetTokenAt(&statement->Tokens, tokens, tokenCount - 1);
        
        ModC_Token* token = *RESULT_TRY(tokenPtrResult, RET_ERROR_S());
        if(token->TokenType != ModC_TokenType_Semicolon)
            return RESULT_VALUE_S(0);
    }
    
    //At least <Identifier> <Assignment> <Value> <Semicolon>
    if(tokenCount < 4)
        return RESULT_VALUE_S(0);
    
    ModC_Result_Uint32 uint32Result = 
        ModC_StatementTokensUnion_ContainsTokenText(&statement->Tokens, 
                                                    tokens, 
                                                    ModC_ConstStringView_FromLiteral("="));
    uint32_t foundIndex = *RESULT_TRY(uint32Result, RET_ERROR_S());
    if(foundIndex == tokenCount)
        return RESULT_VALUE_S(0);
    
    statement->StatementType = ModC_StatementType_Assignment;
    statement->Info = TU_INIT_S(ModC_AssignmentInfo, { .AssignIndexInStatement = foundIndex });
    return RESULT_VALUE_S(0);
}

static inline ModC_Result_Void ModC_TryClassifyCase(ModC_Statement* statement,
                                                    const ModC_TokenList* tokens,
                                                    bool inTypeDecl,
                                                    bool inFuncImpl)
{
    #undef ResultNameState
    #define ResultNameState ModC_Result_Void
    
    if(statement->StatementType == ModC_StatementType_Compound || inTypeDecl || !inFuncImpl)
        return RESULT_VALUE_S(0);
    
    uint32_t tokenCount = ModC_StatementTokensUnion_GetTokenCount(&statement->Tokens);
    CHECK(tokenCount > 0, (""), RET_ERROR_S());
    
    //Check if last token is colon
    {
        ModC_Result_TokenPtr tokenPtrResult =
            ModC_StatementTokensUnion_GetTokenAt(&statement->Tokens, tokens, tokenCount - 1);
        
        ModC_Token* token = *RESULT_TRY(tokenPtrResult, RET_ERROR_S());
        if(token->TokenType != ModC_TokenType_Operator)
            return RESULT_VALUE_S(0);
    
        ModC_ConstStringView tokenText = ModC_Token_TokenTextView(token);
        if(!ModC_ConstStringView_IsEqualLiteral(&tokenText, ":"))
            return RESULT_VALUE_S(0);
    }
    
    //At least <case> <identifier> <colon>
    if(tokenCount != 3)
        return RESULT_VALUE_S(0);
    
    //Check if first token is case
    {
        ModC_Result_TokenPtr tokenPtrResult =
            ModC_StatementTokensUnion_GetTokenAt(&statement->Tokens, tokens, 0);
        
        ModC_Token* token = *RESULT_TRY(tokenPtrResult, RET_ERROR_S());
        if(token->TokenType != ModC_TokenType_Identifier)
            return RESULT_VALUE_S(0);
    
        ModC_ConstStringView tokenText = ModC_Token_TokenTextView(token);
        if(!ModC_ConstStringView_IsEqualLiteral(&tokenText, "case"))
            return RESULT_VALUE_S(0);
    }
    
    statement->StatementType = ModC_StatementType_CaseStatement;
    return RESULT_VALUE_S(0);
}


static inline ModC_Result_Void ModC_CleanAndClassifyStatements( ModC_StatementList* statements, 
                                                                ModC_Allocator tokensAllcoator,
                                                                ModC_Allocator statementsArena,
                                                                ModC_TokenList* tokens,
                                                                const ModC_ConstStringView source,
                                                                ModC_Allocator scratchAllocator)
{
    #undef ResultNameState
    #define ResultNameState ModC_Result_Void
    #undef TaggedUnionNameState
    #define TaggedUnionNameState ModC_StatementTokensUnion
    
    CHECK(statements != NULL, (""), RET_ERROR_S());
    CHECK(tokens != NULL, (""), RET_ERROR_S());
    CHECK(  statementsArena.Type == ModC_AllocatorType_SharedArena ||
            statementsArena.Type == ModC_AllocatorType_OwnedArena, 
            (""),
            RET_ERROR_S());
    
    
    //ModC_StatementIndexList currentProcess
    if(statements->Length == 0)
        return RESULT_VALUE_S(0);
    
    uint32_t currentStatementIndex = 0;
    {
        ModC_Statement* rootStatement = &statements->Data[0];
        
        CHECK(  rootStatement->StatementType == ModC_StatementType_Compound, 
                ("Root node must be compound statement"),
                RET_ERROR_S());
        //Empty?
        if(rootStatement->Tokens.TU_DATA_S(ModC_CompoundStatement).ChildStatements.Length == 0)
            return RESULT_VALUE_S(0);
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
        ModC_TypeEntry* defaultTypeEntry = ModC_Allocator_Malloc(   &scratchAllocator, 
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
        ModC_Statement* prevStatement = &statements->Data[currentStatementIndex];
        do
        {
            bool isEnd = false;
            ModC_Statement* statement = &statements->Data[currentStatementIndex];
            ModC_Result_Uint32 uint32Result = ModC_Statement_Next(  statement, 
                                                                    prevStatement, 
                                                                    statements, 
                                                                    &isEnd);
            currentStatementIndex = *RESULT_TRY(uint32Result, RET_ERROR_S());
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
            
            CHECK(  statement->StatementType == ModC_StatementType_Unknown,
                    ("Unexpected statement type"),
                    RET_ERROR_S());
            
            ModC_Result_Void voidResult = ModC_Statement_Normalize( statement,
                                                                    tokensAllcoator,
                                                                    statementsArena,
                                                                    tokens,
                                                                    scratchAllocator);
            (void)RESULT_TRY(voidResult, DEFER_BREAK(0, RET_ERROR_S()));
            
            uint32_t tokenCount = ModC_StatementTokensUnion_GetTokenCount(&statement->Tokens);
            CHECK(tokenCount > 0, (""), DEFER_BREAK(0, RET_ERROR_S()));
            
            //Classify statements
            static_assert((int)ModC_StatementType_Count == 18, "");
            
            #undef TaggedUnionNameState
            #define TaggedUnionNameState ModC_StatementInfoUnion
            
            #define TRY_CLASSIFY_TYPE_DECLARATION() \
                if(statement->StatementType == ModC_StatementType_Unknown) \
                { \
                    voidResult = ModC_TryClassifyAsTypeDeclaration( statement, \
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
                if(statement->StatementType == ModC_StatementType_Unknown) \
                { \
                    voidResult = ModC_TryClassifyEnumValues(statement, \
                                                            statements, \
                                                            typeScope != -1); \
                }
            
            #define TRY_CLASSIFY_COMPILER_DIRECTIVE() \
                if(statement->StatementType == ModC_StatementType_Unknown) \
                { \
                    voidResult = ModC_TryClassifyAsCompilerDirective(statement, tokens); \
                    (void)RESULT_TRY(voidResult, DEFER_BREAK(0, RET_ERROR_S())); \
                }
            
            #define TRY_CLASSIFY_VAR_DECLARE_ASSIGN() \
                if(statement->StatementType == ModC_StatementType_Unknown) \
                { \
                    voidResult = ModC_TryClassifyAsVariableDeclareAssignment(   statement, \
                                                                                tokens, \
                                                                                source, \
                                                                                typeScope != -1, \
                                                                                funcScope != -1, \
                                                                                &rootTypeHashSet, \
                                                                                &funcTypeHashSet); \
                    (void)RESULT_TRY(voidResult, DEFER_BREAK(0, RET_ERROR_S())); \
                }
            
            #define TRY_CLASSIFY_FUNC_DECLARE() \
                if(statement->StatementType == ModC_StatementType_Unknown) \
                { \
                    voidResult = ModC_TryClassifyAsFunctionDeclaration( statement, \
                                                                        tokens, \
                                                                        source, \
                                                                        typeScope != -1, \
                                                                        funcScope != -1, \
                                                                        &rootTypeHashSet); \
                    (void)RESULT_TRY(voidResult, DEFER_BREAK(0, RET_ERROR_S())); \
                }
            
            #define TRY_CLASSIFY_RETURN() \
                if(statement->StatementType == ModC_StatementType_Unknown) \
                { \
                    voidResult = ModC_TryClassifyAsReturn(  statement, \
                                                            tokens, \
                                                            typeScope != -1, \
                                                            funcScope != -1); \
                    (void)RESULT_TRY(voidResult, DEFER_BREAK(0, RET_ERROR_S())); \
                }
            
            #define TRY_CLASSIFY_INVOKABLE() \
                if(statement->StatementType == ModC_StatementType_Unknown) \
                { \
                    voidResult = ModC_TryClassifyKeywordInvokable(  statement, \
                                                                    tokens, \
                                                                    typeScope != -1, \
                                                                    funcScope != -1); \
                    (void)RESULT_TRY(voidResult, DEFER_BREAK(0, RET_ERROR_S())); \
                }
            
            #define TRY_CLASSIFY_ELSE() \
                if(statement->StatementType == ModC_StatementType_Unknown) \
                { \
                    voidResult = ModC_TryClassifyAsElse(statement, \
                                                        tokens, \
                                                        typeScope != -1, \
                                                        funcScope != -1); \
                    (void)RESULT_TRY(voidResult, DEFER_BREAK(0, RET_ERROR_S())); \
                }
            
            #define TRY_CLASSIFY_ASSIGNMENT() \
                if(statement->StatementType == ModC_StatementType_Unknown) \
                { \
                    voidResult = ModC_TryClassifyAssignment(statement, \
                                                            tokens, \
                                                            typeScope != -1, \
                                                            funcScope != -1); \
                    (void)RESULT_TRY(voidResult, DEFER_BREAK(0, RET_ERROR_S())); \
                }
            
            #define TRY_CLASSIFY_CASE() \
                if(statement->StatementType == ModC_StatementType_Unknown) \
                { \
                    voidResult = ModC_TryClassifyCase(  statement, \
                                                        tokens, \
                                                        typeScope != -1, \
                                                        funcScope != -1); \
                    (void)RESULT_TRY(voidResult, DEFER_BREAK(0, RET_ERROR_S())); \
                }
            
            #define REPORT_FAILURE() \
                do \
                { \
                    ModC_Result_TokenPtr tokenPtrResult = \
                        ModC_StatementTokensUnion_GetTokenAt(&statement->Tokens, tokens, 0); \
                    ModC_Token* tokenPtr = *RESULT_TRY(tokenPtrResult, DEFER_BREAK(0, RET_ERROR_S())); \
                    RETURN_VISUALIZED_ERROR(tokenPtr, source, true, "%s", "Can't classify expression"); \
                } \
                while(0)
            
            //Root
            if(typeScope == -1 && funcScope == -1)
            {
                //In root, you can have:
                //ModC_StatementType_TypeDeclaration
                //ModC_StatementType_FunctionDeclaration
                //ModC_StatementType_VariableDeclaration
                //ModC_StatementType_Compound
                //ModC_StatementType_CompilerDirective
                //ModC_StatementType_VariableDeclareAssignment
                
                TRY_CLASSIFY_TYPE_DECLARATION();
                TRY_CLASSIFY_COMPILER_DIRECTIVE();
                TRY_CLASSIFY_VAR_DECLARE_ASSIGN();
                TRY_CLASSIFY_FUNC_DECLARE();
                if(statement->StatementType == ModC_StatementType_Unknown)
                    REPORT_FAILURE();
            }
            //Inside struct or enum (Which can be inside function impl as well)
            else if(typeScope != -1)
            {
                TRY_CLASSIFY_VAR_DECLARE_ASSIGN();
                TRY_CLASSIFY_ENUM_VALUES();
                if(statement->StatementType == ModC_StatementType_Unknown)
                    REPORT_FAILURE();
            }
            //Inside function impl
            else if(funcScope != -1)
            {
                //In function implementation, you can have:
                //ModC_StatementType_TypeDeclaration
                //ModC_StatementType_VariableDeclaration
                //ModC_StatementType_Compound
                //ModC_StatementType_CompilerDirective
                //ModC_StatementType_Assignment
                //ModC_StatementType_VariableDeclareAssignment
                //ModC_StatementType_PureExpression
                //ModC_StatementType_IfStatement
                //ModC_StatementType_ElseStatement
                //ModC_StatementType_ForStatement
                //ModC_StatementType_WhileStatement
                //ModC_StatementType_SwitchStatement
                
                TRY_CLASSIFY_INVOKABLE();
                TRY_CLASSIFY_ELSE();
                TRY_CLASSIFY_CASE();
                TRY_CLASSIFY_RETURN();
                TRY_CLASSIFY_TYPE_DECLARATION();
                TRY_CLASSIFY_COMPILER_DIRECTIVE();
                TRY_CLASSIFY_VAR_DECLARE_ASSIGN();
                TRY_CLASSIFY_ASSIGNMENT();
                if(statement->StatementType == ModC_StatementType_Unknown)
                    statement->StatementType = ModC_StatementType_PureExpression;
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
