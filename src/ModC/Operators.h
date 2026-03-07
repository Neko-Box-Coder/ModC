#ifndef MODC_OPERATORS_H
#define MODC_OPERATORS_H

#include "ModC/Strings/Strings.h"

#include <stdbool.h>

//https://en.cppreference.com/w/c/language/operator_precedence.html
static inline bool ModC_IsValidComplexOperator(const ModC_ConstStringView tokenText)
{
    return  ModC_ConstStringView_IsEqualLiteral(&tokenText, "++") ||
            ModC_ConstStringView_IsEqualLiteral(&tokenText, "--") ||
            ModC_ConstStringView_IsEqualLiteral(&tokenText, "->") || //TODO: Should we have this?
            ModC_ConstStringView_IsEqualLiteral(&tokenText, "<<") ||
            ModC_ConstStringView_IsEqualLiteral(&tokenText, ">>") ||
            ModC_ConstStringView_IsEqualLiteral(&tokenText, "<=") ||
            ModC_ConstStringView_IsEqualLiteral(&tokenText, ">=") ||
            ModC_ConstStringView_IsEqualLiteral(&tokenText, "==") ||
            ModC_ConstStringView_IsEqualLiteral(&tokenText, "!=") ||
            ModC_ConstStringView_IsEqualLiteral(&tokenText, "&&") ||
            ModC_ConstStringView_IsEqualLiteral(&tokenText, "||") ||
            ModC_ConstStringView_IsEqualLiteral(&tokenText, "+=") ||
            ModC_ConstStringView_IsEqualLiteral(&tokenText, "-=") ||
            ModC_ConstStringView_IsEqualLiteral(&tokenText, "*=") ||
            ModC_ConstStringView_IsEqualLiteral(&tokenText, "/=") ||
            ModC_ConstStringView_IsEqualLiteral(&tokenText, "%=") ||
            ModC_ConstStringView_IsEqualLiteral(&tokenText, "<<=") ||
            ModC_ConstStringView_IsEqualLiteral(&tokenText, ">>=") ||
            ModC_ConstStringView_IsEqualLiteral(&tokenText, "&=") ||
            ModC_ConstStringView_IsEqualLiteral(&tokenText, "^=") ||
            ModC_ConstStringView_IsEqualLiteral(&tokenText, "|=");
}

#endif
