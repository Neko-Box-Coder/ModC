#if !defined(MODC_T)
    #error "MODC_T must be defined, in the format of <name>"
#endif

#if !defined(MODC_MODIFIERS)
    #error "MODC_MODIFIERS must be defined"
#endif

#if !defined(MODC_HAS_NULL_END_FIELD)
    #error "MODC_HAS_NULL_END_FIELD must be defined"
#endif

#include <stdint.h>
#include <stdbool.h>

#define INTERN_MODC_DEFINE_STRING_LIKE(modifers, )
typedef struct
{
    MODC_MODIFIERS char* Data;
    MODC_MODIFIERS int32_t Length; //Excluding end null terminator
    #if MODC_HAS_NULL_END_FIELD
        MODC_MODIFIERS bool NullEnd;
    #endif
} MODC_T;

#undef MODC_T
#undef MODC_MODIFIERS
#undef MODC_HAS_NULL_END_FIELD
