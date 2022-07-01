#ifndef DARRAY_H
#define DARRAY_H

#include "DWatcher.h"
#include "dmem.h"

struct __d_valid_type {};
class __d_invalid_type { __d_invalid_type() {}};

#define TEMPLATE_PERMISSION__INTEGRATE_TYPES \
    public std::conditional<std::is_integral<T>::value \
    || std::is_floating_point<T>::value \
    || std::is_pointer<T>::value, \
__d_valid_type, __d_invalid_type>::type

#define __INTEGRAL(_T_) (std::is_integral<_T_>::value)
#define __FLOATING(_T_) (std::is_floating_point<_T_>::value)
#define __POINTER(_T_) (std::is_pointer<_T_>::value)

#define __PERMISSIONS(X) public std::conditional<X, __d_valid_type, __d_invalid_type>::type
#define __PERMISSIONS__TRIV_INT(_T_) __PERMISSIONS(__INTEGRAL(_T_) || __FLOATING(_T_) || __POINTER(_T_))


enum DArrayMemManager {Direct, Undirect, Auto};
template <class T, DArrayMemManager memType>
class DArray;


template <class _T> struct __const_type {typedef _T const type;};
template <class _T> struct __const_type<_T*> {typedef _T const * type;};


template <class T>
class DTrivialArrayData : __PERMISSIONS__TRIV_INT(T) {
public:
    typedef T* iterator;
    typedef const T* const_iterator;
    typedef const typename __const_type<T>::type const_type;
    typedef T StoredType;
    typedef const T ConstStoredType;

    DTrivialArrayData() {
        data = nullptr;
        alloced = 0;
        size = 0;
    }
    DTrivialArrayData(const DTrivialArrayData &d) {
        data = nullptr;
        alloced = 0;
        size = 0;
        copy_array(d);
    }
    DTrivialArrayData(const T *d, int s) {
        data = nullptr;
        alloced = 0;
        size = 0;
        copy_data(d, s);
    }

    DTrivialArrayData& operator=(const DTrivialArrayData &d) { copy_array(d); return *this; }
    ~DTrivialArrayData() { __dropAll(); }
public:   // ===================== OPEN LAYER:
    // info:
    inline int available() const {return alloced - size;}
    inline int getSize() const {return size;}
    inline int getBytesSize() const {return size * sizeof(T);}
    inline int capacity() const {return alloced;}

    // mem management:
    void reserve(int s) {
        if(s > alloced) __reserve(s);
    }
    void reserveUp(int n) {
        reserve(alloced + n);
    }

    // copying data:
    void copy_array(const DTrivialArrayData &d) {

        reserve(d.size);
        size = d.size;
        copy_mem(data, d.data, d.size);

    }
    void copy_array(const DTrivialArrayData &d, int __size) {
        __size = __size > d.size ? d.size : __size;

        reserve(__size);
        size = __size;
        copy_mem(data, d.data, size);
    }
    void copy_data(const T *src, int srcSize) {

        reserve(srcSize);
        size = srcSize;
        copy_mem(data, src, srcSize);

    }
    void copy_tail(const DTrivialArrayData &d) {
        reserve(d.size());
        copy_mem(data + size, d.data + size, d.size - size);
        size = d.size();
    }
    void copy_tail(const DTrivialArrayData &d, int tailSize) {
        reserve(size + tailSize);
        copy_mem(data + size, d.data + size, tailSize);
        size += tailSize;
    }

    // adding one:
    void push() { __push(__getPlace()); }
    void push(const T &src) { __push(__getPlace(), src); }

    void insert_line(int insertTo, int n) {
        reserve(size + n);
        __moveObjects( (data + insertTo + n), (data + insertTo), (size - insertTo) ) ;
        zero_mem(data + insertTo, n);
        size += n;
    }
    void insert(int insertTo) {
        __reserveUp();
        __moveObjects( (data + insertTo + 1), (data + insertTo), (size - insertTo));
        __push(data + insertTo);
        ++size;
    }
    void insert(int insertTo, const T &src) {
        __reserveUp();
        __moveObjects( (data + insertTo + 1), (data + insertTo), (size - insertTo));
        __push(data + insertTo, src);
        ++size;
    }
    void insert_rough(T *insertTo, const T &src) {
        __reserveUp();
        __moveObjects( insertTo + 1, insertTo, (data + size) - insertTo);
        __push(insertTo, src);
        ++size;
    }

    // adding several:
    void push_array(const DTrivialArrayData &d) { __push_array(d); }
    void push_array(const DTrivialArrayData &d, int n) { __push_array(d, n);}
    void push_data(const T *src, int n) {

        if(available() < n) {
            reserveUp(n - available());
        }
        copy_mem(data + size, src, n);
        size += n;
    }
    void push_empty_line(int n) {
        reserve(size + n);
        zero_mem(data + size, n);
        size += n;
    }

    // removing:
    void remove_by_index(int i) {
        __moveObjects( (data + i),  (data + i + 1),  (size - i - 1) );
        --size;
    }
    void remove_last() {
        --size;
    }
    int remove(const_type &t) {
        int i = index_of(t);
        if(i != -1) remove_by_index(i);
        return i;
    }
    void drop() {
        zero_mem(data, size);
        size = 0;
    }
    void fdrop() {
        size = 0;
    }
    void drop_tail(int firstSaved) {
        zero_mem(data + firstSaved, size - firstSaved);
        size = firstSaved;
    }
    void fdrop_tail(int firstSaved) {
        size = firstSaved;
    }
    void cut(int cutBegin, int cutEnd) {
        __moveObjects(data + cutBegin, data + cutEnd, size - cutEnd);
        size -= (cutEnd-cutBegin);
    }

