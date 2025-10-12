
# ModC Specification

When navigating this document, you might encounter the following syntax:

- `<xxxx>`: Mandatory argument/field
- `[xxxx]`: Optional argument/field
- `...`: An action or content that doesn't matter in the current context

## Core Language

### Basic Structure And Types

#### Header And Source Files
ModC uses extension of `.modh` and `.modc` respectively for header and source files.
Header files are optional if source file is available. 

To include other source file, simply do
```c
#include "path/to/file.modc"    //Including a modc file
#include "path/to/file2.modh"   //Including a modh file
#include <library.modc>         //Including a standard library
```

While the syntax is the same, it works differently from C where it does not just copy and paste the
file content to the current file.

Rather, it will parse the included file first before continuing parsing the rest of this file.

`.modh` files can be generated from `.modc` files or can be manually created as long as the 
declarations in `.modh` match the ones in `.modc`. 

`.modh` files are used when trying to use a compiled library where `.modc` files are not present


#### Primitive Types And Conversions
```modc
bool
char //signed

int
int8
int16
int32
int64

uint
uint8
uint16
uint32
uint64

float
double
```

Similar to C, primitive types will be implicitly converted (when possible) to the highest common type 
in and expression, such as aritmetic operations before being converted back to the assignment type if 
in an assignment statement. 

The rank is determined by the size of the type. In the case where both types have the same size but
one is signed and the other is unsigned, both will be converted to the higher signed type. If this 
is not possible, the code won't be compiled and an error will be shown to ask the user to cast to 
a comparable types.

The result of conversion depends on the C99+ compiler you use, for now.

```modc
{
    int8 a = 5;
    int16 b = 5;
    
    int c = a / b;      //`a` will be casted to `int16` before division
    b = a;              //`a` will be casted to `int16` before assignment
    
    uint16 d = 5;
    int8 e = b / d;     //`b` and `d` will be casted to `int32` before division, then cast to `int8` before assignment
}
```

<!--
```modc
any             //Only available in template or prepile context
typed_union
union           //Only available in direct context

struct string
{
    *char data;
    uint64 size;
    bool null_terminated;
};

struct array<any T>
{
    *T data;
    uint64_t size;
};
```
-->

#### Type Modifiers
```modc
//Location modifiers
extern                      //External linkage
intern                      //Internal
static                      //Static storage

ref                         //Reference, e.g. `ref bool`. Reference is always valid
*                           //Pointer, e.g. `*bool`.
const                       //Constant.

//Parameter modifiers
in                          //Read only reference, e.g. `MyFunction(in bool)`. Function must only read.
out                         //Write only reference, e.g. `MyFunction(out bool)`. Function must only write.
```

<!--
```modc
defer                       //A method associated with the type that is promised to be run at some
```
-->

Unlike in C, Types are always read from left to right
```modc
*bool myBool;                   //Pointer to boolean
const *bool myBoolPtr = ...;    //Constant pointer to boolean
*const bool myBoolPtr = ...;    //Pointer to constant boolean
```

#### Type Aliasing
Just like in C, you can do `typedef` in ModC. The only difference is `typedef` by default is explicit.
To perform implicit `typedef`, you need to use the `implicit` keyword

```modc
typedef <existing type> <alias name>;           //Explicit typedef, conversion needs explicit casting
implicit typedef <existing type> <alias name>;  //Implicit typedef

//For example:
typedef int ID;

{
    int someNum = 5;
    //ID myId = someNum;    //Error: ID is explicitly typedef as int, conversion needs explicit casting
    ID myId = (ID)someNum;  //Okay
}
```

#### Function Declaration
Functions are declared just like in C, with 3 exceptions

1. Empty arguments `()` is the same as void argument `(void)`
2. Variable arguments is not possible (but can be done in another way using prepile context or 
    dynamic array, or in direct context using `...`)
3. If the function only has implementation (Say only in `.modc` file), a declaration will be 
    generated automatically.

```modc
void MyFunction();          //Same as `void MyFunction(void)` in C.
void MyFunction2(int a);

[direct]
void MyC_LikeVariableArgsFunction(...);
```

#### Namespace
Like in other languages, you can do namespace in ModC to group functions or types together. 
It is literally just a syntax sugar of prefixing the namespace identifier to the symbols. 
Nothing more, nothing less.

To specify a namespaced symbol, use the `.` operator.

```modc
namespace <identifier>
{
    ...
}

namespace <identifier1>.<identifier2>.<identifer3>  //Or nested namespace
{
    ...
}

//For example:
namespace NamespaceA.NamespaceB
{
    void MyFunction(int a);
}

//`MyFunction()` is called like this:
NamespaceA.NamespaceB.MyFunction(12);
```

Just like in C++, namespace identifier can be omitted if the type is under the same namespace
```modc
namespace NamespaceA.NamespaceB
{
    void MyFunction(int a) { ... }
    
    void MyFunction2()
    {
        MyFunction(5);                          //Okay
        NamespaceA.NamespaceB.MyFunction(6);    //Also okay, but not needed
    }
}

```

!!! NOTE
    It is recommended to not nest namespace in general, since it is quite verbose.

#### Reference
In ModC, reference (`ref`) allows the user to use a variable without copying it. It is essentially
a temporary reference that points to another variable, similar to a pointer in C.

Reference acts almost like C++ reference, it must be referencing a variable at the declaration time.

Unlike C++, reference type can be changed to reference a different variable. Reference cannot be 
returned by a function or stored in a data structure as well. And finally reference cannot reference
a variable in a child scope.

```modc
struct MyStruct
{
    //ref int MyRef;                    //Error: Reference cannot be stored in a data structure
}

{
    //ref int myRef;                    //Error: Reference must be initialized
    
    int myInt = 5;
    int myInt2 = 6;
    ref int myRef2 = ref myInt;         //Okay
    myRef2 = 8;                         //Okay, `myInt` is now 8
    
    {
        ref int myRef3 = ref myInt;     //Okay
        ref int myRef4 = ref myRef2;    //Okay
        
        myRef2 = ref myInt2;            //Okay
        
        int myInt3 = 7;
        //myRef2 = ref myInt3;          //Error: Reference variable cannot reference a child scope variable
        myRef3 = ref myInt3;            //Okay
    }
}
```

When a reference type is assigned to a non reference type, a copy is made instead.
```modc
{
    int myInt = 5;
    ref int myRef = ref myInt;
    
    int myInt2 = myRef;
    myRef = 1;
    
    Std.Print(myInt2);              //Output: 5
    Std.Print(myRef);               //Output: 1
}
```

When a function parameter is a reference, it can be one of these reference types:

- `ref`: Function can read and write to reference variable
- `in`: Function can only read from reference variable. The equivalent is `const ref`.
- `out`: Function can only write to reference variable

