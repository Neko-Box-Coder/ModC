#if !defined(MODC_T)
    #error "MODC_T must be defined, in the format of <name>"
#endif

#if !defined(MODC_MODIFIERS)
    #error "MODC_MODIFIERS must be defined"
#endif

#if !defined(MODC_HAS_NULL_END_FIELD)
    #error "MODC_HAS_NULL_END_FIELD must be defined"
#endif

typedef struct
{
    MODC_MODIFIERS char* Data;
    MODC_MODIFIERS int32_t Length; //Excluding end null terminator
    #if MODC_HAS_NULL_END_FIELD
        MODC_MODIFIERS bool NullEnd;
    #endif
} MODC_T;



static inline ModC_ConstStringView ModC_StringView_CreateFromCStr(const char* cstr)
{
    return (ModC_StringView){ .Data = cstr, .Length = strlen(cstr), .NullEnd = true };
}

//Returns `ModC_StringView` from `src`, starting from `index` that spans `length` long.
//If `length` is negative, the returned view will reach the end of `src`
static inline ModC_StringView ModC_StringView_Subview(  const ModC_StringView* src, 
                                                        const int32_t index,
                                                        int32_t length)
{
    if(!src)
        return (ModC_StringView){0};
    if(length < 0)
        length = src->Length - index;
    if(index + length > src->Length || index + length < 0)
        return (ModC_StringView){0};
    return (ModC_StringView){ .Data = src->Data + index, .Length = length, .NullEnd = false };
}

//Returns `ModC_StringView` from `src`, starting from `index` that spans `length` long.
//If `length` is negative, the returned view will reach the end of `src`
static inline ModC_StringView ModC_String_Subview(  const ModC_String* src, 
                                                    const int32_t index,
                                                    int32_t length)
{
    if(!src)
        return (ModC_StringView){0};
    if(length < 0)
        length = src->Length - index;
    if(index + length > src->Length || index + length < 0)
        return (ModC_StringView){0};
    return (ModC_StringView){ .Data = src->Data + index, .Length = length, .NullEnd = false };
}
