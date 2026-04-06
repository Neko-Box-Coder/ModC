#ifndef MODC_TOKENIZATION_H
#define MODC_TOKENIZATION_H

#ifndef MODC_DEFAULT_ALLOC
    #define MODC_DEFAULT_ALLOC() ModC_CreateHeapAllocator()
#endif

#include "ModC/Strings/Strings.h"
#include "ModC/Result.h"
#include "ModC/GenericContainers.h"
#include "ModC/Move.h"

#include "static_assert.h/assert.h"

#include <ctype.h>
#include <stdbool.h>

typedef enum ModC_TokenType
{
    ModC_TokenType_Type,
    ModC_TokenType_Keyword,
    ModC_TokenType_Identifier,
    ModC_TokenType_Operator,
    ModC_TokenType_BlockStart,
    ModC_TokenType_BlockEnd,
    ModC_TokenType_InvokeStart,
    ModC_TokenType_InvokeEnd,
    ModC_TokenType_Semicolon,
    ModC_TokenType_StringLiteral,
    ModC_TokenType_CharLiteral,
    ModC_TokenType_IntLiteral,
    ModC_TokenType_FloatLiteral,
    ModC_TokenType_DoubleLiteral,
    ModC_TokenType_BoolLiteral,
    ModC_TokenType_Space,
    ModC_TokenType_Newline,
    ModC_TokenType_Comment,
    ModC_TokenType_Undef,
    ModC_TokenType_Count,   //19
} ModC_TokenType;

typedef enum ModC_CharTokenType
{
    ModC_CharTokenType_Identifier = ModC_TokenType_Identifier,
    ModC_CharTokenType_Operator = ModC_TokenType_Operator,
    ModC_CharTokenType_BlockStart = ModC_TokenType_BlockStart,
    ModC_CharTokenType_BlockEnd = ModC_TokenType_BlockEnd,
    ModC_CharTokenType_InvokeStart = ModC_TokenType_InvokeStart,
    ModC_CharTokenType_InvokeEnd = ModC_TokenType_InvokeEnd,
    ModC_CharTokenType_Semicolon = ModC_TokenType_Semicolon,
    ModC_CharTokenType_StringLiteral = ModC_TokenType_StringLiteral,
    ModC_CharTokenType_CharLiteral = ModC_TokenType_CharLiteral,
    ModC_CharTokenType_IntLiteral = ModC_TokenType_IntLiteral,
    ModC_CharTokenType_Space = ModC_TokenType_Space,
    ModC_CharTokenType_Newline = ModC_TokenType_Newline,
    ModC_CharTokenType_Undef = ModC_TokenType_Undef,
} ModC_CharTokenType;

typedef struct ModC_Token
{
    ModC_TokenType TokenType;
    ModC_StringUnion TokenText;
    int LineIndex;
    int ColumnIndex;
    int SourceIndex;
} ModC_Token;

static inline void ModC_Token_Free(ModC_Token* this);

#define MODC_LIST_NAME ModC_TokenList
#define MODC_VALUE_TYPE ModC_Token
#define MODC_VALUE_FREE(ptr) ModC_Token_Free(ptr)
#include "ModC/List.h"

MODC_DEFINE_RESULT_STRUCT(ModC_Result_TokenList, ModC_TokenList)
MODC_DEFINE_RESULT_STRUCT(ModC_Result_Token, ModC_Token)
MODC_DEFINE_RESULT_STRUCT(ModC_Result_TokenPtr, ModC_Token*)

static inline ModC_ConstStringView ModC_Token_TokenTextView(const ModC_Token* this)
{
    #undef ModC_TaggedUnionName_State
    #define ModC_TaggedUnionName_State ModC_StringUnion
    
    if(!this)
        return ModC_ConstStringView_Create(NULL, 0);
    
    return ModC_StringUnion_GetConstView(&this->TokenText);
}

static inline void ModC_Token_Free(ModC_Token* this)
{
    if(!this)
        return;
    if(this->TokenText.Type != MODC_TAG_TYPE_S(ModC_String))
    {
        *this = (ModC_Token){0};
        return;
    }
    
    ModC_String_Free(&this->TokenText.MODC_TAG_DATA_S(ModC_String));
}

