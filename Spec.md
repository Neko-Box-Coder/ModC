
# ModC Specification

## Core Language

### Header And Source Files
ModC uses extension of `modh` and `modc` respectively for header and source files.
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


### Language Structs
```go
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


### Types
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
```


### Type Modifiers
```c
//Location modifiers
extern                      //External linkage
static                      //Static storage

*                           //Pointer, e.g. `*bool`.
ref                         //Reference, e.g. `ref bool`. Reference is always valid
const                       //Constant.
defer [defer identifier]    //A method associated with the type that is promised to be run
//lease [lease identifier]    //A non owning type that can be staticly binded and expired
//shared                      //A type shared between multiple execution contexts

//Parameter modifiers
in                          //Read only reference, e.g. `in bool`. Function must read.
out                         //Write only reference, e.g. `out bool`. Function must write.
```

Unlike in C, Types are always read from left to right
```go
*bool myBool;                   //Pointer to boolean
const *bool myBoolPtr = ...;    //Constant pointer to boolean
*const bool myBoolPtr = ...;    //Pointer to constant boolean
```

### Type Aliasing
```c
typedef <existing type> <alias name>;           //Explicit typedef, conversion needs explicit casting
implicit typedef <existing type> <alias name>;  //Implicit typedef

//For example:
typedef int ID;

{
    int someNum = 5;
    ID myId = someNum;      //Error: ID is explicitly typedef as int, 
                            //      conversion needs explicit casting
    ID myId = (ID)someNum;  //Okay
}
```

### Namespace
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

### Function Declaration
Functions are declared just like in C, with 2 exceptions
1. Empty arguments `()` is the same as void argument `(void)`
2. Variable arguments is not possible (but can be done in another way using prepile context or 
    dynamic array)

### Function Pointer
In C, function pointers are declared/read from inside out.
```c
//Here's what a function pointer that returns void and takes in an int look like
void (*ptrName)(int);   //A function pointer `(*ptrName)` that returns `void` and accepts `(int)`.

//A function pointer `(*ptrName)` that returns 
//      a function pointer that returns void and takes in an int `void(* (...) (...) ) (int)`
//          and takes in an int and 
//              a function pointer that returns void and takes in an int `(int, void(*)(int))`
void (* (*ptrName) (int, void(*)(int)) ) (int);

//And a typedef can simplify to
typedef void (*CallbackPtr) (int);
CallbackPtr (*ptrName) (int, CallbackPtr);
```

In ModC, function pointers are declared in the order you read it.
```c
//Here's what a function pointer that returns void and takes in an int look like
*(void(int)) ptrName;   //A function pointer `*(...(...))` that returns `void` and accepts `(int)`.

//A function pointer `*( (...) (...) ) ptrName` that returns 
//      a function pointer that returns void and takes in an int `*(void(int))`
//          and takes in an int and 
//              a function pointer that returns void and takes in an int) `(int, *(void(int)))`
*( *(void(int)) (int, *(void(int))) ) ptrName;

//And a typedef can also simplify to
typedef *(void(int)) CallbackPtr;
*(CallbackPtr (int, CallbackPtr)) ptrName;
```

### Function Bindings
Functions can be binded to a struct which can be called with `.`
```cpp
//Given a struct
struct Node
{
    int ID;
};

//And a function like this
void SetId(ref Node this, int id)
{
    this.ID = id;
}

//The function is automatically binded to the struct, which we can do...
Node node;
node.SetId(123);

//Or
*Node nodePtr = ...;
(*nodePtr).SetId(123);
```

And of course we can also bind a function to a pointer type like this
```cpp
void FreeNode(ref *Node this)
{
    if(!this)
        return;
    
    free(this);
    this = null;
}

//Which we can do
*Node nodePtr = CreateNode();
(*nodePtr).SetId(123);
...
nodePtr.FreeNode();

//nodePtr is now null
```

### Function Defer And Defer Binding

In the previous example, we should always be calling `FreeNode()` after calling `CreateNode()`.
This can be guaranteed by using function defer.

A function can have different attributes, which are declared inside `[]` before function declaration. 
One of the attributes is defer. For example

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