    // data access:
    inline T& ref(int i) {return __ref(i);}
    inline const T& ref(int i) const {return __ref(i);}
    inline T* ptr(int i) {return __ptr(i);}
    inline const T* ptr(int i) const {return __ptr(i);}

    inline T * changableData() {return data;}
    inline const T * constData() const {return data;}
    iterator begin() {return data;}
    const_iterator begin() const {return data;}
    iterator end() {return data + size;}
    const_iterator end() const {return data + size;}

    // analysis (const):
    int index_of(const_type &t, int from = 0, int to = 0) const {
        if(from > to)
            std::swap(from, to);
        to = to ? to : size;
        int i = 0;
        auto b = begin() + from;
        auto e = begin() + to;
        while(b!=e) {
            if(*b++ == t) {
                return i + from;
            }
            ++i;
        }
        return -1;
    }
    int count(const_type &t, int from = 0, int to = 0) const {
        if(from > to)
            std::swap(from, to);
        to = to ? to : size;
        int c = 0;
        auto b = begin() + from;
        auto e = begin() + to;
        while(b!=e) {
            if(*b++ == t) ++c;
        }
        return c;
    }
    bool contain(const_type &t, int from = 0, int to = 0) const {
        if(from > to)
            std::swap(from, to);
        to = to ? to : size;
        auto b = begin() + from;
        auto e = begin() + to;
        while(b!=e) {
            if(*b++ == t) return true;
        }
        return false;
    }

    // special:
    inline void swap(int pos1, int pos2) { std::swap(data[pos1], data[pos2]); }
    void relocate(int dstPos, int srcPos, int n) {

        DTrivialArrayData _d;
        _d._reserve(n);
        copy_mem(_d.data, data + srcPos, n);

        if(srcPos > dstPos) {
            move_mem(data + dstPos + n, data + dstPos, srcPos - dstPos);
        } else {
            move_mem(data + srcPos, data + srcPos + n, dstPos - srcPos);
        }
        copy_mem(data + dstPos, _d.data, n);
    }
    inline void sort() { __sort(); }

    // special with binary sorted data:
    int inner_bin_find(const_type &t, int from, int n) const {


        if(n == 0) return -1;
        if(n == 1) return t == ref(from) ? from : -1;

        const int local_pos = ((n-1) / 2) ;
        const int pos = from + local_pos;



        const T &value = ref(pos);

        if( t == value ) return pos;


        if (t > value) {
            return inner_bin_find(t, pos + 1, n - (local_pos + 1));
        } else {
            return inner_bin_find(t, from, local_pos);
        }
    }
    inline int bin_find(const_type &t) const { return inner_bin_find(t, 0, size); }
    int unique_insert_with_sort_pos(const_type &t, int from, int n) const {

        if(n == 0) return from;


        const int local_pos = ((n-1) / 2); // 0
        const int pos = from + local_pos; // 0

        const T &value = ref(pos);

        if( t == value ) return -1;

        if (t > value) {
            return unique_insert_with_sort_pos(t, pos + 1, n - (local_pos + 1));
        } else {
            return unique_insert_with_sort_pos(t, from, local_pos);
        }
    }
    int unique_insert_with_sort(const_type &t) {
        int pos = unique_insert_with_sort_pos(t, 0, size);
        if(pos < 0) return pos;
        if(pos == size) {
            push(t);
            return pos;
        }
        if(pos < size) {
            insert(pos, t);
            return pos;
        }
    }

private:  // ===================== UNDERLAYER:

    // mem management:
    void __reserve(int s) {
        data = reget_mem(data, s);
        alloced = s;
    }
    void __reserveUp() {
        if(alloced == size) {
            int s = ( size + 1 ) * 2;
            __reserve(s);
        }
    }
    T *__getPlace() {
        __reserveUp();
        return data + size++;
    }

    // adding one:
    void __push(T *place) { *place = T(); }
    void __push(T *place, const T &src) { *place = src; }

    // adding several:
    void __push_array(const DTrivialArrayData &d) {
        if(available() < d.size) {
            reserve(size + d.size);
        }
        copy_mem(data + size, d.data, d.size);
        size += d.size;
    }
    void __push_array(const DTrivialArrayData &d, int n) {
        if(available() < n) {
            reserve(size + n);
        }
        copy_mem(data + size, d.data, n);
        size += n;
    }

    // data access:
    inline T& __ref(int i) {return data[i];}
    inline const T& __ref(int i) const {return data[i];}
    inline T* __ptr(int i) {return data + i;}
    inline const T* __ptr(int i) const {return data + i;}
    inline void __set_value(int i, const T &t) { data[i] = t; }
    inline void __set_value(T *pos, const T &t) { *pos = t; }

    // destructing:
    void __dropAll() {
        free_mem(data);
        data = nullptr;
        alloced = 0;
        size = 0;
    }

