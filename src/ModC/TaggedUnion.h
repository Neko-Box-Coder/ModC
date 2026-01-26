#include "MacroPowerToys/SplitList.h"
#include "MacroPowerToys/ConcatListsItems.h"
#include "MacroPowerToys/AppendListsItems.h"
#include "MacroPowerToys/CountTo.h"
#include "MacroPowerToys/Miscellaneous.h"
#include "MacroPowerToys/ArgsCount.h"

#include <stdint.h>

#ifndef MODC_TAGGED_UNION_NAME
    #error "MODC_TAGGED_UNION_NAME is not defined"
#endif

#ifndef MODC_VALUE_TYPES
    #error "MODC_VALUE_TYPES is not defined"
#endif

#define INTERNAL_MODC_FIELD_NAMES \
    MPT_CONCAT_LISTS_ITEMS( MODC_VALUE_TYPES, \
                            MPT_REPEAT_WITH_COMMA( MPT_ARGS_COUNT(MODC_VALUE_TYPES), Field ))

#define INTERNAL_MODC_FIELD_COUNT MPT_ARGS_COUNT(MODC_VALUE_TYPES)

/*
Expands to: 
```c
typedef enum
{
    <MODC_TAGGED_UNION_NAME>_<MODC_VALUE_TYPE_1>Index,
    <MODC_TAGGED_UNION_NAME>_<MODC_VALUE_TYPE_2>Index,
    <MODC_TAGGED_UNION_NAME>_<MODC_VALUE_TYPE_3>Index,
    ...
} <MODC_TAGGED_UNION_NAME>Index
```
*/
typedef enum
{
    MPT_CONCAT_LISTS_ITEMS
    (
        MPT_REPEAT_WITH_COMMA(INTERNAL_MODC_FIELD_COUNT, MODC_TAGGED_UNION_NAME),
        MPT_CONCAT_LISTS_ITEMS
        (
            MPT_CONCAT_LISTS_ITEMS( MPT_REPEAT_WITH_COMMA(INTERNAL_MODC_FIELD_COUNT, _), 
                                    MODC_VALUE_TYPES),
            MPT_REPEAT_WITH_COMMA(INTERNAL_MODC_FIELD_COUNT, Index)
        )
    )
} MPT_DELAYED_CONCAT( MODC_TAGGED_UNION_NAME, Index );


/*
Expands to: 
```c
typedef struct
{
    <MODC_TAGGED_UNION_NAME>Index Index;
    
    union
    {
        <MODC_VALUE_TYPE_1> <MODC_TAGGED_UNION_NAME>_<MODC_VALUE_TYPE_1>Field;
        <MODC_VALUE_TYPE_2> <MODC_TAGGED_UNION_NAME>_<MODC_VALUE_TYPE_2>Field;
        <MODC_VALUE_TYPE_3> <MODC_TAGGED_UNION_NAME>_<MODC_VALUE_TYPE_3>Field;
        ...
    } Data;
} <MODC_TAGGED_UNION_NAME>
```
*/
typedef struct
{
    MPT_DELAYED_CONCAT( MODC_TAGGED_UNION_NAME, Index ) Type;
    
    union
    {
        //Expands to: <MODC_VALUE_TYPE_1> <MODC_TAGGED_UNION_NAME>_<MODC_VALUE_TYPE_1>Field; ...
        MPT_SPLIT_LIST
        (
            ;, 
            
            //Expands to: <MODC_VALUE_TYPE_1> <MODC_TAGGED_UNION_NAME>_<MODC_VALUE_TYPE_1>Field, ...
            MPT_APPEND_LISTS_ITEMS
            (
                MODC_VALUE_TYPES,
                
                //Expands to: <MODC_TAGGED_UNION_NAME>_<MODC_VALUE_TYPE_1>Field, ...
                MPT_CONCAT_LISTS_ITEMS
                (
                    //Expands to: <MODC_TAGGED_UNION_NAME>, <MODC_TAGGED_UNION_NAME>, ...
                    MPT_REPEAT_WITH_COMMA(INTERNAL_MODC_FIELD_COUNT, MODC_TAGGED_UNION_NAME),
                    
                    //Expands to: _<MODC_VALUE_TYPE_1>Field, _<MODC_VALUE_TYPE_2>Field, ...
                    MPT_CONCAT_LISTS_ITEMS
                    (
                        MPT_CONCAT_LISTS_ITEMS( MPT_REPEAT_WITH_COMMA(INTERNAL_MODC_FIELD_COUNT, _),
                                                MODC_VALUE_TYPES),
                        MPT_REPEAT_WITH_COMMA(INTERNAL_MODC_FIELD_COUNT, Field)
                    )
                )
            )
        );
    } Data;
} MODC_TAGGED_UNION_NAME;


//Expands to: <taggedUnionName>_<typeName>Index
#define MODC_TAGGED_TYPE(taggedUnionName, typeName) \
    MPT_DELAYED_CONCAT3(MPT_DELAYED_CONCAT2(MPT_DELAYED_CONCAT(taggedUnionName, _), \
                                            typeName), \
                        Index)

//Expands to: <taggedUnionName>_<typeName>Field
#define MODC_TAGGED_FIELD(taggedUnionName, typeName) \
    MPT_DELAYED_CONCAT3(MPT_DELAYED_CONCAT2(MPT_DELAYED_CONCAT(taggedUnionName, _), \
                                            typeName), \
                        Field)

#define MODC_TAGGED_INIT_FIELD(taggedUnionName, typeName, value) \
        (taggedUnionName) \
        { \
            .Type = MODC_TAGGED_TYPE(taggedUnionName, typeName), \
            .Data.MODC_TAGGED_FIELD(taggedUnionName, typeName) = value \
        } \



#undef MODC_TAGGED_UNION_NAME
#undef MODC_VALUE_TYPES
#undef INTERNAL_MODC_FIELD_NAMES
#undef INTERNAL_MODC_FIELD_COUNT
