/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief socket to communicate with CSPTP
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#include "src/sock.h"
#include "src/log.h"
#include "src/swap.h"

#ifdef HAVE_POLL_H
#include <poll.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef __linux__
#include <linux/net_tstamp.h>
#endif
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define poll(fds, nfds, timeout) WSAPoll(fds, nfds, timeout)
#define close(fd) closesocket(fd)
#define MSG_DONTWAIT (0)
typedef int socklen_t;
#endif /* _WIN32 */

const uint16_t ptp_udp_port = 320;

static inline bool enableTimestamp(int fd, int vclock)
{
    #ifdef __linux__
    struct so_timestamping timestamping;
    int flags = SOF_TIMESTAMPING_TX_HARDWARE |
        SOF_TIMESTAMPING_RX_HARDWARE |
        SOF_TIMESTAMPING_RAW_HARDWARE;
    if(vclock > 0)
        flags |= SOF_TIMESTAMPING_BIND_PHC;
    memset(&timestamping, 0, sizeof(timestamping));
    timestamping.flags = flags;
    timestamping.bind_phc = vclock;
    if(setsockopt(fd, SOL_SOCKET, SO_TIMESTAMPING, &timestamping,
            sizeof(timestamping)) < 0) {
        logp_err("SO_TIMESTAMPING");
        return false;
    }
    #endif /* __linux__ */
    return true;
}

static void a_free(pipaddr self)
{
    if(LIKELY_COND(self != NULL)) {
        free(self->_iPstr);
        free(self);
    }
}
struct sockaddr *a_getAddr(pcipaddr self)
{
    return UNLIKELY_COND(self == NULL) ? NULL : (struct sockaddr *)self->_addr;
}
bool a4_eq(pcipaddr self, pcipaddr other)
{
    struct sockaddr_in *d1, *d2;
    if(UNLIKELY_COND(self == NULL) || other == NULL)
        return false;
    if(self->_domain != other->_domain)
        return false;
    d1 = (struct sockaddr_in *)self->_addr;
    d2 = (struct sockaddr_in *)other->_addr;
    return d1->sin_port == d2->sin_port &&
        d1->sin_addr.s_addr == d2->sin_addr.s_addr;
}
bool a6_eq(pcipaddr self, pcipaddr other)
{
    struct sockaddr_in6 *d1, *d2;
    if(UNLIKELY_COND(self == NULL) || other == NULL)
        return false;
    if(self->_domain != other->_domain)
        return false;
    d1 = (struct sockaddr_in6 *)self->_addr;
    d2 = (struct sockaddr_in6 *)other->_addr;
    return d1->sin6_port == d2->sin6_port &&
        memcmp(d1->sin6_addr.s6_addr, d2->sin6_addr.s6_addr,
            sizeof(struct in6_addr)) == 0;
}
size_t a4_getSize(pcipaddr self)
{
    return sizeof(struct sockaddr_in);
}
size_t a6_getSize(pcipaddr self)
{
    return sizeof(struct sockaddr_in6);
}
size_t a4_getIPSize(pcipaddr self)
{
    return IPV4_ADDR_LEN;
}
size_t a6_getIPSize(pcipaddr self)
{
    return IPV6_ADDR_LEN;
}
static uint16_t a4_getPort(pcipaddr self)
{
    if(LIKELY_COND(self != NULL)) {
        struct sockaddr_in *d = (struct sockaddr_in *)self->_addr;
        return net_to_cpu16(d->sin_port);
    }
    return 0;
}
static uint16_t a6_getPort(pcipaddr self)
{
    if(LIKELY_COND(self != NULL)) {
        struct sockaddr_in6 *d = (struct sockaddr_in6 *)self->_addr;
        return net_to_cpu16(d->sin6_port);
    }
    return 0;
}
static void a4_setPort(pipaddr self, uint16_t port)
{
    if(LIKELY_COND(self != NULL)) {
        struct sockaddr_in *d = (struct sockaddr_in *)self->_addr;
        d->sin_port = cpu_to_net16(port);
    }
}
static void a6_setPort(pipaddr self, uint16_t port)
{
    if(LIKELY_COND(self != NULL)) {
        struct sockaddr_in6 *d = (struct sockaddr_in6 *)self->_addr;
        d->sin6_port = cpu_to_net16(port);
    }
}
static void a4_setAnyIP(pipaddr self)
{
    if(LIKELY_COND(self != NULL)) {
        struct sockaddr_in *d = (struct sockaddr_in *)self->_addr;
        d->sin_addr.s_addr = cpu_to_net32(INADDR_ANY);
    }
}
static void a6_setAnyIP(pipaddr self)
{
    if(LIKELY_COND(self != NULL)) {
        struct sockaddr_in6 *d = (struct sockaddr_in6 *)self->_addr;
        d->sin6_addr = in6addr_any;
    }
}
static bool a4_isAnyIP(pcipaddr self)
{
    if(LIKELY_COND(self != NULL)) {
        struct sockaddr_in *d = (struct sockaddr_in *)self->_addr;
        return d->sin_addr.s_addr == cpu_to_net32(INADDR_ANY);
    }
    return false;
}
static bool a6_isAnyIP(pcipaddr self)
{
    if(LIKELY_COND(self != NULL)) {
        struct sockaddr_in6 *d = (struct sockaddr_in6 *)self->_addr;
        return memcmp(d->sin6_addr.s6_addr, in6addr_any.s6_addr,
                sizeof(struct in6_addr)) == 0;
    }
    return false;
}
static prot a4_getType(pcipaddr self)
{
    return UDP_IPv4;
}
static prot a6_getType(pcipaddr self)
{
    return UDP_IPv6;
}

