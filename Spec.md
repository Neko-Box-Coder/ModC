
# ModC Specification

## Core Language

### Basics Structure And Types

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


#### Normal Types
```go
bool
char

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

struct
enum

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

#### Type Modifiers
```c
//Location modifiers
extern                      //External linkage
static                      //Static storage

*                           //Pointer, e.g. `*bool`.
ref                         //Reference, e.g. `ref bool`. Reference is always valid
const                       //Constant.
defer [defer identifier]    //A method associated with the type that is promised to be run

//Parameter modifiers
in                          //Read only reference, e.g. `MyFunction(in bool)`. Function must only read.
out                         //Write only reference, e.g. `MyFunction(out bool)`. Function must only write.
```

Unlike in C, Types are always read from left to right
```go
*bool myBool;                   //Pointer to boolean
const *bool myBoolPtr = ...;    //Constant pointer to boolean
*const bool myBoolPtr = ...;    //Pointer to constant boolean
```

#### Type Aliasing
Just like in C, you can do `typedef` in ModC. The only difference is `typedef` by default is explicit.
To perform implicit `typedef`, you need to use the `implicit` keyword

```c
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
Functions are declared just like in C, with 2 exceptions
1. Empty arguments `()` is the same as void argument `(void)`
2. Variable arguments is not possible (but can be done in another way using prepile context or 
    dynamic array, or in direct context using `...`)
3. If the function only has implementation (Say only in `.modc` file), a declaration will be 
    generated automatically.

```cpp
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

```cpp
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

### Reference And Pointers
In ModC, there are reference (`ref`) and pointer (`*`) types. They are both the same thing, a type 
that stores the address of another variable.

The only difference between pointer and reference is that pointer can be `null` (where `null` can 
only be assigned to pointer types) but reference cannot.

Reference acts almost like C++ reference, it can only reference one variable for its whole lifetime. 

Unlike in C, pointer arithmetic is not allowed, unless you are in direct context. `array` should be
used instead.

```csharp
{
    int myVar = 5;
    *int myVarPtr = &myVar;
    ref int myVarRef = ref myVar;
    ...
}
```

When a function parameter is a reference, it can be one of these reference types:
- `ref`: Function can read and write to reference variable
- `in`: Function can only read from reference variable. The equivalent is `const ref`.
- `out`: Function can only write to reference variable

```csharp
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

#### Function Pointer
In C, function pointers are declared/read from inside out.
```c
//Here's what a function pointer that returns void and takes in an int look like
void (*ptrName)(int);   //A function pointer `(*ptrName)` that returns `void` and accepts int `(int)`.

//A function pointer `(*ptrName)` that takes in 
//      an int `(int, ...)` and 
//          a function pointer that returns void and takes in an int `(..., void(*)(int))` and 
//              returns a function pointer that returns void and takes in an int `void(* (...) (...) ) (int)`
void (* (*ptrName) (int, void(*)(int)) ) (int);

//And a typedef can simplify to
typedef void (*CallbackPtr) (int);
CallbackPtr (*ptrName) (int, CallbackPtr);
```

In ModC, function pointers are declared in the order you read it.
```c
//Here's what a function pointer that returns void and takes in an int look like
*(void(int)) ptrName;   //A function pointer `*(...(...))` that returns `void` and accepts int `(int)`.

//A function pointer `*( (...) (...) ) ptrName` that returns 
//      a function pointer that returns void and takes in an int `*(void(int))`
//          and takes in an int and 
//              a function pointer that returns void and takes in an int) `(int, *(void(int)))`
*( *(void(int)) (int, *(void(int))) ) ptrName;

//And a typedef can also simplify to
typedef *(void(int)) CallbackPtr;
*(CallbackPtr (int, CallbackPtr)) ptrName;
```

We generally advise using a `typedef` for function pointers as it makes reading a bit easier.

#### Function Bindings And Type Name Prefix
Functions can be called on a variable with `.`, in which case a reference of that variable will be
passed as the first parameter. Regardless of how you declared it.

To avoid name/symbol clashing, a function can be declaring inside the `struct` or declared with
`<type>.<function name>`. This will prefix the type name to the function automatically, and can be
called by selecting with the type using the `.` operator. Just like `namespace`.

This is literally just a syntax sugar of prefixing the type to the symbols. 
Nothing more, nothing less.

```cpp
//Given a struct
struct Node 
{ 
    int ID;
    
    //And a function like this
    void SetId(ref Node this, int id) { this.ID = id; }
};

//Or this
void Node.SetId(ref Node this, int id) { this.ID = id; }

