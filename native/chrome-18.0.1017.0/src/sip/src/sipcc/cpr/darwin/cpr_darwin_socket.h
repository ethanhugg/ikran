/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Cisco Systems SIP Stack.
 *
 * The Initial Developer of the Original Code is
 * Cisco Systems (CSCO).
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Enda Mannion <emannion@cisco.com>
 *  Suhas Nandakumar <snandaku@cisco.com>
 *  Ethan Hugg <ehugg@cisco.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef _CPR_DARWIN_SOCKET_H_
#define _CPR_DARWIN_SOCKET_H_

#include "cpr_types.h"
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>

/*
 * Define socket option levels
 *   SOL_SOCKET
 *   SOL_IP
 *   SOL_TCP
 *   SOL_UDP
 */

/* defined in netinet/in.h */
#define SOL_IP      IPPROTO_IP
#define SOL_TCP     IPPROTO_TCP
#define SOL_UDP     IPPROTO_UDP


/**
 * Set public CPR header file options
 */
#ifdef CPR_USE_SOCKETPAIR
#undef CPR_USE_SOCKETPAIR
#endif
#define SUPPORT_CONNECT_CONST const

/**
 * Define SOCKET_ERROR
 */
#define SOCKET_ERROR   (-1)

/**
 * Define INVALID_SOCKET
 */
#define INVALID_SOCKET (-1)

/**
 * Define cpr_socket_t
 */
typedef int cpr_socket_t;

/**
 * Define cpr_socklen_t
 */
typedef socklen_t cpr_socklen_t;

/**
 * Address family, defined in sys/socket.h
 *  AF_UNSPEC
 *  AF_LOCAL / AF_UNIX
 *  AF_INET
 *  AF_INET6
 *  AF_MAX
 *
 *  AF_NETLYR2 (unique to CNU) interface directly to layer 2, bypass IP
 */
#ifndef AF_UNIX
#define AF_UNIX  AF_LOCAL
#endif


/*
 * Constants and structures defined by the internet system,
 * Per RFC 790, September 1981, based on the BSD file netinet/in.h.
 * IPv6 additions per RFC 2292.
 */


/*
 * Define the following socket options as needed
 *   SO_DEBUG
 *   SO_ACCEPTCONN
 *   SO_REUSEADDR / SO_EXCLUSIVEADDRUSE
 *   SO_KEEPALIVE
 *   SO_DONTROUTE
 *   SO_BROADCAST
 *   SO_USELOOPBACK
 *   SO_LINGER / SO_DONTLINGER
 *   SO_OOBINLINE
 *   SO_SNDBUF
 *   SO_RCVBUF
 *   SO_ERROR
 *   SO_TYPE
 *
 * The following options are available for Unix-only variants
 *   SO_SNDLOWAT
 *   SO_RCVLOWAT
 *   SO_SNDTIMEO
 *   SO_RCVTIMEO
 *   SO_PROTOTYPE - Not documented as being supported by CNU
 */

/* defined in netinet/in.h */
#define SO_DONTLINGER       ((int)(~SO_LINGER))
#define SO_EXCLUSIVEADDRUSE ((int)(~SO_REUSEADDR))

/*
 * Protocols (Base),
 * reference http://www.iana.org/assignments/protocol-numbers.html
 *   IPPROTO_IP
 *   IPPROTO_GGP
 *   IPPROTO_ICMP
 *   IPPROTO_IGMP
 *   IPPROTO_IPV4 / IPPROTO_IPIP
 *   IPPROTO_TCP
 *   IPPROTO_EGP
 *   IPPROTO_PUP
 *   IPPROTO_UDP
 *   IPPROTO_IDP
 *   IPPROTO_IPV6
 *   IPPROTO_ROUTING
 *   IPPROTO_FRAGMENT
 *   IPPROTO_ESP
 *   IPPROTO_AH
 *   IPPROTO_ICMPV6
 *   IPPROTO_NONE
 *   IPPROTO_DSTOPTS
 *   IPPROTO_ND
 *   IPPROTO_EON
 *   IPPROTO_IGRP
 *   IPPROTO_ENCAP
 *   IPPROTO_IPCOMP
 *   IPPROTO_RAW
 *   IPPROTO_MAX
 */

/* defined in netinet/in.h */
#ifndef IPPROTO_IPV4
#define IPPROTO_IPV4         4
#endif

#ifndef IPPROTO_IPIP
#define IPPROTO_IPIP  IPPROTO_IPV4
#endif

//#define IPPROTO_RSVP        46
#define IPPROTO_IGRP        88  /* Cisco/GXS IGRP */
#define IPPROTO_EIGRP       88
#define IPPROTO_IPCOMP     108  /* IP payload compression */

/*
 * Protocols (IPv6)
 *   Assumming if IPV6 is not there, then none of the are
 */
