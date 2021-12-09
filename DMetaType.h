#ifndef DMETATYPE_H
#define DMETATYPE_H
#include <algorithm>
#include <iostream>

template <class T>
class DMetaType
{
    struct Pointer
    {
        typedef T type;
        typedef T pointer;
        typedef T reference;
    };
    struct Large
    {
        typedef T* type;
        typedef T* pointer;
        typedef T& reference;
    };
    struct Small
    {
        typedef T type;
        typedef T* pointer;
        typedef T& reference;
    };

    struct Layout :  std::conditional< std::is_pointer<T>::value, Pointer,
                                                                 typename std::conditional< (sizeof(T) > sizeof(void*)), Large, Small >::type>::type{};

public:
    typedef typename Layout::type main_t;
    typedef typename Layout::pointer pointer_t;
    typedef typename Layout::reference reference_t;

    DMetaType(){init();}
    DMetaType(const T&v){alloc(v);}
    ~DMetaType(){_freeData(Layout());}

    void init(){_init(Layout());}
    void alloc(const T&v){_alloc(v, Layout());}
    reference_t ref(){return _get_r(Layout());}
    pointer_t ptr(){return _get_p(Layout());}

    DMetaType(const DMetaType& f){_copy(f.data, Layout());}

private:
    void _init(Pointer){data = nullptr;}
    void _init(Large){data = new T;}
    void _init(Small){}

    void _alloc(const T&v, Pointer) {data = v;}
    void _alloc(const T&v, Large)   {data = new T(v);}
    void _alloc(const T&v, Small)   {data = v;}

    reference_t _get_r(Pointer){return data;}
    reference_t _get_r(Large)  {return *data;}
    reference_t _get_r(Small)  {return data;}

    pointer_t _get_p(Pointer){return data;}
    pointer_t _get_p(Large)  {return data;}
    pointer_t _get_p(Small)  {return &data;}

    void _freeData(Pointer){data = nullptr;}
    void _freeData(Large){if(data) delete data; data = nullptr;}
    void _freeData(Small){}

    void _copy(main_t from, Pointer){data = from;}
    void _copy(main_t from, Large){data = new T(*from);}
    void _copy(main_t from, Small){data = from;}

private:
    main_t data;
};
template <> class DMetaType<void>;



/*
const int limit = 1;
int unroll_counter = 0;
template <class Single>
bool unrollPack(Single s)
{
    std::cout << "unrollPack: base " << s << " counter: " << unroll_counter++ << std::endl;
    if(unroll_counter < limit)
        return true;
    return false;
}
template <class FirstObject, class ... Args>
bool unrollPack(FirstObject f, Args ... a)
{
    std::cout << "unrollPack: hub" << std::endl;
    if(unrollPack(f))
        unrollPack(a...);
    return true;
}
*/
#endif // DMETATYPE_H
