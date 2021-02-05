#ifndef DMEM_H
#define DMEM_H
#include <stdlib.h>

template <class T> T* get_mem(int s) {return reinterpret_cast<T*>(malloc(sizeof(T)*s));}
template <class T> void set_mem(T*&m, int s) {m = reinterpret_cast<T*>(malloc(sizeof(T)*s));}
template <class T> void reser_mem(T*&m, int s) {m = reinterpret_cast<T*>(realloc(m, sizeof(T)*s));}
#endif // DMEM_H
