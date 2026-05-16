#ifndef MODC_TOKENIZATION_H
#define MODC_TOKENIZATION_H

#ifndef DEFAULT_ALLOC
    #define DEFAULT_ALLOC() CreateHeapAllocator()
#endif

#include "ModC/Strings/Strings.h"
#include "ModC/Result.h"
#include "ModC/GenericContainers.h"
#include "ModC/Move.h"

#include "static_assert.h/assert.h"

#include <ctype.h>
#include <stdbool.h>

typedef enum TokenType
{
    TokenType_Type,
    TokenType_Keyword,
    TokenType_Identifier,
    TokenType_Operator,
    TokenType_BlockStart,
    TokenType_BlockEnd,
    TokenType_InvokeStart,
    TokenType_InvokeEnd,
    TokenType_Semicolon,
    TokenType_StringLiteral,
    TokenType_CharLiteral,
    TokenType_IntLiteral,
    TokenType_FloatLiteral,
    TokenType_DoubleLiteral,
    TokenType_BoolLiteral,
    TokenType_Space,
    TokenType_Newline,
    TokenType_Comment,
    TokenType_Undef,
    TokenType_Count,   //19
} TokenType;

typedef enum CharTokenType
{
    CharTokenType_Identifier = TokenType_Identifier,
    CharTokenType_Operator = TokenType_Operator,
    CharTokenType_BlockStart = TokenType_BlockStart,
    CharTokenType_BlockEnd = TokenType_BlockEnd,
    CharTokenType_InvokeStart = TokenType_InvokeStart,
    CharTokenType_InvokeEnd = TokenType_InvokeEnd,
    CharTokenType_Semicolon = TokenType_Semicolon,
    CharTokenType_StringLiteral = TokenType_StringLiteral,
    CharTokenType_CharLiteral = TokenType_CharLiteral,
    CharTokenType_IntLiteral = TokenType_IntLiteral,
    CharTokenType_Space = TokenType_Space,
    CharTokenType_Newline = TokenType_Newline,
    CharTokenType_Undef = TokenType_Undef,
} CharTokenType;

typedef struct Token
{
    TokenType TokenType;
    StringUnion TokenText;
    int LineIndex;
    int ColumnIndex;
    int SourceIndex;
} Token;

static inline void Token_Free(Token* this);

#define LIST_NAME TokenList
#define VALUE_TYPE Token
#define VALUE_FREE(ptr) Token_Free(ptr)
#include "ModC/List.h"

DEFINE_RESULT_STRUCT(ModC_Result_TokenList, TokenList)
DEFINE_RESULT_STRUCT(ModC_Result_Token, Token)
DEFINE_RESULT_STRUCT(ModC_Result_TokenPtr, Token*)

static inline ConstStringView Token_TokenTextView(const Token* this)
{
    #undef TaggedUnionNameState
    #define TaggedUnionNameState StringUnion
    
    if(!this)
        return ConstStringView_Create(NULL, 0);
    
    return StringUnion_GetConstView(&this->TokenText);
}

static inline String Token_VisualizeLocation(   const Token* this, 
                                                Allocator allocator, 
                                                bool spanWholeLine,
                                                const ConstStringView source)
{
    if(!this || source.Length <= this->SourceIndex)
    {
        return String_FromLiteral(  allocator, 
                                    "INTERNAL FAILURE: !this || source.Length <= "
                                    "this->SourceIndex");
    }
    
    int sourceIndex =   source.Data[this->SourceIndex] == '\n' ? 
                        this->SourceIndex - 1 :
                        this->SourceIndex;

    int charAfter = 0;
    for(int i = sourceIndex; i < source.Length; ++i)
    {
        if(source.Data[i] == '\n')
            break;
        ++charAfter;
    }
    
    int charBefore = 0;
    for(int i = sourceIndex - 1; i >= 0; --i)
    {
        if(source.Data[i] == '\n')
            break;
        ++charBefore;
    }
    
    if(sourceIndex < 0)
        return String_FromFormat(allocator, "Line %d", this->LineIndex + 1);
    
    if(!spanWholeLine)
    {
        return String_FromFormat(   allocator, 
                                    "%5d | %.*s\n      | %*s", 
                                    this->LineIndex + 1, 
                                    charBefore + charAfter, 
                                    &source.Data[sourceIndex - charBefore],
                                    charBefore + 1,
                                    "^");
    }
    else
    {
        String retStr = String_FromFormat(  allocator, 
                                            "%5d | %.*s\n      | %*s", 
                                            this->LineIndex + 1, 
                                            charBefore + charAfter, 
                                            &source.Data[sourceIndex - charBefore],
                                            charBefore + 1,
                                            "^");
        for(int i = 0; i < charAfter - 1; ++i)
            String_AddValue(&retStr, '~');
        return retStr;
    }
}

