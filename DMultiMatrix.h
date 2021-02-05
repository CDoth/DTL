#ifndef DMULTIMATRIX_H
#define DMULTIMATRIX_H
#include "DMatrix.h"
#include "DArray.h"

template <class T>
class DMultiMatrixData
{
public:
    int w;
    int h;
    int size;
    DArray<DMatrix<T>> collection;

    typedef T* data_iterator;
    typedef const T* const_data_iterator;
    typedef typename DArray<DMatrix<T>>::iterator iterator;
    typedef typename DArray<DMatrix<T>>::const_iterator const_iterator;
    iterator begin;
    iterator end;
    data_iterator d_begin;
    data_iterator d_end;


    DMultiMatrixData()
    {
        clear();
    }
    DMultiMatrixData(int _size, int _w, int _h)
    {
        clear();
        w = _w;
        h = _h;
        size = _size;
        int area = w*h;
        int total = area * size;

        place = (T*)malloc(sizeof(T) * total);
        d_begin = place;
        d_end = place + total;

        collection.reserve(size);
        for(auto it = d_begin; it != d_end; it += area)
        {
            collection.mount(DMatrix<T>(w, h, it));
        }

        begin = collection.begin();
        end = collection.end();
    }
    DMultiMatrixData(const DMultiMatrixData& mm)
    {
        qDebug()<<"mm copy";
        clear();
        w = mm.w;
        h = mm.h;
        size = mm.size;
        int area = w*h;
        int total = area * size;
        place = (T*)malloc(sizeof(T) * total);
        d_begin = place;
        d_end = place + total;
        collection.reserve(size);
        auto out_it = mm.collection.begin();
        for(auto it = d_begin; it != d_end; it += area) collection.mount(DMatrix<T>(*out_it++, it));
    }
    ~DMultiMatrixData()
    {
        if(place) free(place), place = nullptr;
    }

private:
    void clear()
    {
        place = nullptr;
        d_begin = nullptr;
        d_end = nullptr;
    }
    T* place;
};

template <class T>
class DMultiMatrix : public DHolder<DMultiMatrixData<T>>
{
public:
    DMultiMatrix() :  DHolder<DMultiMatrixData<T>>(), d(this->alloc()) {}
    DMultiMatrix(int size, int w, int h) :  DHolder<DMultiMatrixData<T>>(), d(this->alloc(size, w, h)){}
    void allocate(int size, int w, int h){data_detach(); d = this->alloc(size, w, h);}
    int size() const {return d->size;}
    int width() const {return d->w;}
    int height() const {return d->h;}
    int matrix_size() const {return d->w * d->h;}

    typedef typename DMultiMatrixData<T>::iterator iterator;
    typedef typename DMultiMatrixData<T>::const_iterator const_iterator;
    typedef typename DMultiMatrixData<T>::data_iterator data_iterator;
    typedef typename DMultiMatrixData<T>::const_data_iterator const_data_iterator;

    DMatrix<T>& operator[](int i) {data_detach(); return d->collection[i];}
    const DMatrix<T>& operator[](int i) const {return d->collection[i];}

    iterator begin(){data_detach(); return d->begin;}
    iterator end(){data_detach(); return d->end;}
    const_iterator constBegin() {return d->begin;}
    const_iterator constEnd() {return d->end;}

    data_iterator data_begin() {data_detach(); return d->d_begin;}
    data_iterator data_end() {data_detach(); return d->d_end;}
    const_data_iterator constDataBegin() {return d->d_begin;}
    const_data_iterator constDataEnd() {return d->d_end;}

private:
    void data_detach(){d = this->try_detach();}
    DMultiMatrixData<T>* d;
};

#endif // DMULTIMATRIX_H
