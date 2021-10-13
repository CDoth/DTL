#include "DRand.h"

#include <windows.h>

inline int mod10(int value)
{
    return value%10;
}
inline int mod100(int value)
{
    return value%100;
}
inline int mod1k(int value)
{
    return value%1000;
}
inline int mod10k(int value)
{
    return value%10000;
}
inline int mod100k(int value)
{
   return value%100000;
}

inline int mod1kk(int value)
{
    return value%1000000;
}
inline int mod10kk(int value)
{
    return value%10000000;
}
inline int mod100kk(int value)
{
    return value%100000000;
}

inline int qmod(int value)
{

if(value>=10 && value<=99)
    return mod10(value);

if(value>=100 && value<=999)
    return mod100(value);

if(value>=1000 && value<=9999)
    return mod1k(value);

if(value>=10000 && value<=99999)
    return mod10k(value);

if(value>=100000 && value<=999999)
    return mod100k(value);

if(value>=1000000 && value<=9999999)
    return mod1kk(value);

if(value>=10000000 && value<=99999999)
    return mod10kk(value);

if(value>=100000000 && value<=999999999)
    return mod100kk(value);
}

std::vector<int> nmod(int value)
{
    std::vector<int> numbers;
    numbers.push_back(    (mod10(value))     );
    numbers.push_back(    (mod100(value) - mod10(value))/10   );
    numbers.push_back(    (mod1k(value) - mod100(value))/100   );
    numbers.push_back(    (mod10k(value) - mod1k(value))/1000   );
    numbers.push_back(    (mod100k(value) - mod10k(value))/10000   );
    numbers.push_back(    (mod1kk(value) - mod100k(value))/100000   );
    numbers.push_back(    (mod10kk(value) - mod1kk(value))/1000000   );
    numbers.push_back(    (mod100kk(value) - mod10kk(value))/10000000   );
    return numbers;
}
int middle(int value)
{
    return (value%1000000)/100;
}


//------------------------------------------
unsigned int seed = 101202303;
int sgen(int r,int q)
{
    SYSTEMTIME st;
    GetLocalTime(&st);

    int y1;
    int y2;
    int y3;

    auto key1 = st.wMilliseconds;
    auto key2 = 245;

    y1 = 18253%(key2+123);
    y2 = 69313%(key2+321);
    y3 = 106033%(key2+512);

    int x = y1%r;
    int m = y2%q;

    int Q1 = m *y1;
    int Q2 = x *y2;

    if(Q1>10000)
        Q1 = Q1/13;

    int H = Q1 * (Q2/30);
    seed = H *mod10(r) * mod10(q);

    std::vector<int> num = nmod(seed);

    int amount = 0;
    int multiply = 1;

    for(auto it:num)
    {
        amount+=it;
        if(it) multiply*=it;
    }
    seed+=amount*multiply+1;
    return seed * 2;
}
float partOf(float PART, float TOTAL)
{
    return PART / TOTAL;
}
float sym_partOf(float PART, float TOTAL)
{
    if(PART > TOTAL) return TOTAL * 100.0f / PART;
    return PART * 100.0f / TOTAL;
}
float xrand()
{
    seed = seed * seed + seed;
    float v = partOf(seed, float(UINT_MAX));



    return v;
}
float xrand(float range)
{
    return xrand() * range;
}
float xrand(float bot, float top)
{
    float v = xrand(top - bot) + bot;
    return v;
}
float xrands()
{
    return xrand(-1.0, 1.0);
}
float xrands(float range)
{
    return xrand(-range, range);
}
float getRandSign()
{
    return xrand() > 0.5f ? 1: -1;
}
void xrand2(float &v)
{
    v = xrand();
}
void xrands2(float &v)
{
    v = xrands();
}
