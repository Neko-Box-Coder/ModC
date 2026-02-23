#ifndef MODC_KEYWORD_H
#define MODC_KEYWORD_H

#include "ModC/Strings/Strings.h"

#include <stdbool.h>

static inline bool ModC_IsInvokableKeyword(const ModC_ConstStringView tokenText)
{
    return  ModC_ConstStringView_IsEqualLiteral(&tokenText, "if") ||
            ModC_ConstStringView_IsEqualLiteral(&tokenText, "while") ||
            ModC_ConstStringView_IsEqualLiteral(&tokenText, "for") ||
            ModC_ConstStringView_IsEqualLiteral(&tokenText, "switch");
}

#endif
