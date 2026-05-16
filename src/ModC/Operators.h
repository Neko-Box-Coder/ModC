#ifndef MODC_OPERATORS_H
#define MODC_OPERATORS_H

#include "ModC/Strings/Strings.h"

#include <stdbool.h>

//https://en.cppreference.com/w/c/language/operator_precedence.html
static inline bool ModC_IsValidComplexOperator(const ConstStringView tokenText)
{
    return  ConstStringView_IsEqualLiteral(&tokenText, "++") ||
            ConstStringView_IsEqualLiteral(&tokenText, "--") ||
            ConstStringView_IsEqualLiteral(&tokenText, "->") || //TODO: Should we have this?
            ConstStringView_IsEqualLiteral(&tokenText, "<<") ||
            ConstStringView_IsEqualLiteral(&tokenText, ">>") ||
            ConstStringView_IsEqualLiteral(&tokenText, "<=") ||
            ConstStringView_IsEqualLiteral(&tokenText, ">=") ||
            ConstStringView_IsEqualLiteral(&tokenText, "==") ||
            ConstStringView_IsEqualLiteral(&tokenText, "!=") ||
            ConstStringView_IsEqualLiteral(&tokenText, "&&") ||
            ConstStringView_IsEqualLiteral(&tokenText, "||") ||
            ConstStringView_IsEqualLiteral(&tokenText, "+=") ||
            ConstStringView_IsEqualLiteral(&tokenText, "-=") ||
            ConstStringView_IsEqualLiteral(&tokenText, "*=") ||
            ConstStringView_IsEqualLiteral(&tokenText, "/=") ||
            ConstStringView_IsEqualLiteral(&tokenText, "%=") ||
            ConstStringView_IsEqualLiteral(&tokenText, "<<=") ||
            ConstStringView_IsEqualLiteral(&tokenText, ">>=") ||
            ConstStringView_IsEqualLiteral(&tokenText, "&=") ||
            ConstStringView_IsEqualLiteral(&tokenText, "^=") ||
            ConstStringView_IsEqualLiteral(&tokenText, "|=");
}

#endif