```modc
void MyFunc(ref int a, in int b, out int c)
{
    a = 5;      //Okay
    //b = 5;    //Error: in reference type is not writable
    c = b;      //Okay
}

{
    int a = 0;
    int b = 1;
    int c = 2;
    
    MyFunc(ref a, in b, out c);
}
```

#### Struct and Enums
Structs and enums in ModC behaves exactly like in C, except: 

1. The user can just reference the struct type identifier without specifying the `struct` keyword.
2. Enum items must be specified via enum type identifer, unless the type can be inferred.
3. Declaration and usage can be "scoped" with namespaces.
4. Semicolon `;` is not required

```modc
struct MyStruct { ... }
enum MyEnum { ZERO }

namespace MyNamespace
{
    struct MyStruct { ... };
    enum MyEnum { ZERO };
}

{
    MyStruct myStruct;
    MyEnum myEnum;
    MyNamespace.MyStruct myStruct2;
    MyNamespace.MyEnum myEnum2 = ZERO;      //`ZERO` is inferred as `MyNamespace.MyEnum.ZERO`
    int enumValue = (int)MyEnum.ZERO + (int)MyNamespace.MyEnum.ZERO;
}
```

Structs can be initialized with `{}` just like in C
```modc
struct MyStruct
{
    int Member;
    char Member2;
}

{
    MyStruct myStruct = {5, 'a'};                           //Without member names
    MyStruct myStruct2 = { .Member = 5, .Member2 = 'a' };   //With member names
    MyStruct myStruct3 = {0};                               //Zero initialize
}
```

Just like in C, if not all the members are initialized, then the rest of the will be zero initialized.
```modc
struct MyStruct
{
    int Member;
    char Member2;
}

{
    MyStruct myStruct = {5};                    //`member` is 5, `member2` is `'\0'`
    MyStruct myStruct2 = { .member2 = 'a' };    //`member` is 0, `member2` is `'a'`
}
```

Unlike in C, you can specify the underlying type for an enum type.
```modc
enum MyEnum : int8
{
    ZERO
}
```

#### Typed Union

Unlike in C, typed union is built into ModC. You can assign value freely but must check the type 
before reading.

```modc
typed_union IntOrChar
{
    int;
    char;
}

{
    IntOrChar intOrChar = ...;
    //int valInt = intOrChar.Get<int>();        //Error: Typed union must be checked before reading
    
    if(intOrChar.Is<int>())
    {
        int valInt = intOrChar.Get<int>();      //Okay
    }
    
    intOrChar = 'b';
    char valChar = intOrChar.Get<char>();       //Okay, it must be char
    
    bool someCondition = ...;
    if(someCondition)
        intOrChar = 5;
    
    //valChar = intOrChar.Get<char>();          //Error: Typed union must be checked before reading
}
```

??? TODO
    Add ability to perform switch

If the member type of a typed union is ambiguous, casting is required when assigning.

```modc
typed_union IntOrUint
{
    int;
    uint32;     //Ambiguous type
}

{
    //IntOrUint intOrUint = 5;          //Error: Assigning ambiguous typed union member type must be casted.
    IntOrUint intOrUint = (uint32)5;    //Okay
}
```

#### Array

Like in C, arrays are not resizable and the size must be known upfront. Values are initialize the 
same as struct.

```modc
{
    array<int, 5> myIntArray = {0, 1, 2, 3, 4};         //Manually setting items
    array<int> myIntArray2 = {0, 1, 2, 3, 4};           //Inferring the array size but number of elements.
                                                        //It is still `array<int, 5>`.
    array<int, 5> myIntArray3 = {5};                    //First value is 5, rest are zero initialized
    
    int myInt = myIntArray.get(3);                      //3
    ref int myIntRef = myIntArray[0];                   //0, same as `myIntArray.get(0);`
    int len = myIntArray.length();                      //5
}
```

Unlike in C, arrays behave like a struct where it is passed by value. Meaning you can return an 
array from a function.

```modc
array<int, 5> GetMyArray()
{
    return {0, 1, 2};       //Returning `{0, 1, 2, 0, 0}`;
}

{
    array<int, 5> myArray = GetMyArray();   //Perfectly fine in ModC
    int myInt = myArray[3];                 //0
}
```

Array with different size is its own type, meaning `array<int, 3>` is different from `array<int, 2>`. 
However a larger size array can be converted to a lower size one implicitly (slicing).

```modc
{
    array<int, 5> myArray = {1, 2, 3, 4, 5};
    ref array<int, 3> myArray2 = ref myArray2;  //`{1, 2, 3}`
}
```

#### String

In ModC, string behaves just like a byte array, except it has an extra element which is a null 
terminator for C compatibility.

```modc
{
    string<5> myString = "abc";     //`{'a', 'b', 'c', \0, \0}`
    string<> myString2 = "Hello";   //Same as `string<6>`
}
```

!!! NOTE
    Other string literal is not supported yet.




### Statements And Expressions

Just like in C, most statements are ending in `;` and the order of them matters, for all statements.

Depending on the statement type, a statement can contain multiple of different expressions.

However, unlike in C, pretty much everything is a statement.

There are different types of statement, some of them can have a block variant.

- Declaration Statement
- Action Statement
- Logic Statement

You don't have to know what they are in order to use the language, since the final composition is
extremely similar to C. But you can expand the admonition below for more details.

