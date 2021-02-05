#ifndef DMATRIXTRACK_H
#define DMATRIXTRACK_H
#include "DMatrix.h"

template <class T>
class DMatrixTrack
{
public:
    DMatrixTrack() : _image(nullptr), _track(nullptr){}
    typedef T* image_t;
    typedef T** track_t;
    void set_image(T* data, int size);
    void set_image(const DMatrix<T>&);
    void make_forward_track(int w_in, int h_in, int kw, int kh, int pw, int ph, int sw, int sh);
    void make_back_track(int w_in, int h_in, int kw, int kh, int pw, int ph, int sw, int sh);

    image_t image() const {return image;}
    track_t track() const {return track;}
    int size() const {return _size;}
private:
    void make_track(int w_in, int h_in, int kw, int kh, int pw, int ph, int sw, int sh, bool make_back);
    image_t _image;
    track_t _track;
    int _size;
    T zero=0;
};
template <class T>
void DMatrixTrack<T>::set_image(T* data, int size)
{
    cpy_mem(_image, data, size);
}
template <class T>
void DMatrixTrack<T>::set_image(const DMatrix<T>& m)
{
    cpy_mem(_image, m.constBegin(), m.area());
}
template <class T>
void DMatrixTrack<T>::make_forward_track(int w_in, int h_in, int kw, int kh, int pw, int ph, int sw, int sh)
{
    make_track(w_in, h_in, kw, kh, pw, ph, sw, sh, true);
}
template <class T>
void DMatrixTrack<T>::make_back_track(int w_in, int h_in, int kw, int kh, int pw, int ph, int sw, int sh)
{
    make_track(w_in, h_in, kw, kh, pw, ph, sw, sh, false);
}
template <class T>
void DMatrixTrack<T>::make_track(int w_in, int h_in, int kw, int kh, int pw, int ph, int sw, int sh, bool make_back)
{
    bool b = make_back;
    kw = kw>w_in?w_in:kw;
    kh = kh>h_in?h_in:kh;
    pw = pw>=kw?kw-1:pw;
    ph = ph>=kh?kh-1:ph;
    sw = sw<=0?1:sw;
    sh = sh<=0?1:sh;
    int w_out = (w_in + 2*pw - kw)/sw + 1;
    int h_out = (h_in + 2*ph - kh)/sh + 1;
    int k_size = kw*kh;
    int in_size = w_in*h_in;
    int out_size = w_out*h_out;

    int image_size = b? in_size : out_size;
    int track_size = b? in_size*k_size : out_size*k_size;
    _size = track_size;
    reset_mem(_image, image_size);
    reset_mem(_track, track_size);
    for(int i=0;i!=track_size;++i) _track[i] = zero;

    struct mpair{mpair(int _=0, int __=0) : w(_), h(__){} int w; int h;};
    mpair mul;
    mpair pad(-pw, -ph);
    mpair pos;
    mpair shift;
    const mpair lim(w_in - kw, h_in - kh);

    int INPUT_INDEX = 0;
    int KERNEL_INDEX = 0;
    int OUT_INDEX = 0;
    int TRACK_I = 0;

    for(int OUT_i=0;OUT_i!=w_out;++OUT_i)
    {
        for(int OUT_j=0;OUT_j!=h_out;++OUT_j)
        {
            pos.w = pad.w<0 ? 0 : pad.w;
            mul.w = pad.w<0 ? kw+pad.w : kw;
            mul.w = pos.w>lim.w ? pos.w-lim.w : kw;

            pos.h = pad.h<0 ? 0 : pad.h;
            mul.h = pad.h<0 ? kh+pad.h : kh;
            mul.h = pos.h>lim.h ? pos.h-lim.h : kh;

            shift.w = (pad.w - pos.w) * -1;
            shift.h = (pad.h - pos.h) * -1;

            for(int i=0;i!=mul.w;++i)
            {
                for(int j=0;j!=mul.h;++j)
                {
                    INPUT_INDEX = (pos.w + i) * h_in + (pos.h + j);
                    KERNEL_INDEX = (i + shift.w) * kh + (j + shift.h);
                    OUT_INDEX = OUT_i * h_out + OUT_j;
                    TRACK_I = (b? INPUT_INDEX:OUT_INDEX) * k_size + KERNEL_INDEX;
                    _track[TRACK_I] = &_image[b? OUT_INDEX:INPUT_INDEX];
                }
            }
            pad.h += sh;
        }
        pad.w += sw;
        pad.h = -ph;
        pos.h = 0;
    }
}
//----------------------------------------------------------------------------------------------------------------------------------------
template <class T>
void convolution(T** track, int track_size, const DMatrix<T>& kernel, DMatrix<T>& out)
{
    T** track_it = track;
    T** track_end = track_it + track_size;

    typedef typename DMatrix<T>::iterator iter;
    typedef typename DMatrix<T>::const_iterator citer;
    iter out_it = out.begin();
    citer kernel_it = kernel.constBegin();
    citer kernel_end = kernel.constEnd();

    T v = 0;
    while(track_it != track_end)
    {
        v = 0;
        kernel_it = kernel.constBegin();
        while(kernel_it != kernel_end)
        {
            v += *kernel_it++ * **track_it++;
        }
        *out_it += v;
        out_it++;
    }
}
template <class T>
void back_convolution(T** oe_track, int track_size, const DMatrix<T>& kernel, DMatrix<T>& in_error, const DMatrix<T>& raw_in, DMatrix<T>& kernel_delta)
{
    T** track_it = oe_track;
    T** track_end = track_it + track_size;

    typedef typename DMatrix<T>::iterator iter;
    typedef typename DMatrix<T>::const_iterator citer;

    iter out_it = in_error.begin();
    citer kernel_it = kernel.constBegin();
    citer kernel_end = kernel.constEnd();

    iter delta_it = kernel_delta.begin();
    citer raw_in_it = raw_in.constBegin();

    while(track_it != track_end)
    {
        kernel_it = kernel.constBegin();
        delta_it = kernel_delta.begin();
        while(kernel_it != kernel_end)
        {
            *delta_it += **track_it * *raw_in_it;
            *out_it += **track_it * *kernel_it;
            kernel_it++;
            track_it++;
            delta_it++;
        }
        out_it++;
        raw_in_it++;
    }
}

