/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief Inplement library calls for system depends unit tests
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

/* C++ standard header */
#include <string>
/* C standard headers */
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#include <threads.h>
/* POSIX headers */
#include <unistd.h>
#include <poll.h>
#include <pwd.h>
#include <dlfcn.h>
#include <dirent.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>
/* GNU/Linux headers */
#include <ifaddrs.h>
#include <sys/timex.h>
#include <sys/ioctl.h>
#include <linux/filter.h>
#include <linux/if_packet.h>
#include <linux/sockios.h>
#include <linux/ptp_clock.h>
#include <linux/ethtool.h>
#include <linux/net_tstamp.h>
/*****************************************************************************/
static const size_t single_print = 1024;
static std::string out_buf;
static std::string err_buf;
static std::string log_buf;
static bool testMode = false;
static time_t mono_sec = 0;
static time_t real_sec = 0;
static bool didInit = false;
static bool tzDl = false;
void useTestMode(bool n)
{
    testMode = n;
    if(n) {
        out_buf.clear();
        err_buf.clear();
        log_buf.clear();
        mono_sec = 0;
        real_sec = 0;
        tzDl = false;
    }
}
const char *getOut()
{
    testMode = false;
    return out_buf.c_str();
}
const char *getErr()
{
    testMode = false;
    return err_buf.c_str();
}
const char *getLog()
{
    testMode = false;
    return log_buf.c_str();
}
void setMono(long t) {mono_sec = t;}
void setReal(long t) {real_sec = t;}