??? info
    **Declaration Statement**
    
    A declaration statement is either any statement that declares a type, or begins with `#`.
    Depending on the content of declaration, a block declaration statement can be followed after.
    
    Block declaration statements can contain a subset of other statement types, depending on the 
    declaration statement.
    ```
    <Declaration Statement>
    [Block Declaration Statement];
    ```
    
    For example
    ```modc
    struct MyStruct             //< Declaration Statement
    {                           //< Block Declaration Statement Begins
        ...
    }                           //< Block Declaration Statement Ends
    
    #include "MyLibrary.modc"   //< Declaration Statement
    
    void MyFunc()               //< Declaration Statement
    {                           //< Block Declaration Statement Begins
        ...
        {                       //< Block Declaration Statement Begins
            int a;              //< Declaration Statement
            a = 5;
        }                       //< Block Declaration Statement Ends

        #if 0                   //< Declaration Statement
        ...
        #endif                  //< Declaration Statement
        ...
    }                           //< Block Declaration Statement Ends
    ```
    
    ---
    
    **Action Statement**
    
    An action statement is as the name suggests, any statement that performs an action, such as
    assignment or calling a function. The action can be prepended by a subset of declaration
    expression.
    
    There's no block action statement variant.
    
    ```
    <Action Statement>;
    ```
    
    For example
    ```modc
    int SomeFunc();
    
    {
        int a = 5;              //< Action Statement
        int b = SomeFunc();     //< Action Statement
        int c;

        SomeFunc();             //< Action Statement
        a = SomeFunc();         //< Action Statement
    }
    ```
    
    ---
    
    **Logic Statement**
    
    A logic statement is a statement that inserts some logic to the program, generally branching and
    looping but can also be something else.
    
    Both logic statement and block logic statement can contain a subset of other statement types, 
    depending on the logic statement.
    
    ```
    <Logic Statement>
    <Block Logic Statement>
    ```
    
    For example
    ```modc
    void SomeAction();
    
    {
        if(true)                    //< Logic Statement
        {                           //< Block Logic Statement Begins
            SomeAction();
            ...
        }                           //< Block Logic Statement Ends
        else                        //< Logic Statement
        {                           //< Block Logic Statement Begins
            ...
        }                           //< Block Logic Statement Ends
        
        while(true)                 //< Logic Statement
        {                           //< Block Logic Statement Begins
            ...
        }                           //< Block Logic Statement Ends
        
        for(int i = 0; i < 5; ++i)  //< Logic Statement, Action Statement x3
        {                           //< Block Logic Statement Begins
            ...
        }                           //< Block Logic Statement Ends
    }
    ```
    
    Similar to C, `{}` can be omitted for block logic statement if it only contains 1 statement.

    ```modc
    {
        if(true)            //< Logic Statement
            SomeAction();
        //  ^
        //  Block Logic Statement Begins
        //  Action Statement
        //  Block Logic Statement Ends
    }
    ```
    
    Unlike C, since it only allows 1 statement when omitting `{}`, this means you cannot do
    
    ```modc
    {
        //if(true)
        //    if(false)             //Error: Multiple statements are not allowed when omitting curly brackets
        //        SomeAction();
        //    else
        //        SomeAction2();
    }
    ```
    
    This avoid confusion like in C such as
    ```c
    {
        if(true)
            if(false)
                SomeAction();
        else
            SomeAction2();
    }
    ```
    
    which gets interpolated by the compiler as
    ```c
    {
        if(true)
            if(false)
                SomeAction();
            else
                SomeAction2();
    }
    ```

#### If, Else and Ternery

Similar to other programming languages, condition statements can be achieved with `if`, `else`, and 
`else if`.

The statements must be ordered as follows

```
if(<expression>)
{
    [Statements when expression is true]
}
else if(<expression>)
{
    [Statements when expression is true]
}
else
{
    [Statements when last expression is false]
}
```

`if` must be the first statement, followed by `else if` optionally as many times as possible and 
finally followed by `else` optionally.

```modc
{
    bool conditionA = ...;
    bool conditionB = ...;
    if(conditionA)
    {
        ...
    }
    else if(conditionB)
    {
        ...
    }
    else
    {
        ...
    }
}
```

If you only have one statement in the `if` block, then you can omit the curly braces.

```modc
void SomeAction();

{
    bool conditionA = ...;
    if(conditionA)
        SomeAction();
}
```

Unlike C however, you cannot omit curly braces if you need to use statements such as `while`, `if`, 
`for`, etc. This is because those count as one statement and a block also count as one statement,
therefore they will be counted as multiple statements in ModC.

This means you cannot do

```modc
void SomeAction();
void SomeAction2();

{
    bool conditionA = ...;
    bool conditionB = ...;
    //if(conditionA)
    //    if(conditionB)        //Error: Multiple statements are not allowed when omitting curly brackets
    //        SomeAction();
    
    //if(conditionA)
    //    while(conditionB)     //Error: Multiple statements are not allowed when omitting curly brackets
    //    {
    //        ...
    //    }
    
    //Okay
    if(conditionA)
        SomeAction();
    else
        SomeAction2();
}
```

??? info "Rationale"
    This avoid confusion like in C such as
    ```c
    {
        if(true)
            if(false)
                SomeAction();
        else
            SomeAction2();
    }
    ```

    which gets interpolated by the compiler as
    ```c
    {
        if(true)
            if(false)
                SomeAction();
            else
                SomeAction2();
    }
    ```

Also unlike in C, assignment and non boolean expressions are not allowed.

```modc
{
    bool a = false;
    //if(a = true)      //Error: Non boolean expression is not allowed in if statement
    //{
    //    ...
    //}
    
    int b = 5;
    //if(b)             //Error: Non boolean expression is not allowed in if statement
    //{
    //    ...
    //}
}
```

Similarly, multiple statements are not allowed as well.
```modc
{
    bool a = false;
    //if(a = true; a)   //Error: Multiple statements is not allowed in if statement
    //{
    //    ...
    //}
}
```

??? info "Rationale"
    This avoid typos when performing comparison like this
    ```c
    {
        int a = 5;
        if(a = 6)   //Typo: supposed to be if(a == 6) instead
        {
            //Oh no, this is being executed
        }
    }
    ```
    
    While multiple statement is nice when creating variable that only scoped to the if statement,
    it can create weird complex scenario that might be weird to read.
    
    ```c++
    int SomeAction();
    
    {
        //Pretty nice, no need to worry about accessing `a` outside
        if(int a = SomeAction(); a == 5)
        {
            ...
        }
        
        ...
        
        //But it makes something like this totally valid
        bool someCondition = ...;
        if(int a = someCondition ? 1 : SomeAction(); a == 1 ? a = 5 : a = 0)
        {
            ...
        }
        
        //ModC forces the user to limit it to:
        {
            int a = someCondition ? 1 : SomeAction();
            a = a == 1 ? 5 : 0;
            if(a == 5)
            {
                ...
            }
        }
    }
    ```

Just like in C, ternery expression is also possible, with the same restriction as `if` for the 
condition part and the evaluation part must evaluate or promotable to the same type.

```modc
{
    bool someCondition = ...;
    int a = someCondition ? 1 : 2;
}
```

#### For, While and Do While

`for`, `while` and `do ... while` works just like in C.

```
for(<statement 1>; <statement 2>; <statement 3>)
{
    [Statements]
}
```

`for` accepts 3 statement and followed by a block statement which can contain other statements.

1. The first one runs before iterating
2. The second one runs before each iteration and stops iterating if evaluated to `false`
3. The third one runs after each iteration

Only the first and third statements can declare and create any variables. These variables can only 
be accessed with the `for` statements themselves as well as the block statement.

Just like with `if`, the `{}` can be omitted if the block only contains one statement.

`break` can be used to exit the iteration early.

`continue` can be used to skip the rest of the statements inside the block statement.

`while` works exactly like `for` that has empty first and third statements. Curly braces (`{}`) 
can be omitted just like `for` and `if` if there's only one statement inside the block.

```
while(<statement 1>)
{
    [Statements]
}
```

`do ... while` works by running the block statement after `do` first before evaluating the 
statement inside `while`. Curly braces (`{}`) after `do` can be omitted just like `for` and 
`if` if there's only one statement. Semicolon `;` is required after `while`. 

```
do
{
    [Statements]
}
while(<statement>);
```

#### Switch

A `switch` statement contains an evaluation statement and a block statement that contains 
mulltiple `case` statements, and optionally a `default` statement.

The statement inside `switch` must evaluate to a type that is representable by an integer.

