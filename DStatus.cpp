#include "DStatus.h"
#include "daran.h"

SimpleStatus::SimpleStatus() : _status(0), _keys(NULL), _nb_keys(0)
{
}
SimpleStatus::SimpleStatus(uint8_t status) : _status(status), _keys(NULL), _nb_keys(0)
{
}
SimpleStatus::~SimpleStatus()
{
    free_mem(_keys);
}
//-----------------------------------------------------------------
void SimpleStatus::status_on(unsigned int UserIndex)
{
    if(UserIndex < 8)
        _status |= (1 << UserIndex);
}
void SimpleStatus::status_off(unsigned int UserIndex)
{
    if(UserIndex < 8)
        _status &= ~(1 << UserIndex);
}
void SimpleStatus::on_all()
{
    _status = 1;
}
void SimpleStatus::off_all()
{
    _status = 0;
}
void SimpleStatus::set_status(uint8_t status)
{
    _status = status;
}
void SimpleStatus::set_user_status(int status, int index)
{
    status ? status_on(index) : status_off(index);
}
void SimpleStatus::set_status(int status[8])
{
    FOR_VALUE(8, i) set_user_status(status[i], i);
}
int SimpleStatus::get_user_status(unsigned int UserIndex)
{
    if(UserIndex < 8)
        return (_status & (1 << UserIndex));
    return 0;
}
uint8_t SimpleStatus::get_total_status()
{
    return _status;
}
void SimpleStatus::invert_status()
{
    _status = ~_status;
}
bool SimpleStatus::compare(uint8_t key)
{
    return _status == key;
}
bool SimpleStatus::compare_inner(unsigned int key_index)
{
    if(key_index < _nb_keys)
        return _status == _keys[key_index];
    return false;
}
int SimpleStatus::add_key(uint8_t key)
{
    _keys = reget_mem(_keys,  ++_nb_keys);
    _keys[_nb_keys-1] = key;
    return _nb_keys - 1;
}
void SimpleStatus::fill_key(unsigned int key_index, unsigned int size)
{
    if(key_index < _nb_keys && size < 8)
    {
        _keys[key_index] = (255 >> (8 - size));
    }
}
void SimpleStatus::fill_status(unsigned int size)
{
    if(size < 8)
        _status = (255 >> (8 - size));
}
//-----------------------------------------------------------------
void SimpleStatus::status_on(unsigned int UserIndex) volatile
{
    if(UserIndex < 8)
        _status |= (1 << UserIndex);
}
void SimpleStatus::status_off(unsigned int UserIndex) volatile
{
    if(UserIndex < 8)
        _status &= ~(1 << UserIndex);
}
void SimpleStatus::on_all() volatile
{
    _status = 1;
}
void SimpleStatus::off_all() volatile
{
    _status = 0;
}
void SimpleStatus::set_status(uint8_t status) volatile
{
    _status = status;
}
int SimpleStatus::get_user_status(unsigned int UserIndex) volatile
{
    if(UserIndex < 8)
    {
        return bool(_status & (1 << UserIndex));
    }
    else return 0;
}
uint8_t SimpleStatus::get_total_status() volatile
{
    return _status;
}
void SimpleStatus::invert_status() volatile
{
    _status = ~_status;
}
bool SimpleStatus::compare(uint8_t key) volatile
{
    return (_status == key);
}
bool SimpleStatus::compare_inner(unsigned int key_index) volatile
{
    if(key_index < _nb_keys)
    {
        return bool(_status == _keys[key_index]);
    }
    return false;
}
int SimpleStatus::add_key(uint8_t key) volatile
{
    _keys = reget_mem(_keys,  ++_nb_keys);
    _keys[_nb_keys-1] = key;
    return _nb_keys - 1;
}
void SimpleStatus::fill_key(unsigned int key_index, unsigned int size) volatile
{
    if(key_index < _nb_keys && size < 8)
    {
        _keys[key_index] = (255 >> (8 - size));
    }
}
void SimpleStatus::fill_status(unsigned int size) volatile
{
    if(size < 8)
        _status = (255 >> (8 - size));
}
//======================================================================================================================================
//====================================================== DualStatus ====================================================================
//======================================================================================================================================
DualStatus::DualStatus(unsigned int users)
    :
      UsersNumber(users)
    , _support_status(NULL)
    , _zero_key(NULL)

    , _main_status(NULL)
    , _key(NULL)

    , _phase(NULL)
    , _value(NULL)
{

    if(  (_support_status = get_zmem<WORD>(UsersNumber)) == NULL  )
    {
        std::cout << "DualStatus error: bad alloc (_support_status)" << std::endl;
    }
    if(  (_main_status = get_zmem<WORD>(UsersNumber)) == NULL  )
    {
        std::cout << "DualStatus error: bad alloc (_main_status)" << std::endl;
    }
    if(  (_phase = get_zmem<WORD>(UsersNumber)) == NULL  )
    {
        std::cout << "DualStatus error: bad alloc (_phase)" << std::endl;
    }
    if(  (_zero_key = get_zmem<WORD>(UsersNumber)) == NULL  )
    {
        std::cout << "DualStatus error: bad alloc (_zero_key)" << std::endl;
    }
    if(  (_key = get_zmem<WORD>(UsersNumber)) == NULL  )
    {
        std::cout << "DualStatus error: bad alloc (_key)" << std::endl;
    }
    if(  (_value = get_zmem<WORD>(UsersNumber)) == NULL  )
    {
        std::cout << "DualStatus error: bad alloc (_value)" << std::endl;
    }
    setDefault();

}
void DualStatus::setUsers(unsigned int users) volatile
{
    UsersNumber = users;
    if(  (_support_status = reget_zmem<WORD>(_support_status, UsersNumber)) == NULL  )
    {
        std::cout << "DualStatus error: bad alloc (_support_status)" << std::endl;
    }
    if(  (_main_status = reget_zmem<WORD>(_main_status, UsersNumber)) == NULL  )
    {
        std::cout << "DualStatus error: bad alloc (_main_status)" << std::endl;
    }
    if(  (_phase = reget_zmem<WORD>(_phase, UsersNumber)) == NULL  )
    {
        std::cout << "DualStatus error: bad alloc (_phase)" << std::endl;
    }
    if(  (_zero_key = reget_zmem<WORD>(_zero_key, UsersNumber)) == NULL  )
    {
        std::cout << "DualStatus error: bad alloc (_zero_key)" << std::endl;
    }
    if(  (_key = reget_zmem<WORD>(_key, UsersNumber)) == NULL  )
    {
        std::cout << "DualStatus error: bad alloc (_key)" << std::endl;
    }
    if(  (_value = reget_zmem<WORD>(_value, UsersNumber)) == NULL  )
    {
        std::cout << "DualStatus error: bad alloc (_value)" << std::endl;
    }
    setDefault();

}
void DualStatus::setDefault() volatile
{
    if(_support_status) set_mem(_support_status, 1, UsersNumber);
    if(_main_status) set_mem(_main_status, 0, UsersNumber);
    if(_phase) set_mem(_phase, 1, UsersNumber);
    if(_key) set_mem(_key, 1, UsersNumber);
    if(_zero_key) set_mem(_zero_key, 0, UsersNumber);
    if(_value) set_mem(_value, 1, UsersNumber);
}
void DualStatus::setDefault(int UserIndex) volatile
{
    if(_support_status) _support_status[UserIndex] = 1;
    if(_main_status) _main_status[UserIndex] = 0;
    if(_phase) _phase[UserIndex] = 1;
    if(_value) _value[UserIndex] = 1;
}
void DualStatus::setAllUseresMainStatus(WORD status) volatile
{
    set_mem(_main_status, status, UsersNumber);
}
void DualStatus::setAllUseresSupportStatus(WORD status) volatile
{
    set_mem(_support_status, status, UsersNumber);
}
void DualStatus::setUserMainStatus(int UserIndex, WORD status) volatile
{
    _main_status[UserIndex] = status;
}
void DualStatus::setUserSupportStatus(int UserIndex, WORD status) volatile
{
    _support_status[UserIndex] = status;
}
DualStatus::WORD DualStatus::getUserStatus(int UserIndex) volatile
{
//        qDebug()<<"gtrd";
    return _main_status[UserIndex];
}
void DualStatus::debug(int UserIndex, const char *message) volatile
{
}
bool DualStatus::compare(int UserIndex) volatile
{
#define CMP_PHASE (_phase[UserIndex])
#define SET_ST(status_) (status_[UserIndex] = _value[UserIndex])
#define CMP_STATUS(status_, key_) (memcmp(status_, key_, UsersNumber * sizeof(WORD)) == 0)

#define INV(x) (x = 1 - x)
#define INV_CURST (INV(_value[UserIndex]))
#define INV_PHASE (INV(_phase[UserIndex]))

#define CMP_STATUS2(status_) \
(_value[UserIndex] ? CMP_STATUS(status_, _key) : CMP_STATUS(status_, _zero_key))

    bool r = false;
    if(CMP_PHASE)
    {
        SET_ST(_main_status);
        if(CMP_STATUS2(_main_status))
        {
            INV_CURST;
            INV_PHASE;
            r = true;
        }
    }
    else
    {
        SET_ST(_support_status);
        if(CMP_STATUS2(_support_status))
        {
            INV_PHASE;
            r = true;
        }
    }

    return r;
}
bool DualStatus::const_compare(int UserIndex) volatile
{
    bool r = false;

    if(CMP_PHASE)
    {
        if(_value[UserIndex] && CMP_STATUS(_support_status, _key))
        {
            r = true;
        }
        else if(!_value[UserIndex] && CMP_STATUS(_support_status, _zero_key))
        {
            r = true;
        }
    }
    else
    {
        if(_value[UserIndex] && CMP_STATUS(_main_status, _zero_key))
        {
            r = true;
        }
        else if(!_value[UserIndex] && CMP_STATUS(_main_status, _key))
        {
            r = true;
        }
    }

    return r;
}

DualStatus::~DualStatus()
{
    clearAll();
}
void DualStatus::clearAll()
{
    free_mem(_support_status);
    free_mem(_zero_key);

    free_mem(_main_status);
    free_mem(_key);

    free_mem(_phase);
    free_mem(_value);
}

#undef CMP_PHASE
#undef SET_ST
#undef CMP_STATUS
#undef INV
#undef INV_CURST
#undef INV_PHASE
#undef CMP_STATUS2
