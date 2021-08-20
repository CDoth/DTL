#ifndef DMATRIXTRACK_H
#define DMATRIXTRACK_H
#include "DMatrix.h"
#include <QDebug>
template <class T>
class DMatrixTrack
{
public:
    DMatrixTrack() : _image(nullptr), _track(nullptr), _k_image(nullptr), _k_track(nullptr){}
    typedef T*  image_t;
    typedef T** track_t;
    void set_image(const DMatrix<T>&);
    void set_k_image(const DMatrix<T>&);

    void make_forward_track(int w_in, int h_in, int kw, int kh, int padding_w, int padding_h, int step_w, int step_h, int stride_w, int stride_h,
                            image_t kernel_image = nullptr, bool make_main = true);
    void make_back_track(int w_in, int h_in, int kw, int kh, int padding_w, int padding_h, int step_w, int step_h, int stride_w, int stride_h,
                         image_t kernel_image = nullptr, bool make_main = true);
    void make_pooling_track(int w_in, int h_in, int pw, int ph, int step_w, int step_h, int stride_w, int stride_h);

    int out_w() const {return ow;}
    int out_h() const {return oh;}

    const image_t image() const {return _image;}
    const track_t begin() const {return _track;}
    const track_t end() const {return _track + _track_size;}

    const track_t k_begin() const {return _k_track;}
    const track_t k_end() const {return _k_track + _k_track_size;}


    int size() const {return _track_size;}
    int k_size() const {return _k_track_size;}

private:
    void make_track(int w_in, int h_in, int kw, int kh, int padding_w, int padding_h, int step_w, int step_h, int stride_w, int stride_h, bool make_back);    
    void make_track2(int w_in, int h_in, int kw, int kh, int padding_w, int padding_h, int step_w, int step_h, int stride_w, int stride_h, bool make_back,
                     image_t kernel_image = nullptr, bool make_main = true);

    image_t _image;
    track_t _track;

    image_t _k_image;
    track_t _k_track;
    int _track_size;
    int _k_track_size;
    T zero;
    int ow;
    int oh;
public:
    int pooling_area;
};

