#include "DTcp.h"
#include <iostream>

#define LOCAL_ERROR(message, ec) DTcp::local_error(message, ec)
#define FUNC_ERROR(message, ec) DTcp::func_error(__func__, message, ec)



int DTcp::receive_to(void* data, int len, int flag)
{
    int tb = 0;
    int rb = 0;
    do
    {
        rb = recv(_socket, (char*)data + tb, len - tb, flag);
        if( rb > 0 ) tb += rb;
        else return rb;

    }while(tb < len);
    return tb;
}
int DTcp::send_it(const void* data, int len, int flag)
{
    int sb = 0;
    if(  (sb = send(_socket, (const char*)data, len, flag)) < 0)
    {
    }
    return sb;
}
int DTcp::unlocked_recv_to(void* data, int len, int flag)
{
    int rb = 0;
    rb = recv(_socket, (char*)data + pr.total_rb, len - pr.total_rb, flag);
    if(rb >= 0) pr.total_rb += rb;
    else return rb;
    if(pr.total_rb == len) {int temp = pr.total_rb; pr.total_rb = 0; return temp;}
    return pr.total_rb;
}
int DTcp::unlocked_send_it(const void *data, int len, int flag)
{
    int sb = 0;
    sb = send(_socket, (const char*)data + ps.total_sb, len - ps.total_sb, flag);

    if(sb >= 0) ps.total_sb += sb;
    else return sb;
    if(ps.total_sb >= len) {int temp = ps.total_sb; ps.total_sb = 0; return temp;}
    return ps.total_sb;
}
int DTcp::recv_packet(void* data, int flag1, int flag2)
{
    int rb = 0;
    int tb = 0;
    char* d = (char*)data;
    int packet_size = 0;
    do
    {
        rb = recv(_socket, (char*)&packet_size + tb, sizeof(int) - tb, flag1);
        if( rb > 0) tb += rb;
        else return tb;
    }while(tb < (int)sizeof(int));
    tb = 0;
    do
    {
        rb = recv(_socket, d + tb, packet_size - tb, flag2);
        if( rb > 0) tb += rb;
        else return tb;
    }while(tb < packet_size);

    return tb;
}
int DTcp::send_packet(const void* data, int len, int flag1, int flag2)
{
    int sb = 0;
    if( (sb = send_it((const char*)&len, (int)sizeof(int), flag1)) < 0)
    {
        FUNC_ERROR("send_it() fault (packet size)", WSAE);
        return sb;
    }
    if( (sb = send_it((const char*)data, len, flag2)) < 0)
    {
        FUNC_ERROR("send_it() fault (data)", WSAE);
        return sb;
    }
    return sb;
}
int DTcp::unlocked_recv_packet(void* data, int* packet_size, int recv_lim, int flag1, int flag2)
{
    int rb = 0;
    if(pr.ps_rb < (int)sizeof(int))
    {
        rb = recv(_socket, (char*)&pr.packet_size + pr.ps_rb, sizeof(int) - pr.ps_rb, flag1);
        if(rb > 0) pr.ps_rb += rb;
        else return 0;
        if(packet_size) *packet_size = pr.packet_size;
    }
    int should_receive = recv_lim? recv_lim : pr.packet_size;
    if(pr.ps_rb >= (int)sizeof(int))
    {
        rb = recv(_socket, (char*)data + pr.total_rb, should_receive - pr.total_rb, flag2);
        if(rb > 0) pr.total_rb += rb;
        else return pr.total_rb;
    }
    if(should_receive && pr.total_rb >= should_receive)
    {
        int t =  should_receive;
        pr.packet_size = 0;
        pr.total_rb = 0;
        pr.ps_rb = 0;
        return t;
    }
    return pr.total_rb;
}
int DTcp::unlocked_send_packet(const void* data, int len, int flag1, int flag2)
{
    int sb = 0;
    if(ps.ps_sb < (int)sizeof(int))
    {
        sb = send(_socket, (const char*)&len, sizeof(int) - ps.ps_sb, flag1);
        if(sb >= 0) ps.ps_sb += sb;
        else return sb;
    }
    if(ps.ps_sb >= (int)sizeof(int))
    {
        sb = send(_socket, (const char*)data + ps.total_sb, len - ps.total_sb, flag2);
        if(sb >= 0) ps.total_sb += sb;
        else return sb;
    }
    if(ps.total_sb >= len)
    {
        ps.total_sb = 0;
        ps.ps_sb = 0;
        return len;
    }
    return ps.total_sb;
}