{
    //The function is automatically binded to the struct, which we can do...
    Node node;
    node.SetId(123);
    
    //Or
    *Node nodePtr = ...;
    (*nodePtr).SetId(123);
    
    //Or
    Node.SetId(node, 123);
    
    //Reference type is treated the same as the non reference counterpart
    ref Node nodeRef = ref node;
    node.SetId(123);
}
```

And we can also bind a function to a pointer type like this
```cpp
struct Node
{
    ...
    void FreeNode(ref *Node this)
    {
        free(this);
        this = null;
    }
};

//Or this
void Node.FreeNode(ref *Node this) { ... }

{
    //Which we can do
    *Node nodePtr = CreateNode();
    (*nodePtr).SetId(123);
    ...
    nodePtr.FreeNode();

    //nodePtr is now null
}
```

Since it is just a syntax sugar, in theory you can do something like this:
```go
struct Node { ... };
struct OtherStruct { ... };

void OtherStruct.FreeNode(ref *Node this) { ... }

{
    *Node nodePtr = ...;
    OtherStruct.FreeNode(nodePtr);
}
```

Which is not recommended, but it does have its use-case.


### ModC Mandatory Checks and Restrictions

#### Reference And Pointer Lifetime Restriction

For now, let's say the assigning value (value on the right of `=`) is called "source".

Without any additional modifiers, a pointer or reference type cannot exist longer than the source.

```csharp
{
    int myVar = 5;
    *int myVarPtr = &myVar;
    ref int myVarRef = ref myVar;
    ...
}
```

In the above example, the source for `myVarPtr` and `myVarRef` is `myVar`. 

This is (normally) ensured by creating the pointer or reference variable at the same scope as the 
source scope or at a child scope.

If the pointer or reference type is a function parameter type, than the scope of them would be
unknown (since the function has no way of telling how scope for the source of parameters live in).
In that case, storing/assigning a pointer parameter will result in error.

```csharp
{
    *int myVarPtr = null;
    {
        int myVar = 5;
        //myVarPtr = &myVar;            //Error: myVarPtr exists longer than myVar
        *int myVarPtr2 = &myVar;        //Okay
        //myVarPtr = myVarPtr2;         //Error: myVarPtr exists longer than myVarPtr2
        ...
        {
            *int myVarPtr3 = &myVar;    //Okay
            ...
        }
    }
}

*int MyFunc(ref SomeStruct this, *int intPtr, ref *int intRefPtr)
{
    //this.SomeIntPtr = intPtr;         //Error: this.SomeIntPtr can exist longer than intPtr
    //intRefPtr = intPtr;               //Error: intRefPtr can exist longer than intPtr
    if(intPtr != null)
        *intPtr = 5;                    //Okay
    SomeStruct myStruct;
    myStruct.SomeIntPtr = intPtr;       //Okay
    ...
    //return intPtr;                    //Error: return_value can exist longer than intPtr
}
```

What this basically does is that pointers and references can only propagate down the callstack, but
not up. You can still return pointers from a function but it requires additional type modifiers.


#### Mandatory Uninitialized Data Check
In ModC, any uninitialized variable cannot be read at all.
```cpp
{
    int myVar;              //Not initialized
    //myVar = myVar + 5;    //Error: Reading uninitialized variable myVar
}
```

#### Mandatory Null Pointer Check
In ModC, pointers (`*`) must be checked against `null` before accessing the member, until it got 
assigned again. So for example
```go
*int GetIntegerPointer() { ... }

{
    *int myVarPtr = null;
    //int myVar = *myVarPtr;            //Error: Trying to use a null pointer.
    
    myVarPtr = GetIntegerPointer();
    //int myVar2 = *myVarPtr;           //Error: Pointer must be checked before accessing
    
    if(myVarPtr != null)
        int myVar2 = *myVarPtr;         //Okay, we are checking `myVarPtr` is not `null`
    
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
    
    if(myVarPtr == null)
        return;
    int myVar2 = *myVarPtr;             //Okay, since `myVarPtr` must not be `null` at this point
}
```

### Unique Type
`unique` can be applied to both pointer types and value types, requiring the variable to be valid 
and have 1 accessible copy at any moment. 

For pointer type, this means the assignment can only be performed between `unique` types and the 
original unique variable will be set to `null` once the assignment is performed.

For value type, it is similar to pointer type where the assignment can only be performed between 
`unique` types but instead of the original variable be setting to `null`, it will get zero 
initialized instead and setting any member pointers to `null`. Basically a `memset()` to zero.

For example:
```go
{
    unique *Node myNodePtr = ...;
    if(myNodePtr == null)
        return;
    
    //ref Node myNodeRef = ref *myNodePtr;          //Error: Referencing unique pointer type's value is not allowed
    ref unique *Node myNodePtrRef = ref myNodePtr;  //Okay, we are just referencing the unique pointer variable itself.
    
    //*Node myNodePtr2 = myNodePtrRef;              //Error: Assigning unique type to non unique type is not allowed
    
    Node myNode = *myNodePtr;                       //Okay, we are just making a copy of the value itself
    
    *Node myNodePtr3 = ...;
    //myNodePtr = myNodePtr3;                       //Error: Assigning non unique type to unique type is not allowed
    //myNodePtrRef = myNodePtr3;                    //Error: Assigning non unique type to unique type is not allowed
    
    unique *Node myNodePtr4 = myNodePtr;            //Okay. we are transferring `myNodePtr` to `myNodePtr4`
    //myNodePtr = null;                             //`myNodePtr` is set to `null` automatically after transferring
}