And the promised defer function must exist before any exit call in the current scope
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

However, it won't stop the user from doing this:
```go
[deferred] 
void FreeNode(ref *Node this) { ... }
[defer(FreeNode)] 
*Node CreateNode() { ... }

{
    *Node nodePtr = CreateNode();
    ...
    FreeNode(null);     //FreeNode is called, promised is fullfiled but nodePtr is now dangling...
}
```

We can specify what the `FreeNode()` function should be called with. 
`return_value` can be used for indicating the return value.

```go
[deferred] 
void FreeNode(ref *Node this) { ... }

//defer( <function to defer> (<what to pass for defer>) )
[defer(FreeNode(return_value))]
*Node CreateNode() { ... }
```
You can also specify a parameter to be deferred

```go
[defer(FreeNode(outNode))] 
void CreateNode(out *Node outNode) { ... }
```

This is called defer binding because we are binding a defer function to a variable.
Now, if the user does this, the code won't transpile.
```go
{
    *Node nodePtr = CreateNode();
    ...
    FreeNode(null);
}       //Error: nodePtr is binded with defer(FreeNode) but no defer function is called
```

Notice we are erroring when the scope ends, not when calling `FreeNode(null)` since we are 
promising to call `FreeNode(nodePtr)`, not prohibiting the call of `FreeNode(null)`

To make life simpler, a `defer` block can be used.
```go
{
    *Node nodePtr = CreateNode();
    defer 
    {
        nodePtr.FreeNode();
    }
    ...
    //nodePtr.FreeNode();   //Equivalent to this
}
```

If the type only has one deferred function, the block can be omitted.
```go
{
    *Node nodePtr = CreateNode();
    defer;
}
```

However it will fail if there's more than 1 binded defer functions to avoid ambiguity of auto defer.
```go
{
    *Node nodePtr = CreateNode();
    *Node nodePtr2 = CreateNode();
    defer;      //Error: Cannot defer automatically due to multiple binded defer functions. 
                //  nodePtr, nodePtr2 have binded defer functions
}
```

### Defer Type
Continuing with the last example, the user can "hoist" the defer as a type. Which then can be 
assigned to other existing object. Effectively making the binded defer function work beyond the 
current scope.

```go
{
    bool someCondition = true;
    defer(FreeNode) *Node outerNodePtr = null;
    
    //We promise to call `FreeNode()` on `outerNodePtr`. 
    //NOTE: It's fine to defer on null type since ModC guarantees null cheeck before accessing 
    //      pointer member.
    defer;
    
    if(someCondition)
    {
        defer(FreeNode) *Node nodePtr = CreateNode();
        ...
                                //NOTE: If `outerNodePtr` is not `null`, the binded defer function
                                //      will be called first before the assignment.
        outerNodePtr = nodePtr; //Moving it outside the current scope.
    }
    ...
}
```

Just like the `defer` block, if there's only 1 deferred function for the type, the binded defer 
function can be omitted.

```go
{
    bool someCondition = true;
    defer *Node outerNodePtr = null;
    defer;
    
    if(someCondition)
    {
        defer *Node nodePtr = CreateNode();
        ...
        outerNodePtr = nodePtr;
    }
    ...
}
```

Once the defer type is assigned to other object, any other assignment to another object is invalid.
If the assignment is inside a branch, the rest of the branches must call the binded defer function.

TODO: Unqiue type?

```go
{
    bool someCondition = true;
    bool someCondition2 = true;
    defer *Node outerNodePtr = null;
    defer *Node outerNodePtr2 = null;
    defer;
    
    if(someCondition)
    {
        defer *Node nodePtr = CreateNode();
        ...
        if(someCondition2)
        {
            outerNodePtr = nodePtr;
            outerNodePtr2 = nodePtr;    //Error: nodePtr has defer type and was assigned to 
                                        //  outerNodePtr already.
        }
        else
            defer{ nodePtr.FreeNode(); };
        
        outerNodePtr2 = nodePtr;    //Error: nodePtr has defer type and maybe assigned to 
                                    //  outerNodePtr already.
    }
    ...
}
```




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
