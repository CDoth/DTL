#include "daran.h"

bool buffer_compare(const void* buffer1, const void* buffer2, int size)
{
    if(buffer1 == nullptr || buffer2 == nullptr) return false;
    uint8_t* b1 = (uint8_t*)buffer1;
    uint8_t* b2 = (uint8_t*)buffer2;
    uint8_t* e1 = (uint8_t*)buffer1 + size;
    while(b1!=e1)  if(*b1++ != *b2++) return false;
    return true;
}
bool buffer_compare2(const uint8_t* buffer1, const uint8_t* buffer2, int size)
{
    uint8_t* e1 = (uint8_t*)buffer1 + size;
    while(buffer1!=e1)  if(*buffer1++ != *buffer2++) return false;
    return true;
}
int find_bytes_pos(const void* sample, const void* buffer, int sample_size, int buffer_size)
{
    if(!sample || !buffer || sample_size < 0 || buffer_size < 0)
        return -1;

    uint8_t* b = (uint8_t*)buffer;
    uint8_t* s = (uint8_t*)sample;
    uint8_t* se = (uint8_t*)sample + sample_size - 1;
    uint8_t* be = (uint8_t*)buffer + buffer_size - sample_size + 1;
    uint8_t* bse = b + sample_size - 1;

    int i=0;
    while(b!=be)
    {
        if(*s == *b && *se == *bse)
            if(buffer_compare(s,b, sample_size)) return i;
        ++b; ++bse; ++i;
    }
    return -1;
}
const void* find_bytes(const void* sample, const void* buffer, int sample_size, int buffer_size)
{
    if(!sample || !buffer || sample_size < 0 || buffer_size < 0)
        return nullptr;

    uint8_t* b = (uint8_t*)buffer;
    uint8_t* s = (uint8_t*)sample;
    uint8_t* se = (uint8_t*)sample + sample_size - 1;
    uint8_t* be = (uint8_t*)buffer + buffer_size - sample_size + 1;
    uint8_t* bse = b + sample_size - 1;


    while(b!=be)
    {
        if(*s == *b && *se == *bse)
            if(buffer_compare(s,b, sample_size)) return b;
        ++b; ++bse;
    }

    return nullptr;
}
const void* find_bytes2(const void *sample, const void *buffer, int sample_size, int buffer_size, const std::map<uint8_t, int> &stop_list)
{
    uint8_t *s = (uint8_t*)sample;
    uint8_t *se = (uint8_t*)sample + sample_size;
    uint8_t *b = (uint8_t*)buffer;
    uint8_t *be = (uint8_t*)buffer + buffer_size - sample_size + 1;


}
int find_difference_pos(void* first, void* second, int range)
{
    if(!first || !second || range < 0)
        return -1;
    uint8_t* b1 = (uint8_t*)first;
    uint8_t* b2 = (uint8_t*)second;
    uint8_t* e = (uint8_t*)first + range;
    int pos = 0;
    while(b1!=e) if(*b1++ != *b2++) return pos; else ++pos;
    return pos;
}
void* find_difference(void* first, void* second, int range)
{
    if(!first || !second || range < 0)
        return nullptr;
    uint8_t* b1 = (uint8_t*)first;
    uint8_t* b2 = (uint8_t*)second;
    uint8_t* e = (uint8_t*)first + range;
    while(b1!=e) if(*b1++ != *b2++) return b1-1;
    return b1-1;
}
double getPart(double bot, double top, double value)
{
    if(bot > top)
        std::swap(bot, top);
    if(value < bot)
        return 0.0;
    if(value > top)
        return 1.0;
    value -= bot;
    return value / (top-bot);
}
double getValueByPart(double bot, double top, double part)
{
    if(bot > top)
        std::swap(bot, top);
    return (part * (top-bot)) + bot;
}
int lf_same_sections(void *original, void *edited, int original_size, int edited_size)
{
    uint8_t* o = (uint8_t*)original;
    uint8_t* e = (uint8_t*)edited;

    int osize = original_size;
    int esize = edited_size;

    int it_num = esize - osize + 1;

    int ar = 10; //<analysis resolution> in bytes
    int ce = -3;//Control point in Edited array
    int co = 0;//Control point in Original array

    std::vector<matchpos> s;

    while(co < osize)
    {
        for(;co!=it_num;++co)
        {
            ce = find_bytes_pos(o + co, e, ar, esize);
            if(ce >= 0)
                break;
        }

        int ol = osize - co - ar; //bytes Left in Original array
        int el = esize - ce - ar; //bytes Left in Edited array
        int ls = ol; //Looking Size
        if( el < ol)
            ls = el;

        int range = find_difference_pos(o + co + ar, e + ce + ar, ls) + ar;

        matchpos m;
        m.pos1 = co;
        m.pos2 = ce;
        m.size = range;

        s.push_back(m);
        co += range;
    }

    for(size_t i=0;i!=s.size();++i)
        std::cout << s[i].pos1 << " - " << s[i].pos1+s[i].size << " now in " << s[i].pos2 << " - " << s[i].pos2+s[i].size << std::endl;
    return 0;
}
void print_byte(uint8_t byte)
{
    int i=0;
    while(i!=8) std::cout << ( (byte >> i)&1 ), ++i;
    std::cout << std::endl;
}
void put_byte(uint8_t byte, uint8_t* bit_array)
{
    int i=0;
    while(i!=8) bit_array[i] = ((byte >> i)&1), ++i;
}
void look_bytes(void *buffer, int size)
{
    uint8_t* b = (uint8_t*)buffer;
    uint8_t* e = (uint8_t*)buffer + size;
    while(b != e) print_byte(*b++);
}
void look_bytes_8bit_value(void *buffer, int size)
{
    uint8_t* b = (uint8_t*)buffer;
    uint8_t* e = (uint8_t*)buffer + size;
    while(b != e) std::cout << *b++ << std::endl;
}
void look_bytes_16bit_value(void *buffer, int size)
{
    uint16_t* b = (uint16_t*)buffer;
    uint16_t* e = (uint16_t*)buffer + size;
    while(b != e) std::cout << *b++ << std::endl;
}
void look_bytes_32bit_value(void *buffer, int size)
{
    uint32_t* b = (uint32_t*)buffer;
    uint32_t* e = (uint32_t*)buffer + size;
    while(b != e) std::cout << *b++ << std::endl;
}
void look_bytes_64bit_value(void *buffer, int size)
{
    uint64_t* b = (uint64_t*)buffer;
    uint64_t* e = (uint64_t*)buffer + size;
    while(b != e) std::cout << *b++ << std::endl;
}
int file_compare(const char* fname1, const char* fname2, int size)
{
    FILE *f1 = fopen(fname1, "rb");
    FILE *f2 = fopen(fname2, "rb");

    char *buffer1 = (char*)malloc(size);
    char *buffer2 = (char*)malloc(size);

    fread(buffer1, size, 1, f1);
    fread(buffer2, size, 2, f2);


    for(int i=0;i!=size;++i)
    {
        if(*buffer1++ != *buffer2)
            return i;
    }
    return -1;
}
void addByte(uint64_t& store, uint8_t& byte)
{
    store = (store<<8)|byte;
}
