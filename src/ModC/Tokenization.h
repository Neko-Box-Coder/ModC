#ifndef MODC_TOKENIZATION_H
#define MODC_TOKENIZATION_H

#ifndef MODC_DEFAULT_ALLOC
    #define MODC_DEFAULT_ALLOC() ModC_CreateHeapAllocator()
#endif

#include "ModC/Strings/Strings.h"
#include "ModC/GenericContainers.h"
#include "static_assert.h/assert.h"

#include <ctype.h>
#include <stdbool.h>

typedef enum
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
    ModC_TokenType_Undef,
    ModC_TokenType_Count,   //18
} ModC_TokenType;

typedef enum
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

typedef struct
{
    ModC_TokenType TokenType;
    ModC_StringOrConstView TokenText;
    int LineIndex;
    int ColumnIndex;
} ModC_Token;

static inline ModC_Token ModC_Token_CreateString(ModC_TokenType type, ModC_String str)
{
    return (ModC_Token){ .TokenType = type, .TokenText = ModC_StringOrConstView_String(str) };
}

static inline ModC_Token ModC_Token_CreateView(ModC_TokenType type, ModC_ConstStringView view)
{
    return (ModC_Token){ .TokenType = type, .TokenText = ModC_StringOrConstView_View(view) };
}

#undef ModC_ResultName
#define ModC_ResultName ModC_Result_Void

static inline ModC_Result_Void ModC_Token_AppendChar(   ModC_Token* this, 
                                                        const ModC_ConstStringView source, 
                                                        char c,
                                                        ModC_Allocator allocator,
                                                        bool forceString)
{
    MODC_ASSERT(this != NULL, (""), MODC_RET_ERROR());
    
    if(this->TokenText.IsString)
    {
        ModC_String_AddValue(&this->TokenText.Value.String, c);
        return MODC_RESULT_VALUE(0);
    }
    
    ModC_ConstStringView* tokenView = &this->TokenText.Value.View;
    if(forceString)
    {
        ModC_String tokenStr = ModC_String_Create(allocator, tokenView->Length + 1);
        ModC_String_AddRange(&tokenStr, tokenView->Data, tokenView->Length);
        this->TokenText = ModC_StringOrConstView_String(tokenStr);
        ModC_String_AddValue(&tokenStr, c);
        return MODC_RESULT_VALUE(0);
    }
    
    MODC_ASSERT(source.Data <= tokenView->Data, 
                ("source: %p, token: %p", source.Data, tokenView->Data),
                MODC_RET_ERROR());
    
    MODC_ASSERT
    (
        source.Data + source.Length > tokenView->Data + tokenView->Length, 
        ("source: %p, token: %p", source.Data + source.Length, tokenView->Data + tokenView->Length),
        MODC_RET_ERROR()
    );
    
    MODC_ASSERT
    (
        tokenView->Data[tokenView->Length] == c, 
        ("token next char: %i, passed char: %i", (int)tokenView->Data[tokenView->Length], (int)c),
        MODC_RET_ERROR()
    );
    
    ++(tokenView->Length);
    return MODC_RESULT_VALUE(0);
}


#define MODC_LIST_NAME ModC_TokenList
#define MODC_VALUE_TYPE ModC_Token
#include "ModC/List.h"

typedef enum 
{
    ModC_CharType_Alpha,    //Including underscore
    ModC_CharType_Numeric,
    ModC_CharType_Space,
    ModC_CharType_Newline,
    ModC_CharType_Hash,
    ModC_CharType_LParen,
    ModC_CharType_RParen,
    ModC_CharType_LCurlyBrace,
    ModC_CharType_RCurlyBrace,
    ModC_CharType_LSquareBrace,
    ModC_CharType_RSquareBrace,
    ModC_CharType_Semicolon,
    ModC_CharType_Assign,
    ModC_CharType_Comma,
    ModC_CharType_Dot,
    ModC_CharType_Slash,
    ModC_CharType_BackSlash,
    ModC_CharType_Plus,
    ModC_CharType_Minus,
    ModC_CharType_Multi,
    ModC_CharType_Modulo,
    ModC_CharType_BitXor,
    ModC_CharType_StringQuote,
    ModC_CharType_CharQuote,
    ModC_CharType_LogicalNot,
    ModC_CharType_BitOr,
    ModC_CharType_BitAnd,
    ModC_CharType_BitNot,
    ModC_CharType_TernaryCondition,
    ModC_CharType_TernaryColon,
    ModC_CharType_Undef,
    ModC_CharType_Count,    //31
} ModC_CharType;


