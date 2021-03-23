#ifndef DTCP_H
#define DTCP_H
#include <stdint.h>
#include "DArray.h"

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <QDebug>
#else

#define SOCKET int
#define INVALID_SOCKET -1
#define SD_BOTH SHUT_RDWR

#define WSAE INVALID_SOCKET
#define SOCKET_ERROR INVALID_SOCKET

#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring> //memset
#include <unistd.h> //close

#endif


#ifdef _WIN32
#define WSAE WSAGetLastError()
static void winsock_start()
{
    char buff[1024];
    WSAStartup(0x202,(WSADATA *)&buff[0]);
}
#endif


class DTcp
{
public:
    enum state {Default = -1, Client = 1, Server = 2};
    enum check_code {recvall = 333, recvless = 222, recverr = 111};

    DTcp();

    void unblock_in();
    void unblock_out();

    int make_server(int port, const char* address  = nullptr);
    int make_client(int port, const char* address  = nullptr);

    int wait_connection();
    int try_connect();

    int send_it(const void* data, int len, int flag = 0);
    int unlocked_send_it(const void* data, int len, int flag = 0);
    int send_packet(const void* data, int len, int flag1 = 0, int flag2 = 0);
    int unlocked_send_packet(const void *data, int len, int flag1 = 0, int flag2 = 0);


    int receive_to(void* data, int len, int flag = 0);
    int unlocked_recv_to(void* data, int len, int flag = 0);
    int recv_packet(void* data, int flag1 = 0, int flag2 = 0);
    int unlocked_recv_packet(void* data, int *packet_size, int recv_lim = 0, int flag1 = 0, int flag2 = 0);




    int check_readability(int sec = 0, int usec = 0);
    int check_writability(int sec = 0, int usec = 0);
    int stop_in();
    int stop_out();
private:


    int set_in(const int& port, const char* address  = nullptr);
    int set_out(const int& port, const char* address = nullptr);

    static void local_error(const char* message, int error_code = 0);
    static void func_error(const char* func_name, const char* message, int error_code = 0);
    static void info(const char* message);
    int is_valid_socket(SOCKET s);
private:
    SOCKET __socket(int af, int type, int protocol);
    SOCKET __accept(SOCKET s, sockaddr* addr, int* addrlen);
    int    __closesocket(SOCKET s);
    int    __bind(SOCKET s, const sockaddr* addr, int namelen);
    int    __listen(SOCKET s, int backlog);
    int    __connect(SOCKET s, const sockaddr* name, int namelen);
    int    __shutdown(SOCKET s, int how);
private:
    SOCKET _socket_in;
    SOCKET _socket_out;

    sockaddr_in _addr_inner;
    sockaddr_in _addr_outer;

    state _me;

    struct packet_recv_info
    {
        int total_rb;
        int packet_size;
        int ps_rb;
    };
    struct packet_send_info
    {
        int total_sb;
        int ps_sb;
    };

    packet_recv_info pr;
    packet_send_info ps;
};





struct script
{
    script() : jump_to(nullptr), repeats(nullptr), counter(0), interupt(false), index(-1) {}
    void* jump_to;
    int* repeats;
    int counter;
    bool interupt;
    int index;
};
struct send_item
{
    send_item() : data(nullptr), size(0), packet(false) {};
    send_item(const void* d, int s, bool p = false) : data(d), size(s), packet(p) {}
    const void* data;
    int size;
    bool packet;
    mutable script s;
};
struct recv_item
{
    recv_item() : data(nullptr), size(0), packet(false) {}
    recv_item(void* d, int s, bool p = false) : data(d), size(s), packet(p) {}
    void* data;
    int size;
    bool packet;
    script s;
};

