/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief interface object
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#ifndef __CSPTP_IF_H_
#define __CSPTP_IF_H_

#include "src/common.h"

typedef struct if_t *pif;
typedef const struct if_t *pcif;

struct if_t {
    bool _init; /* Flag indicate object is Initialized */
    char *_ifName; /* Network interface name */
    int _ifIndex; /* Network interface index */
    int _ptpIndex; /* PTP clock index that belong to interface */
    uint32_t _ipv4; /* Network interface IPv4 address */
    uint8_t _ipv6[IPV6_ADDR_LEN]; /* Network interface IPv6 address */
    bool _haveIpv6; /* We already fetch IPv6 address */
    uint8_t _mac[6]; /* MAC address */
    uint8_t _clockID[8]; /* Clock ID derived from MAC address */

    /**
     * Free this Network interface object
     * @param[in, out] self Network interface object
     */
    void (*free)(pif self);

    /**
     * Initialize using Network interface name
     * @param[in, out] self Network interface object
     * @param[in] ifName Network interface name
     * @param[in] useTwoSteps flag
     * @return true on success
     * @note We only accept interface that support PTP and have a PTP clock or more
     */
    bool (*intIfName)(pif self, const char *ifName, bool useTwoSteps);

    /**
     * Initialize using Network interface index
     * @param[in, out] self Network interface object
     * @param[in] ifIndex Network interface index
     * @param[in] useTwoSteps flag
     * @return true on success
     * @note We only accept interface that support PTP and have a PTP clock or more
     */
    bool (*intIfIndex)(pif self, int ifIndex, bool useTwoSteps);

    /**
     * Is object Initialized?
     * @param[in] self Network interface object
     * @return true if object Initialized
     */
    bool (*isInit)(pcif self);

    /**
     * Get MAC address of Network interface
     * @param[in] self Network interface object
     * @return pointer to MAC address
     */
    const uint8_t *(*getMacAddr)(pcif self);

    /**
     * Get Clock ID for the PTP clock
     * @param[in] self Network interface object
     * @return pointer to Clock ID
     * @note Clock ID is derived from Network interface MAC address
     */
    const uint8_t *(*getClockID)(pcif self);

    /**
     * Get IPv4 address of Network interface
     * @param[in] self Network interface object
     * @return IPv4 address
     * @note IPv4 in network order
     */
    uint32_t (*getIPv4)(pcif self);

    /**
     * Get IPv6 address of Network interface
     * @param[in, out] self Network interface object
     * @param[in, out] ipv6 pointer to store IPv6 address
     * @return true on success
     */
    bool (*getIPv6)(pif self, uint8_t **ipv6);

    /**
     * Get Network interface name
     * @param[in] self Network interface object
     * @return return Network interface name or null if object is not initialized
     */
    const char *(*getIfName)(pcif self);

    /**
     * Get Network interface index
     * @param[in] self Network interface object
     * @return return Network interface index
     */
    int (*getIfIndex)(pcif self);

    /**
     * Get Network interface PTP clock index
     * @param[in] self Network interface object
     * @return return Network interface PTP clock index
     */
    int (*getPTPIndex)(pcif self);

    /**
     * Bind socket to this Network interface
     * @param[in] self Network interface object
     * @param[in] fd socket file description
     * @return true on success
     */
    bool (*bind)(pcif self, int fd);
};

/**
 * Allocate a Network interface object
 * @return pointer to a new interface object or null
 */
pif if_alloc();

#endif /* __CSPTP_IF_H_ */
