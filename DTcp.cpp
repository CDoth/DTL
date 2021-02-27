#include "DTcp.h"
#include <iostream>
#include <QDebug>

#define LOCAL_ERROR(message, ec) DTcp::local_error(message, ec)
#define FUNC_ERROR(message, ec) DTcp::func_error(__func__, message, ec)
#define WSAE WSAGetLastError()


int DTcp::smart_recv(SOCKET s, void *data, int len)
{
    char *d = (char*)data;
    int tb = 0;
    int recv_bytes = 0;

    do
    {
        recv_bytes = recv(s, d + tb, len - tb, 0);
        if( recv_bytes > 0)
        {
//            qDebug()<<"smart receive:"<<recv_bytes<<"total:"<<tb;
            tb += recv_bytes;
        }
        else
        {
//            qDebug()<<"smart receive: fault:"<<recv_bytes;
            return tb;
        }
    }while(tb < len);
    return tb;
}
int DTcp::unlocked_recv_packet(void* data, int* packet_size, int recv_lim, int flag1, int flag2)
{
    int rb = 0;
    if(p.ps_rb < (int)sizeof(int))
    {
        rb = recv(_socket_out, (char*)&p.packet_size + p.ps_rb, sizeof(int) - p.ps_rb, flag1);
        if(rb > 0) p.ps_rb += rb;
        else return 0;
        if(packet_size) *packet_size = p.packet_size;
    }
    int should_receive = recv_lim? recv_lim : p.packet_size;
    if(p.ps_rb >= (int)sizeof(int))
    {
        rb = recv(_socket_out, (char*)data + p.total_rb, should_receive - p.total_rb, flag2);
        if(rb > 0) p.total_rb += rb;
        else return p.total_rb;
    }
    if(should_receive && p.total_rb >= should_receive)
    {
        int t =  should_receive;
        p.packet_size = 0;
        p.total_rb = 0;
        p.ps_rb = 0;
        return t;
    }
    return p.total_rb;
}
inline int DTcp::locked_recv_packet(void* data, int flag1, int flag2)
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
int DTcp::set_in(const int &port, const char *address)
{
    if(port >= 0 && port <= 65535)
    {
        _addr_inner.sin_port = htons(port);
        if(address)
            _addr_inner.sin_addr.S_un.S_addr = inet_addr(address);
        else
            _addr_inner.sin_addr.S_un.S_addr = htonl (INADDR_ANY);
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
        if(address)
            _addr_outer.sin_addr.S_un.S_addr = inet_addr(address);
        else
            _addr_outer.sin_addr.S_un.S_addr = htonl (INADDR_ANY);
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
    if( (accept_socket = accept(s, addr, addrlen)) == INVALID_SOCKET)
    {
        FUNC_ERROR("accept() fault", WSAE);
        return INVALID_SOCKET;
    }
    return accept_socket;
}
int DTcp::__closesocket(SOCKET s)
{
    if(!is_valid_socket(s))
    {
        FUNC_ERROR("wrong socket", s);
        return SOCKET_ERROR;
    }
    if(closesocket(s) == SOCKET_ERROR)
    {
        FUNC_ERROR("closesocket() fault", WSAE);
        return SOCKET_ERROR;
    }

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

        while(WSAE == WSAEWOULDBLOCK)
        {
            qDebug()<<"TRY CONNECT AFTER WSAEWOULDBLOCK";
            connect(s, name, namelen);
        }
        qDebug()<<"WSA Error:"<<WSAE;
        return WSAE;

//        FUNC_ERROR("connect() fault", WSAE);
//        qDebug()<<"errno:"<<errno;
//        return SOCKET_ERROR;
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
//---------------------------------------------------------------------------------------

DTcp::DTcp()
{
    _me = Default;
    ZeroMemory(&_addr_inner,sizeof(sockaddr_in));
    ZeroMemory(&_addr_outer,sizeof(sockaddr_in));
    _addr_inner.sin_family = AF_INET;
    _addr_outer.sin_family = AF_INET;

    _socket_in = INVALID_SOCKET;
    _socket_out = INVALID_SOCKET;

    p.packet_size = 0;
    p.total_rb = 0;
    p.ps_rb = 0;
}

void DTcp::unblock_in()
{
    BOOL l = TRUE;
    ioctlsocket(_socket_in, FIONBIO, (unsigned long* ) &l);
}
void DTcp::unblock_out()
{
    BOOL l = TRUE;
    ioctlsocket(_socket_out, FIONBIO, (unsigned long* ) &l);
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
inline int DTcp::send_it(const void* data, int len, int flag)
{
    return send(_socket_out, (char*)data, len, flag);
}
int DTcp::receive_to(void* data, int len)
{
    return smart_recv(_socket_out, data, len);
}
int DTcp::send_packet(const void* data, int len, int flag1, int flag2)
{
    int sb = 0;
    if( (sb = send_it((const char*)&len, (int)sizeof(int), flag1)) <= 0)
    {
        FUNC_ERROR("send_it() fault (packet size)", 0);
        return sb;
    }
    if( (sb = send_it((const char*)data, len, flag2)) <= 0)
    {
        FUNC_ERROR("send_it() fault (data)", 0);
        return sb;
    }
    return sb;
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
    if( (r_select = select(0, &fdread, nullptr, nullptr, &timeout) ) == SOCKET_ERROR )
    {
        FUNC_ERROR("select() fault", WSAE);
        return 0;
    }
    else
    {
        if(r_select > 0)
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
    if( (r_select = select(0, nullptr, &fdwrite, nullptr, &timeout) ) == SOCKET_ERROR )
    {
        FUNC_ERROR("select() fault", WSAE);
        return 0;
    }
    else
    {
        if(r_select > 0)
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
