/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief interface object
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#include "src/if.h"
#include "src/log.h"

#include <errno.h>
#include <unistd.h>        /* POSIX */
#include <netdb.h>         /* POSIX */
#include <net/if.h>        /* POSIX */
#include <ifaddrs.h>       /* GNU/Linux */
#include <sys/ioctl.h>     /* GNU/Linux */
#include <linux/sockios.h> /* Linux */
#include <linux/ethtool.h> /* Linux */
#include <linux/net_tstamp.h> /* Linux */

static void _free(pif self)
{
    if(LIKELY_COND(self != NULL)) {
        free(self->_ifName);
        free(self);
    }
}
static inline void setClockID(pif self)
{
    /* Follow IEEE EUI-48 to EUI-64 */
    memcpy(self->_clockID, self->_mac, 3); /* first 3 octets */
    self->_clockID[3] = 0xff;
    self->_clockID[4] = 0xfe;
    memcpy(self->_clockID + 5, self->_mac + 3, 3); /* Last 3 octest */
}
static inline bool setHWTS(pif self, int fd, struct ifreq *ifr,
    bool useTwoSteps)
{
    bool have = true;
    struct hwtstamp_config cfg;
    memset(&cfg, 0, sizeof(cfg));
    ifr->ifr_data = (char *)&cfg;
    if(ioctl(fd, SIOCGHWTSTAMP, ifr) < 0) {
        if(errno == ENOTTY)
            log_err("Kernel does not support SIOCGHWTSTAMP");
        else if(errno == EOPNOTSUPP)
            log_err("Device driver does not support SIOCGHWTSTAMP");
        else
            logp_err("SIOCGHWTSTAMP");
        return false;
    }
    if(useTwoSteps) {
        if(cfg.tx_type != HWTSTAMP_TX_ON) {
            cfg.tx_type = HWTSTAMP_TX_ON;
            have = false;
        }
    } else {
        if(cfg.tx_type != HWTSTAMP_TX_ONESTEP_SYNC &&
            cfg.tx_type != HWTSTAMP_TX_ONESTEP_P2P) {
            /* We only use sync */
            cfg.tx_type = HWTSTAMP_TX_ONESTEP_SYNC;
            have = false;
        }
    }
    switch(cfg.rx_filter) {
        /* Any of the follow, support PTPv2 layer 4 Sync */
        case HWTSTAMP_FILTER_ALL:
        case HWTSTAMP_FILTER_PTP_V2_L4_EVENT:
        case HWTSTAMP_FILTER_PTP_V2_L4_SYNC:
        case HWTSTAMP_FILTER_PTP_V2_L4_DELAY_REQ:
        case HWTSTAMP_FILTER_PTP_V2_EVENT:
        case HWTSTAMP_FILTER_PTP_V2_SYNC:
        case HWTSTAMP_FILTER_PTP_V2_DELAY_REQ:
            break;
        default:
            cfg.rx_filter = HWTSTAMP_FILTER_PTP_V2_L4_SYNC;
            have = false;
            break;
    }
    if(have) /* We already have proper setting, no need to change */
        return true;
    if(ioctl(fd, SIOCSHWTSTAMP, ifr) < 0) {
        logp_err("SIOCSHWTSTAMP");
        return false;
    }
    return true;
}
static inline bool initPtp(pif self, int fd, struct ifreq *ifr,
    bool useTwoSteps)
{
    struct sockaddr_in *d;
    struct ethtool_ts_info info;
    /* retrieve corresponding MAC */
    if(ioctl(fd, SIOCGIFHWADDR, ifr) < 0) {
        logp_err("SIOCGIFHWADDR");
        return false;
    }
    memcpy(self->_mac, ifr->ifr_hwaddr.sa_data, 6);
    if(ioctl(fd, SIOCGIFADDR, ifr) < 0) {
        logp_err("SIOCGIFADDR");
        return false;
    }
    d = (struct sockaddr_in *)&ifr->ifr_addr;
    self->_ipv4 = d->sin_addr.s_addr;
    memset(&info, 0, sizeof(struct ethtool_ts_info));
    info.cmd = ETHTOOL_GET_TS_INFO;
    info.phc_index = -1;
    ifr->ifr_data = (char *)&info;
    if(ioctl(fd, SIOCETHTOOL, ifr) < 0) {
        logp_err("SIOCETHTOOL");
        return false;
    }
    if(!setHWTS(self, fd, ifr, useTwoSteps))
        return false;
    self->_ptpIndex = info.phc_index;
    return true;
}
static bool _intIfName(pif self, const char *ifName, bool useTwoSteps)
{
    int fd;
    size_t len;
    struct ifreq ifr;
    bool ret;
    if(UNLIKELY_COND(self == NULL) || ifName == NULL || *ifName == 0)
        return false;
    if(self->_init) {
        log_warning("Already Initialized");
        return false;
    }
    len = strnlen(ifName, IFNAMSIZ + 1);
    if(len > IFNAMSIZ) {
        log_err("IfName is longer than limit %zu", len);
        return false;
    }
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd < 0) {
        logp_err("socket");
        return false;
    }
    ret = false;
    memset(&ifr, 0, sizeof ifr);
    strncpy(ifr.ifr_name, ifName, IFNAMSIZ);
    if(ioctl(fd, SIOCGIFINDEX, &ifr) < 0) {
        logp_err("SIOCGIFINDEX");
        goto err;
    }
    self->_ifIndex = ifr.ifr_ifindex;
    if(!initPtp(self, fd, &ifr, useTwoSteps))
        goto err;
    self->_ifName = strndup(ifName, IFNAMSIZ);
    if(self->_ifName == NULL) {
        log_err("string duplication failed");
        goto err;
    }
    setClockID(self);
    self->_init = true;
    ret = true;
