#ifndef SHA256_H
#define SHA256_H
#include <stdint.h>
#include <string>
#include "dmem.h"



#define SHA256__BYTELEN_FRAGMENT (64)
#define SHA256__BITLEN_FRAGMENT (512)
#define SHA256__BYTELEN_SIZEPLUG (8)
#define SHA256__1BITVALUE (0x80000000)

#define SHA256__HASH_WORDS (8)
#define SHA256__DICT_WORDS (64)



struct SHA256Context {
    uint32_t *hash; // 8

    uint32_t *__dict; // 64
    uint32_t *__frag_hash; // 8

    int __flush_ending;
    uint64_t __bits_size;

    uint8_t *__file_buffer;
    size_t __file_buffer_size;
    int __file_buffer_is_inner;
};
//--------------------------------------------------------
void __sha256__create_file_buffer(SHA256Context *c, size_t buffer_size);
void __sha256__set_file_buffer(SHA256Context *c, uint8_t *buffer, size_t buffer_size);
uint32_t *__sha256__create_hash_storage();
void __sha256__free_hash_storage(uint32_t *__storage);
//--------------------------------------------------------
void __sha256__hash_fragment(SHA256Context *c);
int __sha256__hash_data(const uint8_t *data, size_t size, SHA256Context *c);

int sha256__hash_file_segment(const char *path, size_t buffer_size, SHA256Context *c, size_t start, size_t partSize);
int sha256__hash_file(const char *path, size_t buffer_size, SHA256Context *c, size_t hashBytes = 0);
int sha256__hash_file(const char *path, uint8_t *out_buffer, size_t buffer_size, SHA256Context *c, size_t hashBytes = 0);

//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
void sha256__print_fragment(uint32_t *f, int size);
void sha256__print_hash(SHA256Context *c);
std::string sha256_hash_to_string(SHA256Context *c);
//-------------------------------------------------------
SHA256Context * sha256__create_context();
void sha256__free_context(SHA256Context *c);
int sha256__clear_context(SHA256Context *c);
int sha256__hash_data(const uint8_t *data, size_t size, SHA256Context *c);


#endif // SHA256_H