static inline ModC_Token ModC_Token_FromString(ModC_TokenType type, ModC_String str)
{
    return (ModC_Token){ .TokenType = type, .TokenText = MODC_TAG_INIT_S(ModC_String, str) };
}

static inline ModC_Token ModC_Token_FromView(ModC_TokenType type, ModC_ConstStringView view)
{
    return  (ModC_Token)
            { 
                .TokenType = type, 
                .TokenText = MODC_TAG_INIT_S(ModC_ConstStringView, view)
            };
}

static inline bool ModC_Token_IsSkippable(const ModC_Token* this)
{
    return  !this || 
            this->TokenType == ModC_TokenType_Space || 
            this->TokenType == ModC_TokenType_Newline ||
            this->TokenType == ModC_TokenType_Comment;
}

static inline ModC_ConstStringView ModC_TokenType_ToCStr(ModC_TokenType type)
{
    static_assert((int)ModC_TokenType_Count == 19, "");
    switch(type)
    {
        case ModC_TokenType_Type:
            return ModC_ConstStringView_FromLiteral("ModC_TokenType_Type");
        case ModC_TokenType_Keyword:
            return ModC_ConstStringView_FromLiteral("ModC_TokenType_Keyword");
        case ModC_TokenType_Identifier:
            return ModC_ConstStringView_FromLiteral("ModC_TokenType_Identifier");
        case ModC_TokenType_Operator:
            return ModC_ConstStringView_FromLiteral("ModC_TokenType_Operator");
        case ModC_TokenType_BlockStart:
            return ModC_ConstStringView_FromLiteral("ModC_TokenType_BlockStart");
        case ModC_TokenType_BlockEnd:
            return ModC_ConstStringView_FromLiteral("ModC_TokenType_BlockEnd");
        case ModC_TokenType_InvokeStart:
            return ModC_ConstStringView_FromLiteral("ModC_TokenType_InvokeStart");
        case ModC_TokenType_InvokeEnd:
            return ModC_ConstStringView_FromLiteral("ModC_TokenType_InvokeEnd");
        case ModC_TokenType_Semicolon:
            return ModC_ConstStringView_FromLiteral("ModC_TokenType_Semicolon");
        case ModC_TokenType_StringLiteral:
            return ModC_ConstStringView_FromLiteral("ModC_TokenType_StringLiteral");
        case ModC_TokenType_CharLiteral:
            return ModC_ConstStringView_FromLiteral("ModC_TokenType_CharLiteral");
        case ModC_TokenType_IntLiteral:
            return ModC_ConstStringView_FromLiteral("ModC_TokenType_IntLiteral");
        case ModC_TokenType_FloatLiteral:
            return ModC_ConstStringView_FromLiteral("ModC_TokenType_FloatLiteral");
        case ModC_TokenType_DoubleLiteral:
            return ModC_ConstStringView_FromLiteral("ModC_TokenType_DoubleLiteral");
        case ModC_TokenType_BoolLiteral:
            return ModC_ConstStringView_FromLiteral("ModC_TokenType_BoolLiteral");
        case ModC_TokenType_Space:
            return ModC_ConstStringView_FromLiteral("ModC_TokenType_Space");
        case ModC_TokenType_Newline:
            return ModC_ConstStringView_FromLiteral("ModC_TokenType_Newline");
        case ModC_TokenType_Comment:
            return ModC_ConstStringView_FromLiteral("ModC_TokenType_Comment");
        case ModC_TokenType_Undef:
            return ModC_ConstStringView_FromLiteral("ModC_TokenType_Undef");
        case ModC_TokenType_Count:
            return ModC_ConstStringView_FromLiteral("ModC_TokenType_Count");
        default:
            return (ModC_ConstStringView){0};
    }
}