err:
    close(fd);
    return ret;
}
static bool _intIfIndex(pif self, int ifIndex, bool useTwoSteps)
{
    int fd;
    struct ifreq ifr;
    bool ret;
    if(UNLIKELY_COND(self == NULL) || ifIndex < 0)
        return false;
    if(self->_init) {
        log_warning("Already Initialized");
        return false;
    }
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd < 0) {
        logp_err("socket");
        return false;
    }
    ret = false;
    memset(&ifr, 0, sizeof ifr);
    ifr.ifr_ifindex = ifIndex;
    if(ioctl(fd, SIOCGIFNAME, &ifr) < 0) {
        logp_err("SIOCGIFNAME");
        goto err;
    }
    if(!initPtp(self, fd, &ifr, useTwoSteps))
        goto err;
    /* Leave space for null termination */
    self->_ifName = malloc(IFNAMSIZ + 1);
    if(self->_ifName == NULL) {
        log_err("string duplication failed");
        goto err;
    }
    strncpy(self->_ifName, ifr.ifr_name, IFNAMSIZ);
    /* Make sure we have null termination */
    self->_ifName[IFNAMSIZ] = 0;
    self->_ifIndex = ifIndex;
    setClockID(self);
    self->_init = true;
    ret = true;
err:
    close(fd);
    return ret;
}
static bool _getIPv6(pif self, uint8_t **ipv6)
{
    if(UNLIKELY_COND(self == NULL) || !self->_init || ipv6 == NULL)
        return false;
    if(!self->_haveIpv6) {
        bool s;
        struct ifaddrs *ifa, *ifaddr;
        if(getifaddrs(&ifaddr) == -1) {
            logp_err("getifaddrs");
            return false;
        }
        s = true;
        for(ifa = ifaddr; s && ifa != NULL; ifa = ifa->ifa_next) {
            struct sockaddr *addr = ifa->ifa_addr;
            if(addr != NULL && addr->sa_family == AF_INET6 &&
                strcmp(ifa->ifa_name, self->_ifName) == 0) {
                struct sockaddr_in6 *d = (struct sockaddr_in6 *)addr;
                memcpy(self->_ipv6, &d->sin6_addr.s6_addr, IPV6_ADDR_LEN);
                s = false;
            }
        }
        freeifaddrs(ifaddr);
        if(s) /* We did not found */
            return false;
        self->_haveIpv6 = true;
    }
    *ipv6 = self->_ipv6;
    return true;
}
static bool _bind(pcif self, int fd)
{
    if(UNLIKELY_COND(self == NULL) || !self->_init || fd < 0)
        return false;
    if(setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, self->_ifName,
            strnlen(self->_ifName, IFNAMSIZ)) < 0) {
        logp_err("SO_BINDTODEVICE");
        return false;
    }
    return true;
}
static bool _isInit(pcif self)
{
    return LIKELY_COND(self != NULL) ? self->_init : false;
}
static const uint8_t *_getMacAddr(pcif self)
{
    return LIKELY_COND(self != NULL) && self->_init ? self->_mac : NULL;
}
static const uint8_t *_getClockID(pcif self)
{
    return LIKELY_COND(self != NULL) && self->_init ? self->_clockID : NULL;
}
static uint32_t _getIPv4(pcif self)
{
    return LIKELY_COND(self != NULL) && self->_init ? self->_ipv4 : 0;
}
static const char *_getIfName(pcif self)
{
    return LIKELY_COND(self != NULL) && self->_init ? self->_ifName : NULL;
}
static int _getIfIndex(pcif self)
{
    return LIKELY_COND(self != NULL) && self->_init ? self->_ifIndex : -1;
}
static int _getPTPIndex(pcif self)
{
    return LIKELY_COND(self != NULL) && self->_init ? self->_ptpIndex : -1;
}
pif if_alloc()
{
    pif ret = malloc(sizeof(struct if_t));
    if(ret != NULL) {
        ret->_init = false;
        ret->_haveIpv6 = false;
        ret->_ifName = NULL;
        ret->_ifIndex = -1;
        ret->_ptpIndex = -1;
#define asg(a) ret->a = _##a
        asg(free);
        asg(intIfName);
        asg(intIfIndex);
        asg(isInit);
        asg(getMacAddr);
        asg(getClockID);
        asg(getIPv4);
        asg(getIPv6);
        asg(getIfName);
        asg(getIfIndex);
        asg(getPTPIndex);
        asg(bind);
    } else
        log_err("memory allocation failed");
    return ret;
}