Each `case` statement must contain a value that can be evaluated at compile time and matches the 
evaluated value inside the `switch` statement.

The switch block is skipped if no matched `case` and no `default` is found.

Unlike C, each `case` statement and `default` must be followed by a block statement.

Just like `for`, `while` and `do ... while`, if the block statement for `case` or `default` only 
contains one statement, curly braces can be omitted.

```
switch(<statement>)
{
    case(<statement 1>)
    {
        [Statements]
    }
    case <statement 1>:
    {
        [Statements]
    }
    default:
    {
        [Statements]
    }
}
```

Again unlike C, `break` is not needed and the code will exit the switch block once it reached the
end of the block statement for one of `case` or `default`. `continue` is needed if you want to 
continue to the next `case` or `default`.

```modc
{
    int mySwitch = 3;
    switch(mySwitch)
    {
        case 0:
            Std.Println("0");
        case 1:
            Std.Println("1");
        case 2:
        {
            mySwitch = 5;
            Std.Println("2");
        }
        case 3:
            Std.Println("3");
        default:
            Std.Println("default");
    }
    
    //Outputs: 3
    
    switch(mySwitch)
    {
        case 3:
        {
            Std.Println("3");
            continue;
        }
        case 4:
        {
            Std.Println("4");
            continue;
        }
        default:
        {
            Std.Print("default");
            continue;
        }
    }
    
    //Outputs: 3
    //         4
    //         default
}
```

#### Breaking nested loops

In C or C++, you will need to either use a flag or `goto` to break out of any nested loops quickly.

```cpp
{
    //Using a flag
    for(int i = 0; i < 10; ++i)
    {
        bool breakOut = false;
        for(int j = 0; j < 10; ++j)
        {
            Std.Println("i: ", i, "j: ", j);
            if(i == 5 && j == 5)
            {
                breakOut = true;
                break;
            }
        }
        
        if(breakOut)
            break;
    }
    
    //Using goto
    for(int i = 0; i < 10; ++i)
    {
        for(int j = 0; j < 10; ++j)
        {
            Std.Println("i: ", i, "j: ", j);
            if(i == 5 && j == 5)
                goto breakout;
        }
    }
    breakOut:;
}
```

In ModC, you can just repeat `break` the number of times you need to break out of nested loops

```modc
{
    for(int i = 0; i < 10; ++i)
    {
        for(int j = 0; j < 10; ++j)
        {
            Std.Println("i: ", i, "j: ", j);
            if(i == 5 && j == 5)
                break break;
        }
    }
}
```

#### Order Of Expressions

ModC follows the order of operator precedence specified by C99 standard, with slightly more strict
expression regarding some expressions. Each expression can happen multiple times in a statement.

Precedence order:

1. Associated left to right
    - `<expr>++` / `<expr>--`: suffix increment/decrement
    - `<function>()`: function call
    - `<expr>[]`: pointer/array subscript (direct context)
    - `<expr>.<expr>`: namespace/member access
    - `<expr> = { ... }`: Non primitive type initialization
2. Associated right to left
    - `++<expr>` / `--<expr>`: prefix increment/decrement
    - `+<expr>` / `-<expr>`: positive/negative
    - `!<expr>` / `~<expr>`: logical NOT / bitwise NOT
    - `(<type>)<expr>`: type casting
    - `*<expr>`: pointer dereference
    - `&<expr>`: address of
    - `ref <expr>`: reference of
3. Associated left to right
    - `<expr> * <expr>` / `<expr> / <expr>` / `<expr> % <expr>`: multiplication/division/remainder
4. Associated left to right
    - `<expr> + <expr>` / `<expr> - <expr>`: addition/subtraction
5. Associated left to right
    - `<expr> << <expr>` / `<expr> >> <expr>`: bitshift left/right
6. Associated left to right
    - `<expr> < <expr>` / `<expr> <= <expr>`: less than / less than or equal to
    - `<expr> > <expr>` / `<expr> >= <expr>`: larger than / larger than or equal to
7. Associated left to right
    - `<expr> == <expr>` / `<expr> != <expr>`: equal to / not equal to
8. Associated left to right
    - `<expr> & <expr>`: bitwise AND
    - `<expr> ^ <expr>`: bitwise XOR
    - `<expr> | <expr>`: bitwise OR
    - `<expr> && <expr>`: logical AND
    - `<expr> || <expr>`: logical OR
9. Associated right to left
    - `<expr> ? <expr> : <expr>`: ternary condition

There are a few things here that is different from C.

1. When multiple unique expressions are found that are ranked 8, parenthesis must be added.
```modc
{
    //int a = 3 | 7 & 15;       //Error: Parenthesis `()` must be added to denote desired precedence.
    int a = (3 | 7) & 15;       //Okay
}
```
2. Assignment operations are considered an action that can only happen once in an action statement. 
Here is a list of assignment operations: `=`, `+=`, `-=`, `*=`, `/=`, `%=`, `<<=`, `>>=`, `&=`, `^=`, 
`|=`
```modc
{
    int a;
    int b;
    
    //a = b = 5;            //Error: Multiple assignments are not allowed
    //if(a = 5) { ... }     //Error: Non boolean expression is not allowed in if statement
    
    a = 5;                  //Okay
    b = 5;                  //Okay
}
```
3. Just like C, the order of expressions is not the same as the order of evaluations. There's no 
guarantee on the order of evaluation in a statement with one exception. <br> If there are multiple 
suffix/prefix increment/decrement (`<expr>++`, `++<expr>`, etc.) or function call (`<function>()`) 
found in a statement, those will be evaluated first according to the order of expressions and 
association.
```modc
int alterAndReturnVar(out var) { return var++; }

{
    int a = 5;
    int b = ++a + a++;  //This would be undefined in C. But this is valid in ModC.
                        //`++a` = 6, `a` = 6
                        //`a++` = 6, `a` = 7
                        //`b` = 6 + 6 = 12, `a` = 7
    
    //`alterAndReturnVar(a)` = 7, `a` = 8 (incremented inside the function)
    //`alterAndReturnVar(a)` = 8, `a` = 9 (incremented inside the function)
    //`a` = 9
    //`c` = 7 + 8 + 9 = 24
    int c = alterAndReturnVar(a) + alterAndReturnVar(a) + a;
}
```

### Function Bindings And Type Prefix Function

If the first parameter is a reference to a variable, the function is binded to it and can be called 
with `<variable>.<function>(...)` which will be passed as the first parameter as reference.

```modc
struct Node { ... }

void UpdateNode(ref Node this);

{
    Node node;
    UpdateNode(node);   //Okay
    node.UpdateNode();  //Okay
}
```

A function can be declaring inside the `struct` or declared with `<type>.<function name>`. 

This will prefix the type name to the function automatically, and can be called by with 
`<type>.<function>(...)` using `.`, just like `namespace`.

This is literally just a syntax sugar of prefixing the type to the symbols. 
Nothing more, nothing less.