/*****************************************************************************/
#define sysFuncDec(ret, name, ...)\
    ret (*_##name)(__VA_ARGS__);\
    extern "C" ret name(__VA_ARGS__)
#define sysFuncAgn(ret, name, ...)\
    _##name = (ret(*)(__VA_ARGS__))dlsym(RTLD_NEXT, #name);\
    if(_##name == nullptr) {\
        fputs("Fail allocating " #name "\n", stderr);\
        fail = true;}
/* Allocation can fail, calling need verify! */
#define sysFuncAgZ(ret, name, ...)\
    _##name = (ret(*)(__VA_ARGS__))dlsym(RTLD_NEXT, #name)
sysFuncDec(int, socket, int, int, int) throw();
sysFuncDec(int, close, int);
sysFuncDec(int, select, int, fd_set *, fd_set *, fd_set *, timeval *);
sysFuncDec(int, bind, int, const sockaddr *, socklen_t) throw();
sysFuncDec(int, setsockopt, int, int, int, const void *, socklen_t) throw();
sysFuncDec(ssize_t, recv, int, void *, size_t, int);
sysFuncDec(ssize_t, recvfrom, int, void *, size_t, int, sockaddr *, socklen_t *);
sysFuncDec(ssize_t, sendto, int, const void *buf, size_t len, int flags, const sockaddr *, socklen_t);
sysFuncDec(ssize_t, recvmsg, int, msghdr *, int);
sysFuncDec(ssize_t, sendmsg, int, const msghdr *, int);
sysFuncDec(int, poll, pollfd *fds, nfds_t nfds, int timeout);
sysFuncDec(int, printf, const char *, ...);
sysFuncDec(int, __printf_chk, int, const char *, ...);
sysFuncDec(int, fprintf, FILE *, const char *, ...);
sysFuncDec(int, __fprintf_chk, FILE *, int, const char *, ...);
sysFuncDec(int, puts, const char *);
sysFuncDec(int, vprintf, const char *, va_list);
sysFuncDec(int, __vprintf_chk, int, const char *, va_list);
sysFuncDec(int, vfprintf, FILE *, const char *, va_list);
sysFuncDec(int, __vfprintf_chk, FILE *, int, const char *, va_list);
sysFuncDec(size_t, fwrite, const void *, size_t, size_t, FILE *);
sysFuncDec(void, openlog, const char *, int, int);
sysFuncDec(void, closelog, void);
sysFuncDec(void, syslog, int, const char *, ...);
sysFuncDec(int, clock_gettime, clockid_t , timespec *) throw();
sysFuncDec(int, clock_settime, clockid_t , const timespec *) throw();
sysFuncDec(int, clock_adjtime, clockid_t, timex *) throw();
sysFuncDec(int, thrd_sleep, const timespec*, timespec*);
sysFuncDec(int, ioctl, int, unsigned long, ...) throw();
sysFuncDec(tm *, localtime, const time_t *) throw();
sysFuncDec(int, getifaddrs, ifaddrs **) throw();
sysFuncDec(void, freeifaddrs, ifaddrs *) throw();
sysFuncDec(time_t, time, time_t*) throw();
sysFuncDec(int, open, const char *, int, ...);
sysFuncDec(int, __open_2, const char *, int);
// glibc stat fucntions
sysFuncDec(int, stat, const char *, struct stat *) throw();
sysFuncDec(int, stat64, const char *, struct stat64 *) throw();
sysFuncDec(int, __xstat, int, const char *, struct stat *);
sysFuncDec(int, __xstat64, int, const char *, struct stat64 *);
sysFuncDec(char *, realpath, const char *, char *) throw();
sysFuncDec(char *, __realpath_chk, const char *, char *, size_t) throw();
void initLibSys()
{
    if(didInit) {
        useTestMode(false);
        return;
    }
    bool fail = false;
    sysFuncAgn(int, socket, int, int, int);
    sysFuncAgn(int, close, int);
    sysFuncAgn(int, select, int, fd_set *, fd_set *, fd_set *, timeval *);
    sysFuncAgn(int, bind, int, const sockaddr *, socklen_t);
    sysFuncAgn(int, setsockopt, int, int, int, const void *, socklen_t);
    sysFuncAgn(ssize_t, recv, int, void *, size_t, int);
    sysFuncAgn(ssize_t, recvfrom, int, void *, size_t, int, sockaddr *, socklen_t *);
    sysFuncAgn(ssize_t, sendto, int, const void *buf, size_t len, int flags, const sockaddr *, socklen_t);
    sysFuncAgn(ssize_t, recvmsg, int, msghdr *, int);
    sysFuncAgn(ssize_t, sendmsg, int, const msghdr *, int);
    sysFuncAgn(int, poll, pollfd *fds, nfds_t nfds, int timeout);
    sysFuncAgn(int, printf, const char *, ...);
    sysFuncAgn(int, __printf_chk, int, const char *, ...);
    sysFuncAgn(int, fprintf, FILE *, const char *, ...);
    sysFuncAgn(int, __fprintf_chk, FILE *, int, const char *, ...);
    sysFuncAgn(int, puts, const char *);
    sysFuncAgn(int, vprintf, const char *, va_list);
    sysFuncAgn(int, __vprintf_chk, int, const char *, va_list);
    sysFuncAgn(int, vfprintf, FILE *, const char *, va_list);
    sysFuncAgn(int, __vfprintf_chk, FILE *, int, const char *, va_list);
    sysFuncAgn(size_t, fwrite, const void *, size_t, size_t, FILE *);
    sysFuncAgn(void, openlog, const char *, int, int);
    sysFuncAgn(void, closelog, void);
    sysFuncAgn(void, syslog, int, const char *, ...);
    sysFuncAgn(int, clock_gettime, clockid_t , timespec *);
    sysFuncAgn(int, clock_settime, clockid_t , const timespec *);
    sysFuncAgn(int, clock_adjtime, clockid_t, timex *);
    sysFuncAgn(int, thrd_sleep, const timespec*, timespec*);
    sysFuncAgn(int, ioctl, int, unsigned long, ...);
    sysFuncAgn(tm *, localtime, const time_t *);
    sysFuncAgn(int, getifaddrs, ifaddrs **);
    sysFuncAgn(void, freeifaddrs, ifaddrs *);
    sysFuncAgn(time_t, time, time_t*);
    sysFuncAgn(int, open, const char *, int, ...);
    sysFuncAgn(int, __open_2, const char *, int);
    sysFuncAgZ(int, stat, const char *, struct stat *);
    sysFuncAgZ(int, stat64, const char *, struct stat64 *);
    sysFuncAgn(int, __xstat, int, const char *, struct stat *);
    sysFuncAgn(int, __xstat64, int, const char *, struct stat64 *);
    sysFuncAgn(char *, realpath, const char *, char *);
    sysFuncAgn(char *, __realpath_chk, const char *, char *, size_t);
    if(fail)
       fputs("Fail obtain address of functions\n", stderr);
    didInit = true;
    useTestMode(false);
}
/* Make sure we initialize the library functions call back */
__attribute__((constructor)) static void staticInit(void) { initLibSys(); }
/*****************************************************************************/
#define retTestCond(cond, name, ...)\
    if(!testMode || cond)\
        return _##name(__VA_ARGS__)
#define retTest(name, ...)\
    if(!testMode)\
        return _##name(__VA_ARGS__)
#define retTestStat(nm)\
    if(!testMode) {\
        if(_##nm != nullptr)\
            _##nm(name, sp);\
        else\
            ___x##nm(3, name, sp);\
    }
#define retTestV(name, ...)\
    if(!testMode) {\
        _##name(__VA_ARGS__); return; }
#define retTest0(name)\
    if(!testMode)\
        return _##name()
static inline int retErr(int err)
{
    errno = err;
    return -1;
}
/*****************************************************************************/
#define STAT_BODY\
    if(sp == nullptr)\
        return retErr(EINVAL);\
    if(strcmp("/dev/ptp0", name) != 0)\
        return retErr(EINVAL);\
    sp->st_mode = S_IFCHR;\
    return 0
static inline int l_stat(const char *name, struct stat *sp)
{
    STAT_BODY;
}
static inline int l_stat64(const char *name, struct stat64 *sp)
{
    STAT_BODY;
}
/*****************************************************************************/
#define VA_FPRINT(fd, format, ap) _vfprintf(fd, format, ap)
#define ALL_FPRINTF(fd, getVL)\
    int ret = 0;\
    va_list ap;\
    getVL;\
    if(!testMode || (fd != stdout && fd != stderr))\
        ret = VA_FPRINT(fd, format, ap);\
    else {\
        char str[single_print];\
        ret = vsnprintf(str, sizeof(str), format, ap);\
        if(fd == stdout)\
            out_buf += str;\
        else if(fd == stderr)\
            err_buf += str;\
    }\
    va_end(ap);\
    return ret
#define L_FPRINTF(fd) ALL_FPRINTF(fd, va_start(ap, format))
/*****************************************************************************/
static inline int l_open(const char *name, int flags)
{
    if(strcmp("/dev/ptp0", name) == 0)
        return 7;
    return retErr(EINVAL);
}
static inline char *l_realpath(const char *path, char *resolved)
{
    if(path == nullptr || resolved == nullptr || *path == 0)
        return nullptr;
    if(strchr(path, '/') == nullptr) {
        std::string ret = "/dev/";
        ret += path;
        strcpy(resolved, ret.c_str());
    } else
        strcpy(resolved, path);
    return resolved;
}
/*****************************************************************************/
int socket(int domain, int type, int protocol) throw()
{
    retTest(socket, domain, type, protocol);
    if(domain == AF_INET && type == SOCK_DGRAM && protocol == 0)
        return 7;
    return retErr(EINVAL);
}
int close(int fd)
{
    retTest(close, fd);
    return fd == 7 ? 0 : retErr(EINVAL);
}
int select(int nfds, fd_set *rfds, fd_set *wfds, fd_set *efds, timeval *to)
{
    retTest(select, nfds, rfds, wfds, efds, to);
    return retErr(EINVAL);
}
int bind(int fd, const sockaddr *addr, socklen_t addrlen) throw()
{
    retTest(bind, fd, addr, addrlen);
    if(fd == 7 && addr != nullptr && addrlen == 16 && addr->sa_family == AF_INET) {
      static const uint8_t d[6] = { 1, 64, 1, 2, 5, 1 };
      static const uint8_t d0[6] = { 1, 64 };
      if(memcmp(d, addr->sa_data, 6) == 0 || memcmp(d0, addr->sa_data, 6) == 0)
            return 0;
    }
    return retErr(EINVAL);
}
int setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen) throw()
{
    so_timestamping *ts;
    retTest(setsockopt, fd, level, optname, optval, optlen);
    switch(level) {
        case SOL_SOCKET:
            switch(optname) {
                case SO_BINDTODEVICE:
                    if(fd == 7 && optlen == 7 &&
                       strcmp("enp0s25", (const char*)optval) == 0)
                        return 0;
                case SO_TIMESTAMPING:
                    ts = (so_timestamping *)optval;
                    if(fd == 7 && optlen == sizeof(so_timestamping) &&
                       ts->flags == (SOF_TIMESTAMPING_TX_HARDWARE |
                           SOF_TIMESTAMPING_RX_HARDWARE | SOF_TIMESTAMPING_RAW_HARDWARE) &&
                       ts->bind_phc == 0)
                        return 0;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    return retErr(EINVAL);
}
ssize_t recv(int fd, void *buf, size_t len, int flags)
{
    retTest(recv, fd, buf, len, flags);
    return retErr(EINVAL);
}
ssize_t recvfrom(int fd, void *buf, size_t len, int flags, sockaddr *addr, socklen_t *addrlen)
{
    retTest(recvfrom, fd, buf, len, flags, addr, addrlen);
    if(fd == 7 && len == 10 && (flags == 0 || flags == MSG_DONTWAIT ) && buf != nullptr && addrlen != nullptr && *addrlen == 16 &&
      addr != nullptr && addr->sa_family == AF_INET) {
        static const uint8_t d[6] = { 10, 7, 1, 10, 5, 10 };
        memcpy(addr->sa_data, d, 6);
        memcpy(buf, "test", 4);
        return 4;
    }
    return retErr(EINVAL);
}
ssize_t sendto(int fd, const void *buf, size_t len, int flags, const sockaddr *addr, socklen_t addrlen)
{
    retTest(sendto, fd, buf, len, flags, addr, addrlen);
    if(fd == 7 && len == 10 && flags == 0 && buf != nullptr && addrlen == 16 &&
       memcmp(buf, "test1246xx", 10) == 0 && addr != nullptr && addr->sa_family == AF_INET) {
      static const uint8_t d[6] = { 1, 64, 1, 2, 5, 1 };
      if(memcmp(d, addr->sa_data, 6) == 0)
            return 10;
    }
    return retErr(EINVAL);
}
ssize_t recvmsg(int fd, msghdr *msg, int flags)
{
    retTest(recvmsg, fd, msg, flags);
    return retErr(EINVAL);
}
ssize_t sendmsg(int fd, const msghdr *msg, int flags)
{
    retTest(sendmsg, fd, msg, flags);
    return retErr(EINVAL);
}
int poll(pollfd *fds, nfds_t nfds, int timeout)
{
    retTest(poll, fds, nfds, timeout);
    if(nfds == 1 && timeout == 17 && fds != nullptr && fds[0].fd == 7 && fds[0].events == POLLIN && fds[0].revents == 0) {
        fds[0].revents = POLLIN;
        return 1;
    }
    return retErr(EINVAL);
}
int printf(const char *format, ...)
{
    L_FPRINTF(stdout);
}
int __printf_chk(int, const char *format, ...)
{
    L_FPRINTF(stdout);
}
int fprintf(FILE *fd, const char *format, ...)
{
    L_FPRINTF(fd);
}
int __fprintf_chk(FILE *fd, int, const char *format, ...)
{
    L_FPRINTF(fd);
}
int puts(const char *s)
{
    retTest(puts, s);
    out_buf += s;
    return 0;
}
static inline int in_vfprintf(FILE *fd, const char *format, va_list _ap)
{
    ALL_FPRINTF(fd, va_copy(ap, _ap));
}
int vprintf(const char *format, va_list ap)
{
    return in_vfprintf(stdout, format, ap);
}
int __vprintf_chk(int, const char *format, va_list ap)
{
    return in_vfprintf(stdout, format, ap);
}
int vfprintf(FILE *fd, const char *format, va_list ap)
{
    return in_vfprintf(fd, format, ap);
}
int __vfprintf_chk(FILE *fd, int, const char *format, va_list ap)
{
    return in_vfprintf(fd, format, ap);
}
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *fd)
{
    retTestCond(fd != stderr, fwrite, ptr, size, nmemb, fd);
    out_buf += std::string((const char *)ptr, size * nmemb);
    return 0;
}
void openlog(const char *ident, int option, int facility)
{
    retTest(openlog,ident, option, facility);
}
void closelog(void)
{
    retTest0(closelog);
}
void syslog(int priority, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    if(!testMode)
        vsyslog(priority, format, ap);
    else {
        char str[single_print];
        vsnprintf(str, sizeof(str), format, ap);
        log_buf += str;
    }
    va_end(ap);
}
int clock_gettime(clockid_t clk_id, timespec *tp) throw()
{
    retTest(clock_gettime, clk_id, tp);
    switch(clk_id ) {
        case -61: // fd = 7
        case CLOCK_REALTIME:
            tp->tv_sec = real_sec;
            tp->tv_nsec = 0;
            return 0;
        default:
            break;
    }
    return retErr(EINVAL);
}
int clock_settime(clockid_t clk_id, const timespec *tp) throw()
{
    retTest(clock_settime, clk_id, tp);
    return clk_id == -61 && tp->tv_sec == 4 && tp->tv_nsec == 4 ? 0 : retErr(EINVAL);
}
int clock_adjtime(clockid_t clk_id, timex *tmx) throw()
{
    retTest(clock_adjtime, clk_id, tmx);
    if(tmx->maxerror != 0 || tmx->esterror != 0 ||
        tmx->status != 0 || tmx->constant != 0 || tmx->precision != 0 ||
        tmx->tolerance != 0 || tmx->ppsfreq != 0 || tmx->jitter != 0 ||
        tmx->shift != 0 || tmx->stabil != 0 || tmx->jitcnt != 0 ||
        tmx->calcnt != 0 || tmx->errcnt != 0 || tmx->stbcnt != 0 || tmx->tai != 0)
        return retErr(EINVAL);
    if(clk_id == -61) {
        switch(tmx->modes) {
            case ADJ_SETOFFSET | ADJ_NANO:
                if(tmx->tick != 0 || tmx->freq != 0 || tmx->offset != 0)
                    return retErr(EINVAL);
                if(tmx->time.tv_sec == 93 && tmx->time.tv_usec == 571)
                    break;
                return retErr(EINVAL);
            case ADJ_FREQUENCY:
                if(tmx->tick != 0 || tmx->freq != 654 ||
                    tmx->time.tv_sec != 0 || tmx->time.tv_usec != 0 ||
                    tmx->offset != 0)
                    return retErr(EINVAL);
                break;
            case ADJ_OFFSET | ADJ_NANO:
                if(tmx->tick != 0 || tmx->freq != 0 || tmx->time.tv_sec != 0 ||
                    tmx->time.tv_usec != 0 || tmx->offset != 265963)
                    return retErr(EINVAL);
                break;
            case 0:
                if(tmx->tick != 0 || tmx->freq != 0 || tmx->time.tv_sec != 0 ||
                    tmx->time.tv_usec != 0 || tmx->offset != 0)
                    return retErr(EINVAL);
                tmx->freq = 654;
                break;
            default:
                return retErr(EINVAL);
        }
        return TIME_OK;
    }
    return retErr(EINVAL);
}
int thrd_sleep(const timespec* duration, timespec* remaining)
{
    retTest(thrd_sleep, duration, remaining);
    // Nothing during testing
    return 0;
}
int ioctl(int fd, unsigned long request, ...) throw()
{
    va_list ap;
    va_start(ap, request);
    void *arg = va_arg(ap, void *);
    va_end(ap);
    retTest(ioctl, fd, request, arg);
    if(arg == nullptr)
        return retErr(EFAULT);
    ifreq *ifr = (ifreq *)arg;
    sockaddr_in *d4;
    hwtstamp_config *cfg;
    switch(request) {
        case SIOCGIFHWADDR:
            if(fd != 7 || strcmp("enp0s25", ifr->ifr_name) != 0)
                return retErr(EINVAL);
            memcpy(ifr->ifr_hwaddr.sa_data, "\x1\x2\x3\x4\x5\x6", 6);
            break;
        case SIOCETHTOOL:
            if(fd != 7 || strcmp("enp0s25", ifr->ifr_name) != 0)
                return retErr(EINVAL);
            {
                ethtool_ts_info *info = (ethtool_ts_info *)ifr->ifr_data;
                if(info == nullptr || info->cmd != ETHTOOL_GET_TS_INFO)
                    return retErr(EINVAL);
                info->phc_index = 3;
            }
            break;
        case SIOCGIFINDEX:
            if(fd != 7 || strcmp("enp0s25", ifr->ifr_name) != 0)
                return retErr(EINVAL);
            ifr->ifr_ifindex = 2;
            break;
        case SIOCGIFADDR:
            if(fd != 7 || strcmp("enp0s25", ifr->ifr_name) != 0)
                return retErr(EINVAL);
            d4 = (sockaddr_in *)&ifr->ifr_addr;
            memcpy(&d4->sin_addr.s_addr, "\xc0\xa8\x01\x34", 4);
            break;
        case SIOCGIFNAME:
            if(fd != 7 || ifr->ifr_ifindex != 2)
                return retErr(EINVAL);
            strcpy(ifr->ifr_name, "enp0s25");
            break;
        case SIOCGHWTSTAMP:
            if(fd != 7 || strcmp("enp0s25", ifr->ifr_name) != 0 ||
               ifr->ifr_data == nullptr)
                return retErr(EINVAL);
            cfg = (hwtstamp_config *)ifr->ifr_data;
            cfg->rx_filter = HWTSTAMP_FILTER_PTP_V2_L4_SYNC;
            cfg->tx_type = HWTSTAMP_TX_ON;
            break;
        case SIOCSHWTSTAMP:
            cfg = (hwtstamp_config *)ifr->ifr_data;
            if(fd != 7 || strcmp("enp0s25", ifr->ifr_name) != 0 ||
               cfg == nullptr ||
               cfg->rx_filter != HWTSTAMP_FILTER_PTP_V2_L4_SYNC ||
               cfg->tx_type != HWTSTAMP_TX_ONESTEP_SYNC)
                return retErr(EINVAL);
            break;
        default:
            return retErr(EINVAL);
    }
    return 0;
}
tm *localtime(const time_t *t) throw()
{
    retTest(localtime, t);
    static tm l;
    if(tzDl) {
        l.tm_isdst = 1;
        l.tm_gmtoff = 2 * 3600;
        l.tm_zone = "CEST";
    } else {
        l.tm_isdst = 0;
        l.tm_gmtoff = 1 * 3600;
        l.tm_zone = "CET";
    }
    tzDl = !tzDl;
    return &l;
}
int getifaddrs(ifaddrs **ifap) throw()
{
    retTest(getifaddrs, ifap);
    static sockaddr_in6 addr[1];
    static ifaddrs a[1];
    a[0].ifa_next = nullptr;
    a[0].ifa_name = (char *)"enp0s25";
    sockaddr_in6 *d = (sockaddr_in6 *)(addr + 0);
    a[0].ifa_addr = (sockaddr *)(addr + 0);
    addr[0].sin6_family = AF_INET6;
    memcpy(addr[0].sin6_addr.s6_addr, "\xfe\x80\x0\x0\x0\x0\x0\x0\x0\x42\xa5\xff\xfe\x51\x41\x50", 16);
    *ifap = a;
    return 0;
}
void freeifaddrs(ifaddrs *ifa) throw()
{
    retTestV(freeifaddrs, ifa);
}
time_t time(time_t *arg) throw()
{
    retTest(time, arg);
    return mono_sec;
}
int open(const char *name, int flags, ...)
{
    if(!testMode) {
        if((flags & O_CREAT) == O_CREAT || (flags & O_TMPFILE) == O_TMPFILE) {
            va_list ap;
            va_start(ap, flags);
            mode_t mode = va_arg(ap, mode_t);
            va_end(ap);
            retTest(open, name, flags, mode);
        } else
            retTest(open, name, flags);
    }
    return l_open(name, flags);
}
int __open_2(const char *name, int flags)
{
    retTest(open, name, flags);
    return l_open(name, flags);
}
// glibc stat fucntions
int stat(const char *name, struct stat *sp) throw()
{
    retTestStat(stat);
    return l_stat(name, sp);
}
int stat64(const char *name, struct stat64 *sp) throw()
{
    retTestStat(stat64);
    return l_stat64(name, sp);
}
int __xstat(int ver, const char *name, struct stat *sp)
{
    retTest(__xstat, ver, name, sp);
    return l_stat(name, sp);
}
int __xstat64(int ver, const char *name, struct stat64 *sp)
{
    retTest(__xstat64, ver, name, sp);
    return l_stat64(name, sp);
}
char *realpath(const char *path, char *resolved) throw()
{
    retTest(realpath, path, resolved);
    return l_realpath(path, resolved);
}
char *__realpath_chk(const char *path, char *resolved,
    size_t resolvedlen) throw()
{
    retTest(__realpath_chk, path, resolved, resolvedlen);
    return l_realpath(path, resolved);
}
