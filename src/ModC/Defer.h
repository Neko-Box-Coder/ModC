#ifndef MODC_DEFER_H
#define MODC_DEFER_H

/* Docs
Use in a function like this. Will not work with nested defer scope.
```c
MODC_DEFER_SCOPE_START(scopeId)
...
MODC_DEFER(scopeId, statements);     //Runs second
...
MODC_DEFER(scopeId, statements);     //Runs first
if(...)
{
    MODC_DEFER_BREAK(scopeId, return ...);
}
...
MODC_DEFER_SCOPE_END(scopeId)
```

`MODC_DEFER_BREAK` will go to `MODC_DEFER_SCOPE_END` if it doesn't contain a return statement.
*/


#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#ifndef MODC_MAX_DEFER_COUNT
    #define MODC_MAX_DEFER_COUNT 32
#endif

//Improvised from: https://btmc.substack.com/i/142434521/duffs-device-to-the-rescue
#define MODC_DEFER_SCOPE_START(id) \
    { \
        /* We store a bunch of IDs for jumping with switch, but with -1 as normal executions and */ \
        /* 0 for just normal exit. Doing `__COUNTER__` here to make sure it starts from 1 */ \
        (void)__COUNTER__; \
        int modcDeferIndex = 0; \
        /* We store `MODC_MAX_DEFER_COUNT + 1` where we executes the defers from back to front */ \
        /* and index 0 will be our "exit" point which it could a return */ \
        int modcDefers[MODC_MAX_DEFER_COUNT + 1] = {-1}; (void)modcDefers; \
        uint8_t modcDefersCount = 1; (void)modcDefersCount; \
        modcDeferStart ## id: \
        switch(modcDefers[modcDeferIndex]) \
        { \
            case -1: \
                /* Sets the final execution to 0 for normal exit */ \
                modcDefers[0] = 0;

#define INTERNAL_MODC_DEFER(id, counter, ... /* statements */) \
            do \
            { \
                if(false) \
                { \
                    /* Defer block */ \
                    case counter:; \
                    __VA_ARGS__; \
                    /* Go to previous defer, we run defers in reverse order */ \
                    --modcDeferIndex; \
                    goto modcDeferStart ## id; \
                } \
                /* Register defer */ \
                assert((modcDefersCount < MODC_MAX_DEFER_COUNT + 1) && "Number of defers exceeds maximum"); \
                modcDeferIndex = modcDefersCount; \
                modcDefers[modcDefersCount++] = counter; \
            } \
            while(false)

#define MODC_DEFER(id, ... /* statements */) INTERNAL_MODC_DEFER(id, __COUNTER__, __VA_ARGS__)

#define INTERN_MODC_DEFER_BREAK(id, counter, ... /* statements */) \
            do \
            { \
                /* Register return */ \
                modcDefers[0] = counter; \
                goto modcDeferStart ## id; \
                if(false) \
                { \
                    /* Return statement block */ \
                    case counter:; \
                    __VA_ARGS__; \
                    goto modcDeferScopeEnd ## id; \
                } \
            } \
            while(false)

#define MODC_DEFER_BREAK(id, ... /* statements */) INTERN_MODC_DEFER_BREAK(id, __COUNTER__, __VA_ARGS__)

#define MODC_DEFER_SCOPE_END(id) \
            goto modcDeferStart ## id; \
        } /* switch(modcDefers[modcDeferIndex]) */ \
        goto modcDeferScopeEnd ## id; \
        modcDeferScopeEnd ## id:; \
    }


#endif
