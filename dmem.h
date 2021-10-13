#ifndef DMEM_H
#define DMEM_H
#include <stdlib.h>
#include <string>
#include <iostream>
#include <cstring>
#include <daran.h>


//#define DMEM_COUNT_MEM_MODE

#ifdef DMEM_COUNT_MEM_MODE
struct DMEM_HOLDER
{
    struct memdesc
    {
        void *place;
        int size;
    };

    DMEM_HOLDER() : mem(nullptr), size(0)
    {}
    ~DMEM_HOLDER()
    {
        if(size)
        {
            std::cout << "DMEM_HOLDER: Memory leak: " << size << " blocks" << std::endl;
        }
    }
    void push(void *m, int s)
    {
        if(m)
        {
            mem = reinterpret_cast<memdesc*>(realloc(mem, size+1));
            mem[size].place = m;
            mem[size++].size = s;
        }
        else
            std::cout << "DMEM_HOLDER::push() error: try to add bad pointer: " << m << std::endl;
    }
    void remove(void *m)
    {
        FOR_VALUE(size, i)
        {
            if(mem[i].place == m)
            {
                memmove(&mem[i], &mem[i+1], sizeof(void*) * (size - i - 1));
                mem = reinterpret_cast<memdesc*>(realloc(mem, --size));
                return;
            }
        }
        std::cout << "DMEM_HOLDER::remove() error: try to remove undefined mem block: " << m << std::endl;
    }
    int alloced() {return size;}
    int blockSize(void *m)
    {
        FOR_VALUE(size, i)
        {
            if(mem[i].place == m)
                return mem[i].size;
        }
        return 0;
    }
    int isDmem(void *m)
    {
        FOR_VALUE(size, i)
        {
            if(mem[i].place == m)
                return 1;
        }
        return 0;
    }
    void printBlocks()
    {
        std::cout << "DMEM_HOLDER: mem blocks: " << size << " : " << std::endl;
        FOR_VALUE(size, i)
        {
            std::cout << i << " : place: [" << mem[i].place << "] size: [" << mem[i].size << "]" <<  std::endl;
        }
    }

    memdesc *mem;
    int size;
};
extern DMEM_HOLDER __dmem_holder;
#else
static unsigned int __dmem_alloced = 0;
#endif
struct inc_alloced
{
    #ifdef DMEM_COUNT_MEM_MODE
    inc_alloced(void *m, int s) : _m(m), _s(s){}
    ~inc_alloced(){
        __dmem_holder.push(_m, _s);
    }

void *_m;
int _s;
#else
    ~inc_alloced(){++__dmem_alloced;}
#endif
};
struct inc_realloced
{
    void *_m;
#ifdef DMEM_COUNT_MEM_MODE
    void *_src;
    int _s;
    inc_realloced(void *src, void *m, int s) :  _m(m), _src(src), _s(s){}
    ~inc_realloced()
    {
        if(!_src)
            __dmem_holder.push(_m, _s);
    }
#else
                     inc_realloced(void* m): _m(m){}
                     ~inc_realloced(){if(!_m)++__dmem_alloced;}
#endif
                    };
struct dec_alloced
{
#ifdef DMEM_COUNT_MEM_MODE
    void *_m;
    dec_alloced(void *m) : _m(m){}
    ~dec_alloced()
    {
        __dmem_holder.remove(_m);
    }
#else
    ~dec_alloced(){--__dmem_alloced;}
#endif
};


#ifdef DMEM_COUNT_MEM_MODE
#define INC_ALLOCED(M) inc_alloced _(M)
#define DEC_ALLOCED(M) dec_alloced _(M)
#define INC_REALLOCED(S, M) inc_realloced _(S, M)
template <class T> inline size_t range(int s) noexcept { return (s > 0) ? sizeof(T)*s : 0; }

template <class T> inline void set_mem(T *m, int value, int s) noexcept { if(m) memset(m, value, range<T>(s)); }
template <class T> inline void zero_mem (T *m, int s) noexcept { if(m) memset(m, 0, range<T>(s)); }
template <class T> inline void clear_mem(T *m, int s) noexcept { zero_mem(m,s); free_mem(m); }
template <class T> inline void copy_mem (T *_dst, const T *_src, int s) noexcept { if(_dst&&_src) memcpy(_dst, _src, range<T>(s)); }


