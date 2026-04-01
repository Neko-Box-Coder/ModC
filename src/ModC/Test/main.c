#include <stdio.h>

enum TestEnum
{
    EnumVal1 = 1,
    EnumVal2,
    EnumVal3
}

struct TestStruct
{
    int A;
    TestEnum B;
}

TestStruct RetTestStruct(int a)
{
    return TestStruct();
}

TestEnum RetTestEnum()
{
    return TestEnum.EnumVal1;
}

int main(int argc, char** argv)
{
    TestStruct testStructA = RetTestStruct(1);
    TestEnum testEnum = RetTestEnum();
    
    printf("Hello World");
    
    if(argc == 2)
        printf("2 args");
    else
        printf("other args");
    
    testEnum = EnumVal3;
    
    return 0;
}
