#ifndef MODC_GENERIC_CONTAINERS_H
#define MODC_GENERIC_CONTAINERS_H

/* Docs
Header containing different container types.
All template definitions are defined here.

Define `MODC_DEFAULT_ALLOC` and `MODC_DEFAULT_ALLOC_ARGS` to use non `_ALLOC` macro variants
*/

#include <stdbool.h>

#include "ModC/Result.h"
MODC_DEFINE_RESULT_STRUCT(ModC_Result_Int32, int32_t)

MODC_DEFINE_RESULT_STRUCT(ModC_Result_Bool, bool)

typedef char ModC_Void;
MODC_DEFINE_RESULT_STRUCT(ModC_Result_Void, ModC_Void)


//#define MODC_LIST_NAME ModC_TestString
//#define MODC_VALUE_TYPE char
//#include "ModC/List.h"

//#define MODC_TAGGED_UNION_NAME TEST_UNION
//#define MODC_VALUE_TYPES ModC_String, ModC_StringView
//#include "ModC/TaggedUnion.h"

#endif