template <class T> inline T*   get_mem(int s) noexcept { auto m = reinterpret_cast<T*>(malloc(range<T>(s))); INC_ALLOCED(m); return m; }
template <class T> inline T*   get_zmem(int s) noexcept { auto m = reinterpret_cast<T*>(calloc(range<T>(s), 1)); INC_ALLOCED(m); return m; }
template <class T> inline void set_new_mem(T *&m, int s) noexcept { m = reinterpret_cast<T*>(malloc(range<T>(s))); INC_ALLOCED(m); }
template <class T> inline void set_new_zmem(T *&m, int s) noexcept { m = reinterpret_cast<T*>(calloc(range<T>(s), 1)); INC_ALLOCED(m); }

template <class T> inline void reset_mem(T *&m, int s) noexcept {
    if(m == nullptr || __dmem_holder.isDmem(m)) {
        auto src = m;
        m = reinterpret_cast<T*>(realloc(m, range<T>(s))); INC_REALLOCED(src, m);
    }
    else
        std::cout << "reset_mem: bad block: " << m << std::endl;
}
template <class T> inline T*   reget_mem(T *m, int s) noexcept { reset_mem(m,s); return m; }
template <class T> inline void reset_zmem(T*&m, int s) noexcept { reset_mem(m, s); zero_mem(m,s); }
template <class T> inline T*   reget_zmem(T* m, int s) noexcept { reset_mem(m, s); zero_mem(m,s); return m; }
template <class T> inline void free_mem (T*&m) noexcept {
    if(__dmem_holder.isDmem(m)){
        free(m);
        DEC_ALLOCED(m);
        m = nullptr;
    }
    else
        std::cout << "free_mem: bad block: " << m << std::endl;
}

#else
#define INC_ALLOCED inc_alloced _
#define DEC_ALLOCED dec_alloced _
#define INC_REALLOCED(p) inc_realloced _(p)
template <class T> inline size_t range(int s) noexcept { return (s > 0) ? sizeof(T)*s : 0; }

template <class T> inline void set_mem(T *m, int value, int s) noexcept { memset(m, value, range<T>(s)); }
template <class T> inline void zero_mem (T *m, int s) noexcept { if(m) memset(m,0,range<T>(s)); }
template <class T> inline void clear_mem(T *m, int s) noexcept { zero_mem(m,s); free_mem(m); }
template <class T> inline void copy_mem (T *_dst, const T *_src, int s) noexcept { if(_dst&&_src) memcpy(_dst, _src, range<T>(s)); }

template <class T> inline T*   get_mem(int s) noexcept { INC_ALLOCED; return reinterpret_cast<T*>(malloc(range<T>(s))); }
template <class T> inline T*   get_zmem(int s) noexcept { INC_ALLOCED; return reinterpret_cast<T*>(calloc(range<T>(s), 1)); }
template <class T> inline void set_new_mem(T *&m, int s) noexcept { INC_ALLOCED; m = reinterpret_cast<T*>(malloc(range<T>(s))); }
template <class T> inline void set_new_zmem(T *&m, int s) noexcept { INC_ALLOCED; m = reinterpret_cast<T*>(calloc(range<T>(s), 1)); }

template <class T> inline void reset_mem(T *&m, int s) noexcept { INC_REALLOCED(m); m = reinterpret_cast<T*>(realloc(m, range<T>(s))); }
template <class T> inline T*   reget_mem(T *m, int s) noexcept { INC_REALLOCED(m); return reinterpret_cast<T*>(realloc(m,range<T>(s))); }
template <class T> inline void reset_zmem(T *&m, int s) noexcept { INC_REALLOCED(m); m = reinterpret_cast<T*>(realloc(m, range<T>(s))); zero_mem(m,s); }
template <class T> inline T*   reget_zmem(T *m, int s) noexcept { INC_REALLOCED(m); m = reinterpret_cast<T*>(realloc(m, range<T>(s))); zero_mem(m,s); return m; }