typedef DArray<send_item, Direct> send_content;
struct SENDC
{
    SENDC(DTcp* _t) : t(_t), i(nullptr), e(nullptr) {}
    inline bool empty() const {return c.empty();}
    inline void add_item(send_item item) {c.push_back(item);}
    inline void reserve(int s) {c.reserve(s);}
    inline void complete() {i = c.begin(); e = c.end();}
    inline void clear() {c.clear();}
    inline bool unlocked_send_all()
    {
        if(i == nullptr) return false;
        int sb = 0;
        while( i != e )
        {
            if(i->packet) sb = t->unlocked_send_packet(i->data, i->size);
            else sb = t->unlocked_send_it(i->data, i->size);
            if(sb == i->size) ++i;
            if(sb < 0) break;
        }
        if(i==e)
        {
            c.remove_all();
            i = nullptr;
            e = nullptr;
            return true;
        }
        return false;
    }
    typedef send_content::const_iterator send_iterator;
    inline bool unlocked_send_script()
    {
        if(i == nullptr) return false;
        int sb = 0;
        while( i != e )
        {
            if(i->packet) sb = t->unlocked_send_packet(i->data, i->size);
            else sb = t->unlocked_send_it(i->data, i->size);
            if(sb < 0) break;
            if(sb == i->size)
            {
                if(i->s.jump_to && i->s.repeats)
                {
                    if(i->s.counter < * i->s.repeats)
                    {
                        ++i->s.counter;
                        i = (send_item*)i->s.jump_to;
                    }
                    else ++i;
                }
            }

        }
        if(i==e)
        {
            c.remove_all();
            i = nullptr;
            e = nullptr;
            return true;
        }
        return false;
    }

    send_content c;
    DTcp* t;
    send_iterator i;
    send_iterator e;

    inline send_iterator begin() const {return c.begin();}
    inline send_iterator end() const {return c.end();}
    inline bool send_one(send_iterator it)
    {
        int sb = 0;
        if(it->packet) sb = t->unlocked_send_packet(i->data, i->size);
        else sb = t->unlocked_send_it(i->data, i->size);
        if(sb == i->size) return true;
        return false;
    }
};
typedef DArray<recv_item, Direct> recv_content;
struct RECVC
{
    RECVC(DTcp* _t) : t(_t), i(nullptr), e(nullptr) {}
    inline bool empty() const {return c.empty();}
    inline void add_item(recv_item item) {c.push_back(item);}
    inline void add_script(int from, int to, int* r, bool interupt = false)
    {
        if(from >= 0 && from < c.size() && to >= 0 && to < c.size() && from != to && r)
        {
            c[from].s.jump_to = c.begin() + to;
            c[from].s.repeats = r;
            c[from].s.index = from;
            c[from].s.interupt = interupt;
        }
    }
    inline void reserve(int s) {c.reserve(s);}
    inline void complete() {i = c.begin(); e = c.end();}
    inline void clear() {c.clear(); i = nullptr; e = nullptr;}
    inline bool unlocked_recv_all()
    {
        if(i == nullptr) return false;
        int rb = 0;
        while( i != e )
        {
            if(i->packet) rb = t->unlocked_recv_packet(i->data, &i->size);
            else rb = t->unlocked_recv_to(i->data, i->size);
            if(rb < 0) break;
            if(i->size && rb == i->size) ++i;
        }
        if(i==e)
        {
            c.remove_all();
            i = nullptr;
            e = nullptr;
            return true;
        }
        return false;
    }
    typedef recv_content::iterator recv_iterator;

    inline bool is_over() const {return i && (i==e);}
    inline int step() const {return current_script_index;}

    inline bool unlocked_recv_script()
    {
        if( i == nullptr ) return false;
        int rb = 0;
        while( i != e )
        {
            if(i->packet) rb = t->unlocked_recv_packet(i->data, &i->size);
            else rb = t->unlocked_recv_to(i->data, i->size);
            if(rb < 0) break;

            if(i->size && rb == i->size)
            {
                if(i->s.jump_to && i->s.repeats)
                {
                    auto _s = i->s;
                    current_script_index = i->s.index;
                    if(i->s.counter < *i->s.repeats)
                    {
                        ++i->s.counter;
                        i = (recv_iterator)i->s.jump_to;
                    }
                    else ++i;
                    if(_s.interupt) return true;
                }
                else ++i;
            }
        }
        if(i==e)
        {
            return true;
        }
        return false;
    }

    recv_content c;
    DTcp* t;
    recv_iterator i;
    recv_iterator e;
    int current_script_index;

    inline recv_iterator begin() {return c.begin();}
    inline recv_iterator end() {return c.end();}
    inline bool recv_one(recv_iterator it)
    {
        int rb = 0;
        if(it->packet) rb = t->unlocked_recv_packet(i->data, &i->size);
        else rb = t->unlocked_recv_to(i->data, i->size);
        if(i->size && rb == i->size) return true;
        return false;
    }
};


#endif // DTCP_H
