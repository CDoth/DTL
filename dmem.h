#ifndef DMEM_H
#define DMEM_H
#include <stdlib.h>
#include <string>

unsigned int __dmem_alloced;
#define NOT_FREE_DMEM __dmem_alloced
template <class T> size_t range(unsigned s) {return sizeof(T)*s;}
template <class T> T*   get_mem(int s) {__dmem_alloced++; return reinterpret_cast<T*>(malloc(range<T>(s)));}
template <class T> void set_mem(T*&m, int s) {__dmem_alloced++; m = reinterpret_cast<T*>(malloc(range<T>(s)));}

template <class T> void reset_mem(T*&m, int s) {if(m == nullptr) __dmem_alloced++; m = reinterpret_cast<T*>(realloc(m, range<T>(s)));}
template <class T> T*   reget_mem(T*&m, int s) {if(m == nullptr) __dmem_alloced++; return reinterpret_cast<T*>(realloc(m,range<T>(s)));}

template <class T> void free_mem(T*&m) {if(m){ free(m); __dmem_alloced--;} m = nullptr;}
template <class T> void zero_mem(T*&m, int s) {if(m&&s) memset(m,0,range<T>(s));}
template <class T> void clear_mem(T*&m, int s) {zero_mem(m,s); free_mem(m);}
template <class T> void copy_mem(T* _dst, T* _src, int s) {if(_dst&&_src&&s) memcpy(_dst, _src, range<T>(s));}

#endif // DMEM_H
