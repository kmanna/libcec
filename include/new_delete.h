#ifndef _NEW_DELETE_H
#define _NEW_DELETE_H

#include <stdlib.h>

typedef void __guard;
inline int __cxa_guard_acquire(__guard *g) {return !*(char *)(g);};
inline void __cxa_guard_release (__guard *g) {*(char *)g = 1;};
inline void __cxa_guard_abort (__guard *) {};
inline void __cxa_pure_virtual(int) {};
inline void * operator new(size_t n) {
    void * const p = malloc(n);
    return p;
}
inline void * operator new[](size_t n) {
    void * const p = malloc(n);
    return p;
}

inline void operator delete(void * p) {
    free(p);
}

inline void operator delete[](void * p) {
    free(p);
}

#endif // _NEW_DELETE_H