static int actual_track_size(int w_in, int h_in, int kw, int kh, int pw, int ph,  int step_w, int step_h, int stride_w, int stride_h)
{
    //Line full size:
    int Lpw = w_in + 2 * pw;
    int Lph = h_in + 2 * ph;

    //Virtual kernel size (with stride):
    int kw_v = (kw-1)*stride_w + kw;
    int kh_v = (kh-1)*stride_h + kh;

    //Out size:
    int w_out = (w_in + 2*pw - kw_v)/step_w + 1;
    int h_out = (h_in + 2*ph - kh_v)/step_h + 1;

    //Kernel steps by one line:
    int Npw = ((Lpw - kw_v)/step_w) + 1;
    int Nph = ((Lph - kh_v)/step_h) + 1;

    int Npw_right = (kw_v + ((w_out-1) * step_w)) - (pw + w_in)  ;
    int Nph_bot = (kh_v + ((h_out-1) * step_h)) - (ph + h_in)  ;

    if(Nph_bot < 0) Nph_bot = 0;
    if(Npw_right < 0) Npw_right = 0;
    //Multiplies by one line:
    int Mw1 = Npw * kw;
    int Mh1 = Nph * kh;



    //Lines:
    int Nsw0 = ((1+pw) * (pw + (step_w * (stride_w+1)) - 1))  /  (2 * (step_w * (stride_w+1)));
    int Nsw1 = ((1+Npw_right) * (Npw_right + (step_w * (stride_w+1)) - 1))  /  (2 * (step_w * (stride_w+1)));

    int Nsh0 = ((1+ph) * (ph + (step_h * (stride_h+1)) - 1))  /  (2 * (step_h * (stride_h+1)));
    int Nsh1 = ((1+Nph_bot) * (Nph_bot + (step_h * (stride_h+1)) - 1))  /  (2 * (step_h * (stride_h+1)));


    int F00 = Nsw0 * Nsh0;
    int F01 = Nsw0 * Nsh1;
    int F10 = Nsw1 * Nsh0;
    int F11 = Nsw1 * Nsh1;
    int F = F00 + F01 + F10 + F11;

//    int top_line = Mh1 * Nsh0;
//    int bot_line = Mh1 * Nsh1;
//    int left_line = Mw1 * Nsw0;
//    int right_line = Mw1 * Nsw1;
    int top_line = Mw1 * Nsh0;
    int bot_line = Mw1 * Nsh1;
    int left_line = Mh1 * Nsw0;
    int right_line = Mh1 * Nsw1;
    int less = top_line + bot_line + left_line + right_line  - F;

    int k_area = kw*kh;
    int out_area = w_out * h_out;

//    qDebug()<<"Nsw0:"<<Nsw0<<"Nsw1:"<<Nsw1<<"Nsh0:"<<Nsh0<<"Nsh1:"<<Nsh1<<"F:"<<F<<"res:"<<less;
//    qDebug()<<"less:"<<less<<"top:"<<top_line<<"bot:"<<bot_line<<"left:"<<left_line<<"right:"<<right_line;
//    qDebug()<<"Mh1:"<<Mh1<<"Nsh1:"<<Nsh1<<"Nph:"<<Nph<<"kh:"<<kh<<"Nph_bot:"<<Nph_bot;
//    qDebug()<<"left:"<<left_line<<"Mw1:"<<Mw1<<"Nsw0:"<<Nsw0<<"Npw:"<<Npw;
//    qDebug()<<"full track:"<<out_area * k_area<<"less:"<<less<<"actual track:"<<out_area * k_area - less<<"out:"<<w_out<<h_out;

    return out_area * k_area - less;
}
template <class T>
void DMatrixTrack<T>::set_k_image(const DMatrix<T> &k)
{
    copy_mem(_k_image, k.constBegin(), k.area());
}
template <class T>
void DMatrixTrack<T>::set_image(const DMatrix<T> &m)
{
    copy_mem(_image, m.constBegin(), m.area());
}
template <class T>
void DMatrixTrack<T>::make_forward_track(int w_in, int h_in, int kw, int kh, int padding_w, int padding_h, int step_w, int step_h,
                                         int stride_w, int stride_h, image_t kernel_image, bool make_main)
{
    make_track2(w_in, h_in, kw, kh, padding_w, padding_h, step_w, step_h, stride_w, stride_h, false, kernel_image, make_main);
}
template <class T>
void DMatrixTrack<T>::make_back_track(int w_in, int h_in, int kw, int kh, int padding_w, int padding_h, int step_w, int step_h,
                                      int stride_w, int stride_h, image_t kernel_image, bool make_main)
{
    make_track2(w_in, h_in, kw, kh, padding_w, padding_h, step_w, step_h, stride_w, stride_h, true, kernel_image, make_main);
}
template <class T>
void DMatrixTrack<T>::make_track(int w_in, int h_in, int kw, int kh, int padding_w, int padding_h, int step_w, int step_h,
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
    _track_size = track_size;
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
template <class T>
void DMatrixTrack<T>::make_track2(int w_in, int h_in, int kw, int kh, int padding_w, int padding_h, int step_w, int step_h,
                                 int stride_w, int stride_h, bool make_back, image_t kernel_image, bool make_main)
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
    int _shift = b? in_size : out_size;
    _track_size = actual_track_size(w_in, h_in, kw, kh, padding_w, padding_h, step_w, step_h, stride_w, stride_h) + _shift;
    _k_track_size = _track_size - _shift;
    if(make_main) reset_mem(_image, image_size);
    _k_image = kernel_image ? kernel_image : reget_mem(_k_image, k_size);
    if(make_main) reset_mem(_track, _track_size);
    reset_mem(_k_track, _k_track_size);

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
    int K_TRACK_I = 0;


    for(int OUT_i=0;OUT_i!=w_out;++OUT_i)
    {
        for(int OUT_j=0;OUT_j!=h_out;++OUT_j)
        {
            pos.w = pad.w < 0 ? 0 : pad.w;
            pos.h = pad.h < 0 ? 0 : pad.h;
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



                    if(make_main) _track[TRACK_I++] = &_image[b? OUT_INDEX:INPUT_INDEX];
                    _k_track[K_TRACK_I++] = &_k_image[KERNEL_INDEX];


                }
            }
            if(make_main) _track[TRACK_I++] = nullptr;
            pad.h += step_h;
        }
        pad.w += step_w;
        pad.h = -padding_h;
        pos.h = 0;
    }

//    qDebug()<<"K TRACK I:"<<K_TRACK_I<<"K TRACK SIZE:"<<_k_track_size<<"TRACK SIZE:"<<_track_size;

    ow = w_out;
    oh = h_out;
}

