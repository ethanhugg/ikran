#ifndef _SOCK_PORTING_H_
#define _SOCK_PORTING_H_

#ifdef WIN32
#include <winsock2.h>
#define socklen_t                   int
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#define SOCKET                      int
#define WSADATA                     int
#define INVALID_SOCKET              (-1)
#define SOCKET_ERROR                (-1)

int WSAGetLastError();
int WSAStartup(short wVersionRequested, WSADATA *lpWSAData);
int WSACleanup();
int closesocket(SOCKET sock);

#endif


#endif
