#ifndef DRGBIMAGE_H
#define DRGBIMAGE_H
#include "DMultiMatrix.h"
#include <QImage>


template <class T, int qv = 1>
class DRgbImage
{
public:
    DRgbImage();
    DRgbImage(const char *path, int w = 0, int h = 0);
    DRgbImage(const DRgbImage&, int w = 0, int h = 0);
    DRgbImage(int w, int h);
    ~DRgbImage();
//    DRgbImage& operator=(const DRgbImage&);
    DRgbImage& operator=(const DMultiMatrix<T> &mm);


    typedef T value_type;
    typedef DMatrix<value_type> matrix;


    const matrix& red() const {return _channels[0];}
    const matrix& green() const {return _channels[1];}
    const matrix& blue() const {return _channels[2];}
    DMultiMatrix<value_type>& channels() {return _channels;}

    int width() const {return _width;}
    int height() const {return _height;}



    void fill_channels(const matrix& m) {_channels[0] = m; _channels[1] = m; _channels[2] = m;}
    void fill_channels(const value_type& v) {_channels[0].fill(v); _channels[1].fill(v); _channels[2].fill(v);}

    void open(const char *path, int w = 0, int h = 0);

    template <class rate_value> DMultiMatrix<rate_value> rates(int q);
private:
    DMultiMatrix<value_type> _channels;
    int _width;
    int _height;

    int qvalue = qv;
};

template <class T, int qv>
template <class rate_value>
DMultiMatrix<rate_value> DRgbImage<T, qv>::rates(int q)
{
    DMultiMatrix<rate_value> rates(3, _width, _height);
    for(int i=0;i!=_width;++i)
    {
        for(int j=0;j!=_height;++j)
        {
            rates[0][i][j] = _channels[0][i][j] / q;
            rates[1][i][j] = _channels[1][i][j] / q;
            rates[2][i][j] = _channels[2][i][j] / q;
        }
    }
    return rates;
}
template <class T, int qv>
DRgbImage<T,qv>::DRgbImage()
{
}
template <class T, int qv>
DRgbImage<T,qv>::DRgbImage(const char *path, int w, int h)
{
    QImage qimage(path);
    _width = w? w: qimage.width();
    _height = h? h: qimage.height();
    _channels = DMultiMatrix<value_type>(3, _width, _height);

    for(int i=0;i!=_width;++i)
    {
        for(int j=0;j!=_height;++j)
        {
            _channels[0][i][j] = static_cast<value_type>(QColor(qimage.pixel(i,j)).red()) / static_cast<value_type>(qvalue);
            _channels[1][i][j] = static_cast<value_type>(QColor(qimage.pixel(i,j)).green()) / static_cast<value_type>(qvalue);
            _channels[2][i][j] = static_cast<value_type>(QColor(qimage.pixel(i,j)).blue()) / static_cast<value_type>(qvalue);
        }
    }
}
template <class T, int qv>
DRgbImage<T,qv>::DRgbImage(const DRgbImage &image, int w, int h)
{
    _width = w? w:image._width;
    _height = h? h:image._height;
    _channels = DMultiMatrix<value_type>(3, _width, _height);

    for(int i=0;i!=_width;++i)
    {
        for(int j=0;j!=_height;++j)
        {
            _channels[0][i][j] = image.red()[i][j];
            _channels[1][i][j] = image.green()[i][j];
            _channels[2][i][j] = image.blue()[i][j];
        }
    }
}
template <class T, int qv>
DRgbImage<T,qv>::DRgbImage(int w, int h)
{
    _width = w;
    _height = h;
    _channels = DMultiMatrix<value_type>(3, _width, _height);
}
template <class T, int qv>
DRgbImage<T,qv>::~DRgbImage()
{
}


//template <class T, int qv>
//DRgbImage<T, qv> &DRgbImage<T, qv>::operator=(const DRgbImage &im)
//{

//}
template<class T, int qv>
DRgbImage<T, qv> &DRgbImage<T, qv>::operator=(const DMultiMatrix<T> &mm)
{
    _width = mm.width();
    _height = mm.height();
    _channels = mm;
}
template <class T, int qv>
void DRgbImage<T, qv>::open(const char *path, int w, int h)
{
    QImage qimage(path);
    _width = w? w: qimage.width();
    _height = h? h: qimage.height();
    _channels = DMultiMatrix<value_type>(3, _width, _height);

    for(int i=0;i!=_width;++i)
    {
        for(int j=0;j!=_height;++j)
        {
            _channels[0][i][j] = static_cast<value_type>(QColor(qimage.pixel(i,j)).red()) / static_cast<value_type>(qvalue);
            _channels[1][i][j] = static_cast<value_type>(QColor(qimage.pixel(i,j)).green()) / static_cast<value_type>(qvalue);
            _channels[2][i][j] = static_cast<value_type>(QColor(qimage.pixel(i,j)).blue()) / static_cast<value_type>(qvalue);
        }
    }
}

//------------------------------------------------------------------------------------------
#endif // DRGBIMAGE_H
