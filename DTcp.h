#ifndef DTCP_H
#define DTCP_H


#include <winsock2.h>
#include <windows.h>

static void winsock_start()
{
    char buff[1024];
    WSAStartup(0x202,(WSADATA *)&buff[0]);
}
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

    int receive_to(void* data, int len);
    inline int send_it(const void* data, int len, int flag = 0);

    int send_packet(const void* data, int len, int flag1 = 0, int flag2 = 0);

    int check_readability(int sec = 0, int usec = 0);
    int check_writability(int sec = 0, int usec = 0);


    int stop_in();
    int stop_out();

    int unlocked_recv_packet(void* data, int *packet_size, int flag1 = 0, int flag2 = 0);
    inline int locked_recv_packet(void* data, int flag1 = 0, int flag2 = 0);

private:
    int smart_recv(SOCKET s, void* data, int len);


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

    struct packet_info
    {
        int total_rb;

        int packet_size;
        int ps_rb;
    };

    packet_info p;
};

#endif // DTCP_H
