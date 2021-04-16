#ifndef DMATRIXTRACK_H
#define DMATRIXTRACK_H
#include "DMatrix.h"
#include <QDebug>
template <class T>
class DMatrixTrack
{
public:
    DMatrixTrack() : _image(nullptr), _track(nullptr){}
    typedef T* image_t;
    typedef T** track_t;
    void set_image(T* data, int size);
    void set_image(const DMatrix<T>&);
    void make_forward_track(int w_in, int h_in, int kw, int kh, int padding_w, int padding_h, int step_w, int step_h, int stride_w, int stride_h);
    void make_back_track(int w_in, int h_in, int kw, int kh, int padding_w, int padding_h, int step_w, int step_h, int stride_w, int stride_h);

    int out_w() const {return ow;}
    int out_h() const {return oh;}

    image_t image() const {return _image;}
    track_t track() const {return _track;}
    int size() const {return _size;}

private:
    void make_track(int w_in, int h_in, int kw, int kh, int padding_w, int padding_h, int step_w, int step_h, int stride_w, int stride_h, bool make_back);
    void make_track2(int w_in, int h_in, int kw, int kh, int padding_w, int padding_h, int step_w, int step_h, int stride_w, int stride_h, bool make_back);

    image_t _image;
    track_t _track;
    int _size;
    T zero=0;
    int ow;
    int oh;
};
template <class T>
void DMatrixTrack<T>::set_image(T* data, int size)
{
    copy_mem(_image, data, size);
}
template <class T>
void DMatrixTrack<T>::set_image(const DMatrix<T>& m)
{
    copy_mem(_image, m.constBegin(), m.area());
}
template <class T>
void DMatrixTrack<T>::make_forward_track(int w_in, int h_in, int kw, int kh, int padding_w, int padding_h, int step_w, int step_h,
                                         int stride_w, int stride_h)
{
    make_track2(w_in, h_in, kw, kh, padding_w, padding_h, step_w, step_h, stride_w, stride_h, false);
}
template <class T>
void DMatrixTrack<T>::make_back_track(int w_in, int h_in, int kw, int kh, int padding_w, int padding_h, int step_w, int step_h,
                                      int stride_w, int stride_h)
{
    make_track2(w_in, h_in, kw, kh, padding_w, padding_h, step_w, step_h, stride_w, stride_h,true);
}
template <class T>
void DMatrixTrack<T>::make_track(int w_in, int h_in, int kw, int kh, int padding_w, int padding_h, int step_w, int step_h,
                                 int stride_w, int stride_h, bool make_back)
{
    bool b = make_back;
//    kw = kw>w_in?w_in:kw;
//    kh = kh>h_in?h_in:kh;


    padding_w = padding_w>=kw?kw-1:padding_w;
    padding_h = padding_h>=kh?kh-1:padding_h;
    step_w = step_w<=0?1:step_w;
    step_h = step_h<=0?1:step_h;
    int w_out = (w_in + 2*padding_w - kw)/step_w + 1;
    int h_out = (h_in + 2*padding_h - kh)/step_h + 1;

    int k_size = kw*kh;
    int in_size = w_in*h_in;
    int out_size = w_out*h_out;

    int image_size = b? out_size : in_size;
    int track_size = b? in_size*k_size : out_size*k_size;
    _size = track_size;
    reset_mem(_image, image_size);
    reset_mem(_track, track_size);
    for(int i=0;i!=track_size;++i) _track[i] = &zero;

    struct mpair{int w; int h;};
    mpair mul;
    mpair pad = {-padding_w, -padding_h};
    mpair pos;
    mpair shift;
    const mpair lim = {w_in - kw, h_in - kh};


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
//            mul.w = pos.w>lim.w ? pos.w-lim.w : kw;
            mul.w = pad.w>lim.w ? kw + lim.w - pos.w: mul.w;


            pos.h = pad.h<0 ? 0 : pad.h;
            mul.h = pad.h<0 ? kh+pad.h : kh;
//            mul.h = pos.h>lim.h ? pos.h-lim.h : kh;
            mul.h = pad.h>lim.h ? kh + lim.h - pos.h : mul.h;



            int left = shift.w % stride_w;
            int right = (kw - shift.w) % stride_w;
            int mulw = (kw - shift.w) / stride_w;
//            if(left <= right) ++mulw;

            shift.w = (pad.w - pos.w) * -1;
            shift.h = (pad.h - pos.h) * -1;


//            qDebug()<<"make track"<<"pad"<<pad.w<<"pos:"<<pos.w<<"mul:"<<mul.w<<"kw:"<<kw<<"lim:"<<lim.w;
//            qDebug()<<"mul:"<<mul.w<<mul.h;
            qDebug()<<"shift:"<<shift.w<<"stride:"<<stride_w<<"left:"<<left<<"right:"<<right<<"mulw:"<<mulw;
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
            pad.h += step_h;
        }
        pad.w += step_w;
        pad.h = -padding_h;
        pos.h = 0;
    }

    ow = w_out;
    oh = h_out;
}
template <class T>
void DMatrixTrack<T>::make_track2(int w_in, int h_in, int kw, int kh, int padding_w, int padding_h, int step_w, int step_h,
                                 int stride_w, int stride_h, bool make_back)
{
    struct mpair{int w; int h;};
    const mpair vksize = {(kw-1)*stride_w + kw, (kh-1)*stride_h + kh};

    bool b = make_back;

    padding_w = padding_w>=vksize.w?vksize.w-1:padding_w;
    padding_h = padding_h>=vksize.h?vksize.h-1:padding_h;
    step_w = step_w<=0?1:step_w;
    step_h = step_h<=0?1:step_h;

    int w_out = (w_in + 2*padding_w - vksize.w)/step_w + 1;
    int h_out = (h_in + 2*padding_h - vksize.h)/step_h + 1;

    int k_size = kw*kh;
    int in_size = w_in*h_in;
    int out_size = w_out*h_out;

    int image_size = b? out_size : in_size;
    int track_size = b? in_size*k_size : out_size*k_size; //original k size
    _size = track_size;
    reset_mem(_image, image_size);
    reset_mem(_track, track_size);
    for(int i=0;i!=track_size;++i) _track[i] = &zero;

    mpair mul;
    mpair pad = {-padding_w, -padding_h};
    mpair pos;
    mpair shift;
    mpair cross_area;
    mpair left_blocks;
    mpair right_blocks;
    mpair local_pos;
    const mpair lim = {w_in - vksize.w, h_in - vksize.h};


    int INPUT_INDEX = 0;
    int KERNEL_INDEX = 0;
    int OUT_INDEX = 0;
    int TRACK_I = 0;

//    qDebug()<<"-- make track2:"<<"w out:"<<w_out<<"h out:"<<h_out<<"kw:"<<kw<<"kh:"<<kh<<"vk w:"<<vksize.w<<"vk h:"<<vksize.h
//           <<"stepw:"<<step_w<<"steph:"<<step_h<<"padw:"<<padding_w<<"padh:"<<padding_h<<"stridew:"<<stride_w<<"strideh:"<<stride_h;

    for(int OUT_i=0;OUT_i!=w_out;++OUT_i)
    {
        for(int OUT_j=0;OUT_j!=h_out;++OUT_j)
        {
            pos.w = pad.w<0 ? 0 : pad.w;
            pos.h = pad.h<0 ? 0 : pad.h;
            shift.w = (pad.w - pos.w) * -1;
            shift.h = (pad.h - pos.h) * -1;
            cross_area.w = pad.w < 0 ? vksize.w + pad.w : vksize.w;
            cross_area.w = pos.w > lim.w ? vksize.w - (pos.w - lim.w) : cross_area.w;

            cross_area.h = pad.h < 0 ? vksize.h + pad.h : vksize.h;
            cross_area.h = pos.h > lim.h ? vksize.h - (pos.h - lim.h) : cross_area.h;

            mul.w = cross_area.w / (stride_w+1);
            left_blocks.w = shift.w % (stride_w+1);
            right_blocks.w = cross_area.w % (stride_w+1);
            if(right_blocks.w > 0 && left_blocks.w <= right_blocks.w) ++mul.w;

            mul.h = cross_area.h / (stride_h+1);
            left_blocks.h = shift.h % (stride_h+1);
            right_blocks.h = cross_area.h % (stride_h+1);
            if(right_blocks.h > 0 && left_blocks.h <= right_blocks.h) ++mul.h;


//            qDebug()<<"mulw:"<<mul.w<<"mulh:"<<mul.h<<"posw:"<<pos.w<<"posh:"<<pos.h<<"leftw:"<<left_blocks.w<<"lefth:"<<left_blocks.h;

            shift.w /= (stride_w+1);
            if(left_blocks.w > 0) ++shift.w;
            shift.h /= (stride_h+1);
            if(left_blocks.h > 0) ++shift.h;

            for(int i=0;i!=mul.w;++i)
            {
                local_pos.w = i ? local_pos.w + stride_w+1 : left_blocks.w + pos.w;
                for(int j=0;j!=mul.h;++j)
                {
                    local_pos.h = j ? local_pos.h + stride_h+1 : left_blocks.h + pos.h;

                    INPUT_INDEX = local_pos.w * h_in + local_pos.h;
                    KERNEL_INDEX = (i + shift.w) * kh + (j + shift.h);
                    OUT_INDEX = OUT_i * h_out + OUT_j;

//                    qDebug()<<"iw:"<<local_pos.w<<"ih:"<<local_pos.h<<"kw:"<<shift.w + i << "kh:"<<shift.h + j
//                           <<"INPUT"<<INPUT_INDEX<<"KERNEL:"<<KERNEL_INDEX<<"OUT:"<<OUT_INDEX<<"pw:"<<pos.w<<left_blocks.w;

                    TRACK_I = (b? INPUT_INDEX:OUT_INDEX) * k_size + KERNEL_INDEX;
                    _track[TRACK_I] = &_image[b? OUT_INDEX:INPUT_INDEX];
                }
            }
            pad.h += step_h;
        }
        pad.w += step_w;
        pad.h = -padding_h;
        pos.h = 0;
    }

    ow = w_out;
    oh = h_out;
}
//----------------------------------------------------------------------------------------------------------------------------------------
template <class T>
void convolution2(const DMatrixTrack<T>& tr, const DMatrix<T>& kernel, DMatrix<T>& out)
{
    T** track_it = tr.track();
    T** track_end = track_it + tr.size();

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
        *out_it++ += v;
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
    DDuplexMatrixTrack() {}
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

/*
 test block


void set_rand(int& v)
{
    v = rand()%100;
}
void print_matrix(const DMatrix<int>& m, const char* message = nullptr)
{
    if(message) printf("%s:\n", message);
    for(int i=0;i!=m.width();++i)
    {
        for(int j=0;j!=m.height();++j)
            printf("%d ", m[i][j]);
        printf("\n");
    }
    printf("\n");
}


//    int test_size = 400;
//    DMatrix<int> m(500,500);
//    DMatrix<int> k(7,7);
    int test_size = 1;
    DMatrix<int> m(4,4);
    DMatrix<int> k(2,2);

    m.run(set_rand);
    k.run(set_rand);

    DMatrixTrack<int> main_track;
    main_track.make_forward_track(m.width(), m.height(), k.width(), k.height(), 0, 0, 1, 1, 0, 0);
    DMatrix<int> out(main_track.out_w(), main_track.out_h());
    main_track.set_image(m);

    convolution2(main_track, k, out);

    print_matrix(m, "input");
    print_matrix(k, "kernel");
    print_matrix(out, "out");

 */