template <class T>
void DMatrixTrack<T>::make_pooling_track(int w_in, int h_in, int pw, int ph, int step_w, int step_h, int stride_w, int stride_h)
{
    const int virtual_pw = (pw-1)*stride_w + pw;
    const int virtual_ph = (ph-1)*stride_h + ph;

    ow = (w_in + step_w) / (virtual_pw + step_w);
    oh = (h_in + step_h) / (virtual_ph + step_h);
    pooling_area = pw * ph;

    const int image_size = w_in * h_in;
    _track_size = ow * oh * pooling_area;
    reset_mem(_image, image_size);
    reset_mem(_track, _track_size);

    int track_i = 0;
//    qDebug()<<"vpw:"<<virtual_pw<<"vph:"<<virtual_ph;

    int w_pos = 0;
    int h_pos = 0;
    int window_w_pos = 0;
    int window_h_pos = 0;
    for(int i=0;i != ow; ++i)
    {
        h_pos = 0;
        for(int j=0;j != oh; ++j)
        {
            window_w_pos = 0;
            for(int i_window = 0; i_window != pw; ++i_window)
            {
                window_h_pos = 0;
                for(int j_window = 0; j_window !=ph; ++j_window)
                {
//                    qDebug()<<"w_pos:"<<w_pos<<"window_w_pos:"<<window_w_pos<<"h_pos:"<<h_pos<<"window_h_pos:"<<window_h_pos;
                    _track[track_i++] = &_image[(w_pos+window_w_pos) * h_in + (h_pos + window_h_pos)];

                    window_h_pos += stride_h + 1;
                }
                window_w_pos += stride_w + 1;
            }
            h_pos += step_h + virtual_ph;
        }
        w_pos += step_w + virtual_pw;
    }
}
//----------------------------------------------------------------------------------------------------------------------------------------