{
    unique Node node = ...;
    //Node node2 = node;                            //Error: Assigning non unique type to unique type is not allowed
    unique Node node3 = node;                       //Okay. node is now `{0}`
}

unique Node CreateUniqueNode() { return Node(); }

{
    unique Node node = CreateUniqueNode();          //Okay
    //Node node2 = CreateUniqueNode();              //Error: Assigning unique type to non unique type is not allowed
    ref unique Node nodeRef = ref node;             //Okay, we are just refencing it, not copying it.
}
```

A struct storing any `unique` type must also be unique itself when being declared as a variable and
the unique member must not be copied to other members.

```go
struct MyStruct
{
    unique *Node UniquePtr;
    *Node NotUniquePtr;
};

{
    //MyStruct myStruct;                                //Error: myStruct must be declared as unique type as it has unique type members
    unique MyStruct myStruct2;                          //Okay
    
    myStruct2.UniquePtr = CreateUniqueNode();
    //myStruct2.NotUniquePtr = myStruct2.UniquePtr;     //Error: Assigning unique type to non unique type is not allowed
}
```

The `unique` pointer type property however, can be dropped when it is passed as a parameter to a 
function call. As long as the same unique pointer is only accessible in one parameter.

```go
struct MyStruct
{
    unique *Node UniquePtr;
    *Node NotUniquePtr;
};

void MyNodeFunction(*Node node, *Node node2) { ... }
void MyStructFunction(*MyStruct myStructPtr, *Node node) { ... }
void MyStructOnlyFunction(MyStruct myStruct) { ... }

{
    unique MyStruct myStruct = ...;
    MyNodeFunction(myStruct.UniquePtr, myStruct.NotUniquePtr);      //Okay, we are just passing 2 different pointers
    //MyNodeFunction(myStruct.UniquePtr, myStruct.UniquePtr);       //Error: myStruct.UniquePtr can be accessed by more than one parameter
    
    MyStructFunction(&myStruct, myStruct.NotUniquePtr);             //Okay, pointer to a unique struct is fine, since it is not a copy
    //MyStructFunction(&myStruct, myStruct.UniquePtr);              //Error: myStruct.UniquePtr can be accessed by more than one parameter
    
    //Error: Only unique pointer type can drop the unique type for function parameters
    //MyStructOnlyFunction(myStruct);
}
```

You can still call functions with a `unique` type or modify its member variable, you just cannot make
a copy of the `unique` variable.

```go
struct MyStruct
{
    unique *Node UniquePtr;
    *Node NotUniquePtr;
};

void ModifyNode(*Node node) { ... }

{
    unique MyStruct myStruct = ...;
    myStruct.NotUniquePtr = null;       //Okay
    //MyStruct myStruct2 = myStruct;    //Error: Assigning unique type to non unique type is not allowed
    
    unique *Node nodePtr = ...;
    ModifyNode(nodePtr);                //Okay
    *Node nodePtr2 = nodePtr;           //Error: Assigning unique type to non unique type is not allowed
}
```

### Defer

#### Function Defer

```go
struct Node { ... };

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

```go
//The function being "deferred" must have the attribute `deferred`
[deferred] 
void FreeNode(ref *Node this) { ... }

//The caller PROMISES that the `FreeNode()` function will be called at some point
[defer(FreeNode)]   //defer( <function to defer> )
*Node CreateNode() { ... }

{
    *Node nodePtr = CreateNode();
    ...
    nodePtr.FreeNode(); //Must exist
}
```

