#ifndef MODC_CONTAINERS_H
#define MODC_CONTAINERS_H

/* Docs
Header containing different container types.
All template definitions are defined here.
*/

#include "ModC/Strings/Strings.h"
#include "ModC/Result.h"

typedef struct
{
    bool IsString;
    union
    {
        ModC_String String;
        ModC_ConstStringView View;
    } Value;
} ModC_StringOrConstView;

MODC_DEFINE_RESULT_STRUCT(ModC_ResultInt32, int32_t)

typedef struct
{
    char Dummy;
} ModC_Void;

MODC_DEFINE_RESULT_STRUCT(ModC_ResultVoid, ModC_Void)

#endif
