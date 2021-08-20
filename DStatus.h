#ifndef DSTATUS_H
#define DSTATUS_H
#include <stdint.h>
#include "dmem.h"

class SimpleStatus
{
public:
    SimpleStatus();
    SimpleStatus(uint8_t status);
    ~SimpleStatus();
    //-----------------------------------------------------------------
    void status_on(unsigned int UserIndex);
    void status_off(unsigned int UserIndex);
    void on_all();
    void off_all();
    void set_status(uint8_t status);
    void set_user_status(int status, int index);
    void set_status(int status[8]);
    int get_user_status(unsigned int UserIndex);
    uint8_t get_total_status();
    void invert_status();
    bool compare(uint8_t key);
    bool compare_inner(unsigned int key_index);
    int add_key(uint8_t key);
    void fill_key(unsigned int key_index, unsigned int size);
    void fill_status(unsigned int size);
    //-----------------------------------------------------------------
    void status_on(unsigned int UserIndex) volatile;
    void status_off(unsigned int UserIndex) volatile;
    void on_all() volatile;
    void off_all() volatile;
    void set_status(uint8_t status) volatile;
    int get_user_status(unsigned int UserIndex) volatile;
    uint8_t get_total_status() volatile;
    void invert_status() volatile;
    bool compare(uint8_t key) volatile;
    bool compare_inner(unsigned int key_index) volatile;
    int add_key(uint8_t key) volatile;
    void fill_key(unsigned int key_index, unsigned int size) volatile;
    void fill_status(unsigned int size) volatile;
    //-----------------------------------------------------------------

private:
    uint8_t _status;
    uint8_t *_keys;
    unsigned int _nb_keys;
};


//======================================================================================================================================
//====================================================== DualStatus ====================================================================
//======================================================================================================================================

class DualStatus
{
    typedef uint8_t
    word_type
    ,WORD
    ,*WORD_P
    ,*WORD_ARRAY;
public:
    DualStatus(unsigned int users = 0);
    void setUsers(unsigned int users) volatile;
    void setDefault() volatile;
    void setDefault(int UserIndex) volatile;
    void setAllUseresMainStatus(WORD status) volatile;
    void setAllUseresSupportStatus(WORD status) volatile;
    void setUserMainStatus(int UserIndex, WORD status) volatile;
    void setUserSupportStatus(int UserIndex, WORD status) volatile;
    WORD getUserStatus(int UserIndex) volatile;
    void debug(int UserIndex, const char *message) volatile;
    void debug2(int UserIndex, const char *message, int layer, int fp_num) volatile;
    bool compare(int UserIndex) volatile;
    bool const_compare(int UserIndex) volatile;

    ~DualStatus();

private:
    void clearAll();
    int UsersNumber;

    WORD_ARRAY  _support_status;
    WORD_ARRAY  _zero_key;

    WORD_ARRAY _main_status;
    WORD_ARRAY _key;

    WORD_ARRAY _phase;
    WORD_ARRAY _value;
};

#endif // DSTATUS_H