//TODO: Use TokenType instead
static inline ModC_CharType GetCharType(char c)
{
    if(isalpha(c) || c == '_')
        return ModC_CharType_Alpha;
    else if(isdigit(c))
        return ModC_CharType_Numeric;
    else if(c == ' ' || c == '\t' || c == '\r')
        return ModC_CharType_Space;
    
    static_assert((int)ModC_CharType_Count == 31, "");
    switch(c)
    {
        case '\n':
            return ModC_CharType_Newline;
        case '#':
            return ModC_CharType_Hash;
        case '(':
            return ModC_CharType_LParen;
        case ')':
            return ModC_CharType_RParen;
        case '{':
            return ModC_CharType_LCurlyBrace;
        case '}':
            return ModC_CharType_RCurlyBrace;
        case '[':
            return ModC_CharType_LSquareBrace;
        case ']':
            return ModC_CharType_RSquareBrace;
        case ';':
            return ModC_CharType_Semicolon;
        case '=':
            return ModC_CharType_Assign;
        case ',':
            return ModC_CharType_Comma;
        case '.':
            return ModC_CharType_Dot;
        case '/':
            return ModC_CharType_Slash;
        case '\\':
            return ModC_CharType_BackSlash;
        case '+':
            return ModC_CharType_Plus;
        case '-':
            return ModC_CharType_Minus;
        case '*':
            return ModC_CharType_Multi;
        case '%':
            return ModC_CharType_Modulo;
        case '^':
            return ModC_CharType_BitXor;
        case '"':
            return ModC_CharType_StringQuote;
        case '\'':
            return ModC_CharType_CharQuote;
        case '!':
            return ModC_CharType_LogicalNot;
        case '|':
            return ModC_CharType_BitOr;
        case '&':
            return ModC_CharType_BitAnd;
        case '~':
            return ModC_CharType_BitNot;
        case '?':
            return ModC_CharType_TernaryCondition;
        case ':':
            return ModC_CharType_TernaryColon;
        default:
            return ModC_CharType_Undef;
    }
}

static inline ModC_CharTokenType ModC_CharType_ToCharTokenType(ModC_CharType type)
{
    static_assert((int)ModC_CharType_Count == 31, "");
    static_assert((int)ModC_TokenType_Count == 18, "");
    switch(type)
    {
        case ModC_CharType_Alpha:
            return ModC_CharTokenType_Identifier;
        case ModC_CharType_Numeric:
            return ModC_CharTokenType_IntLiteral;
        case ModC_CharType_Space:
            return ModC_CharTokenType_Space;
        case ModC_CharType_Newline:
            return ModC_CharTokenType_Newline;
        case ModC_CharType_LParen:
            return ModC_CharTokenType_InvokeStart;
        case ModC_CharType_RParen:
            return ModC_CharTokenType_InvokeEnd;
        case ModC_CharType_LCurlyBrace:
            return ModC_CharTokenType_BlockStart;
        case ModC_CharType_RCurlyBrace:
            return ModC_CharTokenType_BlockEnd;
        case ModC_CharType_StringQuote:
            return ModC_CharTokenType_StringLiteral;
        case ModC_CharType_CharQuote:
            return ModC_CharTokenType_CharLiteral;
        case ModC_CharType_Semicolon:
            return ModC_CharTokenType_Semicolon;
        case ModC_CharType_Hash:
        case ModC_CharType_LSquareBrace:
        case ModC_CharType_RSquareBrace:
        case ModC_CharType_Assign:
        case ModC_CharType_Comma:
        case ModC_CharType_Dot:
        case ModC_CharType_Slash:
        case ModC_CharType_BackSlash:
        case ModC_CharType_Plus:
        case ModC_CharType_Minus:
        case ModC_CharType_Multi:
        case ModC_CharType_Modulo:
        case ModC_CharType_BitXor:
        case ModC_CharType_LogicalNot:
        case ModC_CharType_BitOr:
        case ModC_CharType_BitAnd:
        case ModC_CharType_BitNot:
        case ModC_CharType_TernaryCondition:
        case ModC_CharType_TernaryColon:
            return ModC_CharTokenType_Operator;
        case ModC_CharType_Undef:
        case ModC_CharType_Count:
            return ModC_CharTokenType_Undef;
    }
}

#undef ModC_ResultName
#define ModC_ResultName ModC_Result_Bool

