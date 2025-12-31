#ifndef MODC_DEFER_H
#define MODC_DEFER_H

/* Docs
Use in a function like this. Will not work with nested defer scope.
```c
MODC_DEFER_SCOPE_START
{
    ...
    MODC_DEFER(statements);     //Runs second
    ...
    MODC_DEFER(statements);     //Runs first
    if(...)
    {
        MODC_RUN_DEFER_NOW_AND(return ...);
    }
    ...
}
MODC_DEFER_SCOPE_END
```

`MODC_RUN_DEFER_NOW_AND` will go to `MODC_DEFER_SCOPE_END` if it doesn't contain a return statement.
*/


#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#ifndef MODC_MAX_DEFER_COUNT
    #define MODC_MAX_DEFER_COUNT 32
#endif

//Improvised from: https://btmc.substack.com/i/142434521/duffs-device-to-the-rescue
#define MODC_DEFER_SCOPE_START \
    { \
        /* We store a bunch of IDs for jumping with switch, but with -1 as normal executions and */ \
        /* 0 for just normal exit. Doing `__COUNTER__` here to make sure it starts from 1 */ \
        (void)__COUNTER__; \
        int modcDeferIndex = 0; (void)modcDeferIndex; \
        /* We store `MODC_MAX_DEFER_COUNT + 1` where we executes the defers from back to front */ \
        /* and index 0 will be our "exit" point which it could a return */ \
        int modcDefers[MODC_MAX_DEFER_COUNT + 1] = {-1}; (void)modcDefers; \
        uint8_t modcDefersCount = 1; (void)modcDefersCount; \
        modcDeferStart: \
        switch(modcDefers[modcDeferIndex]) \
        { \
            case -1: \
            /* Sets the final execution to 0 for normal exit */ \
            modcDefers[0] = 0;

#define INTERNAL_MODC_DEFER(counter, statements) \
            do \
            { \
                if(false) \
                { \
                    /* Defer block */ \
                    case counter:; \
                    statements; \
                    /* Go to previous defer, we run defers in reverse order */ \
                    --modcDeferIndex; \
                    goto modcDeferStart; \
                } \
                /* Register defer */ \
                assert((modcDefersCount < MODC_MAX_DEFER_COUNT + 1) && "Number of defers exceeds maximum"); \
                modcDeferIndex = modcDefersCount; \
                modcDefers[modcDefersCount++] = counter; \
            } \
            while(false)

#define MODC_DEFER(statements) INTERNAL_MODC_DEFER(__COUNTER__, statements)

#define INTERN_MODC_RUN_DEFER_NOW_AND(counter, expr) \
            do \
            { \
                /* Register return */ \
                modcDefers[0] = counter; \
                goto modcDeferStart; \
                if(false) \
                { \
                    /* Return statement block */ \
                    case counter:; \
                    expr; \
                    goto modcDeferScopeEnd; \
                } \
            } \
            while(false)

#define MODC_RUN_DEFER_NOW_AND(expr) INTERN_MODC_RUN_DEFER_NOW_AND(__COUNTER__, expr)

#define INTERNAL_MODC_DEFER_SCOPE_END(counter) \
            goto modcDeferStart; \
        } /* switch(modcDefers[modcDeferIndex]) */ \
        goto modcDeferScopeEnd; \
        modcDeferScopeEnd:; \
    }

#define MODC_DEFER_SCOPE_END INTERNAL_MODC_DEFER_SCOPE_END(__COUNTER__)


#endif
