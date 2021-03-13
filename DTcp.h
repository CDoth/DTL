#ifndef DTCP_H
#define DTCP_H
#include <stdint.h>
#include "DArray.h"

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
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

//    void forced_send_it(const void* data, int len, int flag = 0);
    int send_it(const void* data, int len, int flag = 0);
    int unlocked_send_it(const void* data, int len, int flag = 0);
    int send_packet(const void* data, int len, int flag1 = 0, int flag2 = 0);
    int unlocked_send_packet(const void* data, int len, int flag1 = 0, int flag2 = 0);


//    void forced_receive_to(void* data, int len, int flag = 0);
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
struct send_item
{
    const void* data;
    int size;
    bool packet;
};
struct recv_item
{
    void* data;
    int size;
    bool packet;
};

typedef DArray<send_item, SlowWriteFastRead> send_content;
struct SENDC
{
    SENDC(DTcp* _t) : t(_t), i(nullptr), e(nullptr) {}
    inline bool empty() const {return c.empty();}
    inline void add_item(send_item item) {c.push_back(item);}
    inline void reserve(int s) {c.reserve(s);}
    inline void complete() {i = c.constBegin(); e = c.constEnd();}
    inline void clear() {c.clear();}
    bool unlocked_send()
    {
        if(i == nullptr) return false;
        int sb = 0;
        while( i != e )
        {
            if(i->packet) sb = t->unlocked_send_packet(i->data, i->size);
            else sb = t->unlocked_send_it(i->data, i->size);
//            qDebug()<<"send item:"<<sb<<i->size<<*(size_t*)i->data;
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
    send_content c;
    DTcp* t;
    send_content::const_iterator i;
    send_content::const_iterator e;
};
typedef DArray<recv_item, SlowWriteFastRead> recv_content;
struct RECVC
{
    RECVC(DTcp* _t) : t(_t), i(nullptr), e(nullptr) {}
    inline bool empty() const {return c.empty();}
    inline void add_item(recv_item item) {c.push_back(item);}
    inline void reserve(int s) {c.reserve(s);}
    inline void complete() {i = c.begin(); e = c.end();}
    inline void clear() {c.clear();}
    bool unlocked_recv()
    {
        if(i == nullptr) return false;
        int rb = 0;
        while( i != e )
        {
            if(i->packet) rb = t->unlocked_recv_packet(i->data, &i->size);
            else rb = t->unlocked_recv_to(i->data, i->size);
//            qDebug()<<"try recv item:"<<rb<<i->size<<*(size_t*)i->data;
            if(i->size && rb == i->size) ++i;
            if(rb < 0) break;
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
    recv_content c;
    DTcp* t;
    recv_content::iterator i;
    recv_content::iterator e;
};


#endif // DTCP_H