static bool a4_setIP4(pipaddr self, const uint8_t *ip)
{
    if(LIKELY_COND(self != NULL) && ip != NULL) {
        struct sockaddr_in *d = (struct sockaddr_in *)self->_addr;
        d->sin_addr.s_addr = *(uint32_t *)ip;
        return true;
    }
    return false;
}
static bool a6_setIP4(pipaddr self, const uint8_t *ip)
{
    return false;
}
static bool a4_setIP4Val(pipaddr self, uint32_t ip)
{
    if(LIKELY_COND(self != NULL)) {
        struct sockaddr_in *d = (struct sockaddr_in *)self->_addr;
        d->sin_addr.s_addr = cpu_to_net32(ip);
        return true;
    }
    return false;
}
static bool a6_setIP4Val(pipaddr self, uint32_t ip)
{
    return false;
}
static bool a4_setIP4Str(pipaddr self, const char *ip)
{
    if(LIKELY_COND(self != NULL) && ip != NULL) {
        struct sockaddr_in *d = (struct sockaddr_in *)self->_addr;
        if(inet_pton(AF_INET, ip, &d->sin_addr.s_addr) == 1)
            return true;
        logp_warning("inet_pton");
    }
    return false;
}
static bool a6_setIP4Str(pipaddr self, const char *ip)
{
    return false;
}
static const uint8_t *a4_getIP4(pcipaddr self)
{
    if(LIKELY_COND(self != NULL)) {
        struct sockaddr_in *d = (struct sockaddr_in *)self->_addr;
        return (uint8_t *)&d->sin_addr.s_addr;
    }
    return NULL;
}
static const uint8_t *a6_getIP4(pcipaddr self)
{
    return NULL;
}
static uint32_t a4_getIP4Val(pcipaddr self)
{
    if(LIKELY_COND(self != NULL)) {
        struct sockaddr_in *d = (struct sockaddr_in *)self->_addr;
        return net_to_cpu32(d->sin_addr.s_addr);
    }
    return 0;
}
static uint32_t a6_getIP4Val(pcipaddr self)
{
    return 0;
}
static const char *a4_getIP4Str(pipaddr self)
{
    if(LIKELY_COND(self != NULL)) {
        const char *ret;
        struct sockaddr_in *d;
        if(self->_iPstr == NULL) {
            void *m = malloc(INET_ADDRSTRLEN);
            if(m == NULL) {
                log_err("memory allocation failed");
                return NULL;
            }
            self->_iPstr = m;
        }
        d = (struct sockaddr_in *)self->_addr;
        ret = inet_ntop(AF_INET, &d->sin_addr.s_addr, self->_iPstr, INET_ADDRSTRLEN);
        if(ret == NULL)
            logp_warning("inet_ntop");
        return ret;
    }
    return NULL;
}
static const char *a6_getIP4Str(pipaddr self)
{
    return NULL;
}
static bool a4_setIP6(pipaddr self, const uint8_t *ip)
{
    return false;
}
static bool a6_setIP6(pipaddr self, const uint8_t *ip)
{
    if(LIKELY_COND(self != NULL) && ip != NULL) {
        struct sockaddr_in6 *d = (struct sockaddr_in6 *)self->_addr;
        memcpy(d->sin6_addr.s6_addr, ip, sizeof(struct in6_addr));
        return true;
    }
    return false;
}
static bool a4_setIP6Str(pipaddr self, const char *ip)
{
    return false;
}
static bool a6_setIP6Str(pipaddr self, const char *ip)
{
    if(LIKELY_COND(self != NULL) && ip != NULL) {
        struct sockaddr_in6 *d = (struct sockaddr_in6 *)self->_addr;
        if(inet_pton(AF_INET6, ip, d->sin6_addr.s6_addr) == 1)
            return true;
        logp_warning("inet_pton");
    }
    return false;
}
static const uint8_t *a4_getIP6(pcipaddr self)
{
    return NULL;
}
static const uint8_t *a6_getIP6(pcipaddr self)
{
    if(LIKELY_COND(self != NULL)) {
        struct sockaddr_in6 *d = (struct sockaddr_in6 *)self->_addr;
        return d->sin6_addr.s6_addr;
    }
    return NULL;
}
static const char *a4_getIP6Str(pipaddr self)
{
    return NULL;
}
static const char *a6_getIP6Str(pipaddr self)
{
    if(LIKELY_COND(self != NULL)) {
        const char *ret;
        struct sockaddr_in6 *d;
        if(self->_iPstr == NULL) {
            void *m = malloc(INET6_ADDRSTRLEN);
            if(m == NULL) {
                log_err("memory allocation failed");
                return NULL;
            }
            self->_iPstr = m;
        }
        d = (struct sockaddr_in6 *)self->_addr;
        ret = inet_ntop(AF_INET6, d->sin6_addr.s6_addr, self->_iPstr, INET6_ADDRSTRLEN);
        if(ret == NULL)
            logp_warning("inet_ntop");
        return ret;
    }
    return NULL;
}
pipaddr addr_alloc(prot type)
{
    size_t len;
    switch(type) {
        case UDP_IPv4:
            len = sizeof(struct sockaddr_in);
            break;
        case UDP_IPv6:
            len = sizeof(struct sockaddr_in6);
            break;
        default:
            log_err("unkown protocol %d", type);
            return NULL;
    }
    pipaddr ret = malloc(sizeof(struct ipaddr_t) + len);
    if(ret != NULL) {
        void *m = (void *)(ret + 1);
        memset(m, 0, len);
        ret->_addr = m;
        ret->_iPstr = NULL;
        ret->_type = type;
#define asg(a) ret->a = a_##a
        asg(free);
        asg(getAddr);
#undef asg
#define asg4(a) ret->a = a4_##a
#define asg6(a) ret->a = a6_##a
        switch(type) {
            case UDP_IPv4: {
                struct sockaddr_in *d = (struct sockaddr_in *)m;
                ret->_domain = AF_INET;
                d->sin_family = AF_INET;
                d->sin_addr.s_addr = cpu_to_net32(INADDR_ANY);
                d->sin_port = cpu_to_net16(ptp_udp_port);
                asg4(getSize);
                asg4(getIPSize);
                asg4(getPort);
                asg4(setPort);
                asg4(setAnyIP);
                asg4(isAnyIP);
                asg4(getType);
                asg4(setIP4);
                asg4(setIP4Val);
                asg4(setIP4Str);
                asg4(getIP4);
                asg4(getIP4Val);
                asg4(getIP4Str);
                asg4(setIP6);
                asg4(setIP6Str);
                asg4(getIP6);
                asg4(getIP6Str);
                asg4(eq);
                ret->getIP = a4_getIP4;
                ret->setIP = a4_setIP4;
                ret->getIPStr = a4_getIP4Str;
                ret->setIPStr = a4_setIP4Str;
                break;
            }
            case UDP_IPv6: {
                struct sockaddr_in6 *d = (struct sockaddr_in6 *)m;
                ret->_domain = AF_INET6;
                d->sin6_family = AF_INET6;
                d->sin6_addr = in6addr_any;
                d->sin6_port = cpu_to_net16(ptp_udp_port);
                asg6(getSize);
                asg6(getIPSize);
                asg6(getPort);
                asg6(setPort);
                asg6(setAnyIP);
                asg6(isAnyIP);
                asg6(getType);
                asg6(setIP4);
                asg6(setIP4Val);
                asg6(setIP4Str);
                asg6(getIP4);
                asg6(getIP4Val);
                asg6(getIP4Str);
                asg6(setIP6);
                asg6(setIP6Str);
                asg6(getIP6);
                asg6(getIP6Str);
                asg6(eq);
                ret->getIP = a6_getIP6;
                ret->setIP = a6_setIP6;
                ret->getIPStr = a6_getIP6Str;
                ret->setIPStr = a6_setIP6Str;
                break;
            }
            default: /* Should not happen */
                break;
        }
    } else
        log_err("memory allocation failed");
    return ret;
}
static bool s_close(psock self)
{
    if(LIKELY_COND(self != NULL) && self->_fd >= 0) {
        int ret = close(self->_fd);
        self->_fd = -1;
        return (ret == 0);
    }
    return false;
}
static void s_free(psock self)
{
    s_close(self);
    free(self);
}
static int s_fileno(pcsock self)
{
    return UNLIKELY_COND(self == NULL) ? -1 : self->_fd;
}
static bool s_init(psock self, prot type)
{
    int domain;
    if(UNLIKELY_COND(self == NULL))
        return false;
    if(self->_fd >= 0) {
        log_warning("socket already initialized");
        return false;
    }
    switch(type) {
        case UDP_IPv4:
            domain = AF_INET;
            break;
        case UDP_IPv6:
            domain = AF_INET6;
            break;
        default:
            log_err("unkown protocol %d", type);
            return false;
    }
    int fd = socket(domain, SOCK_DGRAM, 0);
    if(fd < 0) {
        logp_err("socket");
        return false;
    }
    if(!enableTimestamp(fd, 0)) {
        close(fd);
        return false;
    }
    self->_fd = fd;
    self->_type = type;
    return true;
}
static bool s_initSrv(psock self, pcipaddr address)
{
    int fd;
    if(UNLIKELY_COND(self == NULL))
        return false;
    if(self->_fd >= 0) {
        log_warning("socket already initialized");
        return false;
    }
    if(address == NULL) {
        log_err("Address missing");
        return false;
    }
    fd = socket(address->_domain, SOCK_DGRAM, 0);
    if(fd < 0) {
        logp_err("socket");
        return false;
    }
    if(!enableTimestamp(fd, 0)) {
        close(fd);
        return false;
    }
    if(bind(fd, address->getAddr(address), address->getSize(address)) < 0) {
        logp_err("bind");
        close(fd);
        return false;
    }
    self->_fd = fd;
    self->_type = address->_type;
    return true;
}
static bool s_send(pcsock self, pcbuffer buffer, pcipaddr address)
{
    size_t len;
    ssize_t ret;
    if(UNLIKELY_COND(self == NULL))
        return false;
    if(self->_fd < 0) {
        log_warning("socket is NOT initialized");
        return false;
    }
    if(address == NULL) {
        log_err("address is missing");
        return false;
    }
    if(buffer == NULL) {
        log_err("buffer is missing");
        return false;
    }
    len = buffer->getLen(buffer);
    ret = sendto(self->_fd, (void *)buffer->getBuf(buffer), len, 0,
            address->getAddr(address), address->getSize(address));
    if(len == ret)
        return true;
    else if(ret < 0)
        logp_err("sendto");
    else
        log_warning("send partial data %zu from %zu", ret, len);
    return false;
}
static bool s_poll(pcsock self, int timeout)
{
    int ret;
    struct pollfd fds;
    if(UNLIKELY_COND(self == NULL) || timeout == 0)
        return false;
    if(timeout < 0) {
        timeout = -1;
        log_debug("call infinite poll");
    }
    fds.fd = self->_fd;
    fds.events = POLLIN; /* TODO POLLERR for HW TX timestamp */
    fds.revents = 0;
    ret = poll(&fds, 1, timeout);
    if(ret < 0) {
        logp_err("poll");
        return false;
    }
    return ret > 0 && (fds.revents & POLLIN) > 0;
}
static bool s_recv(pcsock self, pbuffer buffer, pipaddr address, pts ts)
{
    ssize_t ret;
    size_t osize;
    socklen_t size;
    if(UNLIKELY_COND(self == NULL))
        return false;
    if(self->_fd < 0) {
        log_warning("socket is NOT initialized");
        return false;
    }
    if(address == NULL) {
        log_err("address is missing");
        return false;
    }
    if(buffer == NULL) {
        log_err("buffer is missing");
        return false;
    }
    osize = address->getSize(address);
    size = osize;
    getUtcClock(ts); // TODO get RX ts from HW
    ret = recvfrom(self->_fd, (void *)buffer->getBuf(buffer),
            buffer->getSize(buffer), MSG_DONTWAIT, address->getAddr(address), &size);
    if(ret > 0 && osize == size) {
        buffer->setLen(buffer, ret);
        return true;
    }
    if(ret < 0)
        logp_err("recvfrom");
    else if(osize != size)
        log_err("wrong address size %zu != %zu", osize, size);
    else
        log_warning("recvfrom partial %d", ret);
    return false;
}
static prot s_getType(pcsock self)
{
    return UNLIKELY_COND(self == NULL) ? 0 : self->_type;
}

