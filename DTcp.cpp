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
        rb = recv(_socket_out, (char*)data + tb, len - tb, flag);
        if( rb > 0 ) tb += rb;
        else return rb;

    }while(tb < len);
    return tb;
}
int DTcp::send_it(const void* data, int len, int flag)
{
    int sb = 0;
    if(  (sb = send(_socket_out, (const char*)data, len, flag)) < 0)
    {
    }
    return sb;
}
int DTcp::unlocked_recv_to(void* data, int len, int flag)
{
    int rb = 0;
    rb = recv(_socket_out, (char*)data + pr.total_rb, len - pr.total_rb, flag);
    if(rb >= 0) pr.total_rb += rb;
    else return rb;
    if(pr.total_rb == len) {int temp = pr.total_rb; pr.total_rb = 0; return temp;}
    return pr.total_rb;
}
int DTcp::unlocked_send_it(const void *data, int len, int flag)
{
    int sb = 0;
    sb = send(_socket_out, (const char*)data + ps.total_sb, len - ps.total_sb, flag);

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
        rb = recv(_socket_out, (char*)&packet_size + tb, sizeof(int) - tb, flag1);
        if( rb > 0) tb += rb;
        else return tb;
    }while(tb < (int)sizeof(int));
    tb = 0;
    do
    {
        rb = recv(_socket_out, d + tb, packet_size - tb, flag2);
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
        rb = recv(_socket_out, (char*)&pr.packet_size + pr.ps_rb, sizeof(int) - pr.ps_rb, flag1);
        if(rb > 0) pr.ps_rb += rb;
        else return 0;
        if(packet_size) *packet_size = pr.packet_size;
    }
    int should_receive = recv_lim? recv_lim : pr.packet_size;
    if(pr.ps_rb >= (int)sizeof(int))
    {
        rb = recv(_socket_out, (char*)data + pr.total_rb, should_receive - pr.total_rb, flag2);
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
        sb = send(_socket_out, (const char*)&len, sizeof(int) - ps.ps_sb, flag1);
        if(sb >= 0) ps.ps_sb += sb;
        else return sb;
    }
    if(ps.ps_sb >= (int)sizeof(int))
    {
        sb = send(_socket_out, (const char*)data + ps.total_sb, len - ps.total_sb, flag2);
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


int DTcp::set_in(const int &port, const char *address)
{
    if(port >= 0 && port <= 65535)
    {
        _addr_inner.sin_port = htons(port);

#ifdef _WIN32
        if(address)
            _addr_inner.sin_addr.S_un.S_addr = inet_addr(address);
        else
            _addr_inner.sin_addr.S_un.S_addr = htonl (INADDR_ANY);
#else
        if(address)
            _addr_inner.sin_addr.s_addr = inet_addr(address);
        else
            _addr_inner.sin_addr.s_addr = htonl (INADDR_ANY);
#endif
    }
    else
    {
        FUNC_ERROR("invalid port number", port);
        return -1;
    }
    return 0;
}
int DTcp::set_out(const int &port, const char *address)
{
    if(port >= 0 && port <= 65535)
    {
        _addr_outer.sin_port = htons(port);
#ifdef _WIN32
        if(address)
            _addr_outer.sin_addr.S_un.S_addr = inet_addr(address);
        else
            _addr_outer.sin_addr.S_un.S_addr = htonl (INADDR_ANY);
#else
        if(address)
            _addr_outer.sin_addr.s_addr = inet_addr(address);
        else
            _addr_outer.sin_addr.s_addr = htonl (INADDR_ANY);
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
    if(!is_valid_socket(_socket_out))
    {
        FUNC_ERROR("wrong socket", _socket_out);
        return SOCKET_ERROR;
    }
#ifdef _WIN32
    int r = select(0, read_set, write_set, err_set, tv);
#else
    int r = select(_socket_out+1, read_set, write_set, err_set, tv);
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
    _me = Default;
    #ifdef _WIN32
    ZeroMemory(&_addr_inner,sizeof(sockaddr_in));
    ZeroMemory(&_addr_outer,sizeof(sockaddr_in));
    #else
    memset(&_addr_inner, 0, sizeof(sockaddr_in));
    memset(&_addr_outer, 0, sizeof(sockaddr_in));
    #endif
    _addr_inner.sin_family = AF_INET;
    _addr_outer.sin_family = AF_INET;

    _socket_in = INVALID_SOCKET;
    _socket_out = INVALID_SOCKET;

    pr.packet_size = 0;
    pr.total_rb = 0;
    pr.ps_rb = 0;

    ps.total_sb = 0;
    ps.ps_sb = 0;
}
void DTcp::unblock_in()
{
#ifdef _WIN32
    BOOL l = TRUE;
    ioctlsocket(_socket_in, FIONBIO, (unsigned long* ) &l);
#else
    int flags = fcntl(_socket_in, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(_socket_in, F_SETFL, flags);
#endif
}
void DTcp::unblock_out()
{
#ifdef _WIN32
    BOOL l = TRUE;
    ioctlsocket(_socket_out, FIONBIO, (unsigned long* ) &l);
#else
    int flags = fcntl(_socket_out, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(_socket_out, F_SETFL, flags);
#endif
}
int DTcp::make_server(int port, const char *address)
{
//    stop_in();
//    stop_out();
    if( set_in(port, address)) return -1;
    if( (_socket_in = __socket(AF_INET,SOCK_STREAM,0)) == INVALID_SOCKET ) return -1;
    if( __bind(_socket_in, (sockaddr* ) &_addr_inner, sizeof (_addr_inner)) ) return -1;
    if( __listen(_socket_in, 10) ) return -1;
    _me = Server;
    return 0;
}
int DTcp::make_client(int port, const char *address)
{
//    stop_in();
//    stop_out();
    if( set_out(port, address) ) return -1;
    if( (_socket_out = socket(AF_INET,SOCK_STREAM,0)) == INVALID_SOCKET) return -1;
    _me = Client;
    info("Set Client");
    return 0;
}
int DTcp::wait_connection()
{
    if(_me != Server)
    {
        FUNC_ERROR("I am not server", 0);
        return -1;
    }
    if(!is_valid_socket(_socket_in))
    {
        FUNC_ERROR("wrong socket", _socket_in);
        return -1;
    }
    info("wait connection...");
    int new_len = sizeof(sockaddr_in);
    if( (_socket_out = __accept (_socket_in, (sockaddr* ) &_addr_outer, &new_len) ) == INVALID_SOCKET) return -1;
    info("connected");
    return 0;
}
int DTcp::try_connect()
{
    if(_me != Client)
    {
        FUNC_ERROR("I am not client", _me);
        return -1;
    }
    info("try connect...");
    if(__connect (_socket_out, (sockaddr* ) &_addr_outer, sizeof (sockaddr_in))) return -1;
    info("connected");
    return 0;
}


int DTcp::check_readability(int sec, int usec)
{
    if(!is_valid_socket(_socket_out))
    {
        FUNC_ERROR("wrong socket", _socket_out);
        return -1;
    }
    fd_set fdread;
    int r_select = 0;

    struct timeval timeout;
    timeout.tv_sec = sec;
    timeout.tv_usec = usec;

    FD_ZERO(&fdread);
    FD_SET(_socket_out, &fdread);
    if( __select(&fdread, nullptr, nullptr, &timeout)  > 0)
    {
        return FD_ISSET(_socket_out, &fdread);
    }

    return 0;
}
int DTcp::check_writability(int sec, int usec)
{
    if(!is_valid_socket(_socket_out))
    {
        FUNC_ERROR("wrong socket", _socket_out);
        return -1;
    }
    fd_set fdwrite;
    int r_select = 0;

    struct timeval timeout;
    timeout.tv_sec = sec;
    timeout.tv_usec = usec;

    FD_ZERO(&fdwrite);
    FD_SET(_socket_out, &fdwrite);
    if( __select(nullptr, &fdwrite, nullptr, &timeout)  > 0)
    {
        return FD_ISSET(_socket_out, &fdwrite);
    }

    return 0;
}
int DTcp::stop_in()
{
    int r = 0;
    if( (r = __shutdown(_socket_in, SD_BOTH)) ) return r;
    if( (r = __closesocket(_socket_in)) ) return r;
    _socket_in = INVALID_SOCKET;
    return 0;
}
int DTcp::stop_out()
{
    int r = 0;
    if( (r = __shutdown(_socket_out, SD_BOTH)) ) return r;
    if( (r = __closesocket(_socket_out)) ) return r;
    _socket_out = INVALID_SOCKET;
    return 0;
}