static inline void Token_Free(Token* this)
{
    if(!this)
        return;
    if(this->TokenText.Type != TU_TYPE_S(String))
    {
        *this = (Token){0};
        return;
    }
    
    String_Free(&this->TokenText.TU_DATA_S(String));
}

static inline Token Token_FromString(TokenType type, String str)
{
    return (Token){ .TokenType = type, .TokenText = TU_INIT_S(String, str) };
}

static inline Token Token_FromView(TokenType type, ConstStringView view)
{
    return  (Token)
            { 
                .TokenType = type, 
                .TokenText = TU_INIT_S(ConstStringView, view)
            };
}

static inline bool Token_IsSkippable(const Token* this)
{
    return  !this || 
            this->TokenType == TokenType_Space || 
            this->TokenType == TokenType_Newline ||
            this->TokenType == TokenType_Comment;
}

static inline ConstStringView TokenType_ToCStr(TokenType type)
{
    static_assert((int)TokenType_Count == 19, "");
    switch(type)
    {
        case TokenType_Type:
            return ConstStringView_FromLiteral("TokenType_Type");
        case TokenType_Keyword:
            return ConstStringView_FromLiteral("TokenType_Keyword");
        case TokenType_Identifier:
            return ConstStringView_FromLiteral("TokenType_Identifier");
        case TokenType_Operator:
            return ConstStringView_FromLiteral("TokenType_Operator");
        case TokenType_BlockStart:
            return ConstStringView_FromLiteral("TokenType_BlockStart");
        case TokenType_BlockEnd:
            return ConstStringView_FromLiteral("TokenType_BlockEnd");
        case TokenType_InvokeStart:
            return ConstStringView_FromLiteral("TokenType_InvokeStart");
        case TokenType_InvokeEnd:
            return ConstStringView_FromLiteral("TokenType_InvokeEnd");
        case TokenType_Semicolon:
            return ConstStringView_FromLiteral("TokenType_Semicolon");
        case TokenType_StringLiteral:
            return ConstStringView_FromLiteral("TokenType_StringLiteral");
        case TokenType_CharLiteral:
            return ConstStringView_FromLiteral("TokenType_CharLiteral");
        case TokenType_IntLiteral:
            return ConstStringView_FromLiteral("TokenType_IntLiteral");
        case TokenType_FloatLiteral:
            return ConstStringView_FromLiteral("TokenType_FloatLiteral");
        case TokenType_DoubleLiteral:
            return ConstStringView_FromLiteral("TokenType_DoubleLiteral");
        case TokenType_BoolLiteral:
            return ConstStringView_FromLiteral("TokenType_BoolLiteral");
        case TokenType_Space:
            return ConstStringView_FromLiteral("TokenType_Space");
        case TokenType_Newline:
            return ConstStringView_FromLiteral("TokenType_Newline");
        case TokenType_Comment:
            return ConstStringView_FromLiteral("TokenType_Comment");
        case TokenType_Undef:
            return ConstStringView_FromLiteral("TokenType_Undef");
        case TokenType_Count:
            return ConstStringView_FromLiteral("TokenType_Count");
        default:
            return (ConstStringView){0};
    }
}