    // special:
    void __moveObjects(T *dst, T *src, int n) {
        memmove(dst, src, range<T>(n));
    }
    void __sort() {
        std::qsort(
                    data,
                    size,
                    sizeof(T),
                    [](const void *x, const void *y) {
                        if(*reinterpret_cast<const T*>(x) > *reinterpret_cast<const T*>(y)) return 1;
                        if(*reinterpret_cast<const T*>(x) < *reinterpret_cast<const T*>(y)) return -1;
                        return 0;
                    }

                    );
    }

private:
    T *data;
    int alloced;
    int size;
};

template <class T, DArrayMemManager memType>
class DArrayData {
    enum {
        __trivial = std::is_trivial<T>::value,
        __trivial_copy = std::is_trivially_copyable<T>::value,
        __trivial_destruct = std::is_trivially_destructible<T>::value
    };
    struct _direct_placement_layer { enum {direct_placement = 1}; };
    struct _undirect_placement_layer { enum {direct_placement = 0}; };

    // undirect:
    // 1. sizeof(T) > sizeof(void*)
    // 2. sizeof(T) <= sizeof(void*) && (__trivial_copy == false) (__trivial == true/false)
    // 3. UndirectMode

    // direct:
    // 1. sizeof(T) <= sizeof(void*) && (__trivial_copy == true) (__trivial  == true)
    // 2. DirectMode (__trivial  == true)

    struct __metatype : std::conditional
    < (memType == Auto),
    // Auto mode:
        typename std::conditional

        < ( sizeof(T) > sizeof(void*) ),
//            // Large types -> undirect
            _undirect_placement_layer,
//            // Small types:
            typename std::conditional
            <__trivial_copy,
//                // trivially copyable -> direct or cpx direct
                _direct_placement_layer,
//                // no trivially copyable -> undirect
                _undirect_placement_layer
            >::type
        > ::type,

    // Specific modes:
        typename std::conditional
        <(memType == Direct),
            _direct_placement_layer, _undirect_placement_layer
        >::type

//    _undirect_placement_layer


    > ::type

//            _direct_placement_layer

    {};


    struct complex_iterator;
    struct const_complex_iterator;



    struct complex_iterator
    {
        typedef T* pointer_t;
        complex_iterator() : p(nullptr){}
        complex_iterator(T** _p) : p(_p){}
        inline pointer_t ptr() {return *p;}
        inline operator T**() {return p;}
        inline T& operator*() const {return **p;}
        inline T* operator->() const {return *p;}
        inline complex_iterator& operator++(){++p; return *this;}
        inline complex_iterator operator++(int){complex_iterator it(p); ++p; return it;}
        inline complex_iterator& operator--(){--p; return *this;}
        inline complex_iterator operator--(int){complex_iterator it(p); --p; return it;}
        inline  bool operator==(const complex_iterator& it) const noexcept { return p==it.p; }
        inline  bool operator!=(const complex_iterator& it) const noexcept { return p!=it.p; }
        inline  bool operator==(const const_complex_iterator& it) const noexcept { return p==it.p; }
        inline  bool operator!=(const const_complex_iterator& it) const noexcept { return p!=it.p; }
        inline complex_iterator operator+(int i) const {return complex_iterator(p+i);}
        inline complex_iterator operator-(int i) const {return complex_iterator(p-i);}

        T** p;
    };
    struct const_complex_iterator
    {
        typedef T*const pointer_t;
        const_complex_iterator() : p(nullptr){}
        const_complex_iterator(T*const* _p) : p(_p){}
        inline pointer_t ptr() {return *p;}
        inline operator T*const*() {return p;}
        inline const T& operator*() const {return **p;}
        inline const T* operator->() const {return *p;}
        inline const_complex_iterator& operator++(){++p; return *this;}
        inline const_complex_iterator operator++(int) {const_complex_iterator it(p); ++p; return it;}
        inline const_complex_iterator& operator--() {--p; return *this;}
        inline const_complex_iterator operator--(int){const_complex_iterator it(p); --p; return it;}
        inline bool operator==(const const_complex_iterator& it) const noexcept { return p==it.p;}
        inline bool operator!=(const const_complex_iterator& it) const noexcept { return p!=it.p;}
        inline const_complex_iterator operator+(int i) const {return const_complex_iterator(p+i);}
        inline const_complex_iterator operator-(int i) const {return const_complex_iterator(p-i);}
        T*const* p;
    };

public:
    const char *placement() const {
        if(__metatype::direct_placement) {
            return "Direct placement";
        } else {
            return "Undirect placement";
        }
    }
    void info() const {
        std::cout << "Type size: " << sizeof(T)
                  << " placement: " << placement() << " [" << (int)__metatype::direct_placement << "]"
                  << " trvial: " << __trivial
                  << " tr copy: " << __trivial_copy
                  << " tr destr: " << __trivial_destruct

                  << std::endl;
    }

    typedef typename std::conditional< (__metatype::direct_placement), T*, complex_iterator >::type iterator;
    typedef typename std::conditional< (__metatype::direct_placement), const T*, const_complex_iterator >::type const_iterator;
    typedef typename std::conditional< (__metatype::direct_placement), T, T*>::type StoredType;
    typedef typename std::conditional< (__metatype::direct_placement), const T, const T*>::type ConstStoredType;
    typedef StoredType* rawIterator;
    typedef ConstStoredType* constRawIterator;
    typedef const typename __const_type<T>::type const_type;


public:

    DArrayData() :
        data(nullptr),
        alloced(0),
        size(0)
    {
    }
    DArrayData(const DArrayData &d) :
    data(nullptr),
    alloced(0),
    size(0)
    {
        force_array(d);
    }

