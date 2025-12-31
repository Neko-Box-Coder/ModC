#ifndef MODC_CONTAINERS_H
#define MODC_CONTAINERS_H

/* Docs
Header containing different container types.
All template definitions are defined here.

Define `MODC_DEFAULT_ALLOC` and `MODC_DEFAULT_ALLOC_ARGS` to use non `_ALLOC` macro variants
*/

#include "ModC/Strings/Strings.h"
#include "ModC/Result.h"

MODC_DEFINE_RESULT_STRUCT(ModC_ResultInt32, int32_t)

typedef struct
{
    char Dummy;
} ModC_Void;

MODC_DEFINE_RESULT_STRUCT(ModC_ResultVoid, ModC_Void)

#endif
