#ifndef MODC_CHAIN_UTIL_H
#define MODC_CHAIN_UTIL_H

/* Docs
`MODC_CHAIN(funcA, argsA, funcB, argsB, funcC, argsC, ...)`

Equivalent to

`funcC( funcB( funcA(argsA), argsB ), argsC )`
*/

#include "MacroPowerToys/MacroPowerToys.h"

#define INTERN_MODC_ARGS_EMPTY
#define INTERN_MODC_ARGS_NOT_EMPTY ,

#define INTERN_MODC_WRAP_EXPR(expr, b, bArgs) \
    b(  expr \
        MPT_DELAYED_CONCAT(INTERN_MODC_ARGS_, MPT_ARE_ARGS_EMPTY(MPT_REMOVE_PARENTHESIS(bArgs))) \
        MPT_REMOVE_PARENTHESIS(bArgs))

#define INTERN_MODC_CHAIN_4(a, aArgs, b, bArgs) \
    INTERN_MODC_WRAP_EXPR( a(MPT_REMOVE_PARENTHESIS(aArgs)), b, bArgs )

#define INTERN_MODC_CHAIN_6(a, aArgs, b, bArgs, c, cArgs) \
    INTERN_MODC_WRAP_EXPR( INTERN_MODC_CHAIN_4(a, aArgs, b, bArgs), c, cArgs )

#define INTERN_MODC_CHAIN_8(a, aArgs, b, bArgs, c, cArgs, d, dArgs) \
    INTERN_MODC_WRAP_EXPR( INTERN_MODC_CHAIN_6(a, aArgs, b, bArgs, c, cArgs), d, dArgs )

#define INTERN_MODC_CHAIN_10(a, aArgs, b, bArgs, c, cArgs, d, dArgs, e, eArgs) \
    INTERN_MODC_WRAP_EXPR( INTERN_MODC_CHAIN_8(a, aArgs, b, bArgs, c, cArgs, d, dArgs), e, eArgs )

#define INTERN_MODC_CHAIN_12(a, aArgs, b, bArgs, c, cArgs, d, dArgs, e, eArgs, f, fArgs) \
    INTERN_MODC_WRAP_EXPR( INTERN_MODC_CHAIN_10(a, aArgs, b, bArgs, c, cArgs, d, dArgs, e, eArgs), f, fArgs )

#define INTERN_MODC_CHAIN_14(a, aArgs, b, bArgs, c, cArgs, d, dArgs, e, eArgs, f, fArgs, g, gArgs) \
    INTERN_MODC_WRAP_EXPR( INTERN_MODC_CHAIN_12(a, aArgs, b, bArgs, c, cArgs, d, dArgs, e, eArgs, f, fArgs), g, gArgs )


#define MODC_CHAIN( ... ) MPT_OVERLOAD_MACRO(INTERN_MODC_CHAIN, __VA_ARGS__)

static inline int ChainTest(int a)
{
    return a;
}

static inline int ChainTest2(int a)
{
    return a;
}

static inline int ChainTest3(int a)
{
    return a;
}

static inline int ChainTest4(int a)
{
    return a;
}

static inline void ChainTest5()
{
    (void)MODC_CHAIN(   ChainTest, (0), 
                        ChainTest2, (), 
                        ChainTest3, (),
                        ChainTest4, ());
    
}


#endif