    DArrayData& operator=(const DArrayData &d) { _force_data(d); return *this;}
    DArrayData& operator=(DArrayData &d) { _force_data(d); return *this;}
    ~DArrayData() { __dropAll(); }
private:
    StoredType *data;
    int alloced;
    int size;
public:   // ===================== OPEN LAYER:

    // info:
    inline int available() const {return alloced - size;}
    inline int getSize() const {return size;}
    inline int getBytesSize() const {return size * sizeof(StoredType);}
    inline int capacity() const {return alloced;}

    // mem management:
    void reserve(int s) {
        if(s > alloced)
            __reserve(s, __metatype());
    }
    void reserveUp(int n) {
        reserve(alloced + n);
    }

    // copying data:
    void copy_array(const DArrayData &d) {
        if(__metatype::direct_placement && __trivial_copy) {
            copy_mem(data, d.data, std::min(size, d.size));
        } else {
            int s = std::min(size, d.size);
            auto b = data;
            auto e = data + s;
            auto b_src = d.data;
            while(b!=e) {
                _set_value(b++, *b_src++, __metatype());
            }
        }
    }
    void copy_data(const T *src, int srcSize) {

        reserve(srcSize);
        size = srcSize;

        if(__metatype::direct_placement && __trivial_copy) {
            copy_mem(data, src, srcSize);
        } else {
            int s = srcSize;
            auto b = data;
            auto e = data + s;
            auto b_src = src;
            while(b!=e) {
                __set_value(b++, *b_src++, __metatype());
            }
        }
    }
    void force_array(const DArrayData &d) {
        __dropAll();
        __reserve(d.size, __metatype());
        push_array(d);
        size = d.size;
    }
    void force_data(const T *src, int srcSize) {
        __dropAll();
        __reserve(srcSize, __metatype());
        _push_data_line(src, srcSize);
        size = srcSize;
    }

    // adding one:
    void push() {
        __push(__getPlace(), __metatype());
    }
    void push(const T &src) {
        __push(__getPlace(), src, __metatype());
    }
    void insert(int insertTo) {
        __reserveUp();
        __moveObjects( (data + insertTo + 1), (data + insertTo), (size - insertTo));
        __push(data + insertTo, __metatype());
    }
    void insert(int insertTo, const T &src) {
        __reserveUp();
        __moveObjects( (data + insertTo + 1), (data + insertTo), (size - insertTo));
        __push(data + insertTo, src, __metatype());
        ++size;
    }
    void insert_rough(T *insertTo, const T &src) {
        __reserveUp();
        __moveObjects( insertTo + 1, insertTo, (data + size) - insertTo);
        __push(insertTo, src, __metatype());
        ++size;
    }
    void insert_rough(complex_iterator insertTo, const T &src) {
        insert_rough(insertTo.p, src);
    }

    // adding several:
    void push_array(const DArrayData &d) {
        __push_array(d, __metatype());
    }
    void push_data(const T *src, int n) {
        if(__metatype::direct_placement && __trivial_copy && __trivial_destruct) {
            if(available() < n) {
                reserveUp(n - available());
            }
            copy_mem(data + size, src, n);
            size += n;
        } else {
            auto b = src;
            auto e = src + n;
            while(b!=e) {
                push(*b++);
            }
        }
    }
    void push_empty_line(int n) {
        reserve(size + n);
        if(__trivial) {
            size += n;
        } else {
            while(n--) push();
        }
    }

    // removing:
    void remove_by_index(int i) {
        __destruct(i, __metatype());
        __moveObjects( (data + i),  (data + i + 1),  (size - i - 1) );
        --size;
    }
    void remove_last() {
        __destruct(size-1, __metatype());
        --size;
    }
    int remove(const_type &t) {
        int i = index_of(t);
        if(i != -1) remove_by_index(i);
        return i;
    }
    void drop() {
        __destructAll(__metatype());
        zero_mem(data, size);
        size = 0;
    }
    void drop_tail(int firstSaved) {
        __destruct(firstSaved, size, __metatype());
        zero_mem(data + firstSaved, size - firstSaved);
        size = firstSaved;
    }
    void cut(int cutBegin, int cutEnd) {
        __destruct(cutBegin, cutEnd, __metatype());
        __moveObjects(data + cutBegin, data + cutEnd, size - cutEnd);
        size -= (cutEnd-cutBegin);
    }

    // data access:
    T& ref(int i) {return __ref(i, __metatype());}
    const T& ref(int i) const {return __ref(i, __metatype());}
    T* ptr(int i) {return __ptr(i, __metatype());}
    const T* ptr(int i) const {return __ptr(i, __metatype());}

    StoredType * changableData() {return data;}
    const StoredType * constData() const {return data;}
    iterator begin() {return data;}
    const_iterator begin() const {return data;}
    iterator end() {return data + size;}
    const_iterator end() const {return data + size;}

