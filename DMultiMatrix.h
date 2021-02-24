#ifndef DMULTIMATRIX_H
#define DMULTIMATRIX_H
#include "DMatrix.h"
#include "DArray.h"

template <class T>
class DMultiMatrixData
{
public:
    DMultiMatrixData() : data(nullptr), data_size(0), width(0), height(0)
    {
    }
    DMultiMatrixData(int s, int _w, int _h) : data(nullptr), width(_w), height(_h)
    {
        if((data_size = width*height*s))
        {
            collection.reserve(s);

            set_mem(data, data_size);
            zero_mem(data, data_size);
            auto it = data;
            while(s--)
            {
                collection.mount(DMatrix<T>(width,height, it));
                it+=width*height;
            }
        }
    }
    DMultiMatrixData(const DMultiMatrixData& d)
    {
        if((data_size = d.data_size))
        {
            collection.reserve(d.collection.size());
            set_mem(data, data_size);
            zero_mem(data, data_size);
            auto it = data;
            auto src_mtx = d.collection.begin();
            auto src_e = d.collection.end();
            while(src_mtx != src_e)
            {
                collection.mount(DMatrix<T>(*src_mtx, it));
                it+=width*height; ++src_mtx;
            }
        }
    }
    ~DMultiMatrixData()
    {
        qDebug()<<"MultiMatrixData: destroy"<<this;
        free_mem(data);
    }

    typedef typename DArray<DMatrix<T>>::iterator iterator;
    typedef typename DArray<DMatrix<T>>::const_iterator const_iterator;
    typedef T* data_iterator;
    typedef const T* const_data_iterator;

    DArray<DMatrix<T>> collection;
    T*  data;
    int data_size;
    int width;
    int height;
};
template <class T>
class DMultiMatrix
{
public:
    DMultiMatrix()
    {
        DMultiMatrixData<T>* d = new DMultiMatrixData<T>();
        w = new DDualWatcher(d, CloneWatcher);
    }
    DMultiMatrix(int size, int width, int height)
    {
        DMultiMatrixData<T>* d = new DMultiMatrixData<T>(size, width, height);
        w = new DDualWatcher(d, CloneWatcher);
    }
    //----------------------------------------------------------------
    void swap(DMultiMatrix<T>& with)
    {
        std::swap(w, with.w);
    }
    DMultiMatrix(const DMultiMatrix& mm)
    {
        w = mm.w;
        w->refUp();
    }
    DMultiMatrix& operator=(const DMultiMatrix& a)
    {
        if(w != a.w)
        {
            DMultiMatrix<T> tmp(a);
            tmp.swap(*this);
        }
        return *this;
    }
    //----------------------------------------------------------------
    ~DMultiMatrix()
    {
        if( w->pull() == 0) {delete data(); delete w;}
    }

    int size() const {return this->get()->collection.size();}
    int width() const {return this->get()->width;}
    int height() const {return this->get()->height;}
    int matrix_size() const {return this->get()->width * this->get()->height;}

    DMatrix<T>& operator[](int i){detach();return data()->collection[i];}
    const DMatrix<T>& operator[](int i) const {return data()->collection[i];}

    typedef typename DMultiMatrixData<T>::iterator iterator;
    typedef typename DMultiMatrixData<T>::const_iterator const_iterator;
    typedef typename DMultiMatrixData<T>::data_iterator data_iterator;
    typedef typename DMultiMatrixData<T>::const_data_iterator const_data_iterator;

    iterator begin() {detach(); return data()->collection.begin();}
    iterator end() {detach(); return data()->collection.end();}
    data_iterator dataBegin() {detach(); return data()->data;}
    data_iterator dataEnd() {detach(); return data()->data + data()->data_size;}

    const_iterator constBegin() const {return data()->collection.constBegin();}
    const_iterator constEnd() const {data()->collection.constEnd();}
    const_data_iterator constDataBegin() const {return data()->data;}
    const_data_iterator constDataEnd() const {return data()->data + this->get()->data_size;}
private:

    DMultiMatrixData<T>* clone()
    {
        DMultiMatrixData<T>* d = new DMultiMatrixData<T>(*data());
    }
    void detach()
    {
        if(!w->is_unique())
        {
            if(w->is_share() && w->otherSideRefs()) w->disconnect(clone());
            else if(w->is_clone())
            {
                w->refDown();
                w = new DDualWatcher(clone(), CloneWatcher);
            }
        }
    }
    DMultiMatrixData<T>* data() {return reinterpret_cast<DMultiMatrixData<T>* >(w->d());}
    const DMultiMatrixData<T>* data() const {return reinterpret_cast<const DMultiMatrixData<T>* >(w->d());}
    DDualWatcher* w;
};

#endif // DMULTIMATRIX_H
