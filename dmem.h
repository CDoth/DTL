#ifndef DMEM_H
#define DMEM_H
#include <stdlib.h>
#include <string>

#include "QDebug"
unsigned int __dmem_alloced = 0;
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

template <class T> size_t range(unsigned s) {return sizeof(T)*s;}
template <class T> T*   get_mem(int s) {INC_ALLOCED; return reinterpret_cast<T*>(malloc(range<T>(s)));}
template <class T> void set_mem(T*&m, int s) {INC_ALLOCED; m = reinterpret_cast<T*>(malloc(range<T>(s)));}

template <class T> void reset_mem(T*&m, int s) {INC_REALLOCED(m); m = reinterpret_cast<T*>(realloc(m, range<T>(s)));}
template <class T> T*   reget_mem(T* m, int s) {INC_REALLOCED(m); return reinterpret_cast<T*>(realloc(m,range<T>(s)));}

template <class T> void free_mem (T*&m) {if(m){DEC_ALLOCED; free(m); m = nullptr;}}
template <class T> void zero_mem (T*m, int s) {if(m&&s) memset(m,0,range<T>(s));}
template <class T> void clear_mem(T*m, int s) {zero_mem(m,s); free_mem(m);}
template <class T> void copy_mem (T* _dst, T* _src, int s) {if(_dst&&_src&&s) memcpy(_dst, _src, range<T>(s));}

#endif // DMEM_H