    // analysis (const):
    int index_of(const_type &t, int from = 0, int to = 0) const {
        if(from > to)
            std::swap(from, to);
        to = to ? to : size;
        int i = 0;
        auto b = begin() + from;
        auto e = begin() + to;
        while(b!=e) {
            if(*b++ == t) {
                return i + from;
            }
            ++i;
        }
        return -1;
    }
    int count(const T &t, int from = 0, int to = 0) const {
        if(from > to)
            std::swap(from, to);
        to = to ? to : size;
        int c = 0;
        auto b = begin() + from;
        auto e = begin() + to;
        while(b!=e) {
            if(*b++ == t) ++c;
        }
        return c;
    }
    bool contain(const T &t, int from = 0, int to = 0) const {
        if(from > to)
            std::swap(from, to);
        to = to ? to : size;
        auto b = begin() + from;
        auto e = begin() + to;
        while(b!=e) {
            if(*b++ == t) return true;
        }
        return false;
    }

    // special:
    inline void swap(int pos1, int pos2) { std::swap(data[pos1], data[pos2]); }
    void relocate(int dstPos, int srcPos, int n) {
        DArrayData _d;
        _d._reserve(n);
        placementNewMemCopy(_d.data, data + srcPos, n);
        if(srcPos > dstPos) {
            placementNewMemMove(data + dstPos + n, data + dstPos, srcPos - dstPos);
        } else {
            placementNewMemMove(data + srcPos, data + srcPos + n, dstPos - srcPos);
        }
        placementNewMemCopy(data + dstPos, _d.data, n);
    }
    void sort() {
        __sort(__metatype());
    }

    // special with binary sorted data:
    int inner_bin_find(const T &t, int from, int n) const {

        // 0: from == 0 n == 0
        // 1: [0]
        // 2: [0] [1]
        // 3: [0 1] [2]
        // 4: [0 1] [2 3]
        // 5: [0 1]  [2 3 4]
        // 6: [0 1 2] [3 4 5]

        // 0: from == 0 n == 0
        // 1: [7]
        // 2: [3] [11]
        // 3: [3 11] [24]
        // 4: [3 11] [24 32]
        // 5: [3 11]  [24 32 41]
        // 6: [3 11 24] [32 41 55]

        if(n == 0) return -1;
        if(n == 1) return t == ref(from) ? from : -1;

        const int local_pos = ((n-1) / 2) ;
        const int pos = from + local_pos;



        const T &value = ref(pos);

        if( t == value ) return pos;


        if (t > value) {
            return inner_bin_find(t, pos + 1, n - (local_pos + 1));
        } else {
            return inner_bin_find(t, from, local_pos);
        }
    }
    inline int bin_find(const T &t) const { return inner_bin_find(t, 0, size); }
    int unique_insert_with_sort_pos(const T &t, int from, int n) const {

        if(n == 0) return from;


        const int local_pos = ((n-1) / 2); // 0
        const int pos = from + local_pos; // 0

        const T &value = ref(pos);

        if( t == value ) return -1;

        if (t > value) {
            return unique_insert_with_sort_pos(t, pos + 1, n - (local_pos + 1));
        } else {
            return unique_insert_with_sort_pos(t, from, local_pos);
        }
    }
    int unique_insert_with_sort(const T &t) {
        int pos = unique_insert_with_sort_pos(t, 0, size);
        if(pos < 0) return pos;
        if(pos == size) {
            push(t);
            return pos;
        }
        if(pos < size) {
            insert(pos, t);
            return pos;
        }
    }

private:  // ===================== UNDERLAYER:

    // mem management:
    void __reserve(int s, _direct_placement_layer) {

        if(!__trivial_copy || !__trivial_destruct) {

            StoredType *p = get_zmem<StoredType>(s);
            StoredType *_p = p;
            auto b = data;
            auto e = data + size;
            while(b!=e) {
                new (_p) T(*b);
                b->~T();
                ++_p; ++b;
            }
            free_mem(data);
            data = p;
        } else {
            data = reget_mem(data, s);
        }
        alloced = s;
    }
    void __reserve(int s, _undirect_placement_layer) {
        data = reget_mem(data, s);
        alloced = s;
    }
    void __reserveUp() {
        if(alloced == size) {
            int s = ( size + 1 ) * 2;
            reserve(s);
        }
    }
    StoredType *__getPlace() {
        __reserveUp();
        return data + size++;
    }

    // adding one:
    void __push(StoredType *place, _direct_placement_layer) {
        if(__trivial) {
            *place = T();
        } else {
            new (place) T();
        }
    }
    void __push(StoredType *place, const T &src, _direct_placement_layer) {
        if(__trivial_copy) {
            *place = src;
        } else {
            new (place) T(src);
        }
    }
    void __push(StoredType *place, _undirect_placement_layer) {
        *place = new T;
    }
    void __push(StoredType *place, const T &src, _undirect_placement_layer) {
        *place = new T(src);
    }

    // adding several:
    void __push_array(const DArrayData &d, _direct_placement_layer) {

        if(__trivial_copy && __trivial_destruct) {
            if(available() < d.size) {
                reserve(d.size - available());
            }
            copy_mem(data + size, d.data, d.size);
            size += d.size;
        } else {
            auto b = d.data;
            auto e = d.data + d.size;
            while(b!=e) {
                push(*b++);
            }
        }
    }
    void __push_array(const DArrayData &d, _undirect_placement_layer) {

            auto b = d.data;
            auto e = d.data + d.size;
            while(b!=e) {
                push(*(*b++));
            }
    }