And the promised defer function must be called before any exit call in the current scope
```go
[deferred] 
void FreeNode(ref *Node this) { ... }
[defer(FreeNode)] 
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

```go
[deferred] 
void FreeNode(ref *Node this) { ... }
[defer(FreeNode)] 
*Node CreateNode() { ... }

{
    *Node nodePtr = CreateNode();
    *Node nodePtr2 = CreateNode();
    
    ...
    
    //nodePtr.FreeNode();       //Error: Defer function for nodePtr2 is expected instead
    nodePtr2.FreeNode();
    nodePtr.FreeNode();         //Okay
    ...
    nodePtr.FreeNode();         //Okay, since we are not limiting `FreeNode()` to only be called once
}
```


#### Defer Binding

Continuing from the previous example. It won't stop the user from doing this:
```go
[deferred] 
void FreeNode(ref *Node this) { ... }
[defer(FreeNode)] 
*Node CreateNode() { ... }

{
    *Node nodePtr = CreateNode();
    ...
    FreeNode(null);     //FreeNode is called, promised is fulfilled but nodePtr is now dangling...
}
```

We can specify what the `FreeNode()` function should be called with. 
`return_value` can be used for indicating the return value.

The syntax might seem a bit long and verbose. Keep reading and there's a shorten version that will 
be introduced later.

```go
[deferred] 
void FreeNode(ref *Node this) { ... }

//defer( <function to defer> (<what to pass to defer>) )
[defer(FreeNode(return_value))]
*Node CreateNode() { ... }
```
You can also specify a parameter to be deferred, as long as the type matches or convertible.

> [!NOTE]
> Example
> ```go
> [defer(FreeNode(outNode))] 
> void CreateNode(out *Node outNode) { ... }
> ```

This is called defer binding because we are binding a defer function to a variable.
Now, if the user does this, the code won't transpile.
```go
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

Again, the syntax might seem a bit long and verbose. Keep reading and there's a shorten version that 
will be introduced later.

To make life simpler, a `defer` block can be used.
```go
{
    defer(FreeNode) *Node nodePtr = CreateNode();
    defer 
    {
        nodePtr.FreeNode();
    }
    ...
    //nodePtr.FreeNode();   //Equivalent
}
```

If the variable gets assigned to something else before its scope ends, the binded defer function 
will be called before the assignment.
```go
{
    defer(FreeNode) *Node nodePtr = CreateNode();
    defer 
    {
        nodePtr.FreeNode();
    }
    ...
    //nodePtr.FreeNode();   //Equivalent
    nodePtr = null;
}
```

If the type only has one deferred function, the block can be omitted.
```go
{
    defer(FreeNode) *Node nodePtr = CreateNode();
    defer;
}
```

Just like the `defer` block, if there's only 1 deferred function for the type, 
the binded defer function can be omitted.

```go
{
    defer *Node nodePtr = CreateNode();
    defer;
}
```

Similarly, you don't need to specify the defer function in function attributes if the type only has 
one deferred function.
```go
//Assuming `*Node` only has one deferred function which is always `FreeNode()`
//defer(<what to pass to defer>) 
[defer(return_value)]
*Node CreateNode() { ... }
```

However it will fail if there's more than 1 binded defer functions to avoid ambiguity of auto defer.
```go
{
    defer *Node nodePtr = CreateNode();
    defer *Node nodePtr2 = CreateNode();
    //defer;                        //Error: Cannot defer automatically due to multiple binded defer functions.
}
```

#### Unique And Defer

While `defer` prevents any missing cleanup, which can cause memory leaks, it doesn't prevent use 
after free.

Going back to the previous example:
```go
[deferred] 
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

This allows the [Mandatory Null Pointer Check]() to work and prevent accessing freed node. 
Which forces an if guard.

```go
{
    defer *Node nodePtr = CreateNode();
    ...
    nodePtr.FreeNode();
    ...
    //Node node = *nodePtr;         //Error: Pointer must be checked before accessing
    if(nodePtr != null)
    {
        Node node = *nodePtr;       //Okay, `nodePtr` is not `null`
        ...
    }
}
```

However, it won't stop the user from doing this:
```go
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

```go
[deferred] 
void FreeNode(ref unique *Node this) { ... }
[defer(return_value)]
unique *Node CreateNode() { ... }
```

and the previous scenario would look like

```go
{
    unique defer *Node nodePtr = CreateNode();
    //*Node nodePtr2 = nodePtr;             //Error: Unique type cannot be assigned to non unique type
    ...
    nodePtr.FreeNode();
    ...
    //Node node = *nodePtr2;                //`nodePtr2` doesn't exist now
}
```

### Lease Type




## Scratch buffer



---



```go
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




```go
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