This can be used together with function binding mentioned previously, in which case the type prefix
for the function can be omitted and inferred with the type of the variable being called.

```modc
struct Node 
{ 
    int ID;
    int Val;
    
    void SetId(ref Node this, int id) { this.ID = id; }         //A type prefixed function can be declared like this
}

void Node.SetVal(ref Node this, int val) { this.Val = val; }    //Or this

{
    Node myNode = {};
    myNode.SetId(123);              //Okay
    Node.SetVal(myNode, 345);       //Okay
    myNode.SetVal(678);             //Okay
}
```

Function bindings and type prefix function are independent syntax sugar and therefore do not have to
be used together. However, type prefix cannot be inferred and omitted if the prefix type and the 
binded type are different.

```modc
struct Node { ... }
struct OtherStruct { ... }

void OtherStruct.ModifyNode(ref Node node) { ... }

{
    Node myNode = {};
    OtherStruct.ModifyNode(myNode);         //Okay
    
    //Cannot infer the function prefix type, since `myNode` is not `OtherStruct`.
    //myNode.ModifyNode();                  //Error: Node.ModifyNode is not found
    
    //myNode.OtherStruct.ModifyNode();      //Error: No member OtherStruct found for type Node
}
```

### Pointer Type

In ModC, you can obtain a pointer in two ways, either taking an address from existing object or 
getting it externally (Library, libc, runtime, etc...). There's no concept of heap memory in ModC.

Pointer (`*`) works just like in C, but there are a few differences unless you are in direct context.

1. Instead of `NULL`, ModC uses `null` which is a language keyword that can only be used for pointer
types.
2. Creating a pointer with the address of existing object requires the `lease` type modifier, which 
allows ModC to prevent various misuse of the pointer. We will talk about this later in this spec.
3. Pointer arithmetic is not allowed. Use `array` or the standard library containers instead.
4. Struct pointer member can be accessed with `.` operator.

```modc
struct Node 
{
    int ID;
    int Val;
}

{
    *Node nodePtr = ...
    nodePtr.ID = 5;         //Okay
    (*nodePtr).ID = 5;      //Okay but not needed
}


```

Unlike in C, ModC reads from left to right and pointer is part of the variable type. 

```modc
{
    int myInt;                  //An integer
    *int myIntPtr;              //A pointer to an interger
    const *int myIntPtr2;       //A constant pointer to an integer (the pointer can't be changed)
    *const int myIntPtr3;       //A pointer to a constant integer (the integer can't be changed)
}
```

#### Function Pointer
In C, function pointers are declared/read from inside out.
```c
//Here's what a function pointer that returns void and takes in an int look like
void (*ptrName)(int);   //A function pointer `(*ptrName)` that returns `void` and accepts int `(int)`.
```

In ModC, function pointers are declared in the order you read it.
```c
//Here's what a function pointer that returns void and takes in an int look like
*(void(int)) ptrName;   //A function pointer `*(...)` that returns `void` and accepts int `(int)`.
```

??? Example "A More Extreme Example"
    ```c title="C"
    //A function pointer `(*ptrName)` that takes in an int `(int, ...)` and 
    //      a function pointer that returns void and takes in an int `(..., void(*)(int))` 
    //and returns a function pointer that returns void and takes in an int `void(* (...) (...) ) (int)`
    void (* (*ptrName) (int, void(*)(int)) ) (int);

    //And a typedef can simplify to
    typedef void (*CallbackPtr) (int);
    CallbackPtr (*ptrName) (int, CallbackPtr);
    ```

    ```modc title="ModC"
    //A function pointer `*( (...) (...) ) ptrName` that returns 
    //      a function pointer that returns void and takes in an int `*(void(int))`
    //and takes in an int and a function pointer that returns void and takes in an int) `(int, *(void(int)))`
    *( *(void(int)) (int, *(void(int))) ) ptrName;

    //And a typedef can also simplify to
    typedef *(void(int)) CallbackPtr;
    *(CallbackPtr (int, CallbackPtr)) ptrName;
    ```

### Unique Type
`unique` can be applied to both pointer types and value types. When a variable has the `unique` type
modifier, it requires the variable to be valid and have only 1 accessible value at any moment.

Variables with the `unique` modifier type can only be assigned to other variables with the `unique` 
modifer type. After that, the original variable will be invalidated by being set to `null` or 
zeroed initialized (if it is a `struct` or primitive type) immediately automatically. 

Normal variables can also be "promoted" to `unique` explicitly, in which case the source variable 
will be invalidated and "expired" (Will be explained in `lease` section later in this document).



<!--
The assignment can happen from and to any scope, meaning [Reference And Pointer Lifetime Restriction]() 
does not apply to unique types.
-->

For example:
```modc
//Assignment/referencing between different types
{
    unique *Node myNodePtr = ...;
    //*Node myNodePtr2 = myNodePtrRef;              //Error: Assigning unique type to non unique type is not allowed
    
    *Node myNodePtr3 = ...;
    //myNodePtr = myNodePtr3;                       //Error: Assigning non unique type to initialized unique type is not allowed
    unique *Node myNodePtr4 = myNodePtr3;           //Okay, unique type is initialized from non unique type
                                                    //`myNodePtr3` is set to `null` and expired
    //ModifyNode(ref *myNodePtr3);                  //Error: Using expired variable is not allowed
    
    //ref Node myNodeRef = ref *myNodePtr;          //Error: Referencing unique pointer type's value is not allowed
    ref unique *Node myNodePtrRef = ref myNodePtr;  //Okay, we are just referencing the unique pointer variable itself.
    Node myNode = *myNodePtr;                       //Okay, we are just making a copy of the value itself
    
    unique *Node myNode5 = myNodePtr;               //Okay. we are transferring `myNodePtr` to `myNodePtr4`
                                                    //`myNodePtr` is set to `null` and expired
    
    unique Node node = ...;
    //Node node2 = node;                            //Error: Assigning unique type to non unique type is not allowed
    unique Node node3 = node;                       //Okay. node is now `{0}`
}

//Transferring to different scopes
{
    unique *Node myOuterNodePtr = null;
    {
        unique *Node myNodePtr = ...;
        myOuterNodePtr = myNodePtr;                 //Okay, `myNodePtr` is now `null`
    }
}
```

#### Storing Unique Type
A struct storing any `unique` type must also be unique itself when being declared as a variable and
the unique member must not be copied to other members.

```modc
struct MyStruct
{
    unique *Node UniquePtr;
    *Node NotUniquePtr;
}

{
    //MyStruct myStruct;                                //Error: myStruct must be declared as unique type as it has unique type members
    unique MyStruct myStruct2;                          //Okay
    
    myStruct2.UniquePtr = CreateUniqueNode();
    //myStruct2.NotUniquePtr = myStruct2.UniquePtr;     //Error: Assigning unique type to non unique type is not allowed
}
```