    // data access:
    T& __ref(int i, _direct_placement_layer) {return data[i];}
    T& __ref(int i, _undirect_placement_layer) {return *data[i];}
    const T& __ref(int i, _direct_placement_layer) const {return data[i];}
    const T& __ref(int i, _undirect_placement_layer) const {return *data[i];}
    T* __ptr(int i, _direct_placement_layer) {return data + i;}
    T* __ptr(int i, _undirect_placement_layer) {return data[i];}
    const T* __ptr(int i, _direct_placement_layer) const {return data + i;}
    const T* __ptr(int i, _undirect_placement_layer) const {return data[i];}

    void __set_value(int i, const T &t, _direct_placement_layer) { data[i] = t; }
    void __set_value(int i, const T &t, _undirect_placement_layer) { *data[i] = t;}
    void __set_value(StoredType *pos, const T &t, _direct_placement_layer) { *pos = t; }
    void __set_value(StoredType *pos, const T &t, _undirect_placement_layer) { **pos = t; }

    // destructing:
    void __destruct(int i, _direct_placement_layer) {
        if(!__trivial_destruct) {
            data[i].~T();
        }
    }
    void __destruct(int i, _undirect_placement_layer) {
        delete data[i];
    }
    void __destruct(int begin, int end, _direct_placement_layer) {
        if(!__trivial_destruct) {
            auto b = data + begin;
            auto e = data + end;
            while(b!=e) {(--e)->~T();}
        }
    }
    void __destruct(int begin, int end, _undirect_placement_layer) {
        auto b = data + begin;
        auto e = data + end;
        while(b!=e) {delete *(--e);}
    }
    void __destructAll(_direct_placement_layer) {
        if(!__trivial_destruct) {
            auto b = data;
            auto e = data + size;
            while(b!=e) {(--e)->~T();}
        }
    }
    void __destructAll(_undirect_placement_layer) {
        auto b = data;
        auto e = data + size;
        while(b!=e) {delete *(--e);}
    }
    void __dropAll() {
        __destructAll(__metatype());
        free_mem(data);
        data = nullptr;
        alloced = 0;
        size = 0;
    }

    // special:
    void __moveObjects(StoredType *dst, StoredType *src, int n) {
        placementNewMemMove(dst, src, n);
    }
    void __sort(_direct_placement_layer) {
        std::qsort(
                    data,
                    size,
                    sizeof(StoredType),
                    [](const void *x, const void *y) {
                        if(*reinterpret_cast<const StoredType*>(x) > *reinterpret_cast<const StoredType*>(y)) return 1;
                        if(*reinterpret_cast<const StoredType*>(x) < *reinterpret_cast<const StoredType*>(y)) return -1;
                        return 0;
                    }

                    );
    }
    void __sort(_undirect_placement_layer) {
        std::qsort(
                    data,
                    size,
                    sizeof(StoredType),
                    [](const void *x, const void *y) {
                        if(**reinterpret_cast<const StoredType*const*>(x) > **reinterpret_cast<const StoredType*const*>(y)) return 1;
                        if(**reinterpret_cast<const StoredType*const*>(x) < **reinterpret_cast<const StoredType*const*>(y)) return -1;
                        return 0;
                    }

                    );
    }
};