static inline ModC_Result_Bool ModC_Token_IsCharExpandable( ModC_Token* this, 
                                                            char c, 
                                                            ModC_CharTokenType cType)
{
    MODC_ASSERT(this != NULL, (""), MODC_RET_ERROR());
    
    static_assert((int)ModC_TokenType_Count == 18, "");
    ModC_ConstStringView tokenView =    this->TokenText.IsString ? 
                                        ModC_ConstStringView_Create(this->TokenText
                                                                        .Value
                                                                        .String
                                                                        .Data,
                                                                    this->TokenText
                                                                        .Value
                                                                        .String
                                                                        .Length) :
                                        this->TokenText.Value.View;
    switch(this->TokenType)
    {
        case ModC_TokenType_Type:
        case ModC_TokenType_Keyword:
        case ModC_TokenType_Identifier:
        {
            if(cType == ModC_CharTokenType_Identifier || cType == ModC_CharTokenType_IntLiteral)
                return MODC_RESULT_VALUE(true);
            else
                return MODC_RESULT_VALUE(false);
        }
        case ModC_TokenType_Operator:
        {
            if(tokenView.Length >= 3)
                return MODC_RESULT_VALUE(false);
            else if(tokenView.Length == 2)
            {
                if( ModC_CsontStringView_IsEqualCStr(&tokenView, ">>") ||
                    ModC_CsontStringView_IsEqualCStr(&tokenView, "<<"))
                {
                    if(c == '=')
                        return MODC_RESULT_VALUE(true);
                    else
                        return MODC_RESULT_VALUE(false);
                }
                else
                    return MODC_RESULT_VALUE(false);
            }
            
            //Assignment
            switch(c)
            {
                case '=':
                {
                    switch(tokenView.Data[0])
                    {
                        case '+':
                        case '-':
                        case '*':
                        case '%':
                        case '^':
                        case '!':
                        case '|':
                        case '&':
                        case '=':
                        case '/':
                            return MODC_RESULT_VALUE(true);
                        default:
                            return MODC_RESULT_VALUE(false);
                    }
                }
                case '>':
                    return MODC_RESULT_VALUE(tokenView.Data[0] == '-');
                default:
                    return MODC_RESULT_VALUE(false);
            }
        } //case ModC_TokenType_Operator:
        case ModC_TokenType_BlockStart:
        case ModC_TokenType_BlockEnd:
        case ModC_TokenType_InvokeStart:
        case ModC_TokenType_InvokeEnd:
        case ModC_TokenType_Semicolon:
            return MODC_RESULT_VALUE(false);
        case ModC_TokenType_StringLiteral:
        case ModC_TokenType_CharLiteral:
        {
            if(c == '\n')
                return MODC_RESULT_VALUE(false);
            else
                return MODC_RESULT_VALUE(true);
        }
        case ModC_TokenType_IntLiteral:
        case ModC_TokenType_FloatLiteral:
        case ModC_TokenType_DoubleLiteral:
        {
            if(cType == ModC_CharTokenType_IntLiteral)
                return MODC_RESULT_VALUE(true);
            if(this->TokenType == ModC_TokenType_IntLiteral)
                return MODC_RESULT_VALUE(false);
            
            if(c == '.')
            {
                if(ModC_ConstStringView_Find(&tokenView, ".") != tokenView.Length)
                    return MODC_RESULT_VALUE(true);
                else
                    return MODC_RESULT_VALUE(false);
            }
            else
                return MODC_RESULT_VALUE(false);
        }
        case ModC_TokenType_BoolLiteral:
        {
            switch(c)
            {
                case 't':
                case 'r':
                case 'u':
                case 'e':
                    return MODC_RESULT_VALUE(true);
                default:
                    return MODC_RESULT_VALUE(false);
            }
        }
        case ModC_TokenType_Space:
            return MODC_RESULT_VALUE(cType == ModC_CharTokenType_Space);
        case ModC_TokenType_Newline:
            return MODC_RESULT_VALUE(cType == ModC_CharTokenType_Newline);
        case ModC_TokenType_Undef:
            return MODC_RESULT_VALUE(cType == ModC_CharTokenType_Undef);
        case ModC_TokenType_Count:
        default:
            return MODC_ERROR_CSTR("Huh?");
    } //switch(this->TokenType)
    
    return MODC_RESULT_VALUE(false);
}

#include "ModC/Result.h"
MODC_DEFINE_RESULT_STRUCT(ModC_Result_TokenList, ModC_TokenList)

#undef ModC_ResultName
#define ModC_ResultName ModC_Result_TokenList

static inline ModC_Result_TokenList Tokenization(   const ModC_ConstStringView fileContent, 
                                                    ModC_Allocator allocator)
{
    if(fileContent.Length == 0)
        return MODC_RESULT_VALUE( (ModC_TokenList){0} );
    
    ModC_TokenList tokenList = ModC_TokenList_Create(ModC_Allocator_Share(&allocator), 128);
    ModC_CharType firstCharType = GetCharType(fileContent.Data[0]);
    ModC_Token currentToken = 
        ModC_Token_CreateView(  (ModC_TokenType)ModC_CharType_ToCharTokenType(firstCharType), 
                                ModC_ConstStringView_Create(&fileContent.Data[0], 1));
    if(fileContent.Length == 1)
    {
        ModC_TokenList_AddValue(&tokenList, currentToken);
        return MODC_RESULT_VALUE(tokenList);
    }
    
    for(int i = 1; i < fileContent.Length - 1; ++i)
    {
        ModC_CharTokenType charTokenType = ModC_CharType_ToCharTokenType(fileContent.Data[i]);
        ModC_Result_Bool boolResult = ModC_Token_IsCharExpandable(  &currentToken, 
                                                                    fileContent.Data[i], 
                                                                    charTokenType);
        bool appendable = MODC_RESULT_TRY(boolResult, MODC_RET_ERROR());
        if(!appendable)
        {
            
        }
        else
        {
            ModC_Result_Void voidResult = ModC_Token_AppendChar(&currentToken, 
                                                                fileContent, 
                                                                fileContent.Data[i], 
                                                                allocator, 
                                                                false);
            (void)MODC_RESULT_TRY(voidResult, MODC_RET_ERROR());
        }
    }
    
    return MODC_RESULT_VALUE(tokenList);
}

#endif