#### Passing As Parameter
The `unique` pointer type property however, can be dropped when it is passed as a parameter to a 
function call. As long as the same unique pointer is only accessible in one parameter.

```modc
struct MyStruct
{
    unique *Node UniquePtr;
    *Node NotUniquePtr;
}
```

```modc
void MyNodeFunction(*Node node, *Node node2) { ... }

{
    unique MyStruct myStruct = ...;
    MyNodeFunction(myStruct.UniquePtr, myStruct.NotUniquePtr);      //Okay, we are just passing 2 different pointers
    //MyNodeFunction(myStruct.UniquePtr, myStruct.UniquePtr);       //Error: myStruct.UniquePtr can be accessed by more than one parameter
}
```

```modc
void MyStructFunction(*MyStruct myStructPtr, *Node node) { ... }

{
    unique MyStruct myStruct = ...;
    MyStructFunction(&myStruct, myStruct.NotUniquePtr);             //Okay, pointer to a unique struct is fine, since it is not a copy
    //MyStructFunction(&myStruct, myStruct.UniquePtr);              //Error: myStruct.UniquePtr can be accessed by more than one parameter
}
```

```modc
void MyStructOnlyFunction(MyStruct myStruct) { ... }

{
    unique MyStruct myStruct = ...;
    //Error: Only unique pointer type can drop the unique type for function parameters
    //MyStructOnlyFunction(myStruct);
}
```

??? info "Rationale"
    A `unique` type basically enforces a single ownership for a variable and makes tracking ownership
    and expire references and tenants possible. It is also good practice to have a single ownership. 
    Shared ownership is possible via the standard library.

### Function Attributes

In ModC, a function can have a set of function attributes separated by comma (`,`) inside square 
brackets (`[]`) before the function signature to describe different attributes of the function as 
well as its parameters. 

Function attributes have no effect on the function's local variables. 

???+ Example
    ```modc
    struct Node { ... }

    [deferred, expire(this)]
    void FreeNode(ref *Node this) { ... }
    ```

### Defer

#### Function Defer

```modc
struct Node { ... }

void Node.FreeNode(ref *Node this) { ... }
*Node Node.CreateNode() { ... }

{
    *Node nodePtr = CreateNode();
    ...
    nodePtr.FreeNode();
}
```

In the above example, we should always be calling `FreeNode()` after calling `CreateNode()`.
This can be guaranteed by using function defer.

A function can have different attributes, which are declared inside `[]` before function declaration. 
One of the attributes is `defer`. For example

```modc
//The function being "deferred" must have the attribute `deferred`
[deferred] 
void FreeNode(ref *Node this) { ... }

//The caller PROMISES that the `FreeNode()` function will be called at some point
[defer(FreeNode(any))]   //defer( <function to defer>(any) )
*Node CreateNode() { ... }

{
    *Node nodePtr = CreateNode();
    ...
    nodePtr.FreeNode(); //Must exist
}
```

And the promised defer function must be called before any exit call in the current scope
```modc
[deferred] 
void FreeNode(ref *Node this) { ... }

[defer(FreeNode(any))] 
*Node CreateNode() { ... }

{
    *Node nodePtr = CreateNode();
    ...
    if(someCondition)
    {
        nodePtr.FreeNode(); //Must exist
        return;
    }
    ...
    nodePtr.FreeNode();     //Must exist
}
```

The order of the promised defer function also matters. It needs to be in a LIFO (last in first out) 
order.

```modc
[deferred] 
void FreeNode(ref *Node this) { ... }

[defer(FreeNode(any))] 
*Node CreateNode() { ... }

[deferred] 
void CloseFile(ref *file this) { ... }

[defer(CloseFile(any))] 
*file CreateFile(Std.View<char> filename) { ... }

{
    *Node nodePtr = CreateNode();
    *file fileHandle = CreateFile(Std.StringView("./MyFile.txt"));
    
    ...
    
    //nodePtr.FreeNode();       //Error: Defer function for CloseFile() is expected instead
    file.CloseFile();           //Okay
    nodePtr.FreeNode();         //Okay
    ...
    nodePtr.FreeNode();         //Also okay, since we are not limiting `FreeNode()` to only be called once
}
```

#### Defer Block

You can avoid manually writing the defer function in all required places by using the defer block.

```modc
[deferred] 
void FreeNode(ref *Node this) { ... }

[defer(FreeNode(any))] 
*Node CreateNode() { ... }

{
    *Node nodePtr = CreateNode();
    defer
    {
        nodePtr.FreeNode();
    }
    
    ...
    if(someCondition)
    {
        //nodePtr.FreeNode();   //Equivalent
        return;
    }
    ...
    //nodePtr.FreeNode();       //Equivalent
}
```

#### Defer Binding

Continuing from the previous example. Promising which function to be called won't stop the user from 
doing this:
```modc
[deferred] 
void FreeNode(ref *Node this) { ... }

[defer(FreeNode(any))] 
*Node CreateNode() { ... }

{
    *Node nodePtr = CreateNode();
    ...
    FreeNode(null);     //FreeNode is called, promised is fulfilled but nodePtr is now dangling...
}
```

We can specify what the `FreeNode()` function should be called with. 
`return_value` can be used for indicating the return value.

The syntax might seem a bit long and verbose, but it can be shorten by inferring which will be 
explained in a moment.

```modc
[deferred] 
void FreeNode(ref *Node this) { ... }

//defer( <function to defer> (<what to pass to defer>) )
[defer(FreeNode(return_value))]
*Node CreateNode() { ... }
```
You can also specify a parameter to be deferred, as long as the type matches or convertible.

??? Example
    ```modc
    [defer(FreeNode(outNode))] 
    void CreateNode(out *Node outNode) { ... }
    ```

This is called defer binding because we are binding a defer function to a variable.
Now, if the user does this, the code won't compile.
```modc
{
    defer(FreeNode) *Node nodePtr = CreateNode();
    ...
    FreeNode(null);
}       //Error: nodePtr is binded with defer(FreeNode) but no defer function is called
```

Notice we are erroring when the scope ends, not when calling `FreeNode(null)` since we are 
promising to call `FreeNode(nodePtr)`, not prohibiting the call of `FreeNode(null)`

You might also notice the `defer(FreeNode)` type modifer is there as well. When binding a defer 
function to a variable, it get promoted to its type.

The long defer type is specified in this format: `defer(<function to defer>) <type> ...`.

Again, the syntax might seem a bit long and verbose. But it can be shorten by inferring which will be 
explained in a moment.

Similar to defer block, a `defer` call can be used for defer bindings.
```modc
{
    defer(FreeNode) *Node nodePtr = CreateNode();
    defer(nodePtr.FreeNode());
    ...
    //nodePtr.FreeNode();   //Equivalent
}
```

