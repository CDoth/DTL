#include "sha256.h"
#include <bitset>
#include "DDirReader.h"
#include "DThreadHandler.h"
#include <sstream>


uint32_t __SHA256_H[] = {0x6a09e667,
                         0xbb67ae85,
                         0x3c6ef372,
                         0xa54ff53a,
                         0x510e527f,
                         0x9b05688c,
                         0x1f83d9ab,
                         0x5be0cd19
                        };
uint32_t __SHA256_K[] = {
       0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5, 0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
       0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3, 0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
       0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC, 0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
       0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7, 0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
       0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13, 0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
       0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3, 0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
       0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5, 0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
       0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208, 0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2
};
uint8_t __test_message[] = { // hello world
    0b01101000, 0b01100101,
    0b01101100, 0b01101100, 0b01101111, 0b00100000, 0b01110111, 0b01101111,
    0b01110010, 0b01101100, 0b01100100
};
//---------------------------------------------------------------------------------------
void __sha256__create_file_buffer(SHA256Context *c, size_t buffer_size) {

    if(c == nullptr || buffer_size == 0) {
        return;
    }
    if(c->__file_buffer_is_inner == 0) {
        c->__file_buffer = nullptr;
        c->__file_buffer_size = 0;
        c->__file_buffer_is_inner = 1;
    }
    if(c->__file_buffer_size == buffer_size) {
        return;
    }
    if( (c->__file_buffer = reget_zmem<uint8_t>(c->__file_buffer, buffer_size)) == nullptr) {
        c->__file_buffer_size = 0;
        return;
    }
    c->__file_buffer_size = buffer_size;
}
void __sha256__set_file_buffer(SHA256Context *c, uint8_t *buffer, size_t buffer_size) {

    if(c == nullptr || buffer_size == 0 || buffer == nullptr) {
        return;
    }
    if(c->__file_buffer && c->__file_buffer_is_inner) {
        free_mem(c->__file_buffer);
    }
    c->__file_buffer = buffer;
    c->__file_buffer_size = buffer_size;
    c->__file_buffer_is_inner = 0;
}
uint32_t *__sha256__create_hash_storage() {

    uint32_t *s = get_zmem<uint32_t>(SHA256__HASH_WORDS);
    copy_mem(s, __SHA256_H, SHA256__HASH_WORDS);
    return s;
}
void __sha256__free_hash_storage(uint32_t *__storage) {
    if(__storage) {
        zero_mem(__storage, 8);
    }
    free_mem(__storage);
}
//---------------------------------------------------------------------------------------
void __sha256__hash_fragment(SHA256Context *c) {

    uint32_t s0, s1, ch, temp1, temp2, maj;
    uint32_t *W = c->__dict;
    for(int wi = 16; wi != 64; ++wi) {

        s0 = RIGHT_ROTATE32(W[wi-15], 7) xor RIGHT_ROTATE32(W[wi-15], 18) xor RIGHT_SHIFT(W[wi-15], 3);
        s1 = RIGHT_ROTATE32(W[wi-2], 17) xor RIGHT_ROTATE32(W[wi-2], 19) xor RIGHT_SHIFT(W[wi-2], 10);
        W[wi] = W[wi-16] + s0 + W[wi-7] + s1;
    }
    copy_mem(c->__frag_hash, c->hash, 8);
#define _a c->__frag_hash[0]
#define _b c->__frag_hash[1]
#define _c c->__frag_hash[2]
#define _d c->__frag_hash[3]
#define _e c->__frag_hash[4]
#define _f c->__frag_hash[5]
#define _g c->__frag_hash[6]
#define _h c->__frag_hash[7]
    for(int ai = 0; ai != 64; ++ai) {
        s1 = RIGHT_ROTATE32(_e, 6) xor RIGHT_ROTATE32(_e, 11) xor RIGHT_ROTATE32(_e, 25);
        ch = (_e & _f) | (  (~_e) & _g  );
        temp1 = _h + s1 + ch + __SHA256_K[ai] + W[ai];

        s0 = RIGHT_ROTATE32(_a, 2) xor RIGHT_ROTATE32(_a, 13) xor RIGHT_ROTATE32(_a, 22);
        maj = (_a & _b) xor (_a & _c) xor (_b & _c);
        temp2 = s0 + maj;

        _h = _g;
        _g = _f;
        _f = _e;
        _e = _d + temp1;
        _d = _c;
        _c = _b;
        _b = _a;
        _a = temp1 + temp2;
    }
    for(int __hi = 0;__hi != 8; ++__hi) {
        c->hash[__hi] += c->__frag_hash[__hi];
    }

}
int __sha256__hash_data(const uint8_t *data, size_t size, SHA256Context *c) {

    if(!data || !c || !size ) {
        return -1;
    }

    uint8_t * __dict8 = reinterpret_cast<uint8_t*>(c->__dict);
    size_t flush_now;
    size_t flushed = 0;
    int ending_type = 0;



    while(size) { //fragment loop

        zero_mem(c->__dict, 64);


        // copy fragment data
        //--------------------------------------------------------------------------------
        flush_now = size > SHA256__BYTELEN_FRAGMENT ? SHA256__BYTELEN_FRAGMENT : size;
        copy_mem(__dict8, data + flushed, flush_now);


        FOR_VALUE(16, j) {
            c->__dict[j] = BIGTOLITTLE32(c->__dict[j]);
        }

        size -= flush_now;
        flushed += flush_now;



        if(c->__flush_ending && size == 0) {

            size_t free_space = SHA256__BYTELEN_FRAGMENT - flush_now;


            if(free_space) {

                int inv_index = (flush_now - (flush_now % 4)) + (3 - (flush_now % 4));
                __dict8[inv_index] = 0b10000000;
                if(free_space >= SHA256__BYTELEN_SIZEPLUG + 1) {

                    c->__dict[14] = reinterpret_cast<uint32_t*>(&c->__bits_size)[1];
                    c->__dict[15] = reinterpret_cast<uint32_t*>(&c->__bits_size)[0];

                } else {
                    ending_type = 1;
                }
            } else {
                ending_type = 2;
            }

//            __sha256__print_fragment(c->__dict, 16);

        }


        //--------------------------------------------------------------------------------

        __sha256__hash_fragment(c);

    } //fragment loop end



    if(ending_type) {
        zero_mem(c->__dict, 64);
        c->__dict[14] = reinterpret_cast<uint32_t*>(&c->__bits_size)[1];
        c->__dict[15] = reinterpret_cast<uint32_t*>(&c->__bits_size)[0];

        if(ending_type == 2) {
            __dict8[3] = 0b10000000;
        }
//        __sha256__print_fragment(c->__dict, 16);
        __sha256__hash_fragment(c);
    }


    return 0;
}
int __sha256__hash_file_part(FILE *file, size_t size, SHA256Context *c) {


    size_t proc_now;
    while(size) {
        proc_now = (size > c->__file_buffer_size) ? c->__file_buffer_size : size;
        fread(c->__file_buffer, 1, proc_now, file);
        size -= proc_now;

        if(size == 0) {
            c->__flush_ending = 1;
        }

        __sha256__hash_data(c->__file_buffer, proc_now, c);

    }

    return 0;
}
int sha256__hash_file_segment(const char *path, size_t buffer_size, SHA256Context *c, size_t start, size_t partSize) {

    if(!path || !c) {
        return -1;
    }
    FILE *f = fopen(path, "rb");
    if(!f) {
        return -2;
    }

    size_t file_size = get_file_size(path);

    file_size = partSize ?
                ((file_size < partSize) ? file_size : partSize)
                :
                file_size
                ;

    if(file_size == 0) {
        return -3;
    }
    if( start > file_size ) {
        return  -4;
    }


    c->__flush_ending = 0;
    __sha256__create_file_buffer(c, buffer_size);

    c->__bits_size = file_size * 8;

    fseek( f, start, SEEK_SET );
    __sha256__hash_file_part(f, file_size, c);

    fclose(f);

    return 0;
}
int sha256__hash_file(const char *path, size_t buffer_size, SHA256Context *c, size_t hashBytes) {

    if(!path || !c) {
        return -1;
    }
    FILE *f = fopen(path, "rb");
    if(!f) {
        return -2;
    }
    size_t file_size = get_file_size(path);

    file_size = hashBytes ?
                ((file_size < hashBytes) ? file_size : hashBytes)
                :
                file_size
                ;

    if(file_size == 0) {
        return -3;
    }


    c->__flush_ending = 0;
    __sha256__create_file_buffer(c, buffer_size);

    c->__bits_size = file_size * 8;

    __sha256__hash_file_part(f, file_size, c);

    fclose(f);

    return 0;
}
int sha256__hash_file(const char *path, uint8_t *out_buffer, size_t buffer_size, SHA256Context *c, size_t hashBytes) {

    if(!path || !c || !out_buffer) {
        return -1;
    }
    FILE *f = fopen(path, "rb");
    if(!f) {
        return -2;
    }
    size_t file_size = get_file_size(path);

    file_size = hashBytes ?
                ((file_size < hashBytes) ? file_size : hashBytes)
                :
                file_size
                ;

    if(file_size == 0) {
        return -3;
    }

    c->__flush_ending = 0;
    __sha256__set_file_buffer(c, out_buffer, buffer_size);

    c->__bits_size = file_size * 8;

    __sha256__hash_file_part(f, file_size, c);

    return 0;

}
//---------------------------------------------------------------------------------------
void sha256__print_fragment(uint32_t *f, int size) {
    FOR_VALUE(size, i) {
        std::cout << BITSET32(f[i]) << " ";
    }
    std::cout << std::endl;
}
void sha256__print_hash(SHA256Context *c) {

    if(!c) return;
    FOR_VALUE(8, i) std::cout << std::hex << c->hash[i];
    std::cout << std::endl;
}
std::string sha256_hash_to_string(SHA256Context *c) {
    std::stringstream hash;

    FOR_VALUE(8, i) hash << std::hex << c->hash[i];

    return hash.str();
}
//---------------------------------------------------------------------------------------
SHA256Context * sha256__create_context() {


    SHA256Context *c = get_zmem<SHA256Context>(1);
    c->hash = __sha256__create_hash_storage();
    c->__dict = get_zmem<uint32_t>(SHA256__DICT_WORDS);
    c->__frag_hash = get_zmem<uint32_t>(SHA256__HASH_WORDS);

    c->__flush_ending = 0;
    c->__bits_size = 0;

    c->__file_buffer = nullptr;
    c->__file_buffer_size = 0;
    c->__file_buffer_is_inner = 1;


    return c;
}
void sha256__free_context(SHA256Context *c) {
    if(c) {
        __sha256__free_hash_storage(c->hash);
        free_mem(c->__dict);
        free_mem(c->__frag_hash);
        if(c->__file_buffer_is_inner) {
            free_mem(c->__file_buffer);
        }
        zero_mem(c, 1);
    }
    free_mem(c);
}
int sha256__clear_context(SHA256Context *c) {

    if(c == nullptr) return 0;

//    zero_mem(c->hash, SHA256__HASH_WORDS);
    copy_mem(c->hash, __SHA256_H, SHA256__HASH_WORDS);

    zero_mem(c->__dict, SHA256__DICT_WORDS);
    zero_mem(c->__frag_hash, SHA256__HASH_WORDS);

    c->__flush_ending = 0;
    c->__bits_size = 0;

    if(c->__file_buffer)
        zero_mem(c->__file_buffer, c->__file_buffer_size);

    return 0;
}
int sha256__hash_data(const uint8_t *data, size_t size, SHA256Context *c) {
    if(c == nullptr)
        return -1;

    c->__flush_ending = 1;
    c->__bits_size = size * 8;
    return __sha256__hash_data(data, size, c);
}