#ifndef IPPROTO_IPV6
#define IPPROTO_HOPOPTS      0  /* IPv6 hop-by-hop options */
#define IPPROTO_IPV6        41  /* IPv6 */
#define IPPROTO_ROUTING     43  /* IPv6 routing header */
#define IPPROTO_FRAGMENT    44  /* IPv6 fragmentation header */
#define IPPROTO_ICMPV6      58  /* ICMPv6 */
#define IPPROTO_NONE        59  /* IPv6 no next header */
#define IPPROTO_DSTOPTS     60  /* IPv6 destination options */
#endif

/*
 * Protocols (Local)
 * reference, RFC 3692
 *   IPPROTO_UNX       Local sockets Unix protocol
 *   IPPROTO_CDP       Non-Standard at 254, technially this value
 *                     is for experimentation and testing
 */

/* defined in netinet/in.h */

/*
 * Port/socket numbers: network standard functions
 * reference http://www.iana.org/assignments/port-numbers
 *  IPPORT_ECHO
 *  IPPORT_DISCARD
 *  IPPORT_SYSTAT
 *  IPPORT_DAYTIME
 *  IPPORT_NETSTAT
 *  IPPORT_FTP
 *  IPPORT_SSH
 *  IPPORT_TELNET
 *  IPPORT_SMTP
 *  IPPORT_TIMESERVER
 *  IPPORT_NAMESERVER
 *  IPPORT_WHOIS
 *  IPPORT_MTP
 *  IPPORT_HTTP
 *  IPPORT_NTP
 */

/* defined in netinet/in.h */

/*
 * Port/socket numbers: host specific functions
 *
 *  IPPORT_TFTP
 *  IPPORT_RJE
 *  IPPORT_FINGER
 *  IPPORT_TTYLINK
 *  IPPORT_SUPDUP
 */

/* defined in netinet/in.h */

/*
 * UNIX TCP sockets
 *
 *  IPPORT_EXECSERVER
 *  IPPORT_LOGINSERVER
 *  IPPORT_CMDSERVER
 *  IPPORT_EFSSERVER
 */

/* defined in netinet/in.h */

/*
 * UNIX UDP sockets
 *
 *  IPPORT_BIFFUDP
 *  IPPORT_WHOSERVER
 *  IPPORT_ROUTESERVER
 */

/* defined in netinet/in.h */

/*
 * SCCP sockets
 */
#define IPPORT_SCCP       2000

/*
 * tbd: need to finalize placement.
 * Define range of ephemeral ports used for Cisco IP Phones.
 */
#define CIPPORT_EPH_LOW         0xC000
#define CIPPORT_EPH_HI          0xCFFF


/*
 * Ports < IPPORT_RESERVED are reserved for
 * privileged processes (e.g. root).
 *
 * IPPORT_RESERVED
 * IPPORT_USERRESERVED
 */

/* defined in netinet/in.h */

/*
 * Define INADDR constants
 *   INADDR_ANY
 *   INADDR_LOOPBACK
 *   INADDR_BROADCAST
 */

/* defined in netinet/in.h */

/*
 * Define IN_CLASS constants/macros
 *   IN_CLASS{A|B|C|D}(x)
 *   IN_CLASS{A|B|C|D}_NET
 *   IN_CLASS{A|B|C|D}_NSHIFT
 *   IN_CLASS{A|B|C|D}_HOST
 *   IN_CLASS{A|B|C|D}_MAX
 */

typedef struct sockaddr_un cpr_sockaddr_un_t;


#define cpr_sun_len(a) sizeof(a)
void cpr_set_sockun_addr(cpr_sockaddr_un_t *addr, const char *name);

/*
 * To represent desired sockaddr max alignment for platform, a
 * type is chosen which may depend on implementation platform architecture.
 * Type chosen based on alignment size restrictions from <sys/isa_defs.h>.
 * We desire to force up to (but no more than) 64-bit (8 byte) alignment,
 * on platforms where it is possible to do so. (e.g not possible on ia32).
 * For all currently supported platforms by our implementation
 * in <sys/isa_defs.h>, (i.e. sparc, sparcv9, ia32, ia64)
 * type "double" is suitable for that intent.
 *
 * Note: Type "double" is chosen over the more obvious integer type int64_t.
 *   int64_t is not a valid type for strict ANSI/ISO C compilation on ILP32.
 */
typedef double          sockaddr_maxalign_t;

/*
 * WinSock 2 extension -- new options
 */
#define SO_MAX_MSG_SIZE   0x2003    /* maximum message size */


#define SO_NBIO                 0x0400  /* Nonblocking socket I/O operation */
#define SO_ASYNC                0x0800  /* should send asyn notification of
                                         * I/O events */
#define SO_VRFTABLEID           0x1000  /* set VRF routing table id */
#define SO_SRC_SPECIFIED        0x2000  /* Specified Source Address to be used */
#define SO_STRICT_ADDR_BIND     0x4000  /* Accept only those packets that have
                                         * been sent to the address that this
                                         * socket is bound to */
/*
 * Used for getting random port for tls connect
 */
#define TCP_PORT_RETRY_CNT      5
#define TCP_PORT_MASK           0xfff

#endif
