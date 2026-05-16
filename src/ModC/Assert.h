#ifndef MODC_ASSERT_H
#define MODC_ASSERT_H

#include <assert.h>

#ifdef NDEBUG 
    #ifndef ASSERT_HANDLER
        #include <stdio.h>
        #include <stdlib.h>
        #define ASSERT_HANDLER(fatalStr) \
            fprintf(stderr, "\""fatalStr "\" failed in " __FILE__ ":%i:%s()\n", __LINE__, __func__); \
            fflush(stderr); \
            abort()
    #endif
#else
    #ifndef ASSERT_HANDLER
        #define ASSERT_HANDLER(...)
    #endif
#endif


#define ASSERT(expr) \
    if( !(expr) ) \
    { \
        assert(0 && #expr); \
        ASSERT_HANDLER(#expr); \
    }


#endif