template <class T>
class DDuplexMatrixTrack
{
public:
    DDuplexMatrixTrack();
    void make_forward_track(int w_in, int h_in, int kw, int kh, int pw, int ph, int sw, int sh);
    void make_back_track(int w_in, int h_in, int kw, int kh, int pw, int ph, int sw, int sh);

    void set_in_image(T* _image, int size) {forward_track.set_image(_image, size);}
    void set_in_image(const DMatrix<T>& m) {forward_track.set_image(m);}
    void set_out_image(T* _image, int size) {back_track.set_image(_image, size);}
    void set_out_image(const DMatrix<T>& m) {back_track.set_image(m);}

    typedef T** track_t;
    track_t ftrack() const {return forward_track.get_track();}
    track_t btrack() const {return back_track.get_image();}

    void fp(const DMatrix<T>& kernel, DMatrix<T>& out);
    void bp(const DMatrix<T>& kernel, DMatrix<T>& in_error, const DMatrix<T>& raw_in, DMatrix<T>& kernel_delta);
private:

    DMatrixTrack<T> forward_track;
    DMatrixTrack<T> back_track;
};
template <class T>
void DDuplexMatrixTrack<T>::make_forward_track(int w_in, int h_in, int kw, int kh, int pw, int ph, int sw, int sh)
{
    forward_track.make_forward_track(w_in, h_in, kw, kh, pw, ph, sw, sh);
}
template <class T>
void DDuplexMatrixTrack<T>::make_back_track(int w_in, int h_in, int kw, int kh, int pw, int ph, int sw, int sh)
{
    back_track.make_back_track(w_in, h_in, kw, kh, pw, ph, sw, sh);
}
template <class T>
void DDuplexMatrixTrack<T>::fp(const DMatrix<T>& kernel, DMatrix<T>& out)
{
    convolution(forward_track.get_track(), forward_track.size(), kernel, out);
}
template <class T>
void DDuplexMatrixTrack<T>::bp(const DMatrix<T>& kernel, DMatrix<T>& in_error, const DMatrix<T>& raw_in, DMatrix<T>& kernel_delta)
{
    back_convolution(back_track.track(), back_track.size(), kernel, in_error, raw_in, kernel_delta);
}
#endif // DMATRIXTRACK_H