static inline ModC_Result_Void ModC_Token_AppendChar(   ModC_Token* this, 
                                                        const ModC_ConstStringView source, 
                                                        char c,
                                                        ModC_Allocator allocator,
                                                        bool forceString)
{
    #undef ModC_ResultName_State
    #define ModC_ResultName_State ModC_Result_Void
    
    MODC_CHECK(this != NULL, (""), MODC_RET_ERROR_S());
    
    if(this->TokenText.Type == MODC_TAG_TYPE_S(ModC_String))
    {
        ModC_String_AddValue(&this->TokenText.MODC_TAG_DATA_S(ModC_String), c);
        return MODC_RESULT_VALUE_S(0);
    }
    
    ModC_ConstStringView* tokenView = &this->TokenText.MODC_TAG_DATA_S(ModC_ConstStringView);
    MODC_CHECK( source.Data <= tokenView->Data, 
                ("source: %p, token: %p", source.Data, tokenView->Data),
                MODC_RET_ERROR_S());
    
    MODC_CHECK
    (
        source.Data + source.Length >= tokenView->Data + tokenView->Length, 
        ("source end: %p, token end: %p", 
        source.Data + source.Length, 
        tokenView->Data + tokenView->Length),
        MODC_RET_ERROR_S()
    );
    
    if(tokenView->Data[tokenView->Length] != c)
        forceString = true;
    
    if(forceString)
    {
        ModC_String tokenStr = ModC_String_Create(allocator, tokenView->Length + 1);
        ModC_String_AddRange(&tokenStr, tokenView->Data, tokenView->Length);
        ModC_String_AddValue(&tokenStr, c);
        this->TokenText.MODC_TAG_DATA_S(ModC_String) = tokenStr;
        return MODC_RESULT_VALUE_S(0);
    }
    
    ++(tokenView->Length);
    return MODC_RESULT_VALUE_S(0);
}

static inline ModC_CharTokenType ModC_CharTokenType_FromChar(char c)
{
    static_assert((int)ModC_TokenType_Count == 19, "");
    if(isalpha(c) || c == '_')
        return ModC_CharTokenType_Identifier;
    else if(isdigit(c))
        return ModC_CharTokenType_IntLiteral;
    else if(c == ' ' || c == '\t' || c == '\r')
        return ModC_CharTokenType_Space;
    
    switch(c)
    {
        case '\n':
            return ModC_CharTokenType_Newline;
        case '(':
            return ModC_CharTokenType_InvokeStart;
        case ')':
            return ModC_CharTokenType_InvokeEnd;
        case '{':
            return ModC_CharTokenType_BlockStart;
        case '}':
            return ModC_CharTokenType_BlockEnd;
        case '"':
            return ModC_CharTokenType_StringLiteral;
        case '\'':
            return ModC_CharTokenType_CharLiteral;
        case ';':
            return ModC_CharTokenType_Semicolon;
        case '#':
        case '[':
        case ']':
        case '=':
        case ',':
        case '.':
        case '/':
        case '\\':
        case '+':
        case '-':
        case '*':
        case '%':
        case '^':
        case '!':
        case '|':
        case '&':
        case '~':
        case '?':
        case ':':
            return ModC_CharTokenType_Operator;
        default:
            return ModC_CharTokenType_Undef;
    }
}


static inline bool IsLastCharEscaped(ModC_ConstStringView view)
{
    if(view.Length <= 1)    //You can't escape nothing or escape a single character itself
        return false;
    
    int consecBackslash = 0;
    //Starting from second last character, count how many consecutive backslashes
    for(int i = view.Length - 2; i >= 0; --i)
    {
        if(view.Data[i] == '\\')
            ++consecBackslash;
        else
            break;
    }
    
    //If we have odd number of backslahses, last character is escaped
    return (consecBackslash % 2 != 0);
}


