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
        if(data && !placed) free(data);
    }
    void clear()
    {
        placed = 0;
        data = nullptr;
    }
    void rep_alloc(int _size, T* to = nullptr)
    {
        if(_size <= 0) return;
        freeData();
        data = to? placed = 1, to: (T*)malloc(sizeof(T)*size);
        memset(data, 0, sizeof(T)*size);
    }
    DMatrixData()
    {
        clear();
    }
    DMatrixData(int _w, int _h, T* to = nullptr)
    {
        clear();
        w = _w;
        h = _h;
        size = w*h;
        rep_alloc(size, to);
    }
    DMatrixData(const DMatrixData& r, T* to = nullptr)
    {
        clear();
        w = r.w;
        h = r.h;
        size = r.size;
        rep_alloc(size, to);
        memcpy(data, r.data, sizeof(T)*size);
    }
    ~DMatrixData()
    {
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
    void alloc_matrix(int size, T* to = nullptr);
    void clone();
    void unite(const DMatrix<T>& with);

    int height() const {return rep->h;}
    int width()  const {return rep->w;}
    int area()   const {return rep->size;}
private:
    void rep_detach(){rep = this->try_detach();}
    DMatrixData<T>* rep;
};

template <class T>
DMatrix<T>::DMatrix() : DHolder<DMatrixData<T> > ()
{
    rep = this->alloc();
}
template <class T>
DMatrix<T>::DMatrix(int w, int h, T* place) : DHolder<DMatrixData<T> > ()
{
    rep = this->alloc(w,h, place);
}
template <class T>
DMatrix<T>::DMatrix(const DMatrix<T>& m, T* place) : DHolder<DMatrixData<T> > ()
{
    rep = this->alloc(m.width(), m.height(), place);
}
template <class T>
void DMatrix<T>::alloc_matrix(int size, T* to)
{
    rep_detach();
    rep->rep_alloc(size, to);
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
    return rep->data;
}
template <class T>
typename DMatrix<T>::iterator DMatrix<T>::end()
{
    rep_detach();
    return rep->data + rep->size;
}
template <class T>
T* DMatrix<T>::operator[](int i)
{
    rep_detach();
    return &rep->data[i*rep->h];
}
template <class T>
typename DMatrix<T>::const_iterator DMatrix<T>::constBegin() const
{
    return rep->data;
}
template <class T>
typename DMatrix<T>::const_iterator DMatrix<T>::constEnd() const
{
    return rep->data + rep->size;
}
template <class T>
const T* DMatrix<T>::operator[](int i) const
{
    return &rep->data[i*rep->h];
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
    memcpy(rep->data, from.inner->data, sizeof(T)*rep->size);
}
template <class T>
void DMatrix<T>::clone()
{
    this->make_unique();
}
template <class T>
DMatrix<T>::~DMatrix<T>()
{
}
#endif // DMATRIX_H
