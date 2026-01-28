#ifndef MODC_ASSERT_H
#define MODC_ASSERT_H

#include <assert.h>

#ifdef NDEBUG 
    #ifndef MODC_ASSERT_HANDLER
        #include <stdio.h>
        #include <stdlib.h>
        #define MODC_ASSERT_HANDLER(fatalStr) \
            fprintf(stderr, "\""fatalStr "\" failed in " __FILE__ ":%i:%s()\n", __LINE__, __func__); \
            fflush(stderr); \
            abort()
    #endif
#else
    #ifndef MODC_ASSERT_HANDLER
        #define MODC_ASSERT_HANDLER(...)
    #endif
#endif


#define MODC_ASSERT(expr) \
    if( !(expr) ) \
    { \
        assert(0 && #expr); \
        MODC_ASSERT_HANDLER(#expr); \
    }


#endif