If the variable gets assigned to something else before its scope ends, the binded defer function 
will be additionally called before each assignment.
```modc
{
    defer(FreeNode) *Node nodePtr = CreateNode();
    defer(nodePtr.FreeNode());
    ...
    //nodePtr.FreeNode();   //Equivalent
    nodePtr = null;
    ...
    //nodePtr.FreeNode();   //Equivalent
}
```

#### Infer Defer

If the type only has one deferred function, the `defer` call argument can be omitted.
```modc
{
    defer(FreeNode) *Node nodePtr = CreateNode();
    defer;
}
```

Just like the `defer` call, if there's only 1 deferred function for the type, 
the binded defer function can be omitted.

```modc
{
    defer *Node nodePtr = CreateNode();
    defer;
}
```

However it will fail if there's more than 1 binded defer functions to avoid ambiguity of auto defer.
```modc
{
    defer *Node nodePtr = CreateNode();
    defer *Node nodePtr2 = CreateNode();
    //defer;                        //Error: Cannot defer automatically due to multiple binded defer functions.
}
```

Similarly, you don't need to specify the defer function in function attributes if the type only has 
one deferred function.
```modc
//Assuming `*Node` only has one deferred function which is always `FreeNode()`
//defer(<what to pass to defer>) 
[defer(return_value)]
*Node CreateNode() { ... }
```

#### Assignment

It is possible to assign a defer type variable to a parent variable, as long as they are the same 
type.

```modc
{
    defer *Node outerNodePtr = null;
    defer(outerNodePtr);
    
    {
        defer *Node nodePtr2 = CreateNode();
        ...
        //defer function is called on `outerNodePtr` before assignment.
        outerNodePtr = nodePtr2;
    }
    
    //defer function for `outerNodePtr` will be called again
}
```

#### Unique And Defer

While `defer` prevents any missing cleanup, which can cause memory leaks, it doesn't prevent use 
after free.

Going back to the previous example:
```modc
[deferred, expire(this)]
void FreeNode(ref *Node this)
{
    free(this);
    this = null;
}

[defer(return_value)]
*Node CreateNode() { ... }

{
    defer *Node nodePtr = CreateNode();
    defer;
}
```

Since `FreeNode()` can modify `this`, and we are setting it to `null`.

<!--
This allows the [Mandatory Null Pointer Check]() to work and prevent accessing freed node. 
Which forces an if guard.
-->

```modc
{
    defer *Node nodePtr = CreateNode();
    ...
    nodePtr.FreeNode();
    ...
    if(nodePtr != null)
    {
        Node node = *nodePtr;       //Okay, `nodePtr` is not `null`
        ...
    }
}
```

However, it won't stop the user from doing this:
```modc
{
    defer *Node nodePtr = CreateNode();
    *Node nodePtr2 = nodePtr;
    ...
    nodePtr.FreeNode();             //`nodePtr` is now null, but `nodePtr2` is not.
    ...
    Node node = *nodePtr2;          //Oh no, `nodePtr2` is still pointing to stale/freed address
}
```

There are a few ways to solve this. One of them is to limit the user from making a copy of the 
pointer. This can be done with the `unique` keyword. 

```modc
[deferred, expire(this)]
void FreeNode(ref unique *Node this) { ... }

[defer(return_value)]
unique *Node CreateNode() { ... }
```

and the previous scenario would look like

```modc
{
    unique defer *Node nodePtr = CreateNode();
    //*Node nodePtr2 = nodePtr;             //Error: Assigning unique type to non unique type is not allowed
    ...
    nodePtr.FreeNode();
    ...
    //Node node = *nodePtr2;                //`nodePtr2` doesn't exist now
}
```


### Uninitialized Data Check
In ModC, any uninitialized variable cannot be read at all.
```modc
{
    int myVar;              //Not initialized
    //myVar = myVar + 5;    //Error: Reading uninitialized variable myVar
}
```

For struct, it has to be either fully initialized or uninitialized.
```modc
struct Node 
{ 
    int ID;
    int Val;
}

{
    Node node;                  //Not initialized
    //node.ID = 5;              //Error: Partial initialized is not allowed
}
```

<!--
#### Null Pointer Check
In ModC, pointers (`*`) must be checked against `null` before accessing the member. This gets reset 
when it got assigned again. 
```modc
*int GetIntegerPointer() { ... }

{
    *int myVarPtr = null;
    //int myVar = *myVarPtr;            //Error: Trying to use a null pointer.
    
    myVarPtr = GetIntegerPointer();
    //int myVar2 = *myVarPtr;           //Error: Pointer must be checked before accessing
    
    if(myVarPtr != null)
        int myVar2 = *myVarPtr;         //Okay, we are checking `myVarPtr` is not `null`
    
    if(myVarPtr == null)
        return;
    int myVar2 = *myVarPtr;             //Okay, since `myVarPtr` must not be `null` after this point
    
    myVarPtr = GetIntegerPointer();     //`myVarPtr` must be check again after this line
}

//This applies to conditions as well
{
    myVarPtr = GetIntegerPointer();
    bool someCondition = ...;
    if(someCondition)
    {
        if(myVarPtr == null)
            return;
        ...
        int myVar2 = *myVarPtr;         //Okay, since `myVarPtr` must not be `null` at this point
    }
    else
    {
        ...
        //int myVar2 = *myVarPtr;       //Error: Pointer must be checked before accessing
    }
}
```
-->

### Lessee and Lessor

In ModC, the `lease` keyword is used to denote a non owning pointer object. Therefore a pointer of 
an existing object can be assigned by using the `lease` keyword.

The `lease` keyword can only be applied to pointer types. 

When the owning object's (lessor) lifetime ends, it will expire all the leases. 

This is done during compile time.

<!-- Expire, Lease, Bind And Use -->

```modc
{
    *int intPtr = null;
    lease *int intPtr2 = null;
    {
        int intVal = 5;
        //intPtr = &intVal;             //Error: Assigning object address must be leased
        //intPtr = lease &intVal;       //Error: Assigning lease to non lease type is not allowed
        //intPtr2 = &intVal             //Error: Assigning object address must be leased
        intPtr2 = lease &intVal         //Okay
    
    }                                   //`intVal` now expires all the lease
    
    //Std.Print(intPtr);                //Error: Expired pointer cannot be used.
}
```

Lease is being controlled by 3 function attribute keywords:

- `expire(<source variable>)`: Expires the variable that gives out leases
- `lease(<source variable>, <lease variable>)`: Creates a lease from the source variable to the lease
variable
- `use(<lease variable>)`: Tells the compiler that the function uses the lease variable

#### Expire

When a function is invalidating a pointer, it can also invalidate its leases as well. When setting a 
reference to a non lease pointer to `null`, the `expire` function attribute is required.

```modc
struct Node { ... }

[deferred, expire(this)]
void FreeNode(ref *Node this) 
{ 
    free(this);
    this = null;
}

void UseNode(ref Node this) { ... }

{
    unique *Node sourceNode = ...;
    ...
    lease *Node nodePtr = lease sourceNode;
    ...
    sourceNode.FreeNode();      //`nodePtr` is now expired.
    ...
    //UseNode(ref *nodePtr);    //Error: Expired pointer cannot be used.
}
```

