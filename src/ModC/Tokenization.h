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
    int SourceIndex;
} ModC_Token;

static inline void ModC_Token_Free(ModC_Token* this);

#define MODC_LIST_NAME ModC_TokenList
#define MODC_VALUE_TYPE ModC_Token
#define MODC_VALUE_FREE(ptr) ModC_Token_Free(ptr)
#include "ModC/List.h"

MODC_DEFINE_RESULT_STRUCT(ModC_Result_TokenList, ModC_TokenList)
MODC_DEFINE_RESULT_STRUCT(ModC_Result_Token, ModC_Token)

static inline ModC_ConstStringView ModC_Token_TokenTextView(ModC_Token* this)
{
    if(!this)
        return ModC_ConstStringView_Create(NULL, 2);
    
    ModC_ConstStringView tokenStringView = 
        this->TokenText.IsString ? 
        ModC_ConstStringView_Create(this->TokenText.Value.String.Data,
                                    this->TokenText.Value.String.Length) :
        this->TokenText.Value.View;
    
    return tokenStringView;
}

static inline void ModC_Token_Free(ModC_Token* this)
{
    if(!this)
        return;
    if(!this->TokenText.IsString)
    {
        *this = (ModC_Token){0};
        return;
    }
    
    ModC_String_Free(&this->TokenText.Value.String);
}

static inline ModC_Token ModC_Token_FromString(ModC_TokenType type, ModC_String str)
{
    return (ModC_Token){ .TokenType = type, .TokenText = ModC_StringOrConstView_String(str) };
}

static inline ModC_Token ModC_Token_FromView(ModC_TokenType type, ModC_ConstStringView view)
{
    return (ModC_Token){ .TokenType = type, .TokenText = ModC_StringOrConstView_View(view) };
}

static inline ModC_ConstStringView ModC_TokenType_ToCStr(ModC_TokenType type)
{
    static_assert((int)ModC_TokenType_Count == 18, "");
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
        case ModC_TokenType_Undef:
            return ModC_ConstStringView_FromLiteral("ModC_TokenType_Undef");
        case ModC_TokenType_Count:
            return ModC_ConstStringView_FromLiteral("ModC_TokenType_Count");
        default:
            return (ModC_ConstStringView){0};
    }
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
        return MODC_RESULT_VALUE_S(0);
    }
    
    ModC_ConstStringView* tokenView = &this->TokenText.Value.View;
    if(forceString)
    {
        ModC_String tokenStr = ModC_String_Create(allocator, tokenView->Length + 1);
        ModC_String_AddRange(&tokenStr, tokenView->Data, tokenView->Length);
        this->TokenText = ModC_StringOrConstView_String(tokenStr);
        ModC_String_AddValue(&tokenStr, c);
        return MODC_RESULT_VALUE_S(0);
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

static inline ModC_CharTokenType ModC_CharTokenType_FromChar(char c)
{
    static_assert((int)ModC_TokenType_Count == 18, "");
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

#undef ModC_ResultName
#define ModC_ResultName ModC_Result_Bool

static inline ModC_Result_Bool ModC_Token_IsCharPossible(   ModC_Token* this, 
                                                            char c, 
                                                            ModC_CharTokenType cType)
{
    MODC_ASSERT(this != NULL, (""), MODC_RET_ERROR());
    
    static_assert((int)ModC_TokenType_Count == 18, "");
    ModC_ConstStringView tokenStringView = ModC_Token_TokenTextView(this);
    MODC_ASSERT(tokenStringView.Length > 0, (""), MODC_RET_ERROR());
    
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

#undef ModC_ResultName
#define ModC_ResultName ModC_Result_TokenList

static inline ModC_Result_TokenList ModC_Tokenization(  const ModC_ConstStringView fileContent, 
                                                        ModC_Allocator allocator)
{
    if(fileContent.Length == 0)
        return MODC_RESULT_VALUE_S( (ModC_TokenList){0} );
    
    ModC_TokenList tokenList = ModC_TokenList_Create(ModC_Allocator_Share(&allocator), 128);
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
        for(int i = 1; i < fileContent.Length; ++i)
        {
            ModC_CharTokenType charTokenType = ModC_CharTokenType_FromChar(fileContent.Data[i]);
            ModC_Result_Bool boolResult = ModC_Token_IsCharPossible(&currentToken, 
                                                                    fileContent.Data[i], 
                                                                    charTokenType);
            bool* possible = MODC_RESULT_TRY(boolResult, MODC_DEFER_BREAK(0, MODC_RET_ERROR()));
            if(!(*possible))
            {
                ModC_TokenList_AddValue(&tokenList, currentToken);
                
                currentToken = 
                    ModC_Token_FromView((ModC_TokenType)charTokenType, 
                                        ModC_ConstStringView_Create(&fileContent.Data[i], 1));
                currentToken.LineIndex = currentLineIndex;
                currentToken.ColumnIndex = currentColumnIndex;
                currentToken.SourceIndex = i;
            }
            else
            {
                ModC_Result_Void voidResult = ModC_Token_AppendChar(&currentToken, 
                                                                    fileContent, 
                                                                    fileContent.Data[i], 
                                                                    allocator, 
                                                                    false);
                (void)MODC_RESULT_TRY(voidResult, MODC_DEFER_BREAK(0, MODC_RET_ERROR()));
            }
            
            if(fileContent.Data[i] == '\n')
                ++currentLineIndex;
            ++currentColumnIndex;
        }
        
        ModC_TokenList_AddValue(&tokenList, currentToken);
        MODC_MOVE(ModC_TokenList, retList, tokenList);
        MODC_DEFER_BREAK(0, );
    }
    MODC_DEFER_SCOPE_END(0)
    
    return MODC_RESULT_VALUE_S(retList);
}

#endif
