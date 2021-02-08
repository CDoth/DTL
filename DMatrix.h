#ifndef DMATRIX_H
#define DMATRIX_H
#include <DHolder.h>
#include <QDebug>

template <class T>
class DMatrixData
{
public:
    int size;
    int w;
    int h;
    T* data;
    int placed;
    void freeData()
    {
        if(data && !placed)
        {
            qDebug()<<"DMatrixData: freeData: can free"<<this;
            free(data);
        }
    }
    void clear()
    {
        placed = 0;
        data = nullptr;
    }
    void rep_alloc(int _size, T* to = nullptr)
    {
//        qDebug()<<"DMatrixData: rep_alloc"<<this;
        if(_size <= 0) return;
        freeData();
        data = to? placed = 1, to: (T*)malloc(sizeof(T)*size);
        memset(data, 0, sizeof(T)*size);
    }
    DMatrixData()
    {
        qDebug()<<"DMatrixData: create empty"<<this;
        clear();
    }
    DMatrixData(int _w, int _h, T* to = nullptr)
    {
        qDebug()<<"DMatrixData: create with size:"<<_w<<_h<<this;
        clear();
        w = _w;
        h = _h;
        size = w*h;
        rep_alloc(size, to);
    }
    DMatrixData(const DMatrixData& r, T* to = nullptr)
    {
        qDebug()<<"DMatrixData: copy from:"<<&r<<"to:"<<this<<"place:"<<to;
        clear();
        w = r.w;
        h = r.h;
        size = r.size;
        rep_alloc(size, to);
        memcpy(data, r.data, sizeof(T)*size);
    }
    ~DMatrixData()
    {
        qDebug()<<"DMatrixData: destroy"<<this;
        freeData();
        clear();
    }
};
template <class T>
class DMatrix : public DHolder<DMatrixData<T>>
{
public:
    DMatrix();
    DMatrix(int w, int h, T* place = nullptr);
    DMatrix(const DMatrix&, T*);

    ~DMatrix();

    typedef T* iterator;
    typedef const T* const_iterator;

    iterator begin();
    iterator end();
    T* operator[](int i);

    const_iterator constBegin() const;
    const_iterator constEnd() const;
    const T* operator[](int i) const;

    void fill(const T&);
    void run(void (*F)(T&));
    void run_to(const T& (*F)(const T&), DMatrix* to);
    void copy(const DMatrix& from);
    void unite(const DMatrix<T>& with);

    int height() const {return d->h;}
    int width()  const {return d->w;}
    int area()   const {return d->size;}

    void move_data(T* place);
private:
    void rep_detach(){d = this->try_detach();}
    DMatrixData<T>* d;
};
template <class T>
void DMatrix<T>::move_data(T* place)
{
    DMatrixData<T> *md = new DMatrixData<T>(*d, place);
    this->set(md,true);
}
template <class T>
DMatrix<T>::DMatrix() : DHolder<DMatrixData<T>>()
{
    d = this->alloc();
}
template <class T>
DMatrix<T>::DMatrix(int w, int h, T* place) : DHolder<DMatrixData<T>>()
{
    d = this->alloc(w,h,place);
}
template <class T>
DMatrix<T>::DMatrix(const DMatrix<T>& m, T* place) : DHolder<DMatrixData<T>>()
{
    d = this->alloc(*m.d, place);
}
template <class T>
void DMatrix<T>::fill(const T& v)
{
    iterator b = begin();
    iterator e = end();
    while(b != e) *b++ = v;
}
template <class T>
void DMatrix<T>::run(void (*F)(T&))
{
    iterator it = begin();
    while(it != end()) F(*it++);
}
template <class T>
void DMatrix<T>::run_to(const T& (*F)(const T&), DMatrix* to)
{
    iterator it = begin();
    iterator it_to = to->begin();
    while(it != end()) *it_to++ = F(*it++);
}
//---------------------------------------------------------------------------
template <class T>
typename DMatrix<T>::iterator DMatrix<T>::begin()
{
    rep_detach();
    return d->data;
}
template <class T>
typename DMatrix<T>::iterator DMatrix<T>::end()
{
    rep_detach();
    return d->data + d->size;
}
template <class T>
T* DMatrix<T>::operator[](int i)
{
    rep_detach();
    return &d->data[i*d->h];
}
template <class T>
typename DMatrix<T>::const_iterator DMatrix<T>::constBegin() const
{
    return d->data;
}
template <class T>
typename DMatrix<T>::const_iterator DMatrix<T>::constEnd() const
{
    return d->data + d->size;
}
template <class T>
const T* DMatrix<T>::operator[](int i) const
{
    return &d->data[i*d->h];
}
//---------------------------------------------------------------------------
template <class T>
void DMatrix<T>::unite(const DMatrix<T>& with)
{
    iterator b = begin();
    iterator e = end();
    const_iterator b2 = with.constBegin();
    while(b!=e) *b++ += *b2;
}
template <class T>
void DMatrix<T>::copy(const DMatrix& from)
{
    rep_detach();
    memcpy(d->data, from.inner->data, sizeof(T)*d->size);
}
template <class T>
DMatrix<T>::~DMatrix<T>()
{
}

#endif // DMATRIX_H