static inline ModC_Result_Void Token_AppendChar(Token* this, 
                                                const ConstStringView source, 
                                                char c,
                                                Allocator allocator,
                                                bool forceString)
{
    #undef ResultNameState
    #define ResultNameState ModC_Result_Void
    
    CHECK(this != NULL, (""), RET_ERROR_S());
    
    if(this->TokenText.Type == TU_TYPE_S(String))
    {
        String_AddValue(&this->TokenText.TU_DATA_S(String), c);
        return RESULT_VALUE_S(0);
    }
    
    ConstStringView* tokenView = &this->TokenText.TU_DATA_S(ConstStringView);
    CHECK(  source.Data <= tokenView->Data, 
            ("source: %p, token: %p", source.Data, tokenView->Data),
            RET_ERROR_S());
    
    CHECK(  source.Data + source.Length >= tokenView->Data + tokenView->Length, 
            ("source end: %p, token end: %p", 
            source.Data + source.Length, 
            tokenView->Data + tokenView->Length),
            RET_ERROR_S());
    
    if(tokenView->Data[tokenView->Length] != c)
        forceString = true;
    
    if(forceString)
    {
        String tokenStr = String_Create(allocator, tokenView->Length + 1);
        String_AddRange(&tokenStr, tokenView->Data, tokenView->Length);
        String_AddValue(&tokenStr, c);
        this->TokenText.TU_DATA_S(String) = tokenStr;
        return RESULT_VALUE_S(0);
    }
    
    ++(tokenView->Length);
    return RESULT_VALUE_S(0);
}

static inline CharTokenType CharTokenType_FromChar(char c)
{
    static_assert((int)TokenType_Count == 19, "");
    if(isalpha(c) || c == '_')
        return CharTokenType_Identifier;
    else if(isdigit(c))
        return CharTokenType_IntLiteral;
    else if(c == ' ' || c == '\t' || c == '\r')
        return CharTokenType_Space;
    
    switch(c)
    {
        case '\n':
            return CharTokenType_Newline;
        case '(':
            return CharTokenType_InvokeStart;
        case ')':
            return CharTokenType_InvokeEnd;
        case '{':
            return CharTokenType_BlockStart;
        case '}':
            return CharTokenType_BlockEnd;
        case '"':
            return CharTokenType_StringLiteral;
        case '\'':
            return CharTokenType_CharLiteral;
        case ';':
            return CharTokenType_Semicolon;
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
            return CharTokenType_Operator;
        default:
            return CharTokenType_Undef;
    }
}


static inline bool IsLastCharEscaped(ConstStringView view)
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


static inline ModC_Result_Bool Token_IsCharPossible(const Token* this, char c, CharTokenType cType)
{
    #undef ResultNameState
    #define ResultNameState ModC_Result_Bool
    
    CHECK(this != NULL, (""), RET_ERROR_S());
    
    static_assert((int)TokenType_Count == 19, "");
    ConstStringView tokenStringView = Token_TokenTextView(this);
    CHECK(tokenStringView.Length > 0, (""), RET_ERROR_S());
    
    switch((CharTokenType)this->TokenType)
    {
        case CharTokenType_Identifier:
        {
            if(cType == CharTokenType_Identifier || cType == CharTokenType_IntLiteral)
                return RESULT_VALUE_S(true);
            else
                return RESULT_VALUE_S(false);
        }
        case CharTokenType_Operator:
        case CharTokenType_BlockStart:
        case CharTokenType_BlockEnd:
        case CharTokenType_InvokeStart:
        case CharTokenType_InvokeEnd:
        case CharTokenType_Semicolon:
            return RESULT_VALUE_S(false);
        case CharTokenType_StringLiteral:
        case CharTokenType_CharLiteral:
        {
            if(c == '\n')   //You can't have newline in string or char literal
                return RESULT_VALUE_S(false);
            else
            {
                if((CharTokenType)this->TokenType == CharTokenType_CharLiteral)
                {
                    if(tokenStringView.Data[tokenStringView.Length - 1] == '\'')
                        return RESULT_VALUE_S(IsLastCharEscaped(tokenStringView));
                    
                    return RESULT_VALUE_S(true);
                }
                else
                {
                    if(tokenStringView.Data[tokenStringView.Length - 1] == '\"')
                        return RESULT_VALUE_S(IsLastCharEscaped(tokenStringView));
                    
                    return RESULT_VALUE_S(true);
                }
            }
        }
        case CharTokenType_IntLiteral:
        case CharTokenType_Space:
        case CharTokenType_Newline:
        case CharTokenType_Undef:
            return RESULT_VALUE_S(cType == (CharTokenType)this->TokenType);
        default:
            return ERROR_CSTR_S("Huh?");
    } //switch((CharTokenType)this->TokenType)
    
    return RESULT_VALUE_S(false);
}