psock sock_alloc()
{
    psock ret = malloc(sizeof(struct sock_t));
    if(ret != NULL) {
        ret->_fd = -1;
        ret->_type = Invalid_PROTO;
#define asg(a) ret->a = s_##a
        asg(free);
        asg(close);
        asg(fileno);
        asg(init);
        asg(initSrv);
        asg(send);
        asg(recv);
        asg(poll);
        asg(getType);
    } else
        log_err("memory allocation failed");
    return ret;
}
static inline bool try_inet_pton(const char *ip, prot *_type, uint8_t *addr)
{
    int domain;
    prot type = *_type;
    switch(*_type) {
        case UDP_IPv4:
            domain = AF_INET;
            break;
        case UDP_IPv6:
            domain = AF_INET6;
            break;
        default:
            /* IPv6 always have double dots, while IPv4 will never have it! */
            if(strchr(ip, ':') == NULL) {
                domain = AF_INET;
                type = UDP_IPv4;
            } else {
                domain = AF_INET6;
                type = UDP_IPv6;
            }
            break;
    }
    if(inet_pton(domain, ip, addr) == 1) {
        *_type = type;
        return true;
    }
    return false;
}
bool addressStringToBinary(const char *str, prot *_type, uint8_t *addr)
{
    prot type = Invalid_PROTO;
    int e, domain;
    struct addrinfo i, *res, *r;
    if(str == NULL) {
        log_err("address string is missing");
        return false;
    }
    if(UNLIKELY_COND(_type == NULL || addr == NULL))
        return false;
    /* We translate address before we try host name translation */
    if(try_inet_pton(str, _type, addr))
        return true;
    switch(*_type) {
        case UDP_IPv4:
            domain = AF_INET;
            break;
        case UDP_IPv6:
            domain = AF_INET6;
            break;
        default:
            domain = AF_UNSPEC;
            break;
    }
    /* In case we use a simple address */
    memset(&i, 0, sizeof(i));
    i.ai_family = domain;
    i.ai_flags = (AI_V4MAPPED | AI_ADDRCONFIG | AI_PASSIVE);
    /*
     * If host or IP does not exist we get any address
     * In this case we return false!
     */
    e = getaddrinfo(str, NULL, &i, &res);
    if(e == 0) {
        bool run = true;
        struct sockaddr_in *d4;
        struct sockaddr_in6 *d6;
        for(r = res; run && r != NULL; r = r->ai_next) {
            switch(r->ai_family) {
                case AF_INET:
                    d4 = (struct sockaddr_in *)r->ai_addr;
                    if(d4->sin_addr.s_addr != cpu_to_net32(INADDR_ANY)) {
                        *(uint32_t *)addr = d4->sin_addr.s_addr;
                        type = UDP_IPv4;
                    }
                    run = false;
                    break;
                case AF_INET6:
                    d6 = (struct sockaddr_in6 *)r->ai_addr;
                    if(memcmp(d6->sin6_addr.s6_addr, in6addr_any.s6_addr,
                            sizeof(struct in6_addr)) != 0) {
                        memcpy(addr, d6->sin6_addr.s6_addr, sizeof(struct in6_addr));
                        type = UDP_IPv6;
                    }
                    run = false;
                    break;
                default: /* We support IP only! */
                    break;
            }
        }
        freeaddrinfo(res);
        if(type != Invalid_PROTO) {
            *_type = type;
            return true;
        } else
            log_warning("Can not find IP type %d address for) '%s'", *_type, str);
    } else
        log_err("getaddrinfo: %s", gai_strerror(e));
    return false;
}