template <class T, DArrayMemManager memType = Auto>
class DArray :
        public DWatcher
        <

            typename

            std::conditional<

                __INTEGRAL(T) || __FLOATING(T) || __POINTER(T),

                DTrivialArrayData<T>, DArrayData<T, memType>

            >::type

        >
{
public:
    typedef
    typename

    std::conditional<

        __INTEGRAL(T) || __FLOATING(T) || __POINTER(T),

        DTrivialArrayData<T>, DArrayData<T, memType>

    >::type
            __DATA_T;

    typedef DWatcher<__DATA_T> target_type;





#define DATA (reinterpret_cast<__DATA_T*>(this->data()))
#define CDATA (reinterpret_cast<const __DATA_T*>(this->data()))

    DArray() : target_type(true) { //+

        reinterpret_cast<__DATA_T*>(

        this->data()

                    )
                ;

    }
    explicit DArray(int n) {
        this->hold(new __DATA_T);
        appendNs(n);
    }
    void clear() { //+
        detach();
        *this = DArray();
    }
    void drop() {
        detach();
        DATA->fdrop();
    }
    using iterator = typename __DATA_T::iterator;
    using const_iterator = typename __DATA_T::const_iterator;
    typedef const typename __const_type<T>::type const_type;

    // undirect
    // direct + full trivial
    // direct + complex + trivial copy
    // direct + full complex

    inline bool empty() const {return CDATA->getSize() == 0;} //+, +
    inline int size() const { return CDATA->getSize(); } //+, +
    inline int bytesSize() const { return CDATA->getBytesSize();}
    inline int capacity() const {return CDATA->capacity();} //+, +
    inline int available() const {return CDATA->available();} //+, +



    typename __DATA_T::StoredType * changableData() {return DATA->changableData();}
    const typename __DATA_T::StoredType * constData() const {return CDATA->constData();}

    iterator begin() { detach(); return DATA->begin(); } //+, +, +
    iterator end() { detach(); return DATA->end(); } //+, +, +
    iterator first() { detach(); return DATA->begin(); } //+
    iterator last() { detach(); return DATA->end() - 1; } //+

    const_iterator constBegin() const { return CDATA->begin(); } //+
    const_iterator constEnd() const { return CDATA->end(); } //+
    const_iterator constFirst() const { return CDATA->begin(); } //+
    const_iterator constLast() const { return CDATA->end() - 1; } //+

    inline bool inRange(int index) const {return (index > -1 && index < size());}
    T * ptr(int i) {return DATA->ptr(i);}
    const T * ptr(int i) const {return CDATA->ptr(i);}

    T * atPtr(int i) {return inRange ? DATA->ptr(i) : nullptr;}
    const T * atPtr(int i) const {return inRange ? CDATA->ptr(i) : nullptr;}

    inline T& operator[](int i) { detach(); return DATA->ref(i); } //+, +, +
    inline const T& operator[](int i) const { return CDATA->ref(i); } //+, +, +

    T& front() {detach(); return DATA->ref(0);} //+
    T& back() {detach(); return DATA->ref(size() - 1);} //+
    const T& constFront() const {return CDATA->ref(0);} //+
    const T& constBack() const {return CDATA->ref(size() - 1);} //+

    int indexOf(const_type &t) const { return CDATA->index_of(t); } //+, +
    int indexOfWithin(const T &t, int from, int to) const { return CDATA->index_of(t, from, to); } //+, +
    int count(const T &t) const { return CDATA->count(t); } //+
    int countWithin(const T &t, int from, int to) const { return CDATA->count(t, from, to); } //+
    bool contain(const T &t) const { return CDATA->contain(t); } //+
    bool containWithin(const T &t, int from, int to) const {return CDATA->contain(t, from, to);} //+



    void append() { //+, +, +
        detach();
        DATA->push();
    }
    void append(const T &t) { //+, +, +
        detach();
        DATA->push(t);
    }
    void append(const T &t, int n) { //+, +, +
        if(n > 0) {
            detach();
            while(n--) {
                DATA->push(t);
            }
        }
    }
    void appendNs(int n) { //+, +, +
        if(n > 0) {
            detach();
            DATA->push_empty_line(n);
        }
    }
    void appendArray(const DArray<T> &a) {
        detach();
        DATA->push_array(*a.data());
    }
    void appendArray(const DArray<T> &a, int n) {
        DATA->push_array(*a.data(), n);
    }
    void appendData(const T *src, int size) {
        detach();
        DATA->push_data(src, size);
    }
    int unique_insert_pos(const T &t) const {
        return CDATA->unique_insert_with_sort_pos(t, 0, size());
    }
    int unique_sorted_insert(const T &t) {
        detach();
        return DATA->unique_insert_with_sort(t);
    }

    int indexOf_bin(const T &t) const { return CDATA->bin_find(t); }
    int indexOfWithin_bin(const T &t, int from, int to) const {  return CDATA->inner_bin_find(t, from, to - from); }
    bool contain_bin(const T &t) const {return CDATA->bin_find(t) == -1 ? false : true; }
    bool containWithin_bin(const T &t, int from, int to) const {return CDATA->inner_bin_find(t, from, to - from) == -1 ? false : true; }

    void insert(int pos, const T &t) { //+, +
        detach();
        DATA->insert(pos, t);
    }
    void insertBefore(const T &t, const T &beforeThis) { //+, +
        int i = indexOf(beforeThis);
        if(i != -1) insert(i, t);
    }
    void fill(const T &t) { //+, +
        detach();
        auto b = begin();
        auto e = end();
        while(b!=e)  *b++ = t;
    }
    void sort() {
        detach();
        DATA->sort();
    }

#define RUN2(X) \
    auto b = begin(); \
    auto e = end(); \
    while(b!=e) {*(b++) = X();}
#define RUN(X) \
    auto b = begin(); \
    auto e = end(); \
    while(b!=e) { X(*b++);}
#define IRUN(X) \
    auto b = begin(); \
    auto e = end(); \
    int i = 0; \
    while(b!=e) { X(*b++, i++);}
#define CRUN(X) \
    auto b = constBegin(); \
    auto e = constEnd(); \
    while(b!=e) { X(*b++);}
#define ICRUN(X) \
    auto b = constBegin(); \
    auto e = constEnd(); \
    int i = 0; \
    while(b!=e) { X(*b++, i++);}

    void runIn(T (*cb)()) {RUN2(cb);}
    void runIn(void (*cb)(T&)) {RUN(cb);}
    void runIn(void (*cb)(T&, int)) {IRUN(cb);} //+, +
    void runOut(void (*cb)(const T&)) const {CRUN(cb);}
    void runOut(void (*cb)(const T&, int)) {ICRUN(cb);}

    void removeByIndex(int i) { //+, +
        detach();
        DATA->remove_by_index(i);
    }
    int remove(const_type &t) { //+, +
        detach();
        return DATA->remove(t);
    }
    void removeFirst() { //+, +
        detach();
        removeByIndex(0);
    }
    void removeLast() { //+, +
        detach();
        DATA->remove_last();
    }

    void push_back() {append();} //+, +, +
    void push_back(const T &t) {append(t);} //+, +, +
    void push_front(const T &t) {insert(0, t);} //+, +, +
    void pop_back() {removeLast();} //+, +, +
    void pop_front() {removeFirst();} //+, +, +

    void reform(int n) { //+, +, +
        int s = size();
        if(n > s) {
            appendNs(n - s);
        } else if(n < s) {
            cutEnd(s - n);
        }
    }
    void reformUp(int n) {
        int s = size();
        if(n > s) appendNs(n-s);
    }
    void reformDown(int n) {
        if( n < size() )
            DATA->fdrop_tail(n);
    }
    void cut(int start, int end) { //+, +
        if(size() && start >= 0 && start < size() && end >= 0 && end < size()) {
            detach();
            if(start == end) {
                removeByIndex(start);
            } else {
                if(start > end)
                    std::swap(start, end);
                DATA->cut(start, end);
            }
        }
    }
    void cutBegin(int n) { //+, +
        if(n > size() || n < 0) return;
        detach();
        if(n == 1) return removeFirst();
        DATA->cut(0, n);
    }
    void cutEnd(int n) { //+, +
        if(n > size() || n < 0) return;
        if(n == 1) return removeLast();
        DATA->drop_tail(size() - n);
    }

    int reserve(int n) { //+, +
        detach();
        DATA->reserve(n);
        return capacity();
    }

    void swap(DArray &a) {this->swap(a.w);}
    void elementsSwap(int f, int s) { //+, +
        if(f < 0 || f >= size() || s < 0 || s >= size()) return;
        detach();
        DATA->swap(f, s);
    }

    void shiftPart(int from, int n) {
        detach();
        DATA->insert_line(from, n);
    }
    void movePart(int dstPos, int srcPos, int n) { //+, +
        if(srcPos < 0 || dstPos < 0 || srcPos == dstPos || n <= 0 || srcPos > size() - n || dstPos > size() - n)
            return;
        detach();
        DATA->relocate(dstPos, srcPos, n);
    }
    void moveElement(int dstPos, int srcPos) { //+, +
        if(srcPos < 0 || dstPos < 0 || srcPos == dstPos || srcPos > size() || dstPos > size())
            return;
        detach();
        DATA->relocate(dstPos, srcPos, 1);
    }
    void moveElementToEnd(int pos) { //+, +
        if(pos < 0 || pos > size() || pos == size()-1)
            return;
        detach();
        DATA->relocate(size()-1, pos, 1);
    }
    void moveElementToBegin(int pos) { //+, +
        if(pos < 0 || pos > size() || pos == 0)
            return;
        detach();
        DATA->relocate(0, pos, 1);
    }

    void copyArray(const DArray &a) {
        detach();
        DATA->copy_array(*a.data());
    }
    void copyArray(const DArray &a, int n) {
        detach();
        DATA->copy_array(*a.data(), n);
    }
    void copyData(const T *src, int size) {
        detach();
        DATA->copy_data(src, size);
    }
    void copyTail(const DArray &a) {
        if( a.size() > size() ) {
            detach();
            DATA->copy_tail(*a.data());
        }
    }
    void copyTail(const DArray &a, int tailSize) {
        detach();
        DATA->copy_tail(*a.data(), tailSize);
    }
    void force(const DArray &a) {
        detach();
        DATA->force_array(*a.w.data());
    }
    void force(const T *src, int size) {
        detach();
        DATA->force_data(src, size);
    }

private:
    void detach() {
        target_type::detach();

    }
};





