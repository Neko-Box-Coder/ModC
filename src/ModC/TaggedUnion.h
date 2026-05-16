#include "MacroPowerToys/SplitList.h"
#include "MacroPowerToys/ConcatListsItems.h"
#include "MacroPowerToys/AppendListsItems.h"
#include "MacroPowerToys/CountTo.h"
#include "MacroPowerToys/Miscellaneous.h"
#include "MacroPowerToys/ArgsCount.h"

#include <stdint.h>

/* Docs
Define `TU_NAME` for the name of the tagged union.
Define `VALUE_TYPES` for types of the tagged union. This is comma separated.
Pointers need to be typedef to be passed to `VALUE_TYPES`.

Then include this file

#### Definitions
The following types will be defined

```c
typedef enum <TU_NAME>Index
{
    <TU_NAME>_<MODC_VALUE_TYPE_1>Index,
    <TU_NAME>_<MODC_VALUE_TYPE_2>Index,
    <TU_NAME>_<MODC_VALUE_TYPE_3>Index,
    ...
    <TU_NAME>_CountIndex,
} <TU_NAME>Index

typedef struct <TU_NAME>
{
    <TU_NAME>Index Type;
    
    union
    {
        <MODC_VALUE_TYPE_1> <TU_NAME>_<MODC_VALUE_TYPE_1>Field;
        <MODC_VALUE_TYPE_2> <TU_NAME>_<MODC_VALUE_TYPE_2>Field;
        <MODC_VALUE_TYPE_3> <TU_NAME>_<MODC_VALUE_TYPE_3>Field;
        ...
    } Data;
} <TU_NAME>
```

```c
#define TU_NAME MyUnion
#define VALUE_TYPES int,char
#include "TaggedUnion.h"
```

#### Functions:

Define `TaggedUnionNameState` to use `_S` macro variants

Macro:
    Use this macro to match `.Type` from the union object.
```c
//TU_TYPE(TaggedUnionName, typeName)
//TU_TYPE_S(typeName)

if(myUnion.Type == TU_TYPE(MyUnion, int))
{
    ...
}
```

Macro:
    Use this macro to get the data from the union object
```c
//TU_DATA(TaggedUnionName, typeName)
//TU_DATA_S(typeName)

int unionData = myUnion.TU_DATA(MyUnion, int);
```

Macro:
    Use this macro to set the union value
```c
//TU_INIT(TaggedUnionName, typeName, value)
//TU_INIT_S(typeName, value) \

MyUnion myUnion = TU_INIT(MyUnion, int, 5);
```

*/


#ifndef TU_NAME
    #error "TU_NAME is not defined"
#endif

#ifndef VALUE_TYPES
    #error "VALUE_TYPES is not defined"
#endif

#define INTERNAL_MODC_FIELD_NAMES \
    MPT_CONCAT_LISTS_ITEMS( VALUE_TYPES, \
                            MPT_REPEAT_WITH_COMMA( MPT_ARGS_COUNT(VALUE_TYPES), Field ))

#define INTERNAL_MODC_FIELD_COUNT MPT_ARGS_COUNT(VALUE_TYPES)

/*
Expands to: 
```c
typedef enum <TU_NAME>Index
{
    <TU_NAME>_<MODC_VALUE_TYPE_1>Index,
    <TU_NAME>_<MODC_VALUE_TYPE_2>Index,
    <TU_NAME>_<MODC_VALUE_TYPE_3>Index,
    ...
    <TU_NAME>_CountIndex,
} <TU_NAME>Index
```
*/
typedef enum MPT_DELAYED_CONCAT( TU_NAME, Index )
{
    MPT_CONCAT_LISTS_ITEMS
    (
        MPT_REPEAT_WITH_COMMA(INTERNAL_MODC_FIELD_COUNT, TU_NAME),
        MPT_CONCAT_LISTS_ITEMS
        (
            MPT_CONCAT_LISTS_ITEMS( MPT_REPEAT_WITH_COMMA(INTERNAL_MODC_FIELD_COUNT, _), 
                                    VALUE_TYPES),
            MPT_REPEAT_WITH_COMMA(INTERNAL_MODC_FIELD_COUNT, Index)
        )
    ),
    MPT_DELAYED_CONCAT(TU_NAME, _CountIndex)
} MPT_DELAYED_CONCAT( TU_NAME, Index );


