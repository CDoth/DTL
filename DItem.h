#ifndef DITEM_H
#define DITEM_H
#include "DMetaType.h"

#include <QDebug>

namespace DItems
{
    struct item_data
    {
        void* parent;
        void* next;
        bool is_shared;
        int index;
        item_data() : parent(nullptr), next(nullptr), is_shared(false), index(-1){}
    };


//------------------------------------------------------------------------------------------- owner item
    template <class T>
    class OwnerItem
    {
        typedef typename DMetaType<T>::main_t Type;
        typedef typename DMetaType<T>::reference_t Reference;
        typedef typename DMetaType<T>::pointer_t Pointer;
    public:
        OwnerItem() {}
        OwnerItem(const T&v) {_data.alloc(v);}
        operator Reference(){return _data.ref();}
        Pointer get_ptr(){return _data.ptr();}
        Reference get(){return _data.ref();}
    private:
        DMetaType<T> _data;
    };
    template <> class OwnerItem<void>;
//------------------------------------------------------------------------------------------- meta item with std::is_fundamental
    template <class T>
    class TypeWrapper
    {
    public:
        TypeWrapper() : value(T()) {}
        TypeWrapper(const T& v) : value(v){}
        TypeWrapper(T& v) : value(v) {}
//        TypeWrapper(const T&& v) {std::swap(value, v);}
//        TypeWrapper(T&& v) {std::swap(value, v);}

        inline T& operator=(const T& v) {value = v; return value;}
        inline T& operator=(T& v) {value = v;return value;}
        inline T& operator=(const T&& v) {std::swap(value, v);return value;}
        inline T& operator=(T&& v) {std::swap(value, v); return value;}

        inline operator T&(){return value;}
        inline T& operator+(const T& v){return value += v;}
        inline T& operator|(const T& v){return value |= v;}
        inline T& operator-=(const T& v){return value -= v;}
        inline T& operator-(const T& v){return value -= v;}
        inline T& operator~(){value = ~value;return value;}
        inline T& operator*=(const T& v){return value *= v;}
        inline T& operator*(const T& v){return value *= v;}
        inline T& operator/=(const T& v){return value /= v;}
        inline T& operator/(const T& v){return value /= v;}
        inline T& operator++(){return ++value;}
        inline T operator++(int){return value++;}
        inline T& operator--(){return --value;}
        inline T operator--(int){return value--;}
        inline T& operator&=(const T& v){return value &= v;}
        inline T& operator&(const T& v){return value &= v;}
        inline T& operator+=(const T& v){return value += v;}
        inline T& operator|=(const T& v){return value |= v;}
        inline bool operator<=(const T& v){return value <= v;}
        inline bool operator>=(const T& v){return value >= v;}
        inline bool operator!=(const T& v){return value != v;}
        inline bool operator>(const T& v){return value > v;}
        inline bool operator<(const T& v){return value < v;}
        inline bool operator==(const T& v){return value == v;}
        inline bool operator||(const bool& b){return (bool)value || b;}
        inline bool operator!(){return !value;}
        inline bool operator&&(const bool& b){return (bool)value && b;}
    private:
        T value;
    };
    template <class T>
    struct MetaType
    {
        typedef typename std::conditional<std::is_fundamental<T>::value || std::is_pointer<T>::value, TypeWrapper<T>, T>::type type;
    };

    template <class T>
    class MetaItem : public MetaType<T>::type
    {
    public:
        typedef typename MetaType<T>::type type;
        MetaItem() : type(){}
        MetaItem(const T&v) : type(v){}
    };

}
#endif // DITEM_H
