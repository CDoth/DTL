#ifndef DRGBIMAGE_H
#define DRGBIMAGE_H
#include "DMultiMatrix.h"
#include <QImage>


template <class T, int qv>
class DRgbImage
{
public:
    DRgbImage();
    DRgbImage(const char* path, int w = 0, int h = 0);
    DRgbImage(const DRgbImage&, int w = 0, int h = 0);
    DRgbImage(int w, int h);
    ~DRgbImage();
    DRgbImage& operator=(const DRgbImage&);


    typedef T value_type;
    typedef DMatrix<value_type> matrix;


    const matrix& red() const {return _red;}
    const matrix& green() const {return _green;}
    const matrix& blue() const {return _blue;}
    const DMultiMatrix<value_type>& channels() const {return _channels;}

    int width() const {return _width;}
    int height() const {return _height;}

    void set_red(const matrix& m) { _red = m;}
    void set_red(const value_type& v) { _red.fill(v);}

    void set_green(const matrix& m) {_green = m;}
    void set_green(const value_type& v) {_green.fill(v);}

    void set_blue(const matrix& m) {_blue = m;}
    void set_blue(const value_type& v) {_blue.fill(v);}

    void fill_channels(const matrix& m) {_red = m; _green = m; _blue = m;}
    void fill_channels(const value_type& v) {_red.fill(v); _green.fill(v); _blue.fill(v);}

    template <class rate_value> DMultiMatrix<rate_value> rates(int q);
private:
    DMultiMatrix<value_type> _channels;
    matrix _red;
    matrix _green;
    matrix _blue;

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
DRgbImage<T,qv>::DRgbImage(const char* path, int w, int h)
{
    QImage qimage(path);
    _width = w? w: qimage.width();
    _height = h? h: qimage.height();
    _channels = DMultiMatrix<value_type>(3, _width, _height);
    _red = _channels[0]; _red.setMode(ShareWatcher);
    _green = _channels[1]; _green.setMode(ShareWatcher);
    _blue = _channels[2]; _blue.setMode(ShareWatcher);

    for(int i=0;i!=_width;++i)
    {
        for(int j=0;j!=_height;++j)
        {
            _red[i][j] = QColor(qimage.pixel(i,j)).red() / qvalue;
            _green[i][j] = QColor(qimage.pixel(i,j)).green() / qvalue;
            _blue[i][j] = QColor(qimage.pixel(i,j)).blue() / qvalue;
        }
    }
}
template <class T, int qv>
DRgbImage<T,qv>::DRgbImage(const DRgbImage& image, int w, int h)
{
    _width = w? w:image._width;
    _height = h? h:image._height;
    _channels = DMultiMatrix<value_type>(3, _width, _height);
    _red = _channels[0]; _red.setMode(ShareWatcher);
    _green = _channels[1]; _green.setMode(ShareWatcher);
    _blue = _channels[2]; _blue.setMode(ShareWatcher);

    for(int i=0;i!=_width;++i)
    {
        for(int j=0;j!=_height;++j)
        {
            _red[i][j] = image.red()[i][j];
            _green[i][j] = image.green()[i][j];
            _blue[i][j] = image.blue()[i][j];
        }
    }
}
template <class T, int qv>
DRgbImage<T,qv>::DRgbImage(int w, int h)
{
    _width = w;
    _height = h;
    _channels = DMultiMatrix<value_type>(3, _width, _height);
    _red = _channels[0]; _red.setMode(ShareWatcher);
    _green = _channels[1]; _green.setMode(ShareWatcher);
    _blue = _channels[2]; _blue.setMode(ShareWatcher);
}
template <class T, int qv>
DRgbImage<T,qv>::~DRgbImage()
{
}


//------------------------------------------------------------------------------------------
#endif // DRGBIMAGE_H