/*
Expands to: 
```c
typedef struct <TU_NAME>
{
    <TU_NAME>Index Type;
    
    union
    {
        <MODC_VALUE_TYPE_1> <TU_NAME>_<MODC_VALUE_TYPE_1>Field;
        <MODC_VALUE_TYPE_2> <TU_NAME>_<MODC_VALUE_TYPE_2>Field;
        <MODC_VALUE_TYPE_3> <TU_NAME>_<MODC_VALUE_TYPE_3>Field;
        ...
    } Data;
} <TU_NAME>
```
*/
typedef struct TU_NAME
{
    MPT_DELAYED_CONCAT( TU_NAME, Index ) Type;
    
    union
    {
        //Expands to: <MODC_VALUE_TYPE_1> <TU_NAME>_<MODC_VALUE_TYPE_1>Field; ...
        MPT_SPLIT_LIST
        (
            ;, 
            
            //Expands to: <MODC_VALUE_TYPE_1> <TU_NAME>_<MODC_VALUE_TYPE_1>Field, ...
            MPT_APPEND_LISTS_ITEMS
            (
                VALUE_TYPES,
                
                //Expands to: <TU_NAME>_<MODC_VALUE_TYPE_1>Field, ...
                MPT_CONCAT_LISTS_ITEMS
                (
                    //Expands to: <TU_NAME>, <TU_NAME>, ...
                    MPT_REPEAT_WITH_COMMA(INTERNAL_MODC_FIELD_COUNT, TU_NAME),
                    
                    //Expands to: _<MODC_VALUE_TYPE_1>Field, _<MODC_VALUE_TYPE_2>Field, ...
                    MPT_CONCAT_LISTS_ITEMS
                    (
                        MPT_CONCAT_LISTS_ITEMS( MPT_REPEAT_WITH_COMMA(INTERNAL_MODC_FIELD_COUNT, _),
                                                VALUE_TYPES),
                        MPT_REPEAT_WITH_COMMA(INTERNAL_MODC_FIELD_COUNT, Field)
                    )
                )
            )
        );
    } Data;
} TU_NAME;

#undef TU_TYPE
//Expands to: <TaggedUnionName>_<typeName>Index
#define TU_TYPE(TaggedUnionName, typeName) \
    MPT_DELAYED_CONCAT3(MPT_DELAYED_CONCAT2(MPT_DELAYED_CONCAT(TaggedUnionName, _), \
                                            typeName), \
                        Index)

#undef TU_TYPE_S
#define TU_TYPE_S(typeName) TU_TYPE(TaggedUnionNameState, typeName)

#undef TU_DATA
//Expands to: <TaggedUnionName>_<typeName>Field
#define TU_DATA(TaggedUnionName, typeName) \
    Data.MPT_DELAYED_CONCAT3(   MPT_DELAYED_CONCAT2(MPT_DELAYED_CONCAT(TaggedUnionName, _), \
                                                    typeName), \
                                Field)

#undef TU_DATA_S
#define TU_DATA_S(typeName) TU_DATA(TaggedUnionNameState, typeName) 

#undef TU_INIT
#define TU_INIT(TaggedUnionName, typeName, ... /* value */) \
        (TaggedUnionName) \
        { \
            .Type = TU_TYPE(TaggedUnionName, typeName), \
            .TU_DATA(TaggedUnionName, typeName) = __VA_ARGS__ \
        } \

#undef TU_INIT_S
#define TU_INIT_S(typeName, ... /* value */) \
    TU_INIT(TaggedUnionNameState, typeName, __VA_ARGS__)


#undef TU_NAME
#undef VALUE_TYPES
#undef INTERNAL_MODC_FIELD_NAMES
#undef INTERNAL_MODC_FIELD_COUNT
