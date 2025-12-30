#ifndef MODC_CONTAINERS_H
#define MODC_CONTAINERS_H

#include "ModC/Strings/Strings.h"

#include "ModC/Result.h"

MODC_DEFINE_RESULT_STRUCT(ModC_ResultInt32, int32_t)

typedef struct
{
    char Dummy;
} ModC_Void;

MODC_DEFINE_RESULT_STRUCT(ModC_ResultVoid, ModC_Void)

#endif
