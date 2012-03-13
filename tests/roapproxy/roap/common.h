#ifndef _COMMON_
#define _COMMON_
#ifndef WIN32
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <fcntl.h>
#else
#include <winsock2.h>
#include <Windows.h>
#endif

#endif