//Returns list of tokens that are types in `CharTokenType` or `TokenType_Comment`
static inline ModC_Result_TokenList Tokenization(   const ConstStringView fileContent, 
                                                    Allocator allocator)
{
    #undef ResultNameState
    #define ResultNameState ModC_Result_TokenList
    
    if(fileContent.Length == 0)
        return RESULT_VALUE_S( (TokenList){0} );
    
    TokenList tokenList = TokenList_Create(Allocator_Share(&allocator), fileContent.Length / 16);
    TokenList retList = {0};
    
    DEFER_SCOPE_START(0)
    {
        Token currentToken = Token_FromView((TokenType)CharTokenType_FromChar(fileContent.Data[0]), 
                                            ConstStringView_Create(&fileContent.Data[0], 1));
        DEFER(0, TokenList_Free(&tokenList));
        
        if(fileContent.Length == 1)
        {
            TokenList_AddValue(&tokenList, currentToken);
            MOVE(TokenList, retList, tokenList);
            DEFER_BREAK(0, );
        }
    
        int currentLineIndex = fileContent.Data[0] == '\n';
        int currentColumnIndex = 1;
        bool lineComment = false;
        for(int i = 1; i < fileContent.Length; ++i)
        {
            CharTokenType charTokenType = CharTokenType_FromChar(fileContent.Data[i]);
            
            #define APPEND_CHAR_TO_TOKEN() \
                do \
                { \
                    ModC_Result_Void voidResult = Token_AppendChar( &currentToken, \
                                                                    fileContent, \
                                                                    fileContent.Data[i], \
                                                                    allocator, \
                                                                    false); \
                    (void)RESULT_TRY(voidResult, DEFER_BREAK(0, RET_ERROR_S())); \
                } while(0)
            
            #define CREATE_NEW_TOKEN() \
                do \
                { \
                    /* Create new token */ \
                    currentToken = \
                        Token_FromView((TokenType)charTokenType, \
                                            ConstStringView_Create(&fileContent.Data[i], 1)); \
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
            if( currentToken.TokenType == TokenType_Operator && 
                Token_TokenTextView(&currentToken).Length == 1 &&
                Token_TokenTextView(&currentToken).Data[0] == '/' &&
                (fileContent.Data[i] == '*' || fileContent.Data[i] == '/'))
            {
                inComment = true;
                currentToken.TokenType = TokenType_Comment;
                APPEND_CHAR_TO_TOKEN();
                lineComment = fileContent.Data[i] == '/';
            }
            //Check if have finished line comment or block comment
            else if(currentToken.TokenType == TokenType_Comment)
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
                        TokenList_AddValue(&tokenList, currentToken);
                        CREATE_NEW_TOKEN();
                    }
                    else
                        APPEND_CHAR_TO_TOKEN();
                }
                //Block comment
                else
                {
                    ConstStringView tokenView = Token_TokenTextView(&currentToken);
                    if( tokenView.Length >= 4 && 
                        tokenView.Data[tokenView.Length - 2] == '*' &&
                        tokenView.Data[tokenView.Length - 1] == '/')
                    {
                        TokenList_AddValue(&tokenList, currentToken);
                        CREATE_NEW_TOKEN();
                    }
                    else
                        APPEND_CHAR_TO_TOKEN();
                }
            }
            
            if(!inComment)
            {
                ModC_Result_Bool boolResult = Token_IsCharPossible( &currentToken, 
                                                                    fileContent.Data[i], 
                                                                    charTokenType);
                bool possible = *RESULT_TRY(boolResult, DEFER_BREAK(0, RET_ERROR_S()));
                if(!possible)
                {
                    TokenList_AddValue(&tokenList, currentToken);
                    CREATE_NEW_TOKEN();
                }
                else
                    APPEND_CHAR_TO_TOKEN();
            }
            
            NEXT_CHAR();
        } //for(int i = 1; i < fileContent.Length; ++i)
        
        TokenList_AddValue(&tokenList, currentToken);
        MOVE(TokenList, retList, tokenList);
        DEFER_BREAK(0, );
    }
    DEFER_SCOPE_END(0)
    
    return RESULT_VALUE_S(retList);
}

#endif
