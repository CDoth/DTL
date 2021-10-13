#ifndef DARAN_H
#define DARAN_H

#include <stdint.h>
#include <iostream>
#include <vector>
#include <map>

#define FOR_VALUE(X, iterator) for(int iterator=0;iterator!=X;++iterator)


#   define D_RB16(x)                            \
    ((((const uint8_t*)(x))[0] << 8) |          \
      ((const uint8_t*)(x))[1])

#   define D_RL16(x)                            \
    ((((const uint8_t*)(x))[1] << 8) |          \
      ((const uint8_t*)(x))[0])

#   define D_WB16(p, val) do {                  \
        uint16_t d = (val);                     \
        ((uint8_t*)(p))[1] = (d);               \
        ((uint8_t*)(p))[0] = (d)>>8;            \
    } while(0)
#   define D_WL16(p, val) do {                  \
        uint16_t d = (val);                     \
        ((uint8_t*)(p))[0] = (d);               \
        ((uint8_t*)(p))[1] = (d)>>8;            \
    } while(0)
#   define D_WB32(p, val) do {                  \
        uint32_t d = (val);                     \
        ((uint8_t*)(p))[3] = (d);               \
        ((uint8_t*)(p))[2] = (d)>>8;            \
        ((uint8_t*)(p))[1] = (d)>>16;           \
        ((uint8_t*)(p))[0] = (d)>>24;           \
    } while(0)
#   define D_WL32(p, val) do {                  \
        uint32_t d = (val);                     \
        ((uint8_t*)(p))[0] = (d);               \
        ((uint8_t*)(p))[1] = (d)>>8;            \
        ((uint8_t*)(p))[2] = (d)>>16;           \
        ((uint8_t*)(p))[3] = (d)>>24;           \
    } while(0)
#   define D_WL32_P2P(p, p_val) do {                  \
    ((uint8_t*)(p))[0] = ((uint8_t*)(p_val))[0];      \
    ((uint8_t*)(p))[1] = ((uint8_t*)(p_val))[1];      \
    ((uint8_t*)(p))[2] = ((uint8_t*)(p_val))[2];      \
    ((uint8_t*)(p))[3] = ((uint8_t*)(p_val))[3];      \
} while(0)
#   define D_WB32_P2P(p, p_val) do {                  \
    ((uint8_t*)(p))[3] = ((uint8_t*)(p_val))[0];      \
    ((uint8_t*)(p))[2] = ((uint8_t*)(p_val))[1];      \
    ((uint8_t*)(p))[1] = ((uint8_t*)(p_val))[2];      \
    ((uint8_t*)(p))[0] = ((uint8_t*)(p_val))[3];      \
} while(0)

inline bool buffer_compare(const void* buffer1, const void* buffer2, int size);
bool buffer_compare2(const uint8_t* buffer1, const uint8_t* buffer2, int size);
int find_bytes_pos(const void* sample, const void* buffer, int sample_size, int buffer_size);
const void* find_bytes(const void* sample, const void* buffer, int sample_size, int buffer_size);
const void* find_bytes2(const void *sample, const void *buffer, int sample_size, int buffer_size, const std::map<uint8_t, int> &stop_list);
int find_difference_pos(void* first, void* second, int range);
void* find_difference(void* first, void* second, int range);
double getPart(double bot, double top, double value);
double getValueByPart(double bot, double top, double part);
struct matchpos
{
    int pos1;
    int pos2;
    int size;
};
int lf_same_sections(void *original, void *edited, int original_size, int edited_size);
void print_byte(uint8_t byte);
void put_byte(uint8_t byte, uint8_t* bit_array);
void look_bytes(void *buffer, int size);
void look_bytes_8bit_value(void *buffer, int size);
void look_bytes_16bit_value(void *buffer, int size);
void look_bytes_32bit_value(void *buffer, int size);
void look_bytes_64bit_value(void *buffer, int size);
int file_compare(const char* fname1, const char* fname2, int size);
void addByte(uint64_t& store, uint8_t& byte);


#endif // DARAN_H