static inline ModC_Result_Bool ModC_Token_IsCharPossible(   const ModC_Token* this, 
                                                            char c, 
                                                            ModC_CharTokenType cType)
{
    #undef ModC_ResultName_State
    #define ModC_ResultName_State ModC_Result_Bool
    
    MODC_CHECK(this != NULL, (""), MODC_RET_ERROR_S());
    
    static_assert((int)ModC_TokenType_Count == 19, "");
    ModC_ConstStringView tokenStringView = ModC_Token_TokenTextView(this);
    MODC_CHECK(tokenStringView.Length > 0, (""), MODC_RET_ERROR_S());
    
    switch((ModC_CharTokenType)this->TokenType)
    {
        case ModC_CharTokenType_Identifier:
        {
            if(cType == ModC_CharTokenType_Identifier || cType == ModC_CharTokenType_IntLiteral)
                return MODC_RESULT_VALUE_S(true);
            else
                return MODC_RESULT_VALUE_S(false);
        }
        case ModC_CharTokenType_Operator:
        case ModC_CharTokenType_BlockStart:
        case ModC_CharTokenType_BlockEnd:
        case ModC_CharTokenType_InvokeStart:
        case ModC_CharTokenType_InvokeEnd:
        case ModC_CharTokenType_Semicolon:
            return MODC_RESULT_VALUE_S(false);
        case ModC_CharTokenType_StringLiteral:
        case ModC_CharTokenType_CharLiteral:
        {
            if(c == '\n')   //You can't have newline in string or char literal
                return MODC_RESULT_VALUE_S(false);
            else
            {
                if((ModC_CharTokenType)this->TokenType == ModC_CharTokenType_CharLiteral)
                {
                    if(tokenStringView.Data[tokenStringView.Length - 1] == '\'')
                        return MODC_RESULT_VALUE_S(IsLastCharEscaped(tokenStringView));
                    
                    return MODC_RESULT_VALUE_S(true);
                }
                else
                {
                    if(tokenStringView.Data[tokenStringView.Length - 1] == '\"')
                        return MODC_RESULT_VALUE_S(IsLastCharEscaped(tokenStringView));
                    
                    return MODC_RESULT_VALUE_S(true);
                }
            }
        }
        case ModC_CharTokenType_IntLiteral:
        case ModC_CharTokenType_Space:
        case ModC_CharTokenType_Newline:
        case ModC_CharTokenType_Undef:
            return MODC_RESULT_VALUE_S(cType == (ModC_CharTokenType)this->TokenType);
        default:
            return MODC_ERROR_CSTR_S("Huh?");
    } //switch((ModC_CharTokenType)this->TokenType)
    
    return MODC_RESULT_VALUE_S(false);
}


