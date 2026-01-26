#include "MacroPowerToys/SplitList.h"
#include "MacroPowerToys/ConcatListsItems.h"
#include "MacroPowerToys/AppendListsItems.h"
#include "MacroPowerToys/CountTo.h"
#include "MacroPowerToys/Miscellaneous.h"
#include "MacroPowerToys/ArgsCount.h"

#include <stdint.h>

/* Docs
Define `MODC_TAGGED_UNION_NAME` for the name of the tagged union.
Define `MODC_VALUE_TYPES` for types of the tagged union
Then include this file

#### Definitions
The following types will be defined

```c
typedef enum
{
    <MODC_TAGGED_UNION_NAME>_<MODC_VALUE_TYPE_1>Index,
    <MODC_TAGGED_UNION_NAME>_<MODC_VALUE_TYPE_2>Index,
    <MODC_TAGGED_UNION_NAME>_<MODC_VALUE_TYPE_3>Index,
    ...
} <MODC_TAGGED_UNION_NAME>Index

typedef struct
{
    <MODC_TAGGED_UNION_NAME>Index Type;
    
    union
    {
        <MODC_VALUE_TYPE_1> <MODC_TAGGED_UNION_NAME>_<MODC_VALUE_TYPE_1>Field;
        <MODC_VALUE_TYPE_2> <MODC_TAGGED_UNION_NAME>_<MODC_VALUE_TYPE_2>Field;
        <MODC_VALUE_TYPE_3> <MODC_TAGGED_UNION_NAME>_<MODC_VALUE_TYPE_3>Field;
        ...
    } Data;
} <MODC_TAGGED_UNION_NAME>
```

```c
#define MODC_TAGGED_UNION_NAME MyUnion
#define MODC_VALUE_TYPES int,char
#include "TaggedUnion.h"
```

#### Functions:

Define `ModC_TaggedUnionName` to use `_S` macro variants

Macro:
    Use this macro to match `.Type` from the union object.
```c
//MODC_TAGGED_TYPE(ModC_TaggedUnionName, typeName)
//MODC_TAGGED_TYPE_S(typeName)

if(myUnion.Type == MODC_TAGGED_TYPE(MyUnion, int))
{
    ...
}
```

Macro:
    Use this macro to get the field from `.Data` from the union object
```c
//MODC_TAGGED_FIELD(ModC_TaggedUnionName, typeName)
//MODC_TAGGED_FIELD_S(typeName)

int unionData = myUnion.Data.MODC_TAGGED_FIELD(MyUnion, int);
```

Macro:
    Use this macro to set the union value
```c
//MODC_TAGGED_INIT(ModC_TaggedUnionName, typeName, value)
//MODC_TAGGED_INIT_S(typeName, value) \

MyUnion myUnion = MODC_TAGGED_INIT(MyUnion, int, 5);
```

*/


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
    <MODC_TAGGED_UNION_NAME>Index Type;
    
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

#undef MODC_TAGGED_TYPE
//Expands to: <ModC_TaggedUnionName>_<typeName>Index
#define MODC_TAGGED_TYPE(ModC_TaggedUnionName, typeName) \
    MPT_DELAYED_CONCAT3(MPT_DELAYED_CONCAT2(MPT_DELAYED_CONCAT(ModC_TaggedUnionName, _), \
                                            typeName), \
                        Index)

#undef MODC_TAGGED_TYPE_S
#define MODC_TAGGED_TYPE_S(typeName) MODC_TAGGED_TYPE(ModC_TaggedUnionName, typeName)

#undef MODC_TAGGED_FIELD
//Expands to: <ModC_TaggedUnionName>_<typeName>Field
#define MODC_TAGGED_FIELD(ModC_TaggedUnionName, typeName) \
    MPT_DELAYED_CONCAT3(MPT_DELAYED_CONCAT2(MPT_DELAYED_CONCAT(ModC_TaggedUnionName, _), \
                                            typeName), \
                        Field)

#undef MODC_TAGGED_FIELD_S
#define MODC_TAGGED_FIELD_S(typeName) MODC_TAGGED_FIELD(ModC_TaggedUnionName, typeName) 

#undef MODC_TAGGED_INIT
#define MODC_TAGGED_INIT(ModC_TaggedUnionName, typeName, value) \
        (ModC_TaggedUnionName) \
        { \
            .Type = MODC_TAGGED_TYPE(ModC_TaggedUnionName, typeName), \
            .Data.MODC_TAGGED_FIELD(ModC_TaggedUnionName, typeName) = value \
        } \

#undef MODC_TAGGED_INIT_S
#define MODC_TAGGED_INIT_S(typeName, value) MODC_TAGGED_INIT(ModC_TaggedUnionName, typeName, value)


#undef MODC_TAGGED_UNION_NAME
#undef MODC_VALUE_TYPES
#undef INTERNAL_MODC_FIELD_NAMES
#undef INTERNAL_MODC_FIELD_COUNT
