#ifndef MODC_MOVE_H
#define MODC_MOVE_H

#define MODC_MOVE(typename, dst, src) \
    do \
    { \
        dst = src; \
        src = (typename) {0}; \
    } \
    while(0)

#endif