//Returns list of tokens that are types in `ModC_CharTokenType` or `ModC_TokenType_Comment`
static inline ModC_Result_TokenList ModC_Tokenization(  const ModC_ConstStringView fileContent, 
                                                        ModC_Allocator allocator)
{
    #undef ModC_ResultName_State
    #define ModC_ResultName_State ModC_Result_TokenList
    
    if(fileContent.Length == 0)
        return MODC_RESULT_VALUE_S( (ModC_TokenList){0} );
    
    ModC_TokenList tokenList = ModC_TokenList_Create(   ModC_Allocator_Share(&allocator), 
                                                        fileContent.Length / 16);
    ModC_TokenList retList = {0};
    
    MODC_DEFER_SCOPE_START(0)
    {
        ModC_Token currentToken = 
            ModC_Token_FromView((ModC_TokenType)ModC_CharTokenType_FromChar(fileContent.Data[0]), 
                                ModC_ConstStringView_Create(&fileContent.Data[0], 1));
        MODC_DEFER(0, ModC_TokenList_Free(&tokenList));
        
        if(fileContent.Length == 1)
        {
            ModC_TokenList_AddValue(&tokenList, currentToken);
            MODC_MOVE(ModC_TokenList, retList, tokenList);
            MODC_DEFER_BREAK(0, );
        }
    
        int currentLineIndex = fileContent.Data[0] == '\n';
        int currentColumnIndex = 1;
        bool lineComment = false;
        for(int i = 1; i < fileContent.Length; ++i)
        {
            ModC_CharTokenType charTokenType = ModC_CharTokenType_FromChar(fileContent.Data[i]);
            
            #define APPEND_CHAR_TO_TOKEN() \
                do \
                { \
                    ModC_Result_Void voidResult = ModC_Token_AppendChar(&currentToken, \
                                                                        fileContent, \
                                                                        fileContent.Data[i], \
                                                                        allocator, \
                                                                        false); \
                    (void)MODC_RESULT_TRY(voidResult, MODC_DEFER_BREAK(0, MODC_RET_ERROR_S())); \
                } while(0)
            
            #define CREATE_NEW_TOKEN() \
                do \
                { \
                    /* Create new token */ \
                    currentToken = \
                        ModC_Token_FromView((ModC_TokenType)charTokenType, \
                                            ModC_ConstStringView_Create(&fileContent.Data[i], 1)); \
                    currentToken.LineIndex = currentLineIndex; \
                    currentToken.ColumnIndex = currentColumnIndex; \
                    currentToken.SourceIndex = i; \
                } while(0)
            
            #define NEXT_CHAR() \
                do \
                { \
                    if(fileContent.Data[i] == '\n') \
                    { \
                        ++currentLineIndex; \
                        currentColumnIndex = 0; \
                    } \
                    ++currentColumnIndex; \
                    continue; \
                } while(0)
            
            //If we encounter `\<newline>`, ignore it.
            if( fileContent.Data[i] == '\\' && 
                i + 1 < fileContent.Length && 
                fileContent.Data[i + 1] == '\n')
            {
                ++i;
                NEXT_CHAR();
            }
            
            bool inComment = false;
            
            //Check if we are entering line or block comment, if so change type to comment
            if( currentToken.TokenType == ModC_TokenType_Operator && 
                ModC_Token_TokenTextView(&currentToken).Length == 1 &&
                ModC_Token_TokenTextView(&currentToken).Data[0] == '/' &&
                (fileContent.Data[i] == '*' || fileContent.Data[i] == '/'))
            {
                inComment = true;
                currentToken.TokenType = ModC_TokenType_Comment;
                APPEND_CHAR_TO_TOKEN();
                lineComment = fileContent.Data[i] == '/';
            }
            //Check if have finished line comment or block comment
            else if(currentToken.TokenType == ModC_TokenType_Comment)
            {
                if(i == fileContent.Length - 1)
                {
                    APPEND_CHAR_TO_TOKEN();
                    break;
                }
                
                inComment = true;
                //Line comment
                if(lineComment)
                {
                    if(fileContent.Data[i] == '\n')
                    {
                        ModC_TokenList_AddValue(&tokenList, currentToken);
                        CREATE_NEW_TOKEN();
                    }
                    else
                        APPEND_CHAR_TO_TOKEN();
                }
                //Block comment
                else
                {
                    ModC_ConstStringView tokenView = ModC_Token_TokenTextView(&currentToken);
                    if( tokenView.Length >= 4 && 
                        tokenView.Data[tokenView.Length - 2] == '*' &&
                        tokenView.Data[tokenView.Length - 1] == '/')
                    {
                        ModC_TokenList_AddValue(&tokenList, currentToken);
                        CREATE_NEW_TOKEN();
                    }
                    else
                        APPEND_CHAR_TO_TOKEN();
                }
            }
            
            if(!inComment)
            {
                ModC_Result_Bool boolResult = ModC_Token_IsCharPossible(&currentToken, 
                                                                        fileContent.Data[i], 
                                                                        charTokenType);
                bool possible = *MODC_RESULT_TRY(boolResult, MODC_DEFER_BREAK(0, MODC_RET_ERROR_S()));
                if(!possible)
                {
                    ModC_TokenList_AddValue(&tokenList, currentToken);
                    CREATE_NEW_TOKEN();
                }
                else
                    APPEND_CHAR_TO_TOKEN();
            }
            
            NEXT_CHAR();
        } //for(int i = 1; i < fileContent.Length; ++i)
        
        ModC_TokenList_AddValue(&tokenList, currentToken);
        MODC_MOVE(ModC_TokenList, retList, tokenList);
        MODC_DEFER_BREAK(0, );
    }
    MODC_DEFER_SCOPE_END(0)
    
    return MODC_RESULT_VALUE_S(retList);
}

#endif
