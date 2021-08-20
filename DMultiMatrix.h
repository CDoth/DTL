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
    DMultiMatrixData(int s, int _w, int _h, T *place = nullptr) : data(nullptr), width(_w), height(_h)
    {
        if((data_size = width*height*s))
        {
            collection.reserve(s);

            placed = place? 1:0;
            data = place? place : get_zmem<T>(data_size);
            auto it = data;
            while(s--)
            {
                collection.append(DMatrix<T>(width,height, it));
                it+=width*height;
            }
        }
    }
    DMultiMatrixData(const DMultiMatrixData& d)
    {
        if((data_size = d.data_size))
        {
            collection.reserve(d.collection.size());
            set_new_zmem(data, data_size);
            auto it = data;
            auto src_mtx = d.collection.constBegin();
            auto src_e = d.collection.constEnd();
            width = d.width;
            height = d.height;
            while(src_mtx != src_e)
            {
                collection.append(DMatrix<T>(*src_mtx, it));
                it+=width*height; ++src_mtx;
            }
        }
    }
    ~DMultiMatrixData()
    {
//        if(!placed)
//            free_mem(data);
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
    int placed;

    const char *__name; //debug
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
    DMultiMatrix(int size, int width, int height, T *place = nullptr)
    {
        DMultiMatrixData<T>* d = new DMultiMatrixData<T>(size, width, height, place);
        w = new DDualWatcher(d, CloneWatcher);
    }

    //----------------------------------------------------------------
    void swap(DMultiMatrix<T>& with)
    {
        std::swap(w, with.w);
    }
    DMultiMatrix(const DMultiMatrix &mm)
    {
        w = mm.w;
        w->refUp();
    }
    DMultiMatrix& operator=(const DMultiMatrix &a)
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
//        if( w->pull() == 0) {delete data(); delete w;}
    }

    int area() const {return data()->width * data()->height;}
    int size() const {return data()->collection.size();}
    int dataSize() const {return data()->width * data()->height * data()->collection.size();}
    int width() const {return data()->width;}
    int height() const {return data()->height;}

    void setName(const char *name){data()->__name = name;}
    const char *getName(){return data()->__name;}



    DMatrix<T>& operator[](int i){detach();return data()->collection[i];}
    const DMatrix<T>& operator[](int i) const {return data()->collection[i];}

    typedef typename DMultiMatrixData<T>::iterator iterator;
    typedef typename DMultiMatrixData<T>::const_iterator const_iterator;
    typedef typename DMultiMatrixData<T>::data_iterator data_iterator;
    typedef typename DMultiMatrixData<T>::const_data_iterator const_data_iterator;


    void fill(const T& v)
    {
        auto b = begin();
        auto e = end();
        while(b!=e) (*b++).fill(v);
    }
    bool operator==(const DMultiMatrix<T> &mm) const
    {
        if(mm.size() == size() && mm.width() == width() && mm.height() == height())
        {
            auto b = mm.constBegin();
            auto _b = constBegin();
            auto _e = constEnd();
            int i=0;
            while(_b!=_e)
            {
                if(   !((*_b++) == (*b++))    )
                {
                    printf("DMultiMatrix: operator==(): matrix diff: %d\n", i);
                    return false;
                }
                ++i;
            }

//            printf("DMultiMatrix: operator==(): equal\n");
            return true;
        }
        else
        {
            printf("DMultiMatrix: operator==(): MM size diff\n");
            return false;
        }
    }
    iterator begin() {detach(); return data()->collection.begin();}
    iterator end() {detach(); return data()->collection.end();}
    data_iterator dataBegin() {detach(); return data()->data;}
    data_iterator dataEnd() {detach(); return data()->data + data()->data_size;}

    const_iterator constBegin() const {return data()->collection.constBegin();}
    const_iterator constEnd() const {return data()->collection.constEnd();}
    const_data_iterator constDataBegin() const {return data()->data;}
    const_data_iterator constDataEnd() const {return data()->data + this->get()->data_size;}

    void make_unique()
    {
        w->refDown();
        w = new DDualWatcher(clone(), CloneWatcher);
    }
    void setMode(WatcherMode m)
    {
        if(m != w->mode())
        {
            w->refDown();
            w = w->otherSide();
            if(w->otherSideRefs() == 0)
                w->disconnect();
            w->refUp();
        }
    }

public:

    DMultiMatrixData<T>* clone()
    {
        return new DMultiMatrixData<T>(*data());
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


/*
 test block
    DMultiMatrix<int> mm(3,  2,2);

    mm[0].run(set_rand);
    mm[1].run(set_rand);
    mm[2].run(set_rand);

    DMultiMatrix<int> mm2 = mm;
    mm2[0].fill(23);


    {
        auto b = mm.begin();
        auto e = mm.end();
        while(b != e) print_matrix(*b++);

        auto db = mm.dataBegin();
        auto de = mm.dataEnd();
        while(db != de) printf("%d ", *db++);
        printf("\n");
    }

    {
        auto b = mm2.begin();
        auto e = mm2.end();
        while(b != e) print_matrix(*b++);

        auto db = mm2.dataBegin();
        auto de = mm2.dataEnd();
        while(db != de) printf("%d ", *db++);
        printf("\n");
    }
 */
#endif // DMULTIMATRIX_H
