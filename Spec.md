
# ModC Specification

## Core Language

### Language Structs
```cpp
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
```cpp
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
```cpp
//Location modifiers
extern                      //External linkage
static                      //Static storage

*                           //Pointer, e.g. `*bool`
ref                         //Reference, e.g. `ref bool`. Must be valid

//Secondary type modifiers
const                       //Constant
defer [defer identifier]    //A method associated with the type

//Execution context type modifiers
lease [lease identifier]    //A non owning type that can be staticly binded and expired
shared                      //A type shared between multiple execution contexts
```

### Type Aliasing
```cpp
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
```


### Function Pointer
```cpp
//In C
void (*ptrName)(int);   //A function pointer that returns void and takes in an int

//A function pointer that returns (a function pointer that returns void and takes in an int)
//      and takes in an int and (a function pointer that returns void and takes in an int)
void (* (*ptrName) (int, void(*)(int)) ) (int);

//And a typedef can simplify to
typedef void (*CallbackPtr) (int);
CallbackPtr (*ptrName) (int, CallbackPtr);


//In ModC, function pointers are declared in the order you read it, so the equivalents are:
*(void(int)) ptrName;   //A function pointer that returns void and takes in an int

//A function pointer that returns (a function pointer that returns void and takes in an int)
//      and takes in an int and (a function pointer that returns void and takes in an int)
*( *(void(int)) (int, *(void(int))) ) ptrName;

//And a typedef can also simplify to
typedef *(void(int)) CallbackPtr;
*(CallbackPtr (int, CallbackPtr)) ptrName;
```





```cpp
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