int DTcp::set_stat(int port, const char *address)
{
    if(port >= 0 && port <= 65535)
    {
        _addr.sin_port = htons(port);
#ifdef _WIN32
        _addr.sin_addr.S_un.S_addr = address ? inet_addr(address) : htonl(INADDR_ANY);
#else
        _addr.sin_addr.s_addr = address ? inet_addr(address) : htonl(INADDR_ANY);
#endif
    }
    else
    {
        FUNC_ERROR("invalid port number", port);
        return -1;
    }
    return 0;
}

void DTcp::local_error(const char* message, int error_code)
{
    if(error_code) std::cout << message << " " << error_code << std::endl;
    else std::cout << message <<std::endl;
}
void DTcp::func_error(const char* func_name, const char *message, int error_code)
{
    if(error_code) std::cout << "Error in "<<func_name<<": "<<message<<" "<<error_code<<std::endl;
    else std::cout << "Error in " << func_name << ": " << message<< std::endl;
}
void DTcp::info(const char *message)
{
    std::cout << "DTcp: " << message << std::endl;
}
int DTcp::is_valid_socket(SOCKET s)
{
    if(s && s != INVALID_SOCKET) return 1;
    return 0;
}
//---------------------------------------------------------------------------------------
SOCKET DTcp::__socket(int af, int type, int protocol)
{
    SOCKET new_socket = INVALID_SOCKET;
    if( (new_socket = socket(af, type, protocol)) == INVALID_SOCKET)
    {
        FUNC_ERROR("Can't create socket", 0);
        return INVALID_SOCKET;
    }
    return new_socket;
}
SOCKET DTcp::__accept(SOCKET s, sockaddr *addr, int *addrlen)
{
    if(!is_valid_socket(s))
    {
        FUNC_ERROR("wrong socket", s);
        return INVALID_SOCKET;
    }
    SOCKET accept_socket = INVALID_SOCKET;

#ifdef _WIN32
    if( (accept_socket = accept(s, addr, addrlen)) == INVALID_SOCKET)
    {
        FUNC_ERROR("accept() fault", WSAE);
        return INVALID_SOCKET;
    }
#else
    if( (accept_socket = accept(s, addr, (socklen_t *)addrlen)) == INVALID_SOCKET)
    {
        FUNC_ERROR("accept() fault", WSAE);
        return INVALID_SOCKET;
    }
#endif
    return accept_socket;
}
int DTcp::__closesocket(SOCKET s)
{
    if(!is_valid_socket(s))
    {
        FUNC_ERROR("wrong socket", s);
        return INVALID_SOCKET;
    }
#ifdef _WIN32
    if(closesocket(s) == INVALID_SOCKET)
    {
        FUNC_ERROR("closesocket() fault", WSAE);
        return INVALID_SOCKET;
    }
#else
    close(s);
#endif

    return 0;
}
int DTcp::__bind(SOCKET s, const sockaddr *addr, int namelen)
{
    if(!is_valid_socket(s))
    {
        FUNC_ERROR("wrong socket", s);
        return SOCKET_ERROR;
    }
    if(bind(s, addr, namelen) == SOCKET_ERROR)
    {
        FUNC_ERROR("bind() fault", WSAE);
        return SOCKET_ERROR;
    }
    return 0;
}
int DTcp::__listen(SOCKET s, int backlog)
{
    if(!is_valid_socket(s))
    {
        FUNC_ERROR("wrong socket", s);
        return SOCKET_ERROR;
    }
    if(listen(s, backlog) == SOCKET_ERROR)
    {
        FUNC_ERROR("listen() fault", WSAE);
        return SOCKET_ERROR;
    }
    return 0;
}
int DTcp::__connect(SOCKET s, const sockaddr *name, int namelen)
{
    if(!is_valid_socket(s))
    {
        FUNC_ERROR("wrong socket", s);
        return SOCKET_ERROR;
    }
    if(connect(s, name, namelen) == SOCKET_ERROR)
    {
        connect(s, name, namelen);
        return WSAE;
    }
    return 0;
}
int DTcp::__shutdown(SOCKET s, int how)
{
    if(!is_valid_socket(s))
    {
        FUNC_ERROR("wrong socket", s);
        return SOCKET_ERROR;
    }
    if(shutdown(s, how) == SOCKET_ERROR)
    {
        FUNC_ERROR("shutdown() fault", WSAE);
        return SOCKET_ERROR;
    }

    return 0;
}
int DTcp::__select(fd_set* read_set, fd_set* write_set, fd_set* err_set, struct timeval* tv)
{
    if(!is_valid_socket(_socket))
    {
        FUNC_ERROR("wrong socket", _socket);
        return SOCKET_ERROR;
    }
#ifdef _WIN32
    int r = select(0, read_set, write_set, err_set, tv);
#else
    int r = select(_socket+1, read_set, write_set, err_set, tv);
#endif
    if(r == SOCKET_ERROR)
    {
        FUNC_ERROR("select() fault", WSAE);
        return SOCKET_ERROR;
    }
    return r;
}

