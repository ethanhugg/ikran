#include "sockporting.h"
#include <errno.h>

#ifdef WIN32

#else
/*linux*/

int WSAGetLastError()
{
    return errno;
}


int WSAStartup(short wVersionRequested, WSADATA *lpWSAData)
{
    return 0;
}

int WSACleanup()
{
    return 0;
}

int closesocket(SOCKET sock)
{
    return close(sock);
}

#endif