/*
 test block
    DArray<int> v(5);
    DArray<int> v2 = v;
//    v.setMode(ShareWatcher);
//    v2.setMode(ShareWatcher);
    v[0] = 111;
    v[1] = 222;
    v[2] = 333;
    v[3] = 444;
    v[4] = 555;
    v2.append(818);
    v.insert(777, 1);
    v.swap(v2);
    v.push_back(666);

    printf("v: size: %d reserved: %d available: %d cell_size: %d\n", v.size(), v.reserved(), v.available(), v.cell_size());
    printf("v2: size: %d reserved: %d available: %d cell_size: %d\n", v2.size(), v2.reserved(), v2.available(), v2.cell_size());
//    printf("empty: %d type_size_name: %s type_signature_name: %s\n", v.empty(), v.type_size_name(), v.type_signature_name());
    auto b = v.begin();
    auto e = v.end();
    while( b!= e ) printf("v: %d\n", *b++);
    for(int i=0;i!=v2.size();++i) printf("v2: %d\n", v2[i]);
    printf("front: %d back:%d\n", v.front(), v.back());
    printf("first: %d last: %d\n", *v.first(), *v.last());

    v2 = v1;
    v1.setMode(ShareWatcher);
    v2.setMode(ShareWatcher);
    v1[0] = 111;
    v1[1] = 222;
//    v1.setMode(CloneWatcher);
//    v2.setMode(CloneWatcher);
//    v2.make_unique();
    v2[0] = 333;


    qDebug()<<"v1 watcher:";
    qDebug()<<v1.w<<v1.w->mode()<<"refs:"<<v1.w->refs()<<"is clone:"<<v1.w->is_clone()<<"is share:"<<v1.w->is_share()<<"is unique:"<<v1.w->is_unique()
           <<"is other side:"<<v1.w->is_otherSide()<<"other side refs:"<<v1.w->otherSideRefs();
    qDebug()<<"v2 watcher:";
    qDebug()<<v2.w<<v1.w->mode()<<"refs:"<<v2.w->refs()<<"is clone:"<<v2.w->is_clone()<<"is share:"<<v2.w->is_share()<<"is unique:"<<v2.w->is_unique()
           <<"is other side:"<<v2.w->is_otherSide()<<"other side refs:"<<v2.w->otherSideRefs();

    auto b = v1.begin();
    auto e = v1.end();
    while( b != e ) printf("v1 value: %d\n", *b++);
    qDebug()<<"---------";
    b = v2.begin();
    e = v2.end();
    while( b != e ) printf("v2 value: %d\n", *b++);
 * */
#endif // DARRAY_H
