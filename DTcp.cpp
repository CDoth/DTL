#include "DTcp.h"
#include <iostream>

#define LOCAL_ERROR(message, ec) DTcp::local_error(message, ec)
#define FUNC_ERROR(message, ec) DTcp::func_error(__func__, message, ec)
#define WSAE WSAGetLastError()


int DTcp::smart_recv(SOCKET s, void *data, const int &len)
{
    char *d = (char*)data;
    int tb = 0;
    int recv_bytes = 0;

    do
    {
        recv_bytes = recv(s, d + tb, len - tb, 0);
        if( recv_bytes > 0)
            tb += recv_bytes;
        else
            return recv_bytes;
    }while(tb < len);
    return tb;
}
int DTcp::smart_recv_packet(SOCKET s, void* data, int &len)
{
    char *d = (char*)data;
    int tb = 0;
    int recv_bytes = 0;
    int packet_size = 0;
    len = 0;
    char* ps = (char*)&packet_size;
    do
    {
        recv_bytes = recv(s, ps + tb, 4 - tb, 0);
        if( recv_bytes > 0)
            tb += recv_bytes;
        else
        {
            std::cout << "------------------ 1 RECV FAULT " << recv_bytes << std::endl;
            return recv_bytes;
        }
    }while(tb < 4);
    tb=0;
    recv_bytes=0;

    do
    {
        recv_bytes = recv(s, d + tb, packet_size - tb, 0);
        if( recv_bytes > 0)
            tb += recv_bytes;
        else
        {
            std::cout << "------------------ 2 RECV FAULT " << recv_bytes << std::endl;
            return recv_bytes;
        }
    }while(tb < packet_size);

//    std::cout << "smart receive packet. bytes: " << recv_bytes << std::endl;

    len = packet_size;
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

void DTcp::local_error(const char *message, const int &error_code)
{
    if(error_code) std::cout << message << " " << error_code << std::endl;
    else std::cout << message <<std::endl;
}
void DTcp::func_error(const char *func_name, const char *message, const int &error_code)
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
SOCKET DTcp::__socket(const int &af, const int &type, const int &protocol)
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
int DTcp::__bind(SOCKET s, const sockaddr *addr, const int &namelen)
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
int DTcp::__listen(SOCKET s, const int &backlog)
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
int DTcp::__connect(SOCKET s, const sockaddr *name, const int &namelen)
{
    if(!is_valid_socket(s))
    {
        FUNC_ERROR("wrong socket", s);
        return SOCKET_ERROR;
    }
    if(connect(s, name, namelen) == SOCKET_ERROR)
    {
        FUNC_ERROR("connect() fault", WSAE);
        return SOCKET_ERROR;
    }
    return 0;
}
int DTcp::__shutdown(SOCKET s, const int &how)
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

int DTcp::make_server(const int &port, const char *address)
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
int DTcp::make_client(const int &port, const char *address)
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
int DTcp::send_it(void *data, int len)
{
    return send(_socket_out, (char*)data, len, 0);
}
int DTcp::receive_to(void* data, int len)
{
    return smart_recv(_socket_out, data, len);
}
int DTcp::receive_packet(void* data, int& len)
{
//    int packet_size = 0;
//    rb = receive_to(&packet_size, 4); //receive packet size

//    if(rb <= 0 )
//    {
//        qDebug()<<"DTcp::receive_packet() : Error, can't receive packet size:"<<rb;
//        len = 0;
//        return rb;
//    }
//    len = packet_size;

//    if( (rb = receive_to(data, packet_size)) <= 0)
//    {
//        qDebug()<<"DTcp::receive_packet() : Error, can't receive packet data"<<rb<<"packet size:"<<packet_size<<len;
//        len = 0;
//        return rb;
//    }
    return smart_recv_packet(_socket_out, data, len);
}
int DTcp::send_packet(void *data, const int &len)
{
    int sb = 0;
    if( (sb = send_it((char*)&len, 4)) <= 0)
    {
        FUNC_ERROR("send_it() fault (packet size)", 0);
        return sb;
    }
    if( (sb = send(_socket_out, (char*)data, len, 0)) <= 0)
    {
        FUNC_ERROR("send_it() fault (data)", 0);
        return sb;
    }
    return sb;
}
int DTcp::receive_checked_packet(void *data, int& len)
{
    int r = 0;
    r = receive_packet(data, len);

    int code = 0;

    if(r <= 0) code = recverr;
    else
    {
        if(r == len) code = recvall;
        if(r < len) code = recvless;
    }
    send_it(&code, 4);

    return r;
}
int DTcp::send_checked_packet(void *data, int len)
{
    int s = 0;
    s = send_packet(data, len);
    if(s <= 0) return s;

    int r = 0;
    int code = recverr;
    r = receive_to((char*)&code, 4);
    if(code == recvall)
        return s;
    return -1;
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
