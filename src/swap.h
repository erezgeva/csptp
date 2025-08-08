/* SPDX-License-Identifier: GPL-3.0-or-later
   SPDX-FileCopyrightText: Copyright © 2025 Erez Geva <ErezGeva2@gmail.com> */

/** @file
 * @brief host to network and via versa functions
 *
 * @author Erez Geva <ErezGeva2@@gmail.com>
 * @copyright © 2025 Erez Geva
 *
 */

#ifndef __CSPTP_SWAP_H_
#define __CSPTP_SWAP_H_

#include <stdint.h>
#include <arpa/inet.h> /* POSIX */

#if defined __BYTE_ORDER__XX && defined __ORDER_BIG_ENDIAN__XX && __BYTE_ORDER__XX == __ORDER_BIG_ENDIAN__XX
static inline uint64_t htonll(uint64_t value) { return value; }
static inline uint64_t ntohll(uint64_t value) { return value; }
#else
static inline uint64_t htonll(uint64_t value)
{
    uint64_t ret;
    uint32_t *p = (uint32_t *)&ret;
    p[0] = htonl((uint32_t)(value >> 32));
    p[1] = htonl((uint32_t)(value & UINT32_MAX));
    return ret;
}
static inline uint64_t ntohll(uint64_t value)
{
    uint32_t *p = (uint32_t *)&value;
    return ((uint64_t)ntohl(p[0]) << 32) | ntohl(p[1]);
}
#endif

static inline uint16_t cpu_to_net16(uint16_t value) {return htons(value);}
static inline uint32_t cpu_to_net32(uint32_t value) {return htonl(value);}
static inline uint64_t cpu_to_net64(uint64_t value) {return htonll(value);}
static inline uint16_t net_to_cpu16(uint16_t value) {return ntohs(value);}
static inline uint32_t net_to_cpu32(uint32_t value) {return ntohl(value);}
static inline uint64_t net_to_cpu64(uint64_t value) {return ntohll(value);}

#endif /* __CSPTP_SWAP_H_ */