template <class value_type>
struct complex_value_type
{
    value_type *raw;
    value_type *sig;
    value_type *err;
};
template <class T>
void convolution(const DMatrixTrack<T> &tr, const DMatrix<T> &kernel, DMatrix<T> &out)
{
    auto track_it = tr.begin();
    auto track_end =  tr.end();

    auto out_it = out.begin();
    auto kernel_it = kernel.constBegin();
    auto kernel_end = kernel.constEnd();

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
void convolution2(const DMatrixTrack<T> &tr, const DMatrix<T> &kernel, DMatrix<T> &raw_out, DMatrix<T> &act_out, DMatrix<T> &deract_out,
                  T (*_act)(T), T (*_deract)(T))
{
    auto track_it = tr.begin();
    auto track_end = tr.end();

    auto raw_out_it = raw_out.begin();
    auto act_out_it = act_out.begin();
    auto deract_out_it = deract_out.begin();

    auto kernel_it = kernel.constBegin();
    auto kernel_end = kernel.constEnd();

    T v = 0;
    int i=0;
//    qDebug()<<"===================================== CONVOLUTION: ============================";
    while(track_it != track_end)
    {
        v = 0;
        kernel_it = kernel.constBegin();
        while(kernel_it != kernel_end)
        {
//            qDebug()<<"k:"<<*kernel_it<<"in:"<<**track_it<<" = "<<*kernel_it * **track_it;
            v += *kernel_it++ * **track_it++;
            ++i;
        }
//        qDebug()<<"-------- sum:"<< v << "prev:" << *raw_out_it;
        *raw_out_it += v;
        *act_out_it = _act(*raw_out_it);
        *deract_out_it = _deract(*raw_out_it);
        ++raw_out_it; ++act_out_it; ++deract_out_it;
    }
//    qDebug()<<"convolution2():"<<i;
}
template <class T>
void convolution3(const DMatrixTrack<T> &track, DMatrix<T> &raw_out)
{ 
//    qDebug()<<"convolution3";
//    printf("convolution3\n");
    auto track_it = track.begin();
    auto track_end = track.end();

    auto raw_out_it = raw_out.begin();
    auto kernel_it = track.k_begin();

    T v = 0;
    while(track_it != track_end)
    {
        v = 0;
        while(*track_it)
        {
//            qDebug()<<"k:"<<**kernel_it<<"input:"<<**track_it<<"res:"<<**kernel_it * **track_it;

//            printf("in: %d k: %d res: %d\n", **track_it, **kernel_it, **kernel_it * **track_it);
            v += **kernel_it++ * **track_it++;
        }
//        qDebug()<<"sum:"<<v<<"---------------------";
//        printf("sum: %d --------------------\n", v);
        *raw_out_it += v;
        ++raw_out_it;
        ++track_it;
    }
}
template <class T>
void back_convolution(const DMatrixTrack<T> &out_error_track, const DMatrix<T> &kernel, DMatrix<T> &in_error, const DMatrix<T> &raw_in, DMatrix<T> &kernel_delta)
{
    T** track_it = out_error_track.track();
    T** track_end = track_it + out_error_track.size();

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
void back_convolution2(const DMatrixTrack<T> &out_error_track, const DMatrix<T> &kernel, DMatrix<T> &kernel_delta,
                       DMatrix<T> &in_error, const DMatrix<T> &input, T &rate)
{
    auto track_it = out_error_track.begin();
    auto track_end = out_error_track.end();

    auto out_it = in_error.begin();

    auto kernel_it = kernel.constBegin();
    auto kernel_end = kernel.constEnd();
    auto kernel_delta_it = kernel_delta.begin();

    auto input_it = input.constBegin();

//    qDebug()<<"===================================== BACK CONVOLUTION: ============================";
    T v = 0;
    int i=0;
    while(track_it != track_end)
    {
        v = 0;
        kernel_it = kernel.constBegin();
//        kernel_delta_it = kernel_delta.begin();
        while(kernel_it != kernel_end)
        {
//            qDebug()<<"out error:"<<**track_it<<"k:"<<*kernel_it<<"in error:"<<**track_it * *kernel_it<<"input:"<<*input_it<<"rate:"<<rate
//                   <<"delta w:"<<**track_it * *input_it * rate;

//            v += **track_it * *kernel_it;
//            *kernel_delta_it += **track_it * *input_it * rate;

//            ++kernel_delta_it;
            ++kernel_it;
            ++track_it;
            ++i;
        }
//        qDebug()<<"-----------";

        *out_it++ = v;
        ++input_it;
    }

//    qDebug()<<"back_convolution2()"<<i;

}
template <class T>
void back_convolution3(const DMatrixTrack<T> &out_error_track, DMatrixTrack<T> &kernel_delta_track,
                       DMatrix<T> &in_error, const DMatrix<T> &input, T &rate)
{
    auto track_it = out_error_track.begin();
    auto track_end = out_error_track.end();
    auto out_it = in_error.begin();

    auto kernel_it = out_error_track.k_begin();
    auto kernel_delta_it = kernel_delta_track.k_begin();

    auto input_it = input.constBegin();

//    qDebug()<<"===================================== BACK CONVOLUTION: ============================";
    T v = 0;
    while(track_it != track_end)
    {
        v = 0;
        while(*track_it)
        {
//            qDebug()<<"out error:"<<**track_it<<"k:"<<*kernel_it<<"in error:"<<**track_it * *kernel_it<<"input:"<<*input_it<<"rate:"<<rate
//                   <<"delta w:"<<**track_it * *input_it * rate;

            v += **track_it * **kernel_it;
            **kernel_delta_it += **track_it * *input_it * rate;

            ++kernel_delta_it;
            ++kernel_it;
            ++track_it;
        }
        *out_it++ = v;
        ++input_it;
        ++track_it;
    }
}
template <class T>
void back_convolution4(const DMatrixTrack<T> &out_error_track, DMatrix<T> &in_error)
{
    auto track_it = out_error_track.begin();
    auto track_end = out_error_track.end();
    auto out_it = in_error.begin();
    auto kernel_it = out_error_track.k_begin();

    qDebug()<<"===================================== BACK CONVOLUTION: ============================";
    T v = 0;
    while(track_it != track_end)
    {
        v = 0;
        while(*track_it)
        {
            qDebug()<<"out error:"<<**track_it<<"k:"<<**kernel_it<<"in error:"<<**track_it * **kernel_it;

            v += **track_it * **kernel_it;

            ++kernel_it;
            ++track_it;
        }
        qDebug()<<"------ v:"<<v;
        *out_it++ = v;
        ++track_it;
    }
}
template <class T>
void max_pooling(const DMatrixTrack<complex_value_type<T>> &input_pooling_track,
                  DMatrix<T> &out, DMatrix<T> &raw, DMatrix<T*> &error_trace)
{
    auto track_it = input_pooling_track.begin();
    auto track_end = input_pooling_track.end();

    auto out_it = out.begin();
    auto raw_out_it = raw.begin();
    auto error_trace_it = error_trace.begin();

    int i=0;
    T max = 0;
    T raw_value = 0;

    while(track_it != track_end)
    {
        if(i==0)
        {
            max = *(*track_it)->sig;
            raw_value = *(*track_it)->raw;
            *error_trace_it = (*track_it)->err;
            ++track_it;  ++i;
        }
//        qDebug()<<"sig:"<<(*track_it);
        if(*(*track_it)->sig > max)
        {
//            qDebug()<<"max:"<<max;
            max = *(*track_it)->sig;
            raw_value = *(*track_it)->raw;
            *error_trace_it = (*track_it)->err;
        }
        ++track_it; ++i;
        if(i==input_pooling_track.pooling_area)
        {
//            qDebug()<<"max_pooling:"<<&out<<max;
            *out_it++ = max;
            *raw_out_it++ = raw_value;
            ++error_trace_it;
            i=0;
        }
    }
}
template <class T>
void mean_pooling(const DMatrixTrack<complex_value_type<T>> &input_pooling_track,
                 DMatrix<T> &out, DMatrix<T> &raw)
{
    auto track_it = input_pooling_track.begin();
    auto track_end = input_pooling_track.end();

    auto out_it = out.begin();
    auto raw_out_it = raw.begin();
    int i=0;
    T mean = 0;
    T raw_mean = 0;

    while(track_it != track_end)
    {
        mean += *(*track_it)->sig;
        raw_mean += *(*track_it)->raw;

        ++track_it; ++i;
        if(i==input_pooling_track.pooling_area)
        {
            *out_it++ = mean / input_pooling_track.pooling_area;
            *raw_out_it++ = raw_mean / input_pooling_track.pooling_area;
            mean = 0;
            raw_mean = 0;
            i=0;
        }
    }
}
template <class T>
void back_max_pooling(DMatrix<T*> &error_trace, const DMatrix<T> &out_error)
{
    auto error_trace_it = error_trace.begin();
    auto error_trace_end = error_trace.end();
    auto out_error_it = out_error.constBegin();

    while( error_trace_it != error_trace_end )
    {
        **error_trace_it++ = *out_error_it++;
    }
}
template <class T>
void back_mean_pooling(const DMatrixTrack<complex_value_type<T>> &input_pooling_track, const DMatrix<T> &out_error)
{
    auto track_it = input_pooling_track.begin();
    auto track_end = input_pooling_track.end();
    auto out_error_it = out_error.constBegin();

    int i=0;
    while( track_it != track_end )
    {
        *(*track_it)->err = *out_error_it / input_pooling_track.pooling_area;
        ++track_it; ++i;
        if(i==input_pooling_track.pooling_area)
        {
            ++out_error_it;
            i=0;
        }
    }
}
template <class T>
class DDuplexMatrixTrack
{
public:
    DDuplexMatrixTrack() {}
    void make_forward_track(int w_in, int h_in, int kw, int kh, int padding_w, int padding_h, int step_w, int step_h,
                            int stride_w, int stride_h);
    void make_back_track(int w_in, int h_in, int kw, int kh, int padding_w, int padding_h, int step_w, int step_h,
                         int stride_w, int stride_h);

    void set_forward_image(T* _image, int size) {forward_track.set_image(_image, size);}
    void set_forward_image(const DMatrix<T>& m) {forward_track.set_image(m);}
    void set_back_image(T* _image, int size) {back_track.set_image(_image, size);}
    void set_back_image(const DMatrix<T>& m) {back_track.set_image(m);}

    typedef T** track_t;
    DMatrixTrack<T>& ftrack() {return forward_track;}
    DMatrixTrack<T>& btrack() {return back_track;}

    void fp(const DMatrix<T>& kernel, DMatrix<T>& out);
    void bp(const DMatrix<T>& kernel, DMatrix<T>& in_error, const DMatrix<T>& raw_in, DMatrix<T>& kernel_delta);
private:

    DMatrixTrack<T> forward_track;
    DMatrixTrack<T> back_track;
};
template <class T>
void DDuplexMatrixTrack<T>::make_forward_track(int w_in, int h_in, int kw, int kh, int padding_w, int padding_h, int step_w, int step_h,
                                               int stride_w, int stride_h)
{
    forward_track.make_forward_track(w_in, h_in, kw, kh, padding_w, padding_h, step_w, step_h, stride_w, stride_h);
}
template <class T>
void DDuplexMatrixTrack<T>::make_back_track(int w_in, int h_in, int kw, int kh, int padding_w, int padding_h, int step_w, int step_h,
                                            int stride_w, int stride_h)
{
    back_track.make_back_track(w_in, h_in, kw, kh, padding_w, padding_h, step_w, step_h, stride_w, stride_h);
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