template <class T> inline void free_mem (T *&m) noexcept { if(m){ DEC_ALLOCED; free(m); m = nullptr; } }

#endif

#define DMEM_EXPAND_SAVE_CALL
//#define DMEM_REMOVE_SAVE_CALL
template <class T> inline T* expand_mem(T *m, int totalSize, int expandSize, int pos){
    if(m
        #ifdef DMEM_EXPAND_SAVE_CALL
            && expandSize < totalSize
            && expandSize > 0
            && totalSize > 0
            && pos > 0
            && pos < totalSize - expandSize
        #endif
            )
    {
        reset_mem(m, totalSize + expandSize);
        memmove(m + pos + expandSize,
                m + pos,
                range<T>(totalSize - expandSize)
                );
    }
    return m;
}
template <class T> inline T* remove_mem(T *m, int totalSize, int removeSize, int pos){
    if(m
        #ifdef DMEM_REMOVE_SAVE_CALL
            && removeSize < totalSize
            && removeSize > 0
            && totalSize > 0
            && pos > 0
            && pos < totalSize - removeSize
        #endif
            )
    {

//        std::cout << "remove_mem: " << "totalSize: " << totalSize << " removeSize: " << removeSize << " pos: " << pos << " move to: " << (m+pos)
//                  << " from: " << (m+pos+removeSize)
//                  << " size: " << range<T>(totalSize - (pos + removeSize))
//                  << std::endl;

        memmove(m + pos,
                m + pos + removeSize,
                range<T>(totalSize - (pos + removeSize))
                );
        if(totalSize-removeSize)
            reset_mem(m, totalSize - removeSize);
        else
            free_mem(m);
    }
    return m;
}
#undef DMEM_EXPAND_SAVE_CALL
#undef DMEM_REMOVE_SAVE_CALL
template <class T>
void placementNewMemMove(T *dst, const T *src, int n, int srcPlacementNew = 1)
{
    if(dst && src && dst != src && n > 0)
    {
        if(!std::is_trivial<T>::value)
        {
            ptrdiff_t direction = dst - src;
            if(direction > 0)
            {
                auto b_src = src;
                auto e_src = src + n;
                auto e_dst = dst + n;
                while(b_src!=e_src)
                {
                    --e_dst; --e_src;
                    new (e_dst) T(*e_src);
                    if(srcPlacementNew)
                        e_src->~T();
                    else
                        delete e_src;
                }
            }
            else
            {
                auto b_src = src;
                auto e_src = src + n;
                auto b_dst = dst;
                while(b_src!=e_src)
                {
                    new (b_dst) T(*b_src);
                    if(srcPlacementNew)
                        b_src->~T();
                    else
                        delete b_src;
                    ++b_dst; ++b_src;
                }
            }
        }
        else
        {
            memmove(dst, src, range<T>(n));
        }
    }
}
template <class T>
void placementNewMemCopy(T *dst, T *src, int n)
{
    if(dst && src && dst != src && n > 0)
    {
        if(!std::is_trivial<T>::value)
        {
            ptrdiff_t direction = dst - src;
            if(direction > 0)
            {
                auto b_src = src;
                auto e_src = src + n;
                auto e_dst = dst + n;
                while(b_src!=e_src)
                {
                    --e_dst; --e_src;
                    new (e_dst) T(*e_src);
                }
            }
            else
            {
                auto b_src = src;
                auto e_src = src + n;
                auto b_dst = dst;
                while(b_src!=e_src)
                {
                    new (b_dst) T(*b_src);
                    ++b_dst; ++b_src;
                }
            }
        }
        else
        {
            memcpy(dst, src, range<T>(n));
        }
    }
}
template <class T>
T* placementNewRealloc(T *src, int reallocSize, int sourceSize)
{
    T *dst = get_zmem<T>(reallocSize);
    auto b = src;
    auto e = src + sourceSize;
    auto b_dst = dst;
    while(b!=e)
    {
        new (b_dst) T(*b);
        b->~T();
        ++b_dst; ++b;
    }
    free_mem(src);
    return dst;
}
#endif // DMEM_H