#### Use

When a function is using a non owning pointer, it must denote it in its function attribute with the
`use` keyword.

```modc
[use(intPtr)]
void UsingIntPtr(ref lease *int intPtr) { *intPtr = 5; }
```

This applies to struct that are storing a `lease` pointer type as well.

```modc
struct Node;
void ModifyNode(ref Node node) { ... }

struct MyStruct
{
    ...
    lease *Node NodePtr;
}

void UsingMyStruct(ref MyStruct myStruct)
{
    ...         //`MyStruct.NodePtr` is not used, okay.
}

[use(myStruct.NodePtr)]     //Must exist
void UsingMyStruct2(ref MyStruct myStruct)
{
    ...
    ModifyNode(ref *myStruct.NodePtr);
}
```

#### lease

When a function contains code that creates lease from a source variable, it can use the `lease` 
keyword.

A function can only create lease from a non local source variable.

```modc
struct Node;
void ModifyNode(ref Node node) { ... }

[lease(node, return_value)]
lease *Node CreateNodeLease(in Node node)
{
    return lease &node;
}

{
    lease *Node nodeLease = null;
    
    {
        Node node = ...;
        nodeLease = CreateNodeLease(in node);
        ...
    }                                           //`nodeLease` is now expired
    
    //ModifyNode(ref *nodeLease);               //Error: Expired pointer cannot be used.
}
```

```modc
void CreateLocalNodeLease(out lease *Node nodeLease)
{
    Node node;
    //nodeLease = lease &node;                  //Error: lease function attribute is required for leasing non local variable
}
```


---

---

---

---

### Reference And Pointers
In ModC, there are reference (`ref`) and pointer (`*`) types. They are both the same thing, a type 
that stores the address of another variable.

The only difference between pointer and reference is that pointer can be `null` (where `null` can 
only be assigned to pointer types) but reference cannot.

Reference acts almost like C++ reference, it can only reference one variable for its whole lifetime. 

Unlike in C, pointer arithmetic is not allowed, unless you are in direct context. `array` should be
used instead.

```modc
{
    int myVar = 5;
    *int myVarPtr = &myVar;
    ref int myVarRef = ref myVar;
    ...
}
```





### Lease Type




## Scratch buffer



---



```modc
#if 0
tracked<*Node> trackedNode = NULL;

struct tracked<any T, int staticSize>
{
    T Value;
}

struct NodeList
{
    defer tracked<defer array<Node>> nodes;
}

tracked<*Node> GetTrackedNode(int index)
{
    tracked<
}
#endif


[deferred, expire(freeMemory)]
void Free(ref *uint8 freeMemory) {}

[defer(Free(return_value))]
*uint8 Malloc(int size) {}

defer Result<defer *uint8> SafeMalloc(int size)
{
    defer *uint8 allocatedMemory = Malloc(size);
    
    if(allocatedMemory == null)
    {
        Free(allocatedMemory);
        return Error("Failed to allocate memory");
    }
    else
        return CreateResult<>(allocatedMemory)
}

[prepile deferred expire(this)]
code CallResultContentDefer<any T>(ref Result<T> this)
{
    code deferName = info(this.Value()).get_defer_function_name();
    return  "if(" + info(this).get_caller_name() + ".IsValid())\n" + 
            "    " + info(this).get_caller_name() + ".GetRef()." + deferName + "();\n"
            "else\n" +
            "    (void)" + deferName + ";\n";
}

[prepile defer(CallResultContentDefer(return_value))]
lease Result<T> CreateResult<any T>(ref T val) {}


[expire deferred]
void FreeTool(ref *Tool this)
{
}

[defer(FreeTool(outTool))]
void TryMakeTool(ref *Tool outTool, ...)
{
    outTool = Malloc(sizeof(Tool));
}



{
    *Tool outerTool;
    
    {
        *Tool tool;
        TryMakeTool(tool, ...);
        defer(tool);
        
        *Tool tool2 = tool;     //implicit bind(tool, tool2)
        outerTool = tool;       //implicit bind(outerTool, tool)
                                //implicit expire(tool) and expire(tool2)
    }
                                //outerTool exipres now
    outerTool = null;           //outerTool is valid again, but null now.
}


{
    defer *Tool outerTool;
    defer *Tool outerTool2;
    
    {
        defer *Tool tool;       //Expecting to be "transferred" later
        TryMakeTool(tool, ...);
        
        *Tool tool2 = tool;     //implicit bind(tool, tool2)
        outerTool = tool;       //implicit transfer(outerTool, tool)
        outerTool2 = tool;      //implicit transfer(outerTool2, tool). Error!!
    }
}


//Using bind, use, expire
struct Tool { ... }

struct Test
{
    nonown *Tool ExternTool;
};

[bind(tool, this.ExternTool)]
void AssignTool(ref Test this, ref Tool tool)
{
    this.ExternTool = tool;
}

[use(this.ExternTool)]
void CheckExternToolFuel(ref Test this)
{
    this.ExternTool.CheckFuel();
}


struct ManagedTools
{
    defer array<Tools> ToolsList;
}

[expire(this.ToolsList)]
void AddTools(ref ManagedTools this, int count)

[expire(managedTools.ToolsList)]
void AddMoreTools(..., ref ManagedTools managedTools, ...)
{
    managedTools.AddTools(5);
}


{
    Test test;
    if(maybe_true)
        test.AssignTool(ref managedTools.ToolsList[0]);     //lease: managedTools.ToolsList[0], test.ExternTool
    if(maybe_true2)
        test.AssignTool(ref managedTools2.ToolsList[0]);    //lease: managedTools2.ToolsList[0], test.ExternTool

    if(maybe_true)
        AddMoreTools(..., ref managedTools, ...);           //expire: managedTools.ToolsList

    //A function?
    (test.ExternTool, managedTools)                         //expire: managedTools.ToolsList
    {
        
        AddMoreTools(..., ref managedTools, ...);           //expire: managedTools.ToolsList
        test.ExternTool
    }

    test.ExternTool.CheckFuel()                             //Error: test.ExternTool could be expired by previous AddMoreTools()
    test.CheckExternToolFuel();                             //Error: CheckExternToolFuel() uses lease test.ExternTool which could be expired by previous AddMoreTools()
}



//managedTool  (test.externTool);

```




```modc
static
prepile
//own 
const 
mutable
defer
lease
expire
depend

string defer DyanmicString() (prepile)
{
    ...
}

string defer DyanmicString(string) (prepile)
{
    ...
}

void FreeString(*string stringVal)
{
    ...
}


int main()
{
    prepile string defer test = DynamicString();
    
    test = "abc" (prepile);
    if(true)
        test = "abcde" (prepile);      
}


```