//---------------------------------------------------------------------------------------
DTcp::DTcp()
{
    #ifdef _WIN32
    ZeroMemory(&_addr,sizeof(sockaddr_in));
    #else
    memset(&_addr, 0, sizeof(sockaddr_in));
    #endif
    _addr.sin_family = AF_INET;
    _socket = INVALID_SOCKET;

    pr.packet_size = 0;
    pr.total_rb = 0;
    pr.ps_rb = 0;

    ps.total_sb = 0;
    ps.ps_sb = 0;
}
void DTcp::unblock()
{
#ifdef _WIN32
    BOOL l = TRUE;
    ioctlsocket(_socket, FIONBIO, (unsigned long* ) &l);
#else
    int flags = fcntl(_socket, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(_socket, F_SETFL, flags);
#endif
}
int DTcp::make_server(int port, const char *address)
{
    if( set_stat(port, address)) return -1;
    if( (_socket = __socket(AF_INET,SOCK_STREAM,0)) == INVALID_SOCKET ) return -1;
    if( __bind(_socket, (sockaddr* ) &_addr, sizeof (_addr)) ) return -1;
    if( __listen(_socket, 10) ) return -1;
    return 0;
}
int DTcp::make_client(int port, const char *address)
{
    if( set_stat(port, address) ) return -1;
    if( (_socket = __socket(AF_INET,SOCK_STREAM,0)) == INVALID_SOCKET) return -1;
    return 0;
}
int DTcp::wait_connection()
{
    if(!is_valid_socket(_socket))
    {
        FUNC_ERROR("wrong socket", _socket);
        return -1;
    }
    info("wait connection...");
    int new_len = sizeof(sockaddr_in);
    if( (_socket = __accept (_socket, (sockaddr* ) &_addr, &new_len) ) == INVALID_SOCKET) return -1;
    info("connected");
    return 0;
}
int DTcp::try_connect()
{
    info("try connect...");
    if(__connect (_socket, (sockaddr* ) &_addr, sizeof (sockaddr_in))) return -1;
    info("connected");
    return 0;
}
int DTcp::wait_connection(int port, const char *address)
{
    if( set_stat(port, address)) return -1;
    if( (_socket = __socket(AF_INET,SOCK_STREAM,0)) == INVALID_SOCKET ) return -1;
    if( __bind(_socket, (sockaddr* ) &_addr, sizeof (_addr)) ) return -1;
    if( __listen(_socket, 10) ) return -1;
    info("wait connection...");
    int new_len = sizeof(sockaddr_in);
    if( (_socket = __accept (_socket, (sockaddr* ) &_addr, &new_len) ) == INVALID_SOCKET) return -1;
    info("connected");
    return 0;
}
int DTcp::try_connect(int port, const char *address)
{
    if( set_stat(port, address) ) return -1;
    if( (_socket = __socket(AF_INET,SOCK_STREAM,0)) == INVALID_SOCKET) return -1;
    info("try connect...");
    if(__connect (_socket, (sockaddr* ) &_addr, sizeof (sockaddr_in))) return -1;
    info("connected");
    return 0;
}
int DTcp::check_readability(int sec, int usec)
{
    if(!is_valid_socket(_socket))
    {
        FUNC_ERROR("wrong socket", _socket);
        return -1;
    }
    fd_set fdread;
    struct timeval timeout;
    timeout.tv_sec = sec;
    timeout.tv_usec = usec;

    FD_ZERO(&fdread);
    FD_SET(_socket, &fdread);
    if( __select(&fdread, nullptr, nullptr, &timeout)  > 0) return FD_ISSET(_socket, &fdread);
    else return WSAE;
    return 0;
}
int DTcp::check_writability(int sec, int usec)
{
    if(!is_valid_socket(_socket))
    {
        FUNC_ERROR("wrong socket", _socket);
        return -1;
    }
    fd_set fdwrite;
    struct timeval timeout;
    timeout.tv_sec = sec;
    timeout.tv_usec = usec;

    FD_ZERO(&fdwrite);
    FD_SET(_socket, &fdwrite);
    if( __select(nullptr, &fdwrite, nullptr, &timeout)  > 0) return FD_ISSET(_socket, &fdwrite);
    else return WSAE;
    return 0;
}
int DTcp::stop()
{
    int r = 0;
    if( (r = __shutdown(_socket, SD_BOTH)) ) return r;
    if( (r = __closesocket(_socket)) ) return r;
    _socket = INVALID_SOCKET;
    return 0;
}
