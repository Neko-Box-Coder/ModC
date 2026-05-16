#ifndef MODC_KEYWORD_H
#define MODC_KEYWORD_H

#include "ModC/Strings/Strings.h"

#include <stdbool.h>

static inline bool ModC_IsInvokableKeyword(const ConstStringView tokenText)
{
    return  ConstStringView_IsEqualLiteral(&tokenText, "if") ||
            ConstStringView_IsEqualLiteral(&tokenText, "while") ||
            ConstStringView_IsEqualLiteral(&tokenText, "for") ||
            ConstStringView_IsEqualLiteral(&tokenText, "switch");
}

#endif
