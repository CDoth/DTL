#ifndef DMEM_H
#define DMEM_H
#include <stdlib.h>
#include <string>
#include <iostream>
#include <cstring>

static unsigned int __dmem_alloced = 0;
#define NOT_FREE_DMEM __dmem_alloced
struct inc_alloced {~inc_alloced(){++__dmem_alloced;}};
struct inc_realloced{void* p;
                     inc_realloced(void* _p):p(_p){}
                     ~inc_realloced(){if(!p)++__dmem_alloced;}
                    };
struct dec_alloced {~dec_alloced(){--__dmem_alloced;}};

#define INC_ALLOCED inc_alloced _
#define DEC_ALLOCED dec_alloced _
#define INC_REALLOCED(p) inc_realloced _(p)

template <class T> inline size_t range(unsigned s) noexcept {return sizeof(T)*s;}
template <class T> inline T*   get_mem(int s) noexcept {INC_ALLOCED; return reinterpret_cast<T*>(malloc(range<T>(s)));}
template <class T> inline T*   get_zmem(int s) noexcept {INC_ALLOCED; return reinterpret_cast<T*>(calloc(range<T>(s), 1));}
template <class T> inline void set_mem(T*&m, int s) noexcept {INC_ALLOCED; m = reinterpret_cast<T*>(malloc(range<T>(s)));}

template <class T> inline void reset_mem(T*&m, int s) noexcept {INC_REALLOCED(m); m = reinterpret_cast<T*>(realloc(m, range<T>(s)));}
template <class T> inline T*   reget_mem(T* m, int s) noexcept {INC_REALLOCED(m); return reinterpret_cast<T*>(realloc(m,range<T>(s)));}

template <class T> inline void free_mem (T*&m) noexcept {if(m){DEC_ALLOCED; free(m); m = nullptr;}}
template <class T> inline void zero_mem (T*m, int s) noexcept {if(m&&s) memset(m,0,range<T>(s));}
template <class T> inline void clear_mem(T*m, int s) noexcept {zero_mem(m,s); free_mem(m);}
template <class T> inline void copy_mem (T* _dst, const T* _src, int s) noexcept {if(_dst&&_src&&s) memcpy(_dst, _src, range<T>(s));}

/*
 test block

    int* p = get_mem<int>(10);
//    int* p = nullptr;
    set_mem(p, 10);
    for(int i=0;i!=10;++i) p[i] = i;
//    zero_mem(p,10);
//    free_mem(p);
    for(int i=0;i!=10;++i) printf("%d\n", p[i]);
    free_mem(p);
    printf("not free: %d\n", NOT_FREE_DMEM);
 */
#endif // DMEM_H